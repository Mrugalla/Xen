# Xen
A VSTi that transforms input MIDI to polyphonic xenharmonic MPE MIDI.



Parameters

- Xen: The xen scale describes how many pitch classes per octave exist. The most common xen scale is 12tet / 12edo.
- Base Pitch: The base- or reference pitch is the same in all xen scales that you choose.
- Master Tune: You can retune the notes by a few hz with this parameter.
- Pitchbend Range: This parameter has to be aligned with the MPE pitchbend range of the target synth.
- Auto MPE: If this is enabled you don't need to input notes with manually seperated MIDI channels.
- Playmode:
1. The "Rescale" mode just rescales every one of the 128 MIDI values to a distinct pitch in the xen scale.
2. The "Nearest" mode retunes the incoming notes to the nearest pitch of the played key's 12tet interpretation.
- Use Synth: If enabled the plugin outputs audio, so that you can test if everything works correctly.



How to use

1. Instantiate Xen as an instrument plugin.
2. Instantiate an instance of your MPE-compatible target synth.
3. Make sure Xen can output on all MIDI channels.
4. Make sure the target synth can receive all MIDI channels.
5. Send Xen's MIDI output to the target synth's MIDI input.
6. Use Xen's internal synth to check if it works.
7. If some pitches are wrong you need to align the pitchbend range.
8. success :)



Known issues

- Plugin doesn't work in Bitwig, because Bitwig doesn't support VST3 plugins outputting pitchbend.
- In Cubase the timing of the output MIDI is messed up after it loops for some reason.
