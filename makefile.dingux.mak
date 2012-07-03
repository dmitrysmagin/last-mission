#
# 
# makefile for creating dingux version
# 
# make -B -f makefile.dingux.mak
#

APP_NAME = last-mission.dge

# compiler

CC = mipsel-linux-uclibc-gcc

WFLAGS = -Wall -Wextra -march=mips32
CFLAGS := $(WFLAGS) -O3 -std=c99 -fms-extensions -D__GCC__ -D__cdecl="" -D__DINGUX__
LFLAGS = -lSDL -Wl,--strip-all

# source files

SRC = fmopl.c m_core.c m_aux.c m_snd.c m_demo.c	m_data.c m_snd_data.c m_scr.c
SRC := $(SRC) m_gfx_dingoosdl.c m_snd_sdl.c m_gfx_data.c


INC = $(foreach dir, ., $(wildcard $(dir)/*.h))
OBJ = $(patsubst %.c, %.o, $(SRC))

all : $(APP_NAME)

$(APP_NAME) : $(OBJ)
	$(CC) $^ $(LFLAGS) -o $@

%.o : %.c $(INC)
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm -rf ./*.o $(APP_NAME)
