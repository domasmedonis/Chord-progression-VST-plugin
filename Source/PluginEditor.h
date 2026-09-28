#pragma once

#include "PluginProcessor.h"

class AfterhoursChordLabAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AfterhoursChordLabAudioProcessorEditor(AfterhoursChordLabAudioProcessor& processor);
    ~AfterhoursChordLabAudioProcessorEditor() override = default;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    void configureChoice(juce::ComboBox& box, juce::Label& label, const juce::String& name,
        const juce::StringArray& items);
    void refreshPreview();

    AfterhoursChordLabAudioProcessor& audioProcessor;
    juce::ComboBox keyBox;
    juce::ComboBox scaleBox;
    juce::ComboBox barsBox;
    juce::ComboBox moodBox;
    juce::Label keyLabel;
    juce::Label scaleLabel;
    juce::Label barsLabel;
    juce::Label moodLabel;
    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label previewLabel;
    juce::TextButton generateButton { "New progression" };
    juce::TextButton exportButton { "Export MIDI" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> barsAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> moodAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AfterhoursChordLabAudioProcessorEditor)
};