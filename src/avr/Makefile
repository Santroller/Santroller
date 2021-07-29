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

SIGNATURE = ardwiino
SRC = ${VARIANT_ROOT}/main.c
include ${VARIANT_ROOT}/makefile.mk
LUFA_PATH    = ${PROJECT_ROOT}/submodules/lufa/LUFA
CC_FLAGS     += -DUSE_LUFA_CONFIG_HEADER -I${PROJECT_ROOT}/src/avr/lib -I${PROJECT_ROOT}/src/lib -Werror $(REGS) -DARDUINO=1000  -flto -fuse-linker-plugin -ffast-math
CC_FLAGS     += -DARDWIINO_BOARD='"${ARDWIINO_BOARD}"' -DVARIANT=${VARIANT}
CC_FLAGS 	 += -DSIGNATURE='"${SIGNATURE}"' -DVERSION='"${VERSION}"' ${MULTI_ADAPTOR} -DVERSION_MAJOR='${VERSION_MAJOR}' -DVERSION_MINOR='${VERSION_MINOR}' -DVERSION_REVISION='${VERSION_REVISION}' -DMCU='"${MCU}"' -DSTK_VERSION=${STK_VERSION} -DPINS_PER_PORT=7
LD_FLAGS     += $(REGS) -flto -fuse-linker-plugin 
OBJDIR		 = obj
BIN		 	 = bin
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