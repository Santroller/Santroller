#
#             LUFA Library
#     Copyright (C) Dean Camera, 2012.
#
#  dean [at] fourwalledcubicle [dot] com
#           www.lufa-lib.org
#
# --------------------------------------
#         LUFA Project Makefile.
# --------------------------------------

ARCH         = AVR8
OPTIMIZATION = s
F_USB		 = ${F_CPU}

VERSION = $(shell git describe --abbrev=0 --tags)
VERSION_LIST = $(subst ., ,$(subst version-,,$(VERSION)))
VERSION_MAJOR = $(word 1,$(VERSION_LIST))
VERSION_MINOR = $(word 2,$(VERSION_LIST))
VERSION_REVISION = $(word 3,$(VERSION_LIST))
SIGNATURE = ardwiino
MULTI_ADAPTOR=$(if $(findstring -multi,$(EXTRA)),-DDMULTI_ADAPTOR,)
SRC += ${PROJECT_ROOT}/src/shared/controller/guitar_includes.c ${PROJECT_ROOT}/src/avr/lib/bootloader/bootloader.c
LUFA_PATH    = ${PROJECT_ROOT}/lib/lufa/LUFA
CC_FLAGS     += -DUSE_LUFA_CONFIG_HEADER -I${PROJECT_ROOT}/src/shared/output -I${PROJECT_ROOT}/src/avr/shared -I${PROJECT_ROOT}/src/avr/variants/${VARIANT} -I ${PROJECT_ROOT}/src/shared -I ${PROJECT_ROOT}/src/shared/lib -I${PROJECT_ROOT}/lib -I${PROJECT_ROOT}/src/avr/lib -Werror $(REGS) -DARDUINO=1000  -flto -fuse-linker-plugin -ffast-math
CC_FLAGS     += -DARDWIINO_BOARD='"${ARDWIINO_BOARD}"' 
CC_FLAGS 	 += -DSIGNATURE='"${SIGNATURE}"' -DVERSION='"${VERSION}"' ${MULTI_ADAPTOR} -DVERSION_MAJOR='${VERSION_MAJOR}' -DVERSION_MINOR='${VERSION_MINOR}' -DVERSION_REVISION='${VERSION_REVISION}' -DMCU='"${MCU}"'
LD_FLAGS     += $(REGS) -DARDUINO=1000 -DMCU='"${MCU}"' -flto -fuse-linker-plugin 
OBJDIR		 = obj
BIN		 	 = bin
TARGET       = $(BIN)/ardwiino-${ARDWIINO_BOARD}$(if ${MCU_TYPE},-${MCU_TYPE},)-${MCU}-${F_CPU}${MULTI}$(if ${RF},-rf,)
# ----- No changes should be necessary below this line -----
$(info $(SRC))
$(info $(shell mkdir -p $(BIN)))

# Default target
all:

# Reserved registers for faster USB-Serial convertion
# The lower, the better (except 0 and 1)
# r2-5 used for faster USART ISRs
# r6 used for 32u4 optimization
REGS  = -ffixed-r2 -ffixed-r3 -ffixed-r4 -ffixed-r5 -ffixed-r6
# Include LUFA build script makefiles
include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_cppcheck.mk
include $(LUFA_PATH)/Build/lufa_doxygen.mk
include $(LUFA_PATH)/Build/lufa_dfu.mk
include $(LUFA_PATH)/Build/lufa_hid.mk
include $(LUFA_PATH)/Build/lufa_avrdude.mk
include $(LUFA_PATH)/Build/lufa_atprogram.mk