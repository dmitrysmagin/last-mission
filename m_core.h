#ifndef _M_CORE_H_
#define _M_CORE_H_

#define KEY_LEFT	0
#define KEY_RIGHT	1
#define KEY_UP		2
#define KEY_DOWN	3
#define KEY_FIRE	4
#define KEY_PAUSE	5
#define KEY_QUIT	6

#define SC_ESCAPE	0x01
#define SC_1		0x02
#define SC_2		0x03
#define SC_3		0x04
#define SC_4		0x05
#define SC_5		0x06
#define SC_6		0x07
#define SC_7		0x08
#define SC_8		0x09
#define SC_9		0x0A
#define SC_0		0x0B
#define SC_MINUS	0x0C
#define SC_EQUALS	0x0D
#define SC_BACKSPACE	0x0E
#define SC_TAB		0x0F
#define SC_Q		0x10
#define SC_W		0x11
#define SC_E		0x12
#define SC_R		0x13
#define SC_T		0x14
#define SC_Y		0x15
#define SC_U		0x16
#define SC_I		0x17
#define SC_O		0x18
#define SC_P		0x19
#define SC_LEFTBRACKET	0x1A
#define SC_RIGHTBRACKET	0x1B
#define SC_ENTER	0x1C
#define SC_CONTROL	0x1D
#define SC_A		0x1E
#define SC_S		0x1F
#define SC_D		0x20
#define SC_F		0x21
#define SC_G		0x22
#define SC_H		0x23
#define SC_J		0x24
#define SC_K		0x25
#define SC_L		0x26
#define SC_SEMICOLON	0x27
#define SC_QUOTE	0x28
#define SC_TILDE	0x29
#define SC_LSHIFT	0x2A
#define SC_BACKSLASH	0x2B
#define SC_Z		0x2C
#define SC_X		0x2D
#define SC_C		0x2E
#define SC_V		0x2F
#define SC_B		0x30
#define SC_N		0x31
#define SC_M		0x32
#define SC_COMMA	0x33
#define SC_PERIOD	0x34
#define SC_SLASH	0x35
#define SC_RSHIFT	0x36
#define SC_MULTIPLY	0x37
#define SC_ALT		0x38
#define SC_SPACE	0x39
#define SC_CAPSLOCK	0x3A
#define SC_F1		0x3B
#define SC_F2		0x3C
#define SC_F3		0x3D
#define SC_F4		0x3E
#define SC_F5		0x3F
#define SC_F6		0x40
#define SC_F7		0x41
#define SC_F8		0x42
#define SC_F9		0x43
#define SC_F10		0x44
#define SC_NUMLOCK	0x45
#define SC_SCROLLLOCK	0x46
#define SC_HOME		0x47
#define SC_UP		0x48
#define SC_PAGEUP	0x49
#define SC_MINUS2   0x4A
#define SC_LEFT		0x4B
#define SC_RIGHT	0x4D
#define SC_PLUS		0x4E
#define SC_END		0x4F
#define SC_DOWN		0x50
#define SC_PAGEDOWN	0x51
#define SC_INSERT	0x52
#define SC_DELETE	0x53
#define SC_F11		0x57
#define SC_F12		0x58

#define SC_LWIN		0x7D
#define SC_RWIN		0x7E
#define SC_MENU		0x7F

#define GM_EXIT 	(-1)
#define GM_TITLE	0
#define GM_GAME		1
#define GM_CUT		2
#define GM_GAMEOVER	3
#define GM_YOUWIN	4
#define GM_DEMO		5
#define GM_PAUSE	6
#define GM_SPLASH	7

#define SND_LASER_SHOOT			1
#define SND_SHORT_LASER_SHOOT	2
#define SND_ROCKET_SHOOT		3
#define SND_CANNON_SHOOT		4
#define SND_EXPLODE				5
#define SND_CONTACT				6
#define SND_MOVE				7
#define SND_ELEVATOR			8
#define SND_BONUS				9

#define MUSIC_STOP		0
#define MUSIC_INTRO		1
#define MUSIC_GAME		2
#define MUSIC_WIN		MUSIC_INTRO
#define MUSIC_LOSE		MUSIC_INTRO

#define MAX_GARAGES 16

typedef struct TGAMEDATA {
    int ship_score;
    int base_level;
    int num_lives;
    int fuel;
    int health;
    int easy_level;
	int hidden_level_entered;
    int garages[MAX_GARAGES][2];
} TGAMEDATA;

#endif // _M_CORE_H_