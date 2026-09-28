#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <random>

namespace
{
constexpr std::array<std::array<int, 7>, 4> scaleIntervals {{
    {{ 0, 2, 3, 5, 7, 8, 10 }},
    {{ 0, 2, 3, 5, 7, 8, 11 }},
    {{ 0, 1, 3, 5, 7, 8, 10 }},
    {{ 0, 2, 3, 5, 7, 9, 10 }}
}};

constexpr std::array<std::array<std::array<int, 8>, 3>, 4> progressionTemplates {{
    {{{{ 0, 5, 2, 6, 0, 3, 5, 6 }}, {{ 0, 6, 5, 6, 0, 3, 2, 6 }}, {{ 0, 3, 6, 5, 0, 5, 3, 6 }}}},
    {{{{ 0, 3, 4, 6, 0, 5, 3, 4 }}, {{ 0, 5, 3, 4, 0, 2, 5, 3 }}, {{ 0, 2, 5, 3, 0, 4, 2, 5 }}}},
    {{{{ 0, 1, 5, 6, 0, 1, 3, 6 }}, {{ 0, 6, 1, 0, 0, 5, 1, 6 }}, {{ 0, 1, 3, 6, 0, 6, 5, 1 }}}},
    {{{{ 0, 3, 4, 0, 0, 5, 3, 4 }}, {{ 0, 5, 3, 4, 0, 2, 3, 0 }}, {{ 0, 2, 3, 0, 0, 4, 5, 3 }}}}
}};

constexpr int ticksPerQuarter = 960;
constexpr double quartersPerBar = 4.0;

int positiveModulo(int value, int modulus)
{
    const auto result = value % modulus;
    return result < 0 ? result + modulus : result;
}

int parameterChoice(const juce::AudioProcessorValueTreeState& state, const juce::String& id)
{
    return juce::roundToInt(state.getRawParameterValue(id)->load());
}
}

AfterhoursChordLabAudioProcessor::AfterhoursChordLabAudioProcessor()
    : AudioProcessor(BusesProperties()),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout AfterhoursChordLabAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    juce::StringArray keys { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    juce::StringArray scales { "Natural minor", "Harmonic minor", "Phrygian", "Dorian" };
    juce::StringArray moods { "Gritty", "Clouded", "Ominous", "Warm" };

    layout.add(std::make_unique<juce::AudioParameterChoice>("key", "Key", keys, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("scale", "Scale", scales, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("bars", "Length", juce::StringArray { "4 bars", "8 bars" }, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("mood", "Mood", moods, 0));
    return layout;
}

void AfterhoursChordLabAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    activeBar = -1;
    activeNoteCount = 0;
    hasPreviousPpq = false;
}

void AfterhoursChordLabAudioProcessor::releaseResources()
{
    activeBar = -1;
    activeNoteCount = 0;
    hasPreviousPpq = false;
}

bool AfterhoursChordLabAudioProcessor::isBusesLayoutSupported(const BusesLayout&) const
{
    return true;
}

void AfterhoursChordLabAudioProcessor::processBlock(juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    audio.clear();
    midi.clear();

    const auto* playHead = getPlayHead();
    if (playHead == nullptr)
    {
        stopActiveNotes(midi, 0);
        hasPreviousPpq = false;
        return;
    }

    const auto position = playHead->getPosition();
    if (!position.hasValue())
    {
        stopActiveNotes(midi, 0);
        hasPreviousPpq = false;
        return;
    }

    const auto ppqPosition = position->getPpqPosition();
    const auto bpmPosition = position->getBpm();
    const auto playingPosition = position->getIsPlaying();
    if (!ppqPosition.hasValue() || !bpmPosition.hasValue()
        || (playingPosition.hasValue() && !*playingPosition))
    {
        stopActiveNotes(midi, 0);
        hasPreviousPpq = false;
        return;
    }

    const auto ppqStart = *ppqPosition;
    const auto bpm = juce::jmax(1.0, *bpmPosition);
    cachedTempo.store(bpm);
    const auto ppqPerSample = bpm / (60.0 * currentSampleRate);
    const auto ppqEnd = ppqStart + audio.getNumSamples() * ppqPerSample;
    const auto progression = makeCurrentProgression();

    const auto discontinuity = hasPreviousPpq && std::abs(ppqStart - previousBlockEndPpq) > 0.25;
    if (discontinuity)
        stopActiveNotes(midi, 0);

    const auto barAtStart = static_cast<int>(std::floor(ppqStart / quartersPerBar));
    if (activeBar != barAtStart || activeNoteCount == 0)
        startChord(barAtStart, 0, progression, midi);

    auto nextBar = barAtStart + 1;
    while (nextBar * quartersPerBar < ppqEnd)
    {
        const auto boundaryPpq = nextBar * quartersPerBar;
        const auto samplePosition = juce::jlimit(0, juce::jmax(0, audio.getNumSamples() - 1),
            juce::roundToInt((boundaryPpq - ppqStart) / ppqPerSample));
        startChord(nextBar, samplePosition, progression, midi);
        ++nextBar;
    }

    previousBlockEndPpq = ppqEnd;
    hasPreviousPpq = true;
}

void AfterhoursChordLabAudioProcessor::stopActiveNotes(juce::MidiBuffer& midi, int samplePosition)
{
    for (int index = 0; index < activeNoteCount; ++index)
        midi.addEvent(juce::MidiMessage::noteOff(1, activeNotes[static_cast<size_t>(index)]), samplePosition);

    activeNoteCount = 0;
    activeBar = -1;
}

void AfterhoursChordLabAudioProcessor::startChord(int barNumber, int samplePosition,
    const Progression& progression, juce::MidiBuffer& midi)
{
    stopActiveNotes(midi, samplePosition);
    const auto chordIndex = positiveModulo(barNumber, progression.bars);
    const auto& chord = progression.chords[static_cast<size_t>(chordIndex)];

    activeNoteCount = chord.noteCount;
    activeBar = barNumber;
    for (int index = 0; index < activeNoteCount; ++index)
    {
        const auto note = chord.notes[static_cast<size_t>(index)];
        activeNotes[static_cast<size_t>(index)] = note;
        midi.addEvent(juce::MidiMessage::noteOn(1, note, static_cast<juce::uint8>(92)), samplePosition);
    }
}

AfterhoursChordLabAudioProcessor::Progression AfterhoursChordLabAudioProcessor::buildProgression(
    int root, int scale, int bars, int mood, uint32_t progressionSeed)
{
    Progression progression;
    progression.bars = bars == 8 ? 8 : 4;

    std::mt19937 random(progressionSeed);
    std::uniform_int_distribution<int> templateChoice(0, 2);
    std::uniform_int_distribution<int> seventhRoll(0, 99);
    const auto selectedTemplate = templateChoice(random);
    const auto& intervals = scaleIntervals[static_cast<size_t>(juce::jlimit(0, 3, scale))];
    const auto& degrees = progressionTemplates[static_cast<size_t>(juce::jlimit(0, 3, mood))]
        [static_cast<size_t>(selectedTemplate)];
    const auto rootPitch = juce::jlimit(0, 11, root);

    for (int bar = 0; bar < progression.bars; ++bar)
    {
        auto& chord = progression.chords[static_cast<size_t>(bar)];
        chord.degree = degrees[static_cast<size_t>(bar)];
        chord.noteCount = seventhRoll(random) < (mood == 1 ? 78 : 48) ? 4 : 3;

        for (int voice = 0; voice < chord.noteCount; ++voice)
        {
            const auto scaleIndex = chord.degree + voice * 2;
            const auto octave = scaleIndex / 7;
            const auto pitch = rootPitch + intervals[static_cast<size_t>(scaleIndex % 7)] + octave * 12;
            auto note = 48 + pitch;

            if (bar > 0 && voice < progression.chords[static_cast<size_t>(bar - 1)].noteCount)
            {
                const auto previous = progression.chords[static_cast<size_t>(bar - 1)].notes[static_cast<size_t>(voice)];
                while (note - previous > 6)
                    note -= 12;
                while (previous - note > 6)
                    note += 12;
            }

            chord.notes[static_cast<size_t>(voice)] = juce::jlimit(36, 84, note);
        }
    }

    return progression;
}

AfterhoursChordLabAudioProcessor::Progression AfterhoursChordLabAudioProcessor::makeCurrentProgression() const
{
    return buildProgression(parameterChoice(parameters, "key"),
        parameterChoice(parameters, "scale"),
        parameterChoice(parameters, "bars") == 0 ? 4 : 8,
        parameterChoice(parameters, "mood"), seed.load());
}

void AfterhoursChordLabAudioProcessor::generateNewProgression()
{
    seed.fetch_add(1);
}

bool AfterhoursChordLabAudioProcessor::exportProgression(const juce::File& file) const
{
    const auto progression = makeCurrentProgression();
    const auto tempo = juce::jlimit(20.0, 400.0, cachedTempo.load());
    const auto microsPerQuarter = juce::roundToInt(60000000.0 / tempo);
    juce::MidiMessageSequence sequence;
    sequence.addEvent(juce::MidiMessage::tempoMetaEvent(microsPerQuarter), 0.0);

    const auto noteLength = progression.bars * quartersPerBar * ticksPerQuarter * 0.96;
    for (int bar = 0; bar < progression.bars; ++bar)
    {
        const auto startTick = bar * quartersPerBar * ticksPerQuarter;
        const auto endTick = startTick + noteLength / progression.bars;
        const auto& chord = progression.chords[static_cast<size_t>(bar)];
        for (int voice = 0; voice < chord.noteCount; ++voice)
        {
            const auto note = chord.notes[static_cast<size_t>(voice)];
            sequence.addEvent(juce::MidiMessage::noteOn(1, note, static_cast<juce::uint8>(92)), startTick);
            sequence.addEvent(juce::MidiMessage::noteOff(1, note), endTick);
        }
    }

    sequence.updateMatchedPairs();
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(ticksPerQuarter);
    midiFile.addTrack(sequence);

    juce::FileOutputStream stream(file);
    if (!stream.openedOk())
        return false;

    midiFile.writeTo(stream);
    stream.flush();
    return !stream.getStatus().failed();
}

juce::AudioProcessorEditor* AfterhoursChordLabAudioProcessor::createEditor()
{
    return new AfterhoursChordLabAudioProcessorEditor(*this);
}

bool AfterhoursChordLabAudioProcessor::hasEditor() const { return true; }
const juce::String AfterhoursChordLabAudioProcessor::getName() const { return JucePlugin_Name; }
bool AfterhoursChordLabAudioProcessor::acceptsMidi() const { return false; }
bool AfterhoursChordLabAudioProcessor::producesMidi() const { return true; }
bool AfterhoursChordLabAudioProcessor::isMidiEffect() const { return true; }
double AfterhoursChordLabAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int AfterhoursChordLabAudioProcessor::getNumPrograms() { return 1; }
int AfterhoursChordLabAudioProcessor::getCurrentProgram() { return 0; }
void AfterhoursChordLabAudioProcessor::setCurrentProgram(int) {}
const juce::String AfterhoursChordLabAudioProcessor::getProgramName(int) { return {}; }
void AfterhoursChordLabAudioProcessor::changeProgramName(int, const juce::String&) {}

void AfterhoursChordLabAudioProcessor::getStateInformation(juce::MemoryBlock& destinationData)
{
    auto state = parameters.copyState();
    state.setProperty("seed", static_cast<juce::int64>(seed.load()), nullptr);
    if (const auto xml = state.createXml())
        copyXmlToBinary(*xml, destinationData);
}

void AfterhoursChordLabAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (const auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        const auto state = juce::ValueTree::fromXml(*xml);
        if (state.isValid() && state.hasType(parameters.state.getType()))
        {
            parameters.replaceState(state);
            seed.store(static_cast<uint32_t>(state.getProperty("seed", 1741)));
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AfterhoursChordLabAudioProcessor();
}