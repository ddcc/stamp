# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


CFLAGS   += -Wall -pthread
ifeq ($(CFG),debug)
  CFLAGS += -O0 -ggdb3 -fno-omit-frame-pointer
else
  CFLAGS += -O3 -ggdb3 -DNDEBUG
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

STM := ../../tinySTM

# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
