

						The Last Mission Replica
				for Win32, Dos32, Linux and Dingoo Native OS
                		                  v0.6, 18 March 2012

					Dmitry Smagin exmortis[_at_]yandex.ru

 DESCRIPTION

The Last Mission is a side-view arcade game without scrolling (viewpoint moves from 
screen to screen) with map of big dimensions. The game takes its inspiration from 
games such as Underwurlde and Starquake.

You control a tank-like robot which can be divided in two: you rotate caterpillar and 
head-cannon, and the head part can fly off on its own. However, the head can only 
survive separately for a short amount of time, and your restart position is dictated by the 
location of the body, even if the head has moved forward through further screens. 
Therefore, the difficulty of the game was in making it possible to advance with the 
assembled robot's two parts.


 INFORMATION

This started as an exact replica of the self-booter PC version. Later the 4-color CGA graphics
was redrawn to 256 colors (MSX2 version was the example). Adlib is used for sound and music to retain retro feel.

 CONTROLS

For Dos32, Win32 native/SDL and Linux SDL use:

ARROWS	- move
SPACE	- fire
ENTER	- pause
ESCAPE	- quit
S	- toggle scaler x1 or x2 (SDL and Win32 GDI)
F	- toggle fullscreen/windowed (SDL)

For Dingoo A320 use:
D-PAD	- move
A/B/X/Y	- fire
START	- start game or pause
SELECT	- quit
L	- toggle fullscreen upscale no/coarse/bilinear (slow)
R	- toggle frameskip 0/1

The source is made as cross-platform as possible and could be built for the following systems:


1) PMODE/W DOS Extender using OpenWatcom
    VGA 320x200 fullscreen only (of course), real adlib or compatible for sound.
    Tested successfully on: Win98, WinXP + VDMSound/SoundFX2000 (music is slower though), DosBox 0.65 or higher.
    On plain 64-bit Windows systems this port will NOT run in any way except in an emulator.

2) Win32 gdi+winmm using OpenWatcom or MinGW
    GDI32 is used for graphics and adlib soft emulator + WinMM for sound. Only windowed
    mode is supported with 2 upscale renderers: system gdi32 (slow on some machines)
    and soft.
    Tested on: WinNT 4.0 (sound doesn't work), Win98, Win2000, WinXP, Win7 64-bit, Linux+Wine1.3 (works really well)

2.5) Win32 SDL using MinGW
    SDL is used for everything, windowed x1 or x2, fullscreen.
    Tested on: Win98, Win2000, WinXP, Win7 64-bit, Linux+Wine1.3 (sound lags)

3) Linux SDL using GCC (32-bit)
    SDL is used for everything, windowed x1 or x2, fullscreen.
    Tested on: Xubuntu 10.10 (32-bit), Xubuntu 11.11, Puppylinux 5

4) Dingoo A320 Native OS using unofficial Dingoo SDK and mipsel-gcc toolchain
    SDL port for Native OS is used, video 320x240x16 and fullscreen only.
    Tested on: Dingoo A320 :)

5) Dingux for A320, A380 and RZX-50 using SiENcE's mipsel-linux-uclibc-gcc toolchain
    SDL is used, optional bilinear upscale to 480x272 (RZX-50) or 400x240 (A380).
    Tested on: Dingoo A320, Ritmix RZX-50, may work on Dingoo A380
