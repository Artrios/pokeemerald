TOOLCHAIN := $(DEVKITARM)
COMPARE ?= 0
MODERN ?= 0

ifeq (compare,$(MAKECMDGOALS))
  COMPARE := 1
endif

# don't use dkP's base_tools anymore
# because the redefinition of $(CC) conflicts
# with when we want to use $(CC) to preprocess files
# thus, manually create the variables for the bin
# files, or use arm-none-eabi binaries on the system
# if dkP is not installed on this system

ifneq (,$(TOOLCHAIN))
ifneq ($(wildcard $(TOOLCHAIN)/bin),)
export PATH := $(TOOLCHAIN)/bin:$(PATH)
endif
endif

HOSTCC ?= cc
PREFIX := arm-none-eabi-
#AR = arm-none-eabi-ar
#AS = arm-none-eabi-as
CC = arm-none-eabi-gcc
AS := $(PREFIX)as
AR := $(PREFIX)ar
LD := $(PREFIX)ld
OBJCOPY := $(PREFIX)objcopy

MODERNCC := $(PREFIX)gcc
PATH_MODERNCC := PATH="$(PATH)" $(MODERNCC)


ifeq ($(MODERN),0)
CC1             := ../tools/agbcc/bin/agbcc$(EXE)
override CFLAGS += -I$(AGBDIR)/include -Wimplicit -Wparentheses -Werror -O2 -g
else
CC1              = $(shell $(PATH_MODERNCC) --print-prog-name=cc1) -quiet
override CFLAGS += -I$(AGBDIR)/include -mthumb -mthumb-interwork -O2 -mtune=arm7tdmi -march=armv4t -fno-toplevel-reorder -Wno-pointer-to-int-cast
endif


OFILES = ma_bios.o ma_api.o ma_sub.o md5c.o ma_ango.o ma_var.o
LIB = libma.a
AGBDIR = ./agb

ASFLAGS = -I$(AGBDIR)/include -mthumb-interwork
ARFLAGS = rs

.SUFFIXES: $(SUFFIXES) .dat
.dat.o:
	$(OBJCOPY) -I binary -O elf32-littlearm $< $@
.c.s:
	$(CC1) $(CFLAGS) -S $<

$(LIB): $(OFILES)
	$(AR) $(ARFLAGS) $@ $^

clean:
	rm -f $(LIB) $(OFILES) ardata ardata.exe

# disable warnings
md5c.o: CFLAGS := $(CFLAGS) -w

# dependence
ma_bios.o: $(wildcard *.h)
ma_api.o: $(wildcard *.h)
ma_sub.o: $(wildcard *.h)
md5c.o: $(wildcard *.h)
ma_ango.o: $(wildcard *.h)
ma_var.o: $(wildcard *.h)