# Obvious
A VST plugin which can control OBS Studio. Obvious is written using the JUCE framework. Tested in Bitwig on Mac OS X.

# Features
## Sources
- Transform scale
- Relative scale
- Crop
- Toggle visibility
- Text typing, for a typewriter-like effect

## Filters
- Hue
- Saturation
- Roll speed
- Opacity

## Media
Play, pause, stop and cursor position.

# Installation
Copy the Obvious.vst3 into your plugins folder. This will be one of the following:
- All users: `/Library/Audio/Plug-Ins/VST3`
- Only one user: `/Users/{username}/Library/Audio/Plug-Ins/VST3`

Then in OBS:
- Go to the `Tools` menu and select `Scripts`
- In the `Scripts` window, click on the `+` to add a script
- Browse and select `Obvious.lua` from the `OBS lua scripts` folder in the repo. Ths script should be kept in the same folder as ljsocket.lua
- Select `Obvious.lua` from the list of scripts in the `Scripts` window, and set the `Port` to an available and accessible network port (the default value of 11111 is usually ok)

# Usage
- Add an instance of Obvious to a track in your DAW
- Set the IP address and port to match the IP of the machine running OBS, and the same port number specified in the settings for Obvious.lua in OBS. If OBS and the DAW are running on the same machine, leave the IP address set to 127.0.0.1
- Choose the `Command category` and `Command` you want to send to OBS
- Enter the `Scene`, `Source` and `Filter` as appropriate
- Moving the slider or clicking/toggling the button will send the selected command to OBS
- The slider and button can be automated and/or controlled using a MIDI or OSC controller
- Multiple instances of Obvious can be added to your DAW and connect to OBS simultaneously, for controlling multiple parameters

# Credit
- The Obvious VST and Obvious.lua were written by Bjørn Felle
- The VST is written using the JUCE framework
- The lua script utilises ljsocket.lua by CapsAdmin: https://github.com/CapsAdmin/luajitsocket
