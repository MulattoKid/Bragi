# TODO

- Pausing playback of a song results in the audio visualization flickering
- Port everything from C++ -> C
- Custom resolution
- Optimize DFT
- Add more audio visualizations
    - Some kind of flow
    - https://twitter.com/XorDev/status/1520074812634320902
- Slow down songs during playback
    - ~~Perform sample-rate conversion during loading on WAV files~~
    - Perform sample-rate conversion on all supported formats
    - Adjust slow down during playback after songs aren't loaded all at once into memory
- Support not loading entire sound files into memory
    - Load a part of the sound file, and upon finish playback load more (probably want to thread this)
    - Ensure this works with the sample-rate conversion
- Support fullscreen-exclusive, fullscreen-borderless, almost-fullscreen-borderless
- Support audio formats
    - WAV
    - FLAC
    - MPEG4 (M4A)

# DONE
- ~~Allow for disabling audio visualization~~
    - When audio visualization is disabled, avoid getting playback position, computing DFT and rendering scene
- ~~Add Vulkan 1.3 support and make use of dynamic rendering~~
    - Moved renderer to Vulkan 1.3 (required)
    - Moved both scenes 'scene_columns' and 'scene_ui' to use dynamic rendering
