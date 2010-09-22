CC ?= gcc

TARGETS = uts_elm_controlbar_add_func \
		  uts_elm_controlbar_tab_item_append_func \
		  uts_elm_controlbar_tab_item_prepend_func \
		  uts_elm_controlbar_tab_item_insert_before_func \
		  uts_elm_controlbar_tab_item_insert_after_func \
		  uts_elm_controlbar_tool_item_append_func \
		  uts_elm_controlbar_tool_item_prepend_func \
		  uts_elm_controlbar_tool_item_insert_before_func \
		  uts_elm_controlbar_tool_item_insert_after_func \
		  uts_elm_controlbar_object_item_append_func \
		  uts_elm_controlbar_object_item_prepend_func \
		  uts_elm_controlbar_object_item_insert_before_func \
		  uts_elm_controlbar_object_item_insert_after_func \

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
