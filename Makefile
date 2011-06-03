# GNU Makefile

build ?= debug

default: all

include Makerules

OUT := build/$(build)

QUIET_CC = @ echo "   " CC $@ ;
QUIET_AR = @ echo "   " AR $@ ;
QUIET_LINK = @ echo "   " LINK $@ ;

CC_CMD = $(QUIET_CC) $(CC) -o $@ -c $< $(CFLAGS)
AR_CMD = $(QUIET_AR) $(AR) cru $@ $^
LINK_CMD = $(QUIET_LINK) $(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

HDRS := $(wildcard *.h)
OBJS := $(addprefix $(OUT)/, \
	$(SYS_OBJ) sysevent.o rune.o runetype.o strlcpy.o \
	glext.o font.o image.o vector.o \
	model_obj.o model_iqm.o model_iqe.o \
	terrain.o \
	untitled.o )
APP := $(OUT)/untitled

$(OUT) :
	mkdir -p $@

$(OUT)/%.o : %.c $(HDRS) | $(OUT)
	$(CC_CMD)
$(OUT)/%.o : %.m $(HDRS) | $(OUT)
	$(CC_CMD)

$(OUT)/font.o : font.c $(HDRS) | $(OUT)
	$(CC_CMD) $(FT_CFLAGS)

$(APP) : $(OBJS) | $(OUT)
	$(LINK_CMD)

all: $(APP)

clean:
	rm -rf $(OUT)
