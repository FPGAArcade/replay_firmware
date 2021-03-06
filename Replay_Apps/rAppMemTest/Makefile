# Hey Emacs, this is a -*- makefile -*-
#
# $Id: Makefile 216 2013-12-15 21:08:38Z wolfgang.scherr $
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

# object, list, etc. directory
OUTPUT_DIR = build

# build number
BUILD_NUM = $$(./buildnum)

# MCU name and submodel
MCU      = arm7tdmi
SUBMDL   = AT91SAM7S256

# Toolchain prefix (i.e arm-elf -> arm-elf-gcc.exe)
TCHAIN =  arm-none-eabi
FLASH_TOOL = OPENOCD
#USE_THUMB_MODE = YES
USE_THUMB_MODE = NO

## Create ROM-Image (final)
#RUN_MODE=ROM_RUN
## Create RAM-Image (debugging)
RUN_MODE=RAM_RUN

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
SRC = $(TARGET).c
SRC += ../rAppLib/common/syscalls.c
SRC += ../rAppLib/hardware.c
SRC += ../rAppLib/fpga.c
SRC += ../rAppLib/osd.c
SRC += ../rAppLib/printf.c
SRC += ../rAppLib/common/Cstartup_SAM7.c

# List C source files here which must be compiled in ARM-Mode.
# use file-extension c for "c-only"-files
SRCARM =

# List C++ source files here.
# use file-extension cpp for C++-files (use extension .cpp)
CPPSRC =

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
ASRCARM  = ../rAppLib/common/Cstartup.S
ASRCARM += ../rAppLib/common/swi_handler.S

## Output format. (can be s19, ihex or binary)
## (binary i.e. for openocd and SAM-BA, hex i.e. for lpc21isp and uVision)
#FORMAT = s19
#FORMAT = ihex
FORMAT = binary

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
EXTRAINCDIRS  = ../rAppLib/common ../rAppLib

# List any extra directories to look for library files here.
#     Each directory must be seperated by a space.
EXTRA_LIBDIRS  = ../rAppLib/common ../rAppLib

# Path to Linker-Scripts
LINKERSCRIPTPATH = ../rAppLib/common

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
CDEFS =  -D$(RUN_MODE)

# Place -I options here
CINCS =

# Place -D or -U options for ASM here
ADEFS =  -D$(RUN_MODE)

ifdef VECTOR_LOCATION
CDEFS += -D$(VECTOR_LOCATION)
ADEFS += -D$(VECTOR_LOCATION)
endif

CDEFS += -D__WinARM__
ADEFS += -D__WinARM__

CDEFS += -D__BUILDNUMBER__=\"$(BUILD_NUM)\"

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
CFLAGS += -Wredundant-decls -Wreturn-type -Wshadow -Wunused
CFLAGS += -Wa,-adhlns=$(addprefix $(BUILDDIR), $(subst $(suffix $<),.lst,$<))
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))

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
LDFLAGS = -nostartfiles -Wl,-Map=$(BUILDDIR)$(TARGET).map,--cref,--gc-sections
LDFLAGS += -lc
LDFLAGS += $(NEWLIBLPC) $(MATH_LIB)
LDFLAGS += -lc -lgcc
LDFLAGS += $(CPLUSPLUS_LIB)
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
CC = arm-none-eabi-gcc
CPP = arm-none-eabi-g++
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size
NM = arm-none-eabi-nm
REMOVE = rm -f
COPY = cp

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
COBJARM   = $(addprefix $(BUILDDIR),$(SRCARM:.c=.o))
AOBJ      = $(addprefix $(BUILDDIR),$(ASRC:.S=.o))
AOBJARM   = $(addprefix $(BUILDDIR),$(ASRCARM:.S=.o))
CPPOBJ    = $(addprefix $(BUILDDIR),$(CPPSRC:.cpp=.o))
CPPOBJARM = $(addprefix $(BUILDDIR),$(CPPSRCARM:.cpp=.o))

# Define all listing files.
LST  = $(addprefix $(BUILDDIR), $(ASRC:.S=.lst) $(ASRCARM:.S=.lst) $(SRC:.c=.lst) $(SRCARM:.c=.lst))
LST += $(addprefix $(BUILDDIR), $(CPPSRC:.cpp=.lst) $(CPPSRCARM:.cpp=.lst))

# Compiler flags to generate dependency files.
### GENDEPFLAGS = -Wp,-M,-MP,-MT,$(*F).o,-MF,.dep/$(@F).d
GENDEPFLAGS = -MD -MP -MF .dep/$(@F).d

# Combine all necessary flags and optional flags.
# Add target processor to flags.
ALL_CFLAGS  = -mcpu=$(MCU) $(THUMB_IW) -I. $(CFLAGS) $(GENDEPFLAGS)
ALL_ASFLAGS = -mcpu=$(MCU) $(THUMB_IW) -I. -x assembler-with-cpp $(ASFLAGS)


# Default target.
#all: begin gccversion sizebefore build sizeafter finished end
all: begin $(BUILDDIR)/isset gccversion build finished end

# set up directory for output files, inluding subdirs for libs
$(BUILDDIR)/isset:
	mkdir -p $(addprefix $(BUILDDIR), $(EXTRA_LIBDIRS))
	echo ok > $(BUILDDIR)/isset
#	mkdir -p $(OUTDIR)

ifeq ($(FORMAT),ihex)
build: elf hex lss sym
hex: $(BUILDDIR)$(TARGET).hex
IMGEXT=hex
else
ifeq ($(FORMAT),binary)
build: elf bin lss sym
bin: $(BUILDDIR)$(TARGET).bin
IMGEXT=bin
else
ifeq ($(FORMAT),s19)
build: elf s19 lss sym
s19: $(BUILDDIR)$(TARGET).s19
IMGEXT=s19
else
$(error "$(MSG_FORMATERROR) $(FORMAT)")
endif
endif
endif

elf: $(BUILDDIR)$(TARGET).elf
lss: $(BUILDDIR)$(TARGET).lss
sym: $(BUILDDIR)$(TARGET).sym

# Eye candy.
begin:
	@echo
	@echo $(MSG_BEGIN)
	@touch main.c

finished:
	@echo $(MSG_ERRORS_NONE)

end:
	@echo $(MSG_END)
	@echo


# Display compiler version information.
gccversion :
	@$(CC) --version


# Create final output file (.hex) from ELF output file.
$(BUILDDIR)%.hex: $(BUILDDIR)%.elf
	@echo
	@echo $(MSG_FLASH) $@
	$(OBJCOPY) -O $(FORMAT) $< $@

# Create final output file (.bin) from ELF output file.
$(BUILDDIR)%.bin: $(BUILDDIR)%.elf
	@echo
	@echo $(MSG_FLASH) $@
	$(OBJCOPY) -O $(FORMAT) $< $@

# Create final output file (.s19) from ELF output file.
$(BUILDDIR)%.s19: $(BUILDDIR)%.elf
	@echo
	@echo $(MSG_FLASH) $@
	$(OBJCOPY) -Osrec --srec-forceS3 $< $@


# Create extended listing file from ELF output file.
# testing: option -C
$(BUILDDIR)%.lss: $(BUILDDIR)%.elf
	@echo
	@echo $(MSG_EXTENDED_LISTING) $@
	$(OBJDUMP) -h -S -C $< > $@


# Create a symbol table from ELF output file.
$(BUILDDIR)%.sym: $(BUILDDIR)%.elf
	@echo
	@echo $(MSG_SYMBOL_TABLE) $@
	$(NM) -n $< > $@


# Link: create ELF output file from object files.
.SECONDARY : $(BUILDDIR)$(TARGET).elf
.PRECIOUS : $(AOBJARM) $(AOBJ) $(COBJARM) $(COBJ) $(CPPOBJ) $(CPPOBJARM)
$(BUILDDIR)%.elf:  $(AOBJARM) $(AOBJ) $(COBJARM) $(COBJ) $(CPPOBJ) $(CPPOBJARM)
	@echo
	@echo $(MSG_LINKING) $@
	$(CC) $(THUMB) $(ALL_CFLAGS) $(AOBJARM) $(AOBJ) $(COBJARM) $(COBJ) $(CPPOBJ) $(CPPOBJARM) --output $@ $(LDFLAGS)
#       $(CPP) $(THUMB) $(ALL_CFLAGS) $(AOBJARM) $(AOBJ) $(COBJARM) $(COBJ) $(CPPOBJ) $(CPPOBJARM) --output $@ $(LDFLAGS)

# Compile: create object files from C source files. ARM/Thumb
$(COBJ) : $(BUILDDIR)%.o : %.c
	@echo
	@echo $(MSG_COMPILING) $<
	$(CC) -c $(THUMB) $(ALL_CFLAGS) $(CONLYFLAGS) $< -o $@

# Compile: create object files from C source files. ARM-only
$(COBJARM) : $(BUILDDIR)%.o : %.c
	@echo
	@echo $(MSG_COMPILING_ARM) $<
	$(CC) -c $(ALL_CFLAGS) $(CONLYFLAGS) $< -o $@

# Compile: create object files from C++ source files. ARM/Thumb
$(CPPOBJ) : $(BUILDDIR)%.o : %.cpp
	@echo
	@echo $(MSG_COMPILINGCPP) $<
	$(CPP) -c $(THUMB) $(ALL_CFLAGS) $(CPPFLAGS) $< -o $@

# Compile: create object files from C++ source files. ARM-only
$(CPPOBJARM) : $(BUILDDIR)%.o : %.cpp
	@echo
	@echo $(MSG_COMPILINGCPP_ARM) $<
	$(CPP) -c $(ALL_CFLAGS) $(CPPFLAGS) $< -o $@


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
	@echo
	@echo $(MSG_ASSEMBLING_ARM) $<
	$(CC) -c $(ALL_ASFLAGS) $< -o $@


# Target: clean project.
clean: begin clean_list finished end


clean_list :
	@echo
	@echo $(MSG_CLEANING)
	$(REMOVE) $(TARGET).hex
	$(REMOVE) $(TARGET).bin
	$(REMOVE) $(TARGET).s19
	$(REMOVE) $(TARGET).BUILDDIR
	$(REMOVE) $(TARGET).elf
	$(REMOVE) $(TARGET).map
	$(REMOVE) $(TARGET).BUILDDIR
	$(REMOVE) $(TARGET).a90
	$(REMOVE) $(TARGET).sym
	$(REMOVE) $(TARGET).lnk
	$(REMOVE) $(TARGET).lss
	$(REMOVE) $(COBJ)
	$(REMOVE) $(CPPOBJ)
	$(REMOVE) $(AOBJ)
	$(REMOVE) $(COBJARM)
	$(REMOVE) $(CPPOBJARM)
	$(REMOVE) $(AOBJARM)
	$(REMOVE) $(LST)
	$(REMOVE) $(SRC:.c=.s)
	$(REMOVE) $(SRC:.c=.d)
	$(REMOVE) $(SRCARM:.c=.s)
	$(REMOVE) $(SRCARM:.c=.d)
	$(REMOVE) $(CPPSRC:.cpp=.s)
	$(REMOVE) $(CPPSRC:.cpp=.d)
	$(REMOVE) $(CPPSRCARM:.cpp=.s)
	$(REMOVE) $(CPPSRCARM:.cpp=.d)


# Include the dependency files.
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)


# Listing of phony targets.
.PHONY : all begin finish end sizebefore sizeafter gccversion \
build elf hex bin s19 lss sym clean clean_list program

