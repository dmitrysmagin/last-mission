
APP_NAME = last-mission-sdl

# compiler

CC = gcc

CFLAGS = -Wall -O2 -std=c99 -fms-extensions
LFLAGS = -s -lSDL_gfx -lSDL -lSDL_mixer -lm

# source files

OBJ =	m_data.o m_scr.o m_scr_lines.o
OBJ +=	demo.o engine.o input.o main.o random.o room.o sound.o sprites.o video.o

all : $(APP_NAME)

$(APP_NAME) : $(OBJ)
	$(CC) $^ $(LFLAGS) -o $@

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm -rf ./*.o $(APP_NAME)
