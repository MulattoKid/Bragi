# Bragi
> Bragi (/ˈbrɑːɡi/; Old Norse: [ˈbrɑɣe]) is the skaldic god of poetry in Norse mythology. 

*Bragi* is my attempt at creating my ideal music player. I'm creating it for myself, but if you like it, then that's great!

*Bragi* consists of three parts:
1) the sound player
2) the UI
3) the audio visualization engine (can be disabled completetly in the future)

I want to be able to use *Bragi* while programming, playing games, and casting it to a big screen to get pretty colors synced with the music.

## Key Combos
- `Esc` : quit
- `Shift + Enter` : open/close UI

## Commands
- `play <path to playlist>` : start playing a playlist (shuffle state determines initial song)
- `pause` : pause playback of current song
- `resume` : resume playback of current song
- `next` : skip to next song in playlist (shuffle and loop state determines next song)
- `previous` (TODO) skip to previous song in playlist (shuffle and loop state determines next song)
- Looping
    - `loop_no` (default) : playback stops when last song in playlist finishes playback
    - `loop` : when the last song in a playlist finishes playback, the playlist starts over
    - `loop_single` : when a song finishes playback, repeat it
- Shuffle
    - `shuffle_no` (default) : a playlist is played back in chronological order (setting this has an effect on the currently playing playlist)
    - `shuffle` : a playlist is shuffled and played back in a random order (setting this has an effect on the currently playing playlist)
- Fullscreen
    - `taskbar_show` (default) : shows taskbar
    - `taskbar_hide` : hide taskbar (fullscreen mode)

## Playlist File Documentation
- `.txt` files ending with a newline
- Each line (except the last one) has the full path to an audio file

## Audio File Format Support
- WAV/RIFF
- FLAC (more complete support in progress)

## System Requirements
- Windows
- GPU with Vulkan 1.0 support
