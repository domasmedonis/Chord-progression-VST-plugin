#include "PluginEditor.h"

namespace
{
const juce::Colour background { 0xff141914 };
const juce::Colour panel { 0xff202720 };
const juce::Colour accent { 0xffd4f05b };
const juce::Colour secondary { 0xfff18d68 };
const juce::Colour muted { 0xff98a398 };
}

AfterhoursChordLabAudioProcessorEditor::AfterhoursChordLabAudioProcessorEditor(
    AfterhoursChordLabAudioProcessor& processor)
    : AudioProcessorEditor(&processor), audioProcessor(processor)
{
    setSize(640, 370);

    titleLabel.setText("AFTERHOURS", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(27.0f, juce::Font::bold)));
    titleLabel.setColour(juce::Label::textColourId, accent);
    addAndMakeVisible(titleLabel);

    subtitleLabel.setText("CHORD LAB     /     MIDI GENERATOR", juce::dontSendNotification);
    subtitleLabel.setFont(juce::Font(juce::FontOptions(11.0f)));
    subtitleLabel.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(subtitleLabel);

    configureChoice(keyBox, keyLabel, "KEY", { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" });
    configureChoice(scaleBox, scaleLabel, "SCALE", { "Natural minor", "Harmonic minor", "Phrygian", "Dorian" });
    configureChoice(barsBox, barsLabel, "LENGTH", { "4 bars", "8 bars" });
    configureChoice(moodBox, moodLabel, "MOOD", { "Gritty", "Clouded", "Ominous", "Warm" });

    keyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.parameters, "key", keyBox);
    scaleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.parameters, "scale", scaleBox);
    barsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.parameters, "bars", barsBox);
    moodAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.parameters, "mood", moodBox);
    keyBox.onChange = [this] { refreshPreview(); };
    scaleBox.onChange = [this] { refreshPreview(); };
    barsBox.onChange = [this] { refreshPreview(); };
    moodBox.onChange = [this] { refreshPreview(); };

    previewLabel.setFont(juce::Font(juce::FontOptions(17.0f, juce::Font::bold)));
    previewLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(previewLabel);

    generateButton.setColour(juce::TextButton::buttonColourId, accent);
    generateButton.setColour(juce::TextButton::textColourOffId, background);
    generateButton.onClick = [this]
    {
        audioProcessor.generateNewProgression();
        refreshPreview();
    };
    addAndMakeVisible(generateButton);

    exportButton.setColour(juce::TextButton::buttonColourId, panel);
    exportButton.setColour(juce::TextButton::textColourOffId, accent);
    exportButton.onClick = [this]
    {
        juce::FileChooser chooser("Export progression as MIDI",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Afterhours.mid"), "*.mid");
        if (chooser.browseForFileToSave(true))
        {
            auto file = chooser.getResult();
            if (file.getFileExtension().isEmpty())
                file = file.withFileExtension("mid");
            const auto succeeded = audioProcessor.exportProgression(file);
            exportButton.setButtonText(succeeded ? "MIDI exported" : "Export failed");
        }
    };
    addAndMakeVisible(exportButton);

    refreshPreview();
}

void AfterhoursChordLabAudioProcessorEditor::configureChoice(juce::ComboBox& box, juce::Label& label,
    const juce::String& name, const juce::StringArray& items)
{
    label.setText(name, juce::dontSendNotification);
    label.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    label.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(label);

    box.addItemList(items, 1);
    box.setColour(juce::ComboBox::backgroundColourId, panel);
    box.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    box.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff384238));
    box.setColour(juce::ComboBox::arrowColourId, accent);
    addAndMakeVisible(box);
}

void AfterhoursChordLabAudioProcessorEditor::refreshPreview()
{
    static const juce::StringArray romanNumerals {
        "i", "bII", "bIII", "iv", "v", "bVI", "bVII"
    };
    const auto progression = audioProcessor.makeCurrentProgression();
    juce::String preview;
    for (int bar = 0; bar < progression.bars; ++bar)
    {
        if (bar > 0)
            preview += "     ";
        preview += romanNumerals[progression.chords[static_cast<size_t>(bar)].degree];
    }
    previewLabel.setText(preview, juce::dontSendNotification);
}

void AfterhoursChordLabAudioProcessorEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(background);
    graphics.setColour(secondary.withAlpha(0.8f));
    graphics.fillRect(0, 0, 5, getHeight());
    graphics.setColour(panel);
    graphics.fillRect(28, 83, getWidth() - 56, 1);
    graphics.fillRoundedRectangle(28.0f, 205.0f, static_cast<float>(getWidth() - 56), 68.0f, 4.0f);
    graphics.setColour(accent.withAlpha(0.6f));
    graphics.fillRect(28, 205, 3, 68);
}

void AfterhoursChordLabAudioProcessorEditor::resized()
{
    titleLabel.setBounds(30, 22, 260, 34);
    subtitleLabel.setBounds(32, 56, 300, 18);

    constexpr int firstX = 30;
    constexpr int columnWidth = 140;
    constexpr int gap = 12;
    const auto placeChoice = [](juce::Label& label, juce::ComboBox& box, int x)
    {
        label.setBounds(x, 111, 125, 18);
        box.setBounds(x, 135, 130, 38);
    };
    placeChoice(keyLabel, keyBox, firstX);
    placeChoice(scaleLabel, scaleBox, firstX + columnWidth + gap);
    placeChoice(barsLabel, barsBox, firstX + (columnWidth + gap) * 2);
    placeChoice(moodLabel, moodBox, firstX + (columnWidth + gap) * 3);

    previewLabel.setBounds(45, 219, getWidth() - 90, 38);
    generateButton.setBounds(30, 302, 230, 42);
    exportButton.setBounds(getWidth() - 210, 302, 180, 42);
}