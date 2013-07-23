

						The Last Mission SDL
					for Win32, Linux and OpenDingux
						v0.8, 23 July 2013

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
was redrawn to 256 colors (MSX2 version was the example). New features like new weapons, docks and
bonuses were added in v0.7 thanks to Alexey Pavlov. Since version 0.8 digital sound
effects and music is used from Mark Braga (thanks).

 CONTROLS

For Win32 and Linux SDL use:

ARROWS	- move
SPACE	- fire
ENTER	- pause
ESCAPE	- quit
S	- toggle scaler x1 or x2
F	- toggle fullscreen/windowed

For OpenDingux SDL use:
D-PAD	- move
A/B/X/Y	- fire
START	- start game or pause
SELECT	- quit
L	- toggle fullscreen upscale no/coarse/bilinear (slow)
R	- toggle frameskip 0/1

The source is made as cross-platform as possible and could be built for all systems featuring SDL
