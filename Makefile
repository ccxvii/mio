# GNU Makefile

build ?= debug

default: all

include Makerules

OUT := build/$(build)

ifeq "$(verbose)" ""
QUIET_CC = @ echo "   " CC $@ ;
QUIET_AR = @ echo "   " AR $@ ;
QUIET_LINK = @ echo "   " LINK $@ ;
endif

CC_CMD = $(QUIET_CC) $(CC) -o $@ -c $< $(CFLAGS)
AR_CMD = $(QUIET_AR) $(AR) cru $@ $^
LINK_CMD = $(QUIET_LINK) $(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

HDRS := $(wildcard *.h)
OBJS := $(addprefix $(OUT)/, \
	$(SYS_OBJ) sysevent.o rune.o runetype.o strlcpy.o \
	glew.o font.o image.o vector.o shader.o \
	model_obj.o model_iqm.o \
	terrain.o console.o )
UNTITLED := $(OUT)/untitled
VIEWER := $(OUT)/viewer

$(OUT) :
	mkdir -p $@

$(OUT)/%.o : %.c $(HDRS) | $(OUT)
	$(CC_CMD)
$(OUT)/%.o : %.m $(HDRS) | $(OUT)
	$(CC_CMD)

$(OUT)/font.o : font.c $(HDRS) | $(OUT)
	$(CC_CMD) $(FT_CFLAGS)

$(UNTITLED) : $(OBJS) $(OUT)/untitled.o | $(OUT)
	$(LINK_CMD)

$(VIEWER) : $(OBJS) $(OUT)/viewer.o | $(OUT)
	$(LINK_CMD)

all: $(UNTITLED) $(VIEWER)

clean:
	rm -rf $(OUT)
