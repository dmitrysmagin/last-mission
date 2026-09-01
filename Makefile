
APP_NAME = last-mission-sdl

# compiler

CC = gcc

SDL2 ?= 1

CFLAGS = -Wall -O2 -std=c99 -fms-extensions -Isrc
LFLAGS = -s -lm

SRC = src

ifeq ($(SDL2),1)
CFLAGS += -DSDL2
LFLAGS += -lSDL2 -lSDL2_mixer
COMPAT = $(SRC)/sdl_compat.o
else
LFLAGS += -lSDL -lSDL_mixer
COMPAT =
endif

OBJ =	$(SRC)/data.o $(SRC)/demo.o $(SRC)/editor.o $(SRC)/engine.o $(SRC)/engine-scrolling.o \
	$(SRC)/engine-flipscreen.o $(SRC)/engine-scroller.o $(SRC)/engine-coordinates.o \
	$(SRC)/engine-collision.o \
	$(SRC)/input.o $(SRC)/main.o $(SRC)/random.o $(SRC)/room.o \
	$(SRC)/sound.o $(SRC)/sprites.o $(SRC)/video.o $(SRC)/world.o \
	$(SRC)/object.o $(SRC)/object_bfg.o $(SRC)/object_enemy.o \
	$(SRC)/object_garage.o $(SRC)/object_laser.o $(SRC)/timer.o \
	$(COMPAT)

all : $(APP_NAME)

$(APP_NAME) : $(OBJ)
	$(CC) $^ $(LFLAGS) -o $@

$(SRC)/%.o : $(SRC)/%.c
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm -rf $(SRC)/*.o $(APP_NAME)
