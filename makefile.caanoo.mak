#
# This will make a Linux executable using GNU GCC
#
# make -f makefile.caanoo.mak to create SDL build
#
#

APP_NAME = last-mission-sdl

# compiler

CC = arm-gph-linux-gnueabi-gcc

CFLAGS = -Wall -O1 -std=c99 -fms-extensions -D__GCC__ -D__cdecl="" -D__CAANOO__ -D__JOYSTICK__
LFLAGS = -lSDL -Wl,--strip-all

# source files

SRC = fmopl.c m_core.c m_aux.c m_snd.c m_demo.c	m_data.c m_snd_data.c m_scr.c
SRC := $(SRC) m_gfx_sdl.c m_snd_sdl.c


INC = $(foreach dir, ., $(wildcard $(dir)/*.h))
OBJ = $(patsubst %.c, %.o, $(SRC))

all : $(APP_NAME)

$(APP_NAME) : $(OBJ)
	$(CC) $^ $(LFLAGS) -o $@

%.o : %.c $(INC)
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf ./*.o $(APP_NAME)
