#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

class AfterhoursChordLabAudioProcessor final : public juce::AudioProcessor
{
public:
    struct Chord
    {
        std::array<int, 4> notes {};
        int noteCount = 3;
        int degree = 0;
    };

    struct Progression
    {
        std::array<Chord, 8> chords {};
        int bars = 4;
    };

    AfterhoursChordLabAudioProcessor();
    ~AfterhoursChordLabAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;
    void getStateInformation(juce::MemoryBlock& destinationData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;
    Progression makeCurrentProgression() const;
    void generateNewProgression();
    bool exportProgression(const juce::File& file) const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static Progression buildProgression(int root, int scale, int bars, int mood, uint32_t seed);
    void stopActiveNotes(juce::MidiBuffer& midi, int samplePosition);
    void startChord(int barNumber, int samplePosition, const Progression& progression, juce::MidiBuffer& midi);

    std::atomic<uint32_t> seed { 1741 };
    std::atomic<double> cachedTempo { 140.0 };
    double currentSampleRate = 44100.0;
    double previousBlockEndPpq = 0.0;
    int activeBar = -1;
    std::array<int, 4> activeNotes {};
    int activeNoteCount = 0;
    bool hasPreviousPpq = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AfterhoursChordLabAudioProcessor)
};