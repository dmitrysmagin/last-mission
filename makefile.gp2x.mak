#
# This will make a Linux executable using GNU GCC
#
# make -f makefile.gp2x.mak to create SDL build
#
#

APP_NAME = last-mission-sdl

# compiler

CC = arm-open2x-linux-gcc

CFLAGS = -Wall -O1 -std=c99 -fms-extensions -D__GCC__ -D__cdecl="" -D__GP2X__ -D__JOYSTICK__
LFLAGS = -static -L/opt/open2x/gcc-4.1.1-glibc-2.3.6/lib/ -lSDL -lm -lpthread -Wl,--strip-all
INCLUDE = -I/opt/open2x/gcc-4.1.1-glibc-2.3.6/include/

# source files

SRC = fmopl.c m_core.c m_aux.c m_snd.c m_demo.c	m_data.c m_snd_data.c m_scr.c
SRC := $(SRC) m_scr_lines.c m_gfx_sdl.c m_snd_sdl.c


INC = $(foreach dir, ., $(wildcard $(dir)/*.h))
OBJ = $(patsubst %.c, %.o, $(SRC))

all : $(APP_NAME)

$(APP_NAME) : $(OBJ)
	$(CC) $^ $(LFLAGS) -o $@

%.o : %.c $(INC)
	$(CC) -c $(CFLAGS) $(INCLUDE) $< -o $@

clean :
	rm -rf ./*.o $(APP_NAME)
