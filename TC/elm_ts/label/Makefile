CC ?= gcc

TARGETS = utc_UIFW_elm_label_add_func \
	  utc_UIFW_elm_label_label_get \
	  utc_UIFW_elm_label_line_wrap_get \
	  utc_UIFW_elm_label_wrap_height_get \
	  utc_UIFW_elm_label_wrap_width_get

PKGS = elementary

LDFLAGS = `pkg-config --libs $(PKGS)`
LDFLAGS += $(TET_ROOT)/lib/tet3/tcm_s.o
LDFLAGS += -L$(TET_ROOT)/lib/tet3 -ltcm_s
LDFLAGS += -L$(TET_ROOT)/lib/tet3 -lapi_s

CFLAGS = -I. `pkg-config --cflags $(PKGS)`
CFLAGS += -I$(TET_ROOT)/inc/tet3
CFLAGS += -Wall

all: $(TARGETS)

$(TARGET): %: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *~ *.o tet_captured tet_lock $(TARGETS)
