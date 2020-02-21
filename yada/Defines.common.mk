# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


CFLAGS += -DLIST_NO_DUPLICATES
CFLAGS += -DMAP_USE_RBTREE
CFLAGS += -DSET_USE_RBTREE
CFLAGS += -DMERGE_HEAP -DMERGE_LIST -DMERGE_RBTREE # -DMERGE_QUEUE -DMERGE_ELEMENT -DMERGE_MESH -DMERGE_REGION

PROG := yada
SRCS += \
	coordinate.c \
	element.c \
	mesh.c \
	region.c \
	yada.c \
	$(LIB)/heap.c \
	$(LIB)/list.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/pair.c \
	$(LIB)/queue.c \
	$(LIB)/random.c \
	$(LIB)/rbtree.c \
	$(LIB)/thread.c \
	$(LIB)/vector.c \
#
OBJS := ${SRCS:.c=.o}


# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
