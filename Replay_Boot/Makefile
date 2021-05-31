# Hey Emacs, this is a -*- makefile -*-
#
# WinARM template makefile
# by Martin Thomas, Kaiserslautern, Germany
# <eversmith@heizung-thomas.de>
#
# based on the WinAVR makefile written by Eric B. Weddington, J�rg Wunsch, et al.
# Released to the Public Domain
# Please read the make user manual!
#
# Addons by W. Scherr (ws_arcade <at> pin4.at):
#   - separate build directory for binaries
#   - added build-number-generator for created binaries
#
# On command line:
#
# make all = Make software.
#
# make clean = Clean out built project files.
#
# make program = Download the hex file to the device
#
# (TODO: make filename.s = Just compile filename.c into the assembler code only)
#
# To rebuild project do "make clean" then "make all".
#
# Changelog:
# - 17. Feb. 2005  - added thumb-interwork support (mth)
# - 28. Apr. 2005  - added C++ support (mth)
# - 29. Arp. 2005  - changed handling for lst-Filename (mth)
# -  1. Nov. 2005  - exception-vector placement options (mth)
# - 15. Nov. 2005  - added library-search-path (EXTRA_LIB...) (mth)
# -  2. Dec. 2005  - fixed ihex and binary file extensions (mth)
# - 22. Feb. 2006  - added AT91LIBNOWARN setting (mth)
# - 19. Apr. 2006  - option FLASH_TOOL (default lpc21isp); variable IMGEXT (mth)
# - 23. Jun. 2006  - option USE_THUMB_MODE -> THUMB/THUMB_IW
# -  3. Aug. 2006  - added -ffunction-sections -fdata-sections to CFLAGS
#                    and --gc-sections to LDFLAGS. Only available for gcc 4 (mth)
# -  4. Aug. 2006  - pass SUBMDL-define to frontend (mth)
# - 11. Nov. 2006  - FLASH_TOOL-config, TCHAIN-config (mth)
# - 28. Mar. 2007  - remove .dep-Directory with rm -r -f and force "no error"
# - 24. Apr. 2007  - added "both" option for format (.bin and .hex)
# - 20. Aug. 2007  - extraincdirs in asflags, passing a "board"-define
# hacked about by mikeJ
# hacked again by Wolfgang Scherr

SHELL=bash

# object, list, etc. directory
OUTPUT_DIR = build
$(shell mkdir -p $(OUTPUT_DIR))

# setup host
ifeq ($(OS),Windows_NT)
	HOST = Windows
	APP_SUFFIX = exe
	USBDL = usbdl.exe
	ASTYLE = AStyle.exe
else
	HOST = $(shell uname -s)
	APP_SUFFIX = elf
	USBDL = usbdl_$(HOST).elf
	ASTYLE = astyle_$(HOST)
endif

# build date
VER_DATE:="$(shell date +%Y%m%d)"

##
## Generate a C header file with a macro defining the current git hash (incl dirty state)
##

GIT:=$(shell git describe --tag --always --dirty="+")
GIT_HEADER:=$(OUTPUT_DIR)/git.h
GIT_DEFINE:='\#define GIT $(GIT)'
OLD_DEFINE:='$(shell [[ -e $(GIT_HEADER) ]] && cat $(GIT_HEADER) || echo "nothing")'
$(info GIT = $(GIT))
$(info $(shell [[ $(OLD_DEFINE) != $(GIT_DEFINE) ]] && echo $(GIT_DEFINE) > $(GIT_HEADER)))


# MCU name and submodel
MCU      = arm7tdmi
SUBMDL   = AT91SAM7S256

# Toolchain prefix (i.e arm-elf -> arm-elf-gcc.exe)
TCHAIN_PREFIX  =  arm-none-eabi-
USE_THUMB_MODE = YES
#USE_THUMB_MODE = NO

#USE_NEWLIB = YES
USE_NEWLIB = NO

ifeq (1,${HOSTED})
USE_THUMB_MODE = NO
USE_NEWLIB = YES
endif

ifeq (1,${VIDOR}) 
USE_NEWLIB = YES
endif

## Create ROM-Image (final)
RUN_MODE=ROM_RUN
## Create RAM-Image (debugging)
#RUN_MODE=RAM_RUN

# not used in this example!
## Exception-Vector placement only supported for "ROM_RUN"
## (placement settings ignored when using "RAM_RUN")
## - Exception vectors in ROM:
VECTOR_LOCATION=VECTORS_IN_ROM
## - Exception vectors in RAM:
#VECTOR_LOCATION=VECTORS_IN_RAM

# Target file name (without extension).
TARGET = main
# build directory

# List C source files here. (C dependencies are automatically generated.)
# use file-extension c for "c-only"-files
SRC = common/syscalls.c
SRC += $(wildcard *.c)
SRC += $(wildcard fatfs/*.c)

ifeq (,${HOSTED})
SRC += $(wildcard hardware/*.c)
#SRC += $(wildcard usb/*.c) # mtp is not supported with fatfs
SRC += usb/msc.c usb/usb_hardware.c
SRC += common/Cstartup_SAM7.c
else
SRC += hardware_host/hardware.c usb/msc.c usb/usb_hardware.c hardware_host/embedded.c
endif

# tests
SRC += tests/fullfat-test.c
SRC += tests/exfat-test.c

SRCBIN += ../loader_embedded/loader.bin
SRCRAW += ../loader_embedded/replayhand.raw
SRCTXT += ../loader_embedded/replay.ini

ifeq ($(USE_NEWLIB),NO)
SRC += $(wildcard libc/*.c) $(wildcard libc/stdlib/*.c) $(wildcard libc/string/*.c)
endif

# List C source files here which must be compiled in ARM-Mode.
# use file-extension c for "c-only"-files
SRCARM = hardware/irq.c

# List C++ source files here.
# use file-extension cpp for C++-files (use extension .cpp)
CPPSRC = vidor.cpp

# List C++ source files here which must be compiled in ARM-Mode.
# use file-extension cpp for C++-files (use extension .cpp)
#CPPSRCARM = $(TARGET).cpp
CPPSRCARM =

# List Assembler source files here.
# Make them always end in a capital .S.  Files ending in a lowercase .s
# will not be considered source files but generated files (assembler
# output from the compiler), and will be deleted upon "make clean"!
# Even though the DOS/Win* filesystem matches both .s and .S the same,
# it will preserve the spelling of the filenames, and gcc itself does
# care about how the name is spelled on its command-line.
ASRC =

# List Assembler source files here which must be assembled in ARM-Mode..
ASRCARM  = common/Cstartup.S
ASRCARM += common/swi_handler.S

# Optimization level, can be [0, 1, 2, 3, s].
# 0 = turn off optimization. s = optimize for size.
# (Note: 3 is not always the best optimization level. See avr-libc FAQ.)
OPT = s
#OPT = 0

# Debugging format.
# Native formats for AVR-GCC's -g are stabs [default], or dwarf-2.
# AVR (extended) COFF requires stabs, plus an avr-objcopy run.
DEBUG = stabs
#DEBUG = dwarf-2

# List any extra directories to look for include files here.
#     Each directory must be seperated by a space.
EXTRAINCDIRS  =

# List any extra directories to look for library files here.
#     Each directory must be seperated by a space.
EXTRA_LIBDIRS  = ./common

# Path to Linker-Scripts
LINKERSCRIPTPATH = ./common

# We need a slash for the output directory
BUILDDIR = $(OUTPUT_DIR)/

## Using the Atmel AT91_lib produces warning with
## the default warning-levels.
## yes - disable these warnings; no - keep default settings
AT91LIBNOWARN = yes
#AT91LIBNOWARN = no

# Compiler flag to set the C Standard level.
# c89   - "ANSI" C
# gnu89 - c89 plus GCC extensions
# c99   - ISO C99 standard (not yet fully implemented)
# gnu99 - c99 plus GCC extensions
CSTANDARD = -std=gnu99

# Place -D or -U options for C here
CDEFS =  -D$(RUN_MODE) -DVER_DATE=$(VER_DATE)

# Create small firmware w/o the embedded core
#CDEFS +=  -DFPGA_DISABLE_EMBEDDED_CORE

# Place -I options here
CINCS =

# Place -D or -U options for ASM here
ADEFS =  -D$(RUN_MODE)

ifdef VECTOR_LOCATION
CDEFS += -D$(VECTOR_LOCATION)
ADEFS += -D$(VECTOR_LOCATION)
endif

# Compiler flags.

ifeq ($(USE_THUMB_MODE),YES)
THUMB    = -mthumb
THUMB_IW = -mthumb-interwork
else
THUMB    =
THUMB_IW =
endif

#  -g*:          generate debugging information
#  -O*:          optimization level
#  -f...:        tuning, see GCC manual and avr-libc documentation
#  -Wall...:     warning level
#  -Wa,...:      tell GCC to pass this to the assembler.
#    -adhlns...: create assembler listing
#
# Flags for C and C++ (arm-elf-gcc/arm-elf-g++)
CFLAGS = -g$(DEBUG)
CFLAGS += $(CDEFS) $(CINCS)
CFLAGS += -O$(OPT)
CFLAGS += -Wall -Wcast-align -Wimplicit
CFLAGS += -Wpointer-arith -Wswitch
CFLAGS += -Wredundant-decls -Wreturn-type -Wshadow -Wunused -Werror
ifeq (,${HOSTED})
CFLAGS += -Wa,-adhlns=$(addprefix $(BUILDDIR), $(subst $(suffix $<),.lst,$<))
endif
ifeq ($(USE_NEWLIB),NO)
CFLAGS += -nostdinc -I./libc/include -Wno-attributes -Wno-redundant-decls -D_ANSI_LIBRARY
endif
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -D$(SUBMDL)

# flags only for C
CONLYFLAGS += -Wnested-externs
CONLYFLAGS += $(CSTANDARD)

ifneq ($(AT91LIBNOWARN),yes)
#AT91-lib warnings with:
CFLAGS += -Wcast-qual
CONLYFLAGS += -Wmissing-prototypes
CONLYFLAGS += -Wstrict-prototypes
CONLYFLAGS += -Wmissing-declarations
endif

# flags only for C++ (arm-elf-g++)
# CPPFLAGS = -fno-rtti -fno-exceptions
CPPFLAGS =

# Assembler flags.
#  -Wa,...:    tell GCC to pass this to the assembler.
#  -ahlns:     create listing
#  -g$(DEBUG): have the assembler create line number information
ASFLAGS = $(ADEFS) -Wa,-adhlns=$(addprefix $(BUILDDIR),$(<:.S=.lst)),--g$(DEBUG)


#Additional libraries.

# Extra libraries
#    Each library-name must be seperated by a space.
#    To add libxyz.a, libabc.a and libefsl.a:
#    EXTRA_LIBS = xyz abc efsl
#EXTRA_LIBS = efsl
EXTRA_LIBS = 

#Support for newlibc-lpc (file: libnewlibc-lpc.a)
#NEWLIBLPC = -lnewlib-lpc

MATH_LIB = -lm

# CPLUSPLUS_LIB = -lstdc++


# Linker flags.
#  -Wl,...:     tell GCC to pass this to linker.
#    -Map:      create map file
#    --cref:    add cross reference to  map file
LDFLAGS = -nostartfiles -nostdlib -Wl,-Map=$(BUILDDIR)$(TARGET).map,--cref,--gc-sections
ifeq ($(USE_NEWLIB),YES)
LDFLAGS += -lc
endif
LDFLAGS += -lgcc
LDFLAGS += $(patsubst %,-L%,$(EXTRA_LIBDIRS))
LDFLAGS += $(patsubst %,-l%,$(EXTRA_LIBS))

# Set Linker-Script Depending On Selected Memory and Controller
ifeq ($(RUN_MODE),RAM_RUN)
LDFLAGS +=-T$(LINKERSCRIPTPATH)/$(SUBMDL)-RAM.ld
else
LDFLAGS +=-T$(LINKERSCRIPTPATH)/$(SUBMDL)-ROM.ld
endif

# Define directories, if needed.
## DIRARM = c:/WinARM/
## DIRARMBIN = $(DIRAVR)/bin/
## DIRAVRUTILS = $(DIRAVR)/utils/bin/

# Define programs and commands.
#SHELL = arm-none-eabi-sh
CC = $(TCHAIN_PREFIX)gcc
CPP = $(TCHAIN_PREFIX)g++
OBJCOPY = $(TCHAIN_PREFIX)objcopy
OBJDUMP = $(TCHAIN_PREFIX)objdump
SIZE = $(TCHAIN_PREFIX)size
NM = $(TCHAIN_PREFIX)nm
REMOVE = rm -f
COPY = cp

ifeq (1,${HOSTED})
CC = cc
CPP = c++
OBJCOPY = :
OBJDUMP = objdump
SIZE = size
NM = nm

ifeq (,$(findstring clang,$(shell $(CC) 2>&1)))
CFLAGS += -Wno-unused-but-set-variable -Wno-redundant-decls
endif

SUBMDL=HOSTED

SRCARM =
ASRCARM =
OPT = 0
DEBUG=
LDFLAGS = -lc -lpthread -lncurses

endif

ifeq (1,${VIDOR})
ARDUINO_ROOT?=$(PWD)/.arduino
ARDUINO_PORTABLE=$(ARDUINO_ROOT)/portable/packages/arduino
ARDUINO_BOARD_VERSION=1.8.11
OBJCOPY = $(TCHAIN_PREFIX)objcopy
TCHAIN_PREFIX = $(ARDUINO_PORTABLE)/tools/arm-none-eabi-gcc/7-2017q4/bin/arm-none-eabi-
CFLAGS_WARN = #-Wall -Wextra -Wno-expansion-to-defined
CFLAGS_DEF  = -DF_CPU=48000000L -DARDUINO=10809 -DARDUINO_SAMD_MKRVIDOR4000 -DARDUINO_ARCH_SAMD -D__SAMD21G18A__
CFLAGS_DEF += -DUSB_VID=0x2341 -DUSB_PID=0x8056 -DUSBCON "-DUSB_MANUFACTURER=\"Arduino LLC\"" "-DUSB_PRODUCT=\"Arduino MKR Vidor 4000\"" -DUSE_BQ24195L_PMIC
CFLAGS_INC  = "-I$(ARDUINO_PORTABLE)/tools/CMSIS/4.5.0/CMSIS/Include/"
CFLAGS_INC += "-I$(ARDUINO_PORTABLE)/tools/CMSIS-Atmel/1.2.0/CMSIS/Device/ATMEL/"
CFLAGS_INC += "-I$(ARDUINO_PORTABLE)/hardware/samd/$(ARDUINO_BOARD_VERSION)/cores/arduino"
CFLAGS_INC += "-I$(ARDUINO_PORTABLE)/hardware/samd/$(ARDUINO_BOARD_VERSION)/variants/mkrvidor4000"
CFLAGS_INC += "-I$(ARDUINO_PORTABLE)/hardware/samd/$(ARDUINO_BOARD_VERSION)/libraries/SPI"
CFLAGS_INC += "-I$(ARDUINO_ROOT)/portable/sketchbook/libraries/ArduinoBLE/src"
CFLAGS      = -c -g -Os $(CFLAGS_WARN) $(CFLAGS_DEF) $(CFLAGS_INC) -DVER_DATE=$(VER_DATE)
CONLYFLAGS 	= -std=gnu11 -ffunction-sections -fdata-sections -nostdlib --param max-inline-insns-single=500
CPPFLAGS    = -std=gnu++11 -ffunction-sections -fdata-sections -fno-threadsafe-statics -nostdlib --param max-inline-insns-single=500 -fno-rtti -fno-exceptions
THUMB_IW    =
MCU         = cortex-m0plus
SUBMDL      = ARDUINO_ARCH_SAMD
SRC         = $(wildcard *.c)
SRC        += tests/fullfat-test.c
SRC        += tests/exfat-test.c
SRCARM      =
ASRCARM     =
endif

# Define Messages
# English
MSG_ERRORS_NONE = Errors: none
MSG_BEGIN = "-------- begin (mode: $(RUN_MODE)) --------"
MSG_END = --------  end  --------
MSG_SIZE_BEFORE = Size before:
MSG_SIZE_AFTER = Size after:
MSG_FLASH = Creating load file for Flash:
MSG_EXTENDED_LISTING = Creating Extended Listing:
MSG_SYMBOL_TABLE = Creating Symbol Table:
MSG_LINKING = Linking:
MSG_COMPILING = Compiling C:
MSG_COMPILING_ARM = "Compiling C (ARM-only):"
MSG_COMPILINGCPP = Compiling C++:
MSG_COMPILINGCPP_ARM = "Compiling C++ (ARM-only):"
MSG_ASSEMBLING = Assembling:
MSG_ASSEMBLING_ARM = "Assembling (ARM-only):"
MSG_CLEANING = Cleaning project:
MSG_FORMATERROR = Can not handle output-format
MSG_LPC21_RESETREMINDER = You may have to bring the target in bootloader-mode now.

# Define all object files.
COBJ      = $(addprefix $(BUILDDIR),$(SRC:.c=.o))
COBJARM   = $(addprefix $(BUILDDIR),$(SRCARM:.c=.arm.o))
AOBJ      = $(addprefix $(BUILDDIR),$(ASRC:.S=.o))
AOBJARM   = $(addprefix $(BUILDDIR),$(ASRCARM:.S=.o))
CPPOBJ    = $(addprefix $(BUILDDIR),$(CPPSRC:.cpp=.o))
CPPOBJARM = $(addprefix $(BUILDDIR),$(CPPSRCARM:.cpp=.o))
BINOBJ    = $(addprefix $(BUILDDIR),$(notdir $(SRCBIN:.bin=.o)))
RAWOBJ    = $(addprefix $(BUILDDIR),$(notdir $(SRCRAW:.raw=.o)))
TXTOBJ    = $(addprefix $(BUILDDIR),$(notdir $(SRCTXT:.ini=.o)))
EXTOBJ    = $(BINOBJ) $(RAWOBJ) $(TXTOBJ)
VPATH += $(sort $(dir $(SRCRAW)) $(dir $(SRCBIN)))

# Define all listing files.
LST  = $(addprefix $(BUILDDIR), $(ASRC:.S=.lst) $(ASRCARM:.S=.lst) $(SRC:.c=.lst) $(SRCARM:.c=.lst))
LST += $(addprefix $(BUILDDIR), $(CPPSRC:.cpp=.lst) $(CPPSRCARM:.cpp=.lst))

# Compiler flags to generate dependency files.
### GENDEPFLAGS = -Wp,-M,-MP,-MT,$(*F).o,-MF,.dep/$(@F).d
GENDEPFLAGS = -MD -MP -MF .dep/$(@F).d
DEPMAKEFILE = .dep/Makefile

# Combine all necessary flags and optional flags.
# Add target processor to flags.
ifeq (,${HOSTED})
ALL_CFLAGS  = -mcpu=$(MCU) $(THUMB_IW) -I. $(CFLAGS) $(GENDEPFLAGS)
ALL_ASFLAGS = -mcpu=$(MCU) $(THUMB_IW) -I. -x assembler-with-cpp $(ASFLAGS)
else
ALL_CFLAGS  = -I. $(CFLAGS) $(GENDEPFLAGS)

endif

# Default target.
#all: begin gccversion sizebefore build sizeafter finished end
all: begin $(BUILDDIR)isset gccversion build finished end

usbdl: $(BUILDDIR)$(TARGET).s19
	../Replay_Update/FW/$(USBDL) load $(BUILDDIR)$(TARGET).s19

# set up directory for output files, inluding subdirs for libs
$(BUILDDIR)isset: $(SRC)
	@echo
	mkdir -p $(addprefix $(BUILDDIR), $(sort $(filter-out .,$(^D))))
	@echo ok > $(BUILDDIR)isset

ifeq (1,${HOSTED})
build: elf bin s19 sym size
else ifeq (1,${VIDOR})
build: lib vidor_postproc
verify: all
	$(ARDUINO_ROOT)/arduino --verbose --verify $(BUILDDIR)replay_mkrvidor4000/examples/Replay_Boot/Replay_Boot.ino
install: all
	$(ARDUINO_ROOT)/arduino --verbose --upload $(BUILDDIR)replay_mkrvidor4000/examples/Replay_Boot/Replay_Boot.ino
else
build: elf hex bin s19 lss sym size
install: all usbdl
endif

elf: $(BUILDDIR)$(TARGET).elf
hex: $(BUILDDIR)$(TARGET).hex
bin: $(BUILDDIR)$(TARGET).bin
s19: $(BUILDDIR)$(TARGET).s19
lss: $(BUILDDIR)$(TARGET).lss
sym: $(BUILDDIR)$(TARGET).sym
lib: $(BUILDDIR)libreplay.a

vidor_postproc: lib
	@echo "Creating 'Replay MKR Vidor 4000' library folder under '$(BUILDDIR)replay_mkrvidor4000'"
	@rm -rf "$(BUILDDIR)replay_mkrvidor4000"
	@rm -rf "$(ARDUINO_ROOT)/portable/sketchbook/libraries/replay_mkrvidor4000"
	@ln -s "$(PWD)/$(BUILDDIR)replay_mkrvidor4000" "$(ARDUINO_ROOT)/portable/sketchbook/libraries/replay_mkrvidor4000"
	@cp -r "hardware_vidor/replay_mkrvidor4000" "$(BUILDDIR)replay_mkrvidor4000"
	@BUILDNUM=`git describe --tags --always --abbrev=0`; sed 's/{BUILDNUM}/'"$$BUILDNUM"'/' hardware_vidor/replay_mkrvidor4000/library.properties > $(BUILDDIR)replay_mkrvidor4000/library.properties
	@cp "Replay_Boot.ino" "$(BUILDDIR)replay_mkrvidor4000/examples/Replay_Boot"
	@cp "replay.h" "$(BUILDDIR)replay_mkrvidor4000/src"
	@cp "$(BUILDDIR)libreplay.a" "$(BUILDDIR)replay_mkrvidor4000/src/cortex-m0plus"


# Eye candy.
begin:
	@echo
	@echo $(MSG_BEGIN)

finished:
	@echo $(MSG_ERRORS_NONE)

end:
	@echo $(MSG_END)
	@echo

size: $(BUILDDIR)$(TARGET).elf
	@echo
	@$(SIZE) $<
	@echo

# Display compiler version information.
gccversion :
	@echo
	@$(CC) --version


# Create final output file (.hex) from ELF output file.
$(BUILDDIR)%.hex: $(BUILDDIR)%.elf
	@echo
	@echo $(MSG_FLASH) $@
	@$(OBJCOPY) -Oihex $< $@

# Create final output file (.bin) from ELF output file.
$(BUILDDIR)%.bin: $(BUILDDIR)%.elf
	@echo
	@echo $(MSG_FLASH) $@
	@$(OBJCOPY) -Obinary $< $@

# Create final output file (.s19) from ELF output file.
$(BUILDDIR)%.s19: $(BUILDDIR)%.elf
	@echo
	@echo $(MSG_FLASH) $@
	@$(OBJCOPY) -Osrec --srec-forceS3 $< $@

# Create extended listing file from ELF output file.
# testing: option -C
$(BUILDDIR)%.lss: $(BUILDDIR)%.elf
	@echo
	@echo $(MSG_EXTENDED_LISTING) $@
	@$(OBJDUMP) -h -S -C $< > $@

# Create a symbol table from ELF output file.
$(BUILDDIR)%.sym: $(BUILDDIR)%.elf
	@echo
	@echo $(MSG_SYMBOL_TABLE) $@
	@$(NM) -n $< > $@

# Link: create ELF output file from object files.
.SECONDARY : $(BUILDDIR)$(TARGET).elf
.PRECIOUS : $(AOBJARM) $(AOBJ) $(COBJARM) $(COBJ) $(CPPOBJ) $(CPPOBJARM) $(EXTOBJ)
ifeq (,${HOSTED})
$(BUILDDIR)%.elf:  $(AOBJARM) $(AOBJ) $(COBJARM) $(COBJ) $(CPPOBJ) $(CPPOBJARM) $(EXTOBJ)
	@echo
	@echo $(MSG_LINKING) $@
	@$(CC) $(THUMB) $(ALL_CFLAGS) $(AOBJARM) $(AOBJ) $(COBJARM) $(COBJ) $(CPPOBJ) $(CPPOBJARM) $(EXTOBJ) --output $@ $(LDFLAGS)
#       $(CPP) $(THUMB) $(ALL_CFLAGS) $(AOBJARM) $(AOBJ) $(COBJARM) $(COBJ) $(CPPOBJ) $(CPPOBJARM) --output $@ $(LDFLAGS)
else	# collapse ifeq() once we convert from objcopy to incbin
$(BUILDDIR)%.elf:  $(AOBJARM) $(AOBJ) $(COBJARM) $(COBJ) $(CPPOBJ) $(CPPOBJARM) $(EXTOBJ)
	@echo $(MSG_LINKING) $@
	@$(CC) $(THUMB) $(ALL_CFLAGS) $(AOBJARM) $(AOBJ) $(COBJARM) $(COBJ) $(CPPOBJ) $(CPPOBJARM) --output $@ $(LDFLAGS)
endif

# Archive: create a static archive
$(BUILDDIR)libreplay.a : $(AOBJARM) $(AOBJ) $(COBJARM) $(COBJ) $(CPPOBJ) $(CPPOBJARM) $(EXTOBJ)
	@echo "Archiving and stripping:" $@
	@$(TCHAIN_PREFIX)ar rcs $@ $(AOBJARM) $(AOBJ) $(COBJARM) $(COBJ) $(CPPOBJ) $(CPPOBJARM) $(EXTOBJ)
	@$(TCHAIN_PREFIX)strip --strip-debug $@

# Compile: create object files from C source files. ARM/Thumb
$(COBJ) : $(BUILDDIR)%.o : %.c | $(BUILDDIR)isset
	@echo $(MSG_COMPILING) $<
	@$(CC) -c $(THUMB) $(ALL_CFLAGS) $(CONLYFLAGS) $< -o $@

# Compile: create object files from C source files. ARM-only
$(COBJARM) : $(BUILDDIR)%.arm.o : %.c | $(BUILDDIR)isset
	@echo $(MSG_COMPILING_ARM) $<
	@$(CC) -c $(ALL_CFLAGS) $(CONLYFLAGS) $< -o $@

# Compile: create object files from C++ source files. ARM/Thumb
$(CPPOBJ) : $(BUILDDIR)%.o : %.cpp
	@echo $(MSG_COMPILINGCPP) $<
	@$(CPP) -c $(THUMB) $(ALL_CFLAGS) $(CPPFLAGS) $< -o $@

# Compile: create object files from C++ source files. ARM-only
$(CPPOBJARM) : $(BUILDDIR)%.o : %.cpp
	@echo
	@echo $(MSG_COMPILINGCPP_ARM) $<
	$(CPP) -c $(ALL_CFLAGS) $(CPPFLAGS) $< -o $@

$(BINOBJ) : $(BUILDDIR)%.o : %.bin
$(RAWOBJ) : $(BUILDDIR)%.o : %.raw
$(BINOBJ) $(RAWOBJ): 
	@echo
	@echo Obj-copying compressed binary $<
	@gzip < $< > $(basename $@)
	@$(OBJCOPY) -I binary -O elf32-littlearm -B arm --rename-section .data=.text,alloc,load,readonly,data,contents $(basename $@) $@	

$(TXTOBJ) : $(BUILDDIR)%.o : %.ini
	@echo
	@echo Obj-copying raw $<
	@cd $(dir $<) && $(OBJCOPY) -I binary -O elf32-littlearm -B arm --rename-section .data=.text,alloc,load,readonly,data,contents $(notdir $<) $(abspath $@)	

hardware_host/embedded.c: $(EXTOBJ)
vidor.cpp: $(EXTOBJ)

# Compile: create assembler files from C source files. ARM/Thumb
## does not work - TODO - hints welcome
##$(COBJ) : %.s : %.c
##      $(CC) $(THUMB) -S $(ALL_CFLAGS) $< -o $@


# Assemble: create object files from assembler source files. ARM/Thumb
$(AOBJ) : $(BUILDDIR)%.o : %.S
	@echo
	@echo $(MSG_ASSEMBLING) $<
	$(CC) -c $(THUMB) $(ALL_ASFLAGS) $< -o $@


# Assemble: create object files from assembler source files. ARM-only
$(AOBJARM) : $(BUILDDIR)%.o : %.S
	@echo $(MSG_ASSEMBLING_ARM) $<
	@$(CC) -c $(ALL_ASFLAGS) $< -o $@
	@echo

$(AOBJARM) $(AOBJ) $(COBJARM) $(COBJ) $(CPPOBJ) $(CPPOBJARM): $(DEPMAKEFILE)

$(DEPMAKEFILE): Makefile
	@touch $@

# Target: clean project.
clean: begin clean_list finished end


clean_list : $(SRC)
	@echo
	@echo $(MSG_CLEANING)
	$(REMOVE) -r .dep
	$(REMOVE) $(BUILDDIR)isset
	$(REMOVE) $(BUILDDIR)$(TARGET).hex
	$(REMOVE) $(BUILDDIR)$(TARGET).bin
	$(REMOVE) $(BUILDDIR)$(TARGET).elf
	$(REMOVE) $(BUILDDIR)$(TARGET).map
	$(REMOVE) $(BUILDDIR)$(TARGET).a90
	$(REMOVE) $(BUILDDIR)$(TARGET).sym
	$(REMOVE) $(BUILDDIR)$(TARGET).lnk
	$(REMOVE) $(BUILDDIR)$(TARGET).lss
	$(REMOVE) $(COBJ)
	#$(REMOVE) $(CPPOBJ)
	#$(REMOVE) $(AOBJ)
	$(REMOVE) $(COBJARM)
	#$(REMOVE) $(CPPOBJARM)
	#$(REMOVE) $(AOBJARM)
	$(REMOVE) $(LST)
	$(REMOVE) $(SRC:.c=.s)
	$(REMOVE) $(SRC:.c=.d)
	$(REMOVE) $(SRCARM:.c=.s)
	$(REMOVE) $(SRCARM:.c=.d)
	$(REMOVE) $(CPPSRC:.cpp=.s)
	$(REMOVE) $(CPPSRC:.cpp=.d)
	# $(REMOVE) $(CPPSRCARM:.cpp=.s)
	# $(REMOVE) $(CPPSRCARM:.cpp=.d)
	$(REMOVE) $(EXTOBJ)
	$(REMOVE) $(EXTOBJ:.o=)
	$(REMOVE) $(GIT_HEADER)
	$(REMOVE) -r $(addprefix $(BUILDDIR), $(sort $(filter-out .,$(^D))))
	git clean -fd build/ .dep/

astyle :
	../tools/AStyle/bin/$(ASTYLE) --options=../astyle.opt -n "*.h" "*.c"
	../tools/AStyle/bin/$(ASTYLE) --options=../astyle.opt -n "hardware/*.c" "hardware/*.h" 
	../tools/AStyle/bin/$(ASTYLE) --options=../astyle.opt -n "hardware_host/*.c"
	../tools/AStyle/bin/$(ASTYLE) --options=../astyle.opt -n "hardware_vidor/*.c" # "hardware_vidor/*.h" "hardware_vidor/*.cpp"
	../tools/AStyle/bin/$(ASTYLE) --options=../astyle.opt -n "libc/*"
	../tools/AStyle/bin/$(ASTYLE) --options=../astyle.opt -n "tests/*.c" "tests/*.h"
#	../tools/AStyle/bin/$(ASTYLE) --options=../astyle.opt -n "usb/*.c" "usb/*.h"

# Include the dependency files.
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)


# Listing of phony targets.
.PHONY : all begin finish end sizebefore sizeafter gccversion \
build elf hex bin s19 lss sym clean clean_list program size

