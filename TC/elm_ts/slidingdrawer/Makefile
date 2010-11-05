CC ?= gcc

TARGETS = uts_elm_slidingdrawer_add_func \
	  uts_elm_slidingdrawer_content_set \
	  uts_elm_slidingdrawer_content_unset \
	  uts_elm_slidingdrawer_pos_set \
	  uts_elm_slidingdrawer_max_drag_value_set \
	  uts_elm_slidingdrawer_drag_value_set

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
	rm -f $(TARGETS)
	rm -f tet_captured
	rm -f *~
