## Sound System

The `SoundSystem` is responsible for interfacing with audio hardware and emiting audio.   
Currently uses `SDL_mixer` for backend, but migration to FMOD is planned (along with implementation of a richer interface that would expose FMOD's fancy features).    

We distinguish 2 types of audio: **sounds** and **music**

### Sounds
Short, typically few seconds, can easily fit entirely in memory.   
They typically either get played one-shot (shooting, hurt sound), or in a loop (doors opening).  

All sounds used in game are defined in `sound_resources.h` in the `NC_SOUNDS()` X-macro. Their names must correspond with names of `.wav` files in the `content/sound/` folder. They also have to be valid C++ identifiers.      
Functions usually accept an argument of type `SoundID`. All possible `SoundID`s can be found in the namespace `Sounds` - e.g.
```c++
SoundID sound_to_play = Sounds::plasma_rifle;
```
The simplest way to play a sound is to use the method `SoundSystem.play_oneshot(SoundID, float volume, SoundLayer)`. This returns nothing and just plays the specified sound once, on the specified sound layer and with specified volume. No spatial effect or any other fancy stuff is being performed. This is typically the right way to play non-diegetic sounds (e.g. button click in the menu) or sounds performed by the player character (e.g. shooting, player movement, death etc.).   

`SoundLayer` can currently be either `game` or `ui`. This distinction currently serves mainly to ensure `game` sounds can be disabled in the game replay that's shown as menu background on game start.     
You can manually enable or disable sound layers by calling `SoundSystem.enable_layer(SoundLayer, bool enable)`.   

Other options is `SoundSystem.play(SoundID, float volume, SoundLayer)` - this one returns a `SoundHandle` which can be used to control the sound while it's playing - e.g. pause or kill it, modify its volume etc. . Sounds currently don't support pitch adjustments.


To play a spatial sound, `SoundSystem` itself is not sufficient - you need to either directly create a `SoundEmitter` entity, or call the helper function `GameHelpers::play_3d_sound(vec3 position, SoundID, float range, float base_volume)`.   
This plays a sound at a specific position in the game world, whose volume will change based on player's distance. We currently don't support the emitter position to be moving, but that isn't typically a problem, since sounds are very short and their originators don't travel great distance over a single sound's lifetime.   

```c++
SoundSystem::get().play_oneshot(Sounds::hurt, 1.0f, SoundLayer::game); // plays the hurt sound

SoundHandle handle = SoundSystem::get().play(Sounds::hurt, 1.0f, SoundLayer::game); // plays the hurt sound
handle.set_paused(true); // immeditely pauses the sound

GameHelpers::get().play_3d_sound(enemy_position, Sounds::hurt, 20.0f /*radius of the hearable area*/, 1.0f /*base volume*/); // plays the hurt sound at `enemy_position`, always using `SoundLayer::game`
```

### Music

Music is typically a single long (few minutes) track, that plays non-diegetically in background. SDL_mixer has dedicated support for music in a way that lets it get streamed from disk in parts and not loaded as a whole into memory. However, only a single music track is allowed to be playing at any time.   

Since music files are typically quite big, we support compression for them - currently all music files are expected to have the .mp3 file extension.   

Music tracks don't have to be defined anywhere to be used, you just pass around their name as a `Token`.

You can play a specific music track by calling `SoundSystem.play_music()` (that immediatelly stops the previous track if any was playing). The music will always loop.   
```c++
SoundSystem::get().play_music(Token("ambient1")); // Plays "content/sound/ambient1.mp3"
```

You typically don't want to call `SoundSystem::play_music()` anywhere in the game. Standard way is instead to specify the soundtrack for every specific level inside the Level Editor. Just write its name in the field `Level.music`. That soundtrack will be automatically played on level load.