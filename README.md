# Xen
This VSTi plugin allows you to globally set and modulate the tuning system via MTS-ESP or to transform input MIDI to MPE in the given tuning system. A basic non-tweakable synthesizer is used to playback your MIDI inputs in the desired tuning system. This is intended to be used to test if everything works correctly, but you usually wanna mute the plugin. The plugin also outputs the modified MIDI data, which is especially useful in MPE mode.

<img width="221" height="232" alt="xen instance" src="https://github.com/user-attachments/assets/cdb99ec4-2b77-4d83-86ce-74e64124b9a7" />

--- Parameters ---

1. Xen Snap: If disabled, you can explore non-octave equal temperament tuning systems.
2. Xen: The equal temperament scale describes how many pitch classes per octave exist. The most common one is 12tet.
3. Anchor Freq: The frequency of the note number 69 in all available tuning systems.
4. Steps in 12: Instead of giving you unique pitch classes for each note, it picks the pitches that are the closest to 12tet from your tuning system.
5. Pitchbend Range: If you use MPE, this parameter has to be aligned with the pb range of the target synth.
6. Mode: You can either run the plugin in MPE or MTS-ESP Mode.

How to use with MPE:

1. Add Xen (instrument)
2. Add MPE-compatible target synth
3. Route all MIDI channels from Xen into MIDI input of target synth
4. Align pitchbend range between Xen and target synth

How to use with MTS-ESP:

1. Install MTS-ESP on your computer: https://github.com/ODDSound/MTS-ESP
2. Add Xen (instrument)
3. Add MTS-ESP-compatible target synths (no routing needed)

No matter if you use MPE or MTS-ESP, you can use Xen's synth to check if everything works correctly.
