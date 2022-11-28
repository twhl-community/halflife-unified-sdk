# Sound System

The Unified SDK provides a new sound system to fix bugs and increase or remove some limits.

The sound system currently provides support for music and sentence playback.

The sound system uses OpenAL to handle sample mixing, distance attenuation and other sound properties and effects.

## Music playback

Music playback is similar to the engine's music playback with the following differences:

* Music does not stop on level change
* Has no internal file count limit
* Supports the following file formats:
	* Wave (+ IMA-ADPCM encoding)
	* MP3
	* Ogg Vorbis
	* Ogg Opus
	* FLAC
	* WavPack
	* Musepack

The `MP3Volume` engine cvar controls music volume and is set by the music volume slider in the options dialog.

### Entities

* [ambient_music](/docs/entityguide/entities/ambient_music.md)

### Console commands

#### music

Syntax: `music <command> [arguments]`

The `music` command handles all music playback. It is the equivalent of the engine's `cd` and `mp3` commands.

Commands:
| Name | Purpose |
| --- | --- |
| on | Enables music playback |
| off | Disables music playback and stops any music that is currently playing |
| reset | Enables music playback and stops any music that is currently playing |
| play <relative_filename> | Plays the specified file |
| loop <relative_filename> | Loops the specified file |
| stop | Stops any music that is currently playing |
| fadeout | Fades out and stops any music that is currently playing. Uses the `MP3FadeTime` engine cvar to control fadeout time |
| pause | Pauses any music that is currently playing |
| resume | Resumes music if it was paused |
| info | If any music is playing or paused, prints the current filename. The volume is also printed |

## Sentence playback

Sentence playback relies on OpenAL for distance attenuation, volume and room effects using the EFX extension (OpenAL's version of the EAX extension).

Room effects use almost identical parameters to the EAX implementation that was once used by the game but since the original implementation used software mixing and fixed position 3D sound sources the results aren't identical.

Half-Life, like Quake, uses software mixing regardless of which audio API is used to actually play it. This means distance calculations are done before the audio is sent to the EAX 3D sound sources.  
  
To avoid duplicating distance effects Half-Life has 4 3D sources placed around the player. If you imagine a box 2x2 units wide and long centered on the player, the sources are placed at the corners of the box:

<img src="https://i.imgur.com/Lkfigi8.png"/>

(Image not to scale)

The new sentences system lets OpenAL handle distance calculations so sound sources are handled properly by the EFX extension, so the sound effects are working properly, but sound different from the original effects.

The effects do sound correct in-game but it's always going to be different.

Regular sound playback is also fully supported, the only thing preventing its full use is that the sound precache list can't yet be sent to the client to map sound indices back to filenames.

The `volume` engine cvar controls sound volume and is set by the sound effects volume slider in the options dialog.

### Changes to sentence limitations

* Maximum number of sentences: was **1536**, now **65535**, can be further raised by changing the network message used to send them to the client but this will increase bandwidth usage
* Maximum number of sentence groups: was **200**, now **unlimited**
* Maximum number of words in a sentences: was **32**, now **unlimited**
* Maximum sentence name length: was **15** ASCII characters, now unlimited but warns if it exceeds **31**
* Maximum number of unique sounds: was **1024**, now **unlimited**
* Maximum sentence line length: was **511** characters, now **unlimited**
* Maximum number of concurrent sounds of all kinds (static, dynamic, ambient): was **128**, now **unlimited**

Sentences are now paused when the game is paused and **will not be cut off**.

### Console commands and variables

#### cl_snd_openal

Syntax: `cl_snd_openal <1|0>` (Default `1`)

Controls whether the OpenAL sound system is used to play sounds. If it's disabled the engine's sound system is used instead. Note that only the first 1536 sentences listed in `sentences.txt` will play due to the engine's original limitations.

#### cl_snd_room_off

Syntax: `cl_snd_room_off <1|0>` (Default `0`)

Like the engine's `room_off` cvar this controls whether the EFX-based room effects are disabled. When disabled all sounds play as if room type 0 is used.

#### cl_snd_playstatic and cl_snd_playdynamic

Syntax: `cl_snd_playstatic|cl_snd_playdynamic <sound_name> [volume] [attenuation] [pitch]`

These commands play a sound on the player entity. Static sounds are played on a `CHAN_STATIC` channel, dynamic sounds are played on a `CHAN_VOICE` channel.

Default values:
| Name | Value | Range | |
| --- | --- | --- | --- |
| volume | 1 | [0, 1] | |
| attenuation | 0.8 | [0, +infinity] | normal attenuation, equivalent to `ambient_generic` large radius |
| pitch | 100 | [1, 255] | normal, unaltered pitch |

#### cl_snd_stopsound

Syntax: `cl_snd_stopsound`

Stops all sounds that are currently playing.
