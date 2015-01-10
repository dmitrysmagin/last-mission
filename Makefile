
APP_NAME = last-mission-sdl

# compiler

CC = gcc

CFLAGS = -Wall -O2 -std=c99 -fms-extensions
LFLAGS = -s -lSDL -lSDL_mixer -lm

# source files

OBJ =	m_core.o m_aux.o m_demo.o m_data.o m_gfx_data.o \
	m_scr.o m_scr_lines.o

#OBJ +=	fmopl.o m_snd.o m_snd_data.o m_snd_sdl.o sound_old.o
OBJ +=	sound.o

OBJ +=	m_gfx_sdl.o

all : $(APP_NAME)

$(APP_NAME) : $(OBJ)
	$(CC) $^ $(LFLAGS) -o $@

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm -rf ./*.o $(APP_NAME)
