
APP_NAME = last-mission-sdl

# compiler

CC = gcc

CFLAGS = -Wall -O2 -std=c99 -fms-extensions -Isrc
LFLAGS = -s -lSDL -lSDL_mixer -lm

# source files

SRC = src
OBJ =	$(SRC)/data.o $(SRC)/demo.o $(SRC)/editor.o $(SRC)/engine.o \
	$(SRC)/input.o $(SRC)/main.o $(SRC)/random.o $(SRC)/room.o \
	$(SRC)/sound.o $(SRC)/sprites.o $(SRC)/video.o $(SRC)/world.o \
	$(SRC)/object.o $(SRC)/object_bfg.o $(SRC)/object_enemy.o \
	$(SRC)/object_garage.o $(SRC)/object_laser.o $(SRC)/timer.o

all : $(APP_NAME)

$(APP_NAME) : $(OBJ)
	$(CC) $^ $(LFLAGS) -o $@

$(SRC)/%.o : $(SRC)/%.c
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm -rf $(SRC)/*.o $(APP_NAME)
