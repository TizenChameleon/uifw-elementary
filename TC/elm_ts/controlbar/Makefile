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
		  uts_elm_controlbar_object_item_del_func \
		  uts_elm_controlbar_object_item_select_func \
		  uts_elm_controlbar_object_item_visible_set_func \
		  uts_elm_controlbar_object_item_visible_get_func \
		  uts_elm_controlbar_object_item_disable_set_func \
		  uts_elm_controlbar_item_icon_set_func \
		  uts_elm_controlbar_object_item_icon_get_func \
		  uts_elm_controlbar_item_label_get_func \
		  uts_elm_controlbar_item_label_set_func \
		  uts_elm_controlbar_selected_item_get_func \
		  uts_elm_controlbar_first_item_get_func \
		  uts_elm_controlbar_last_item_get_func \
		  uts_elm_controlbar_items_get_func \
		  uts_elm_controlbar_item_prev_func \
		  uts_elm_controlbar_item_next_func \
		  uts_elm_controlbar_item_view_set_func \
		  uts_elm_controlbar_item_view_get_func \
          uts_elm_controlbar_mode_set_func \
          uts_elm_controlbar_alpha_set_func \
          uts_elm_controlbar_item_auto_align_set_func \
          uts_elm_controlbar_vertical_set_func

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
