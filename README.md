# OCN2MID
Ocean (Jonathan Dunn) (GB/GBC) to MIDI converter

This tool converts music from Game Boy and Game Boy Color games using Ocean's sound engine, programmed by Jonathan Dunn, to MIDI format.

It works with ROM images. To use it, you must specify the name of the ROM followed by the number of the bank containing the sound data (in hex).
For games that contain multiple banks of music, you must run the program multiple times specifying where each different bank is located. However, in order to prevent files from being overwritten, the MIDI files from the previous bank must either be moved to a separate folder or renamed.

Examples:
* OCN2MID "Hook (E) [!].gb" 3
* OCN2MID "Robocopy (U) (V1.1).gb" 4
* OCN2MID "Jurassic Park (E) (M5) [!].gb" E

This tool was created primarily using the official source code for Beauty and the Beast as reference, with additional reverse-engineering done for other games which have differences in the format.

Once again, a "prototype" program, OCN2TXT, is also included.

Supported games:
* The Addams Family: Pugsley's Scavenger Hunt
* Beauty and the Beast: A Board Game Adventure
* Darkman
* Desert Strike: Return to the Gulf
* Hook
* Hudson Hawk
* Jurassic Park
* Jurassic Park Part 2: The Chaos Continues
* Lemmings
* The Little Mermaid II: Pinball Frenzy
* Mr. Nutz
* Navy Seals
* NBA 3-on-3 Featuring Kobe Bryant
* Pang
* Parasol Stars
* RoboCop (GB)
* RoboCop 2
* Super Hunchback
* Super James Pond

## To do:
  * Support for the NES version of the sound engine
  * GBS file support
