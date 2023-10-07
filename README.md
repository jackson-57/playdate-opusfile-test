# playdate-opusfile-test
A minimal test app for the [Playdate](https://play.date/) game console that attempts opening Opus audio files using the [opusfile](https://github.com/xiph/opusfile) library. May or may not crash on hardware. This example doesn't implement audio playback, only reading basic metadata.

# Build instructions
Clone the repository, then clone the submodules using `git submodule update --init --recursive`. Build with CMake, following the instructions from [Inside Playdate](https://sdk.play.date/2.0.3/Inside%20Playdate%20with%20C.html#_command_line).