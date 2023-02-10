# Include the core environment definitions; this will set $(TOP).
include ../../py/mkenv.mk

# Include py core make definitions.
include $(TOP)/py/py.mk

# Set makefile-level MicroPython feature configurations.
MICROPY_ROM_TEXT_COMPRESSION ?= 0
MICROPY_USE_READLINE ?= 1
#CFLAGS += -DMICROPY_USE_READLINE=1 
#-DMICROPY_USE_READLINE_HISTORY=1

# Define toolchain and other tools.
CROSS_COMPILE ?= arm-none-eabi-
DFU ?= $(TOP)/tools/dfu.py
PYDFU ?= $(TOP)/tools/pydfu.py

# we want the linker to be the gcc not ld
LD = $(CROSS_COMPILE)gcc

# Set CFLAGS.
CFLAGS += -I. -I$(TOP) -I$(BUILD) -Icmsis -Ispl -DSYS_NS -DUSE_STDPERIPH_DRIVER -DGD32W515TX
CFLAGS += -Wall -Werror -std=c99 -nostdlib
CFLAGS += -mthumb -mcpu=cortex-m33 -mfloat-abi=softfp -march=armv8-m.main+dsp+fp
CSUPEROPT = -Os # save some code space for performance-critical code

# Select debugging or optimisation build.
ifeq ($(DEBUG), 1)
CFLAGS += -Og -ggdb3
else
CFLAGS += -Os -DNDEBUG
CFLAGS += -fdata-sections -ffunction-sections
endif

# Set linker flags.
LDFLAGS += -mthumb -mcpu=cortex-m33 -mfloat-abi=softfp -march=armv8-m.main+dsp+fp
LDFLAGS += --specs=nosys.specs --specs=nano.specs -T cmsis/GD32W515TI_DEFAULT.ld  -Wl,--gc-sections,--relax
ifeq ($(DEBUG), 1)
LDFLAGS += -ggdb3
endif


# Define the required source files.
SRC_C += main.c system.c $(BUILD)/_frozen_mpy.c
SRC_C += $(wildcard cmsis/*.c)
SRC_C += $(wildcard spl/*.c)


APP_SHARED_SRC_C = $(addprefix shared/,\
	readline/readline.c \
    runtime/gchelper_native.c \
	runtime/pyexec.c \
	)

SRC_S += $(wildcard cmsis/*.s)

SRC_QSTR += shared/readline/readline.c shared/runtime/pyexec.c

# Define the required object files.
OBJ += $(PY_CORE_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_S:.s=.o))
OBJ += $(addprefix $(BUILD)/, $(APP_SHARED_SRC_C:.c=.o))

# Define the top-level target, the main firmware.
all: $(BUILD)/firmware.bin

$(BUILD)/_frozen_mpy.c: $(TOP)/tests/frozen/frozentest.mpy $(BUILD)/genhdr/qstrdefs.generated.h
	$(ECHO) "MISC freezing bytecode"
	$(Q)$(TOP)/tools/mpy-tool.py -f -q $(BUILD)/genhdr/qstrdefs.preprocessed.h -mlongint-impl=none $< > $@

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $^
	$(Q)$(SIZE) $@

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(ECHO) "Create $@"
	$(Q)$(OBJCOPY) -O binary $^ $@

deploy: $(BUILD)/firmware.dfu
	$(Q)$(PYTHON) $(PYDFU) -u $^

# Include remaining core make rules.
include $(TOP)/py/mkrules.mk
