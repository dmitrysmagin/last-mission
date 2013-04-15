# make
# make -B -f makefile.dingoo.mak

# Define the applications properties here:

APP_NAME = last-mission



# Define the compiler settings here:

CPP       = mipsel-linux-g++
CC        = mipsel-linux-gcc
LD        = mipsel-linux-ld

SOURCE    = .

INCLUDE   = -I$(DINGOO_SDK)/include -I$(DINGOO_SDK)/include/SDL -I../lib -I$(MIPSTOOLS)/mipsel-linux/include -I$

W_OPTS    = -mips32 -finline-functions -fomit-frame-pointer -msoft-float -mno-abicalls -fno-pic -fno-builtin -fno-exceptions
CC_OPTS   = -Wall -Wextra -G0 -O3 $(INCLUDE) $(W_OPTS) -D_DEBUG -DMPU_JZ4740 -c -std=c99 -fms-extensions -D__cdecl="" -D__DINGOO__
CC_OPTS   += -DNDEBUG=1  ## allow assert() calls to be removed, if left fails to build with Dingoo native

LIB_PATH  = $(DINGOO_SDK)/lib
LIBS      =  -lSDL -lm -lsml -lc -ljz4740 -lgcc

LD_SCRIPT = $(DINGOO_SDK)/lib/dingoo.xn
LD_OPTS   = -nodefaultlibs --script $(LD_SCRIPT) -L$(LIB_PATH) $(LIBS) -o $(APP_NAME).elf



# Find all source files

SRC_C   = fmopl.c m_aux.c m_core.c m_data.c m_demo.c m_gfx_data.c m_gfx_dingoosdl.c m_scr.c m_scr_lines.c m_snd.c m_snd_data.c m_snd_sdl.c
OBJ_C   = $(patsubst %.c, %.o, $(SRC_C))
OBJ     = $(OBJ_C)



# Compile rules.

.PHONY : all

all : $(APP_NAME).app

$(APP_NAME).app : $(APP_NAME).elf
	$(DINGOO_SDK)/tools/elf2app/elf2app $(APP_NAME)

$(APP_NAME).elf : $(OBJ)
	$(LD) $^ $(LD_OPTS)

$(OBJ_C) : %.o : %.c
	$(CC) $(CC_OPTS) -o $@ $<


# Clean rules

.PHONY : clean

clean :
	rm -f $(OBJ) $(APP_NAME).elf $(APP_NAME).bin $(APP_NAME).app
