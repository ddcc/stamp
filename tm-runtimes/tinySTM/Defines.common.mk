# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


CFLAGS   += -Wall -pthread
ifeq ($(CFG),debug)
  CFLAGS += -O0 -ggdb3
else
  CFLAGS += -O3 -DNDEBUG -g
endif
CFLAGS   += -I$(LIB)
CPPFLAGS += $(CFLAGS)
LD       := $(CC)
LIBS     += -lpthread -lm

# Remove these files when doing clean
OUTPUT +=

LIB := ../lib

STM := ../../tinySTM

LOSTM := ../../OpenTM/lostm


# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
