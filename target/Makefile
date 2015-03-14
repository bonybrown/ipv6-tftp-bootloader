#
# Makefile for dsPIC
#

TARGET	= boot
PROJECT_SOURCE=  main.c serial.c network.c i2c.c mc24aa00.c enc28j60.c timer.c

CPU := 33FJ128GP204


# check mplab version
MPLAB_VER := $(shell xc16-gcc -dumpversion)


# check C30 support files version. linker scripts were deficient for this device before 330
C30_VER_MIN := 1021
C30_VER := $(shell xc16-gcc --version | grep VERSION | cut -c 21-)
C30_OK := $(filter $(C30_VER_MIN),$(firstword $(sort  $(C30_VER) $(C30_VER_MIN))))
ifndef C30_OK 
${error C30 version is $(C30_VER) - require $(C30_VER_MIN) or above. Check PATH or upgrade}
endif

# grab path to MPLAB support files (cygwin, on win32 only)
MPLABC_BINW=$(shell dirname "`which xc16-gcc`")
MPLABC_HOME=$(shell dirname "$(MPLABC_BINW)")
#MPLABC_HOME=$(shell cygpath -m "$(MPLABC_HOMEW)")

### Compiler definitions
CC       = xc16-gcc -omf=elf
LD       = xc16-gcc -omf=elf 
AS       = xc16-as
AR       = xc16-ar
OBJCOPY  = xc16-objcopy
STRIP    = xc16-strip
BIN2HEX  = xc16-bin2hex


## -I/usr/local/include

CFLAGSNO = -mcpu=$(CPU) -c -Wall -I"$(MPLABC_HOME)/support/dsPIC33F/h" -ffunction-sections -D__dsPIC$(CPU)__=1
CFLAGS  += $(CFLAGSNO)
LDFLAGS = -mcpu=$(CPU) -Wl,--gc-sections,-Tp$(CPU).gld,--report-mem,--Map="$(TARGET).map",--heap=128

## Flags MPLABX applies to xc16-gcc (not yet applied)
#	--defsym=__MPLAB_DEBUG=1, --defsym=__MPLAB_BUILD=1,
#  dependancy annotation:   -MMD -MF <blah>.o.d

## Microchip linker discards _main symbol from librarian .a file, so cannot link
#  the result   http://www.microchip.com/forums/m550276-print.aspx
# This causes much longer builds as Contiki cannot be pre-compiled to archived elf format

CUSTOM_RULE_ALLOBJS_TO_TARGETLIB=1
CUSTOM_RULE_LINK=1


PROJECT_OBJECTFILES=	  $(PROJECT_SOURCE:.c=.o)

all:	ccvercheck $(PROJECT_OBJECTFILES)
	$(LD) $(LDFLAGS) $(PROJECT_OBJECTFILES) -o $(TARGET) 
	$(BIN2HEX) $(TARGET) -omf=elf

ccvercheck:
	$(info xc16-gcc version is '$(MPLAB_VER)' recommend 4.5.1)

#.PRECIOUS: %.$(TARGET).hex %.$(TARGET).elf %.$(TARGET).out 

## rewrite end taget:  .hex <--bin2hex-- .out  <--linker--  *.co 
#$(TARGET).out: %.co $(PROJECT_OBJECTFILES) $(PROJECT_LIBRARIES)
#	$(LD) $(LDFLAGS) $(TARGET_STARTFILES) ${filter-out %.a,$^} ${filter %.a,$^} $(TARGET_LIBFILES) -o $@

#$(TARGET).hex: $(TARGET).out
#	$(BIN2HEX) $^ -omf=elf

#%: %.$(TARGET).hex
#	@

clean:	
	$(RM) $(TARGET).map $(TARGET).bin $(TARGET).hex $(TARGET) $(PROJECT_OBJECTFILES)

OUT=$(shell readlink -f *.hex)

prog:
	pk2cmd -r -p -m -f$(OUT)

bin:
	srec_cat $(OUT) -Intel -Exclude 0x01F00000 0x02000000 -Output $(TARGET).bin -Binary
	