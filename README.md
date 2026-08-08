# The Last Mission SDL

**Version 0.8, 23 July 2013**

**Author:** Dmitry Smagin (exmortis at yandex.ru)

**Platforms:** Win32, Linux, OpenDingux

## Description

The Last Mission is a side-view arcade game without scrolling (viewpoint moves from screen to screen) with a map of big dimensions. The game takes its inspiration from games such as *Underwurlde* and *Starquake*.

You control a tank-like robot which can be divided in two: you rotate the caterpillar and head-cannon, and the head part can fly off on its own. However, the head can only survive separately for a short amount of time, and your restart position is dictated by the location of the body, even if the head has moved forward through further screens. Therefore, the difficulty of the game lies in making it possible to advance with both parts of the assembled robot.

## Information

This started as an exact replica of the self-booter PC version. Later the 4-color CGA graphics were redrawn to 256 colors (MSX2 version was used as the example). New features like new weapons, docks, and bonuses were added in v0.7 thanks to Alexey Pavlov. Since version 0.8, digital sound effects and music from Mark Braga are included.

## Controls

### Win32 and Linux (SDL)

| Key | Action |
|-----|--------|
| Arrows | Move |
| Space | Fire |
| Enter | Pause |
| Escape | Quit |
| S | Toggle scaler (x1 or x2) |
| F | Toggle fullscreen/windowed |

### OpenDingux (SDL)

| Button | Action |
|--------|--------|
| D-Pad | Move |
| A / B / X / Y | Fire |
| Start | Start game or pause |
| Select | Quit |
| L | Toggle fullscreen upscale (no/coarse/bilinear) |
| R | Toggle frameskip (0/1) |

## Building

The source is made as cross-platform as possible and can be built for all systems featuring SDL.
