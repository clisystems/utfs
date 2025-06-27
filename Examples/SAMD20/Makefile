#==========================================================
# Makefile of project
#==========================================================


TARGET = ARM
BOARD ?= SAMD20_BOB1

# Basic system
C_COMPILE = 
RFS_ROOT=
NAME = test
DFLAGS ?=
OPT ?= -O0
GCC_PREFIX = 

ifeq ($(BOARD),SAMD20_BOB1)
	TOOLCHAIN_DIR = $(HOME)/programs/gcc-arm-none-eabi-10.3-2021.10
	GCC_PREFIX = arm-none-eabi-
	C_COMPILE = $(TOOLCHAIN_DIR)/bin/$(GCC_PREFIX)
	RFS_ROOT = $(TOOLCHAIN_DIR)/arm-none-eabi/
	EDBG_PATH = $(HOME)/programs
	
	# change this to match the processor and linker file
	STACK_SIZE = 4096
	RAM_SIZE = 16384
	FLASH_SIZE = 131072
	
	DFLAGS += -D__ARM__
	DFLAGS += -D ARM_MATH_CM0PLUS=true
	DFLAGS += -D BOARD_SAMD20_BOB1
	DFLAGS += -D SYSTICK_MODE
	DFLAGS += -D __SAMD20J17__
	
	#CFLAGS += -D=__samd20j17a__
	CFLAGS += -mcpu=cortex-m0plus
	
	PART = samd20j17
	
endif


# Setup output parameters
PATH_TARGET = 
PATH_OBJ    = bin/
CC 			= $(C_COMPILE)gcc
CXX 		= $(C_COMPILE)g++
OBJCOPY 	= $(C_COMPILE)objcopy
SIZE 		= $(C_COMPILE)size
AS 			= $(C_COMPILE)as
STRIP 		= $(C_COMPILE)strip

CFLAGS += -Wall -Werror-implicit-function-declaration 
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -std=gnu99 
CFLAGS += -g $(OPT)
CFLAGS += -Wno-unused-function -Wno-unused-variable  -Wno-unused-but-set-variable
CFLAGS += -Wno-format
#CFLAGS += -Wno-pointer-to-int-cast -Wno-switch
# Prototype errors
#CFLAGS += -Wstrict-prototypes -Wmissing-prototypes
DFLAGS += -DDEBUG


# AFS related items
######################################################
CFLAGS += -Xlinker -Map=$(PATH_OBJ)/$(NAME).map

ASF_PATH = $(HOME)/programs/xdk-asf-3.52.0


ASF_INC += $(ASF_PATH)/common/utils
ASF_INC += $(ASF_PATH)/sam0/drivers/extint
ASF_INC += $(ASF_PATH)/sam0/drivers/extint/extint_sam_d_r_h
ASF_INC += $(ASF_PATH)/sam0/drivers/port
ASF_INC += $(ASF_PATH)/sam0/drivers/nvm
ASF_INC += $(ASF_PATH)/sam0/drivers/sercom
ASF_INC += $(ASF_PATH)/sam0/drivers/sercom/usart
ASF_INC += $(ASF_PATH)/sam0/drivers/sercom/spi
ASF_INC += $(ASF_PATH)/sam0/drivers/system
ASF_INC += $(ASF_PATH)/sam0/drivers/system/clock
ASF_INC += $(ASF_PATH)/sam0/drivers/system/interrupt
ASF_INC += $(ASF_PATH)/sam0/drivers/system/pinmux
ASF_INC += $(ASF_PATH)/sam0/drivers/system/power
ASF_INC += $(ASF_PATH)/sam0/drivers/system/power/power_sam_d_r_h
ASF_INC += $(ASF_PATH)/sam0/drivers/system/reset
ASF_INC += $(ASF_PATH)/sam0/drivers/system/reset/reset_sam_d_r_h
ASF_INC += $(ASF_PATH)/sam0/drivers/tc
ASF_INC += $(ASF_PATH)/sam0/drivers/tc/tc_sam_d_r_h
ASF_INC += $(ASF_PATH)/sam0/utils
ASF_INC += $(ASF_PATH)/sam0/utils/header_files
ASF_INC += $(ASF_PATH)/sam0/utils/preprocessor
ASF_INC += $(ASF_PATH)/thirdparty/CMSIS/Include
ASF_INC += $(ASF_PATH)/thirdparty/CMSIS/Lib/GCC

# Different sources for diffrerent CPUs
ifeq ($(BOARD),SAMD20_BOB1)
ASF_INC += $(ASF_PATH)/sam0/drivers/system/clock/clock_samd20
ASF_INC += $(ASF_PATH)/sam0/drivers/system/interrupt/system_interrupt_samd20
ASF_INC += $(ASF_PATH)/sam0/utils/cmsis/samd20/include
ASF_INC += $(ASF_PATH)/sam0/utils/cmsis/samd20/source
endif


# Additional search paths for libraries.
ASF_LIB_PATH = -L $(ASF_PATH)/thirdparty/CMSIS/Lib/GCC

# List of libraries to use during linking.
#ASF_LIBS = -l arm_cortexM0l_math

# Different linker scripts for diffrerent CPUs
ifeq ($(BOARD),SAMD20_BOB1)
ASF_LINKER_SCRIPT_FLASH = samd20j17_flash.ld
endif

# Assembly Startup File
ASF_ASM_FILES = 

# C files
#$(ASF_PATH)/sam0/drivers/extint/extint_callback.c \
       $(ASF_PATH)/sam0/drivers/sercom/sercom_interrupt.c             \
       $(ASF_PATH)/sam0/drivers/sercom/usart/usart_interrupt.c        \
       $(ASF_PATH)/sam0/drivers/tc/tc_interrupt.c                     \
       $(ASF_PATH)/sam0/utils/stdio/read.c                            \
       $(ASF_PATH)/sam0/utils/stdio/write.c                           \
       $(ASF_PATH)/common2/services/delay/sam0/systick_counter.c      \
		$(ASF_PATH)/sam0/drivers/sercom/spi/spi.c                      \
       $(ASF_PATH)/sam0/drivers/extint/extint_sam_d_r_h/extint.c      \		                            
ASF_C_FILES = \
       $(ASF_PATH)/common/utils/interrupt/interrupt_sam_nvic.c        \
       $(ASF_PATH)/sam0/drivers/sercom/sercom.c                       \
       $(ASF_PATH)/sam0/drivers/nvm/nvm.c	                          \
       $(ASF_PATH)/sam0/drivers/sercom/usart/usart.c                  \
       $(ASF_PATH)/sam0/drivers/tc/tc_sam_d_r_h/tc.c                  \
       $(ASF_PATH)/sam0/drivers/port/port.c                           \
       $(ASF_PATH)/sam0/drivers/system/interrupt/system_interrupt.c   \
       $(ASF_PATH)/sam0/drivers/system/pinmux/pinmux.c                \
       $(ASF_PATH)/sam0/drivers/system/system.c                       \
       $(ASF_PATH)/sam0/utils/syscalls/gcc/syscalls.c \
 
 # Different sources for diffrerent CPUs
ifeq ($(BOARD),SAMD20_BOB1)
ASF_C_FILES += $(ASF_PATH)/sam0/utils/cmsis/samd20/source/gcc/startup_samd20.c
ASF_C_FILES += $(ASF_PATH)/sam0/utils/cmsis/samd20/source/system_samd20.c
ASF_C_FILES += $(ASF_PATH)/sam0/drivers/system/clock/clock_samd20/clock.c
ASF_C_FILES += $(ASF_PATH)/sam0/drivers/system/clock/clock_samd20/gclk.c
endif

# Make objects
ASF_OBJ_FILES = $(patsubst %.c,%.o,$(ASF_C_FILES))
ASF_OBJ_FILES+= $(patsubst %.s,%.o,$(ASF_ASM_FILES))


# Programmer parameters
######################################################
ifeq ($(BOARD),SAMD20_BOB1)
EDBG_PARAMS  = -t samd20
endif


# Tinyprint2 from Prosource
######################################################
DFLAGS += -D ENABLE_MINIPRINTF
CFLAGS += -fno-builtin-printf -fno-builtin-sprintf 

# Directories and files
######################################################
DIRS = ./ ./lib/ #./drivers/ ./system/

#Find all c/cpp files, skip OLD/ directory
SRC_FILES = $(shell find . -name '*.c' -not -path "./OLD/*")
CXX_SRC_FILES = $(shell find . -name '*.cpp' -not -path "./OLD/*)
OBJ_FILES = $(subst ./,$(PATH_OBJ),$(patsubst %.c,%.o,$(SRC_FILES)))
CXX_OBJ_FILES = $(subst ./,$(PATH_OBJ),$(patsubst %.cpp,%.o,$(CXX_SRC_FILES)))

# Defaults
######################################################
# Need to include headers for libraries
INCLUDE += $(foreach dir,$(DIRS),-I $(dir))
INCLUDE += $(foreach INC,$(ASF_INC),-I $(INC))

# Need to link in libs
LIBS = $(ASF_LIBS)

LIB_PATH += $(ASF_LIB_PATH)

# Use libs
LDFLAGS += --specs=nosys.specs

LDFLAGS += -lc -lm -lgcc

# Linker flags
LNFLAGS = -Wl,--gc-sections
LNFLAGS += -Wl,-T $(ASF_LINKER_SCRIPT_FLASH)
 
# Sections and rules
######################################################

# Remove SILENT to see the build cmd
.SILENT:

.PHONY: all all-pre all-post clean new hex bin size size2

all: all-pre $(NAME).elf hex bin size size2 all-post
	
all-pre:
	echo "   -- Building $(NAME).elf"
	echo "   -- Opt: $(OPT)"
	echo "   -- GCC: $(CC)"
	echo "   -- BOARD: $(BOARD)"
	mkdir -p $(foreach dir,$(DIRS),$(PATH_OBJ)$(dir))
	
all-post:
#	echo "Done..."

$(NAME).elf:  $(OBJ_FILES) $(FREERTOS_OBJ) $(ASF_OBJ_FILES) 
	echo "  LNK $@"
	$(CC) $(CFLAGS) $(LNFLAGS) -s -o $(PATH_TARGET)$@ $(foreach dir,$(DIRS),$(PATH_OBJ)$(dir)*.o) $(ASF_OBJ_FILES) $(LIB_PATH) $(LIBS) $(LDFLAGS)

hex:
	echo "  HEX $(NAME).hex"
	$(OBJCOPY) -O ihex $(NAME).elf $(NAME).hex

bin:
	echo "  BIN $(NAME).bin"
	$(OBJCOPY) -O binary $(NAME).elf $(NAME).bin

size:
	echo " SIZE"
	$(SIZE) -G $(NAME).elf
	
size2:
	echo "SIZE2"
	
	$(SIZE) -G $(NAME).elf |grep "^[[:space:]]*[0-9]"|awk '{ \
	print "      Prog Data RAM  : " $$2 " bytes"; \
	print "      Prog Blank RAM : " $$3-$(STACK_SIZE) " bytes"; \
	print "      Fixed Stack RAM: " $(STACK_SIZE) " bytes"; \
	print " ";\
	print "      RAM Used       : " $$3+$$2 " bytes of " $(RAM_SIZE) " bytes "; \
	print "      \033[1mRAM used       : " int((($$3+$$2)/$(RAM_SIZE))*10000)/100 "%\033[0m"; \
	print "      RAM remaining  : " $(RAM_SIZE)-$$3-$$2 " bytes"; \
	print " ";\
	print "      Prog Flash     : " $$1 " bytes"; \
	print "      \033[1mFLASH used     : " int(( ($$1+$$2) /$(FLASH_SIZE))*10000)/100 "% (" ($$1+$$2) " of " $(FLASH_SIZE) ")\033[0m"; \
	print " ";\
	print "      \033[1mBIN size       : " $$1+$$2 " bytes (Code+Data RAM)" "(" $$1 "+" $$2 ")\033[0m"; \
	print "      FLASH remaining: " int( $(FLASH_SIZE) - ($$1+$$2)) " bytes"; \
	}'

%.o:%.c
	echo "   CC LIB/$(shell basename $@)"
	$(CC) $(CFLAGS) $(INCLUDE) $(DFLAGS) -c -o $@ $<

%.o:%.s
	echo "  ASM LIB/$(shell basename $@)"
	$(CC) $(DFLAGS) -c -o $@ $<
	
$(PATH_OBJ)%.o: %.c
	echo "   CC $@"
	$(CC) $(CFLAGS) $(INCLUDE) $(DFLAGS) -c -o $@ $<

$(PATH_OBJ)%.o: %.cpp
	echo "  CXX $@"
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDE) $(DFLAGS) -c -o $@ $<

download:
	echo " DNLD $(NAME).bin"
	sudo $(EDBG_PATH)/edbg $(EDBG_PARAMS) -b -s 0 -p -f $(NAME).bin

new: clean all

clean:
	echo "   -- Cleaning"
	echo "   RM .o"
	rm -rf $(ASF_OBJ_FILES)
	rm -rf $(NAME).elf
	rm -rf $(NAME).bin
	rm -rf $(NAME).hex
	rm -rf $(PATH_OBJ)*
	rm -f $(PATH_TARGET)$(OUTLIB) 
	rm -rf $(PATH_OBJ)
