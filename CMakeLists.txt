cmake_minimum_required(VERSION 3.22)

project(AfterhoursChordLab VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(AfterhoursChordLab
    COMPANY_NAME "Afterhours Audio"
    BUNDLE_ID "audio.afterhours.chordlab"
    PLUGIN_MANUFACTURER_CODE Aftr
    PLUGIN_CODE Ahcl
    PRODUCT_NAME "Afterhours Chord Lab"
    FORMATS VST3
    IS_SYNTH FALSE
    IS_MIDI_EFFECT TRUE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(AfterhoursChordLab PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_features(AfterhoursChordLab PRIVATE cxx_std_17)
target_compile_definitions(AfterhoursChordLab PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)
target_link_libraries(AfterhoursChordLab PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)