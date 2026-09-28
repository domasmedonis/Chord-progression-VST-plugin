# Afterhours Chord Lab

A JUCE VST3 MIDI-effect prototype for generating moody, melodic hip-hop chord progressions. Its presets use broad underground trap and rap harmony cues rather than reproducing any one artist's material.

## Features

- Generate four- or eight-bar progressions in 12 keys, with natural minor, harmonic minor, Phrygian, and Dorian scales.
- Choose Gritty, Clouded, Ominous, or Warm harmonic palettes and roll alternate progressions.
- Send MIDI chords in time with the host transport and tempo.
- Export the current progression as a Standard MIDI File. Export uses the last tempo reported by the host, or 140 BPM if the host has not reported one.
- Save plugin parameters and the current generation seed with the DAW project.

## Build on Windows

Install Visual Studio 2022 with the Desktop development with C++ workload, CMake 3.22 or newer, and Git. CMake downloads the pinned JUCE 8.0.6 source during configuration.

From a Visual Studio Developer PowerShell in this directory:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

The VST3 bundle is produced under `build/AfterhoursChordLab_artefacts/Release/VST3`. Rescan that folder in FL Studio's Plugin Manager if it was not copied to the system VST3 directory automatically.

## Use in FL Studio

Load Afterhours Chord Lab as a VST3 MIDI effect, set its MIDI output port in the FL Studio wrapper, and route that port to an instrument that receives MIDI. Start the transport to hear the progression; chord changes follow the host's four-quarter-note bars. Use Export MIDI to save the same generated chord sequence to a file for import into a Piano roll.

The generated MIDI contains chord notes only. Sound selection, bass lines, rhythm, and arrangement remain in the receiving instrument and project.