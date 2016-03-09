
APP_NAME = last-mission-sdl

# compiler

CC = gcc

CFLAGS = -Wall -O2 -std=c99 -fms-extensions
LFLAGS = -s -lSDL -lSDL_mixer -lm

# source files

OBJ =	data.o demo.o editor.o engine.o input.o main.o random.o \
	room.o sound.o sprites.o video.o world.o object.o \
	object_bfg.o object_enemy.o object_garage.o \
	object_laser.o timer.o

all : $(APP_NAME)

$(APP_NAME) : $(OBJ)
	$(CC) $^ $(LFLAGS) -o $@

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm -rf ./*.o $(APP_NAME)
