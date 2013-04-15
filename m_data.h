#define NUM_SCREENS 101
#define NUM_PATTERNS 124

extern unsigned char SkyMap[9][21];
extern unsigned char LOGO[];
extern unsigned char STATUSBAR1[7*43];
extern unsigned char *SCREENS[NUM_SCREENS];
extern unsigned char *PATTERNS[NUM_PATTERNS];
extern unsigned char *SCREENINFOS[NUM_SCREENS];
extern short *SCREENLINES[NUM_SCREENS];

#define SH_DEAD 0
#define SH_ACTIVE 1

#define AI_STATIC						0 // breakable wall or non-moving enemy
#define AI_RANDOM_MOVE					1 // randomly moving enemy
#define AI_KAMIKADZE					2
#define AI_ELECTRIC_SPARKLE_VERTICAL	3 // electric sparkle going up-down
#define AI_CEILING_CANNON				4 // ceiling cannon spawning kamikazes
#define AI_HOMING_MISSLE				5
#define AI_CANNON						6
#define AI_ELECTRIC_SPARKLE_HORIZONTAL	7 // electric sparkle going left-right
#define AI_EXPLOSION					8 // does one animation cycle
#define AI_BRIDGE						9 // appears if bonded ship, disappear otherwise
#define AI_BULLET						10
#define AI_ELEVATOR						11
#define AI_SMOKE						12
#define AI_BONUS						13
#define AI_SHOT							14
#define AI_GARAGE						15
#define AI_SPARE_SHIP					16
#define AI_HOMING_SHOT					17
#define AI_HIDDEN_AREA_ACCESS			18
#define AI_BFG_SHOT						19

#define GARAGE_WIDTH 48
#define GARAGE_HEIGHT 18

#define SHIP_TYPE_LASER 0
#define SHIP_TYPE_MACHINE_GUN 51
#define SHIP_TYPE_ROCKET_LAUNCHER 53
#define SHIP_TYPE_OBSERVER 8
#define SHIP_TYPE_BFG 55

#define BONUS_FACEBOOK 47
#define BONUS_TWITTER 48
#define BONUS_HP 52