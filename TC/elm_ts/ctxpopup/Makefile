CC ?= gcc

TARGETS = uts_elm_ctxpopup_add_func \
	  uts_elm_ctxpopup_horizontal_set \
	  uts_elm_ctxpopup_horizontal_get \
	  uts_elm_ctxpopup_item_append \
	  uts_elm_ctxpopup_item_icon_set \
	  uts_elm_ctxpopup_item_label_set \
	  uts_elm_ctxpopup_item_del \
	  uts_elm_ctxpopup_item_disabled_set \
     uts_elm_ctxpopup_item_disabled_get \
     uts_elm_ctxpopup_clear \
	  uts_elm_ctxpopup_item_label_get \
	  uts_elm_ctxpopup_item_icon_get \
     uts_elm_ctxpopup_direction_priority_set \
     uts_elm_ctxpopup_direction_priority_get \
	  uts_elm_ctxpopup_content_set \
     uts_elm_ctxpopup_content_unset \
	  uts_elm_ctxpopup_hover_parent_set \
	  uts_elm_ctxpopup_hover_parent_get
	    


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
