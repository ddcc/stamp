# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


CFLAGS   += -Wall -pthread
ifeq ($(CFG),debug)
  CFLAGS += -O0 -ggdb3 -DTM_DEBUG -fno-omit-frame-pointer
else
  CFLAGS += -O3 -g -DNDEBUG
endif
ifeq ($(ORIGINAL),1)
  CFLAGS += -DORIGINAL
endif
CFLAGS   += -I$(LIB)
CPPFLAGS += $(CFLAGS)
LD       := $(CC)
LIBS     += -lpthread -lm

# Remove these files when doing clean
OUTPUT +=

LIB := ../lib

STM := ../../tardisTM

LOSTM := ../../OpenTM/lostm


# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
