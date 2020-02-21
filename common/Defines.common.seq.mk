# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


CFLAGS   += -Wall -pthread -DORIGINAL
ifeq ($(CFG),debug)
  CFLAGS += -O0 -ggdb3 -DTM_DEBUG -fno-omit-frame-pointer
else
  CFLAGS += -O3 -g -DNDEBUG
endif
CFLAGS   += -I$(LIB)
CPPFLAGS += $(CFLAGS)
LD       := $(CC)
LIBS     += -lpthread -lm

# Remove these files when doing clean
OUTPUT +=

LIB := ../lib

# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
