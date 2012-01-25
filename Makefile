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
	gl3w.o rune.o strlcpy.o vector.o \
	zip.o cache.o image.o shader.o \
	model.o model_iqm.o model_iqe.o model_obj.o \
	font.o draw.o console.o )

A_EXE := a.exe

$(OUT) :
	mkdir -p $@

$(OUT)/%.o : %.c $(HDRS)
	$(CC_CMD)
$(OUT)/%.o : %.m $(HDRS)
	$(CC_CMD)

$(A_EXE) : $(OBJS) $(OUT)/a.o
	$(LINK_CMD)

%.iqe: %.dae
	assiqe -l $< | iqe-ryzom $(<:%.dae=%.material) > $@

%.mesh.iqe: %.dae
	assiqe -l -m $< | iqe-ryzom > $@

%.anim.iqe: %.dae
	assiqe -l -a $< | iqe-ryzom > $@

%.iqm: %.iqe
	iqm $@ $<

all: $(OUT) $(A_EXE)

clean:
	rm -rf $(OUT)
