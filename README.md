# Scratch-3DS

A W.I.P. runtime made in C++ aimed to bring any Scratch 3 project over to the Nintendo 3DS.


![Software running a simple Scratch Project](https://raw.githubusercontent.com/NateXS/Scratch-3DS/refs/heads/main/scratchcats3ds.gif)

## Controls


![Controls](https://raw.githubusercontent.com/NateXS/Scratch-3DS/refs/heads/main/scratch%203ds%20controls.png)

To use the mouse you must enter mouse mode by holding L. use the D-pad to move the mouse, and press R to click.

## Features

- **Sound Support**: WAV audio playback with volume control and sound effects
- **Sprite Rendering**: Support for PNG, JPG, and SVG images with automatic vector-to-bitmap conversion
- **Input Handling**: Keyboard, mouse, and touch input
- **Control Blocks**: Loops, conditionals, and event handling
- **Variables and Lists**: Data storage and manipulation
- **Custom Blocks**: User-defined procedures and functions

## Limitations

As this is in a very W.I.P state, you will encounter many bugs, crashes, and things that will just not work. 

**List of known limitations:**
- Performance on old 3DS starts to tank with a lot of blocks running.
- If you have a bunch of large images, some may not load.
- Images cannot be over 1024x1024 in resolution.
- Extensions (eg: pen and music extensions) are not yet supported.
- Some blocks may lead to crashing / unintended behavior. (Please open an issue if you know a block that's causing problems.)


## Unimplimented blocks
- All say and think blocks
- All Costume Effects
- Cloud variables
- Show/hide variable | Show/hide list
- When backdrop switches to
- When this sprite clicked
- When loudness > ___
- All Color touching blocks
- Set drag mode
- Loudness

## Roadmap / to-do list
- Pen support (Highly Requested!)
- Better start menu
- File picker for Scratch projects
- Ability to remap controls
- Get all unimplemented blocks working
- Look into getting some Turbowarp extensions working
- Cloud variables (maybe.....)

## Building

In order to build the project, you will need to have Devkitpro's SDKs installed with the DevkitARM toolchain and libctru.

- Devkitpro's install instructions are available at : https://devkitpro.org/wiki/Getting_Started

Set up the environment variables if you haven't already:

```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM
```

Then build:

```bash
make clean
make
```

## Running

### Easy Way

Download the 3dsx file from the Releases tab.

Place the 3dsx file in the `3ds/` folder of your 3DS SD card, along with the Scratch project you want to run.
- The Scratch project MUST be named `project.sb3` , all lowercase.

Then it should be as simple as opening the homebrew launcher on your 3DS and running the app.

### Building with Embedded Project

Download the source code from the releases tab and unzip it.

Make a `romfs` folder inside the unzipped source code and put the Scratch project inside of that.
- The Scratch project MUST be named `project.sb3` , all lowercase.
- For faster load times/less limitations, you can also unzip the sb3 project file and put the contents into a new folder called `project`.
- If you would like to change the name of the app, go inside the `Makefile` and edit `APP_TITLE`, `APP_DESCRIPTION` and `APP_AUTHOR` to whatever you please.

Then build using the instructions above.

## Other info

This project is not affiliated with Scratch or the Scratch team.
