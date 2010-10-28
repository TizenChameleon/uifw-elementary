CC ?= gcc

TARGETS = utc_UIFW_elm_scrolled_entry_add_func \
		  utc_UIFW_elm_scrolled_entry_single_line_set_func \
		  utc_UIFW_elm_scrolled_entry_single_line_get_func \
		  utc_UIFW_elm_scrolled_entry_password_set_func \
		  utc_UIFW_elm_scrolled_entry_password_get_func \
		  utc_UIFW_elm_scrolled_entry_entry_set_func \
		  utc_UIFW_elm_scrolled_entry_entry_get_func \
		  utc_UIFW_elm_scrolled_entry_selection_get_func \
		  utc_UIFW_elm_scrolled_entry_entry_insert_func \
		  utc_UIFW_elm_scrolled_entry_line_wrap_set_func \
		  utc_UIFW_elm_scrolled_entry_line_char_wrap_set_func \
		  utc_UIFW_elm_scrolled_entry_editable_set_func \
		  utc_UIFW_elm_scrolled_entry_editable_get_func \
		  utc_UIFW_elm_scrolled_entry_select_none_func \
		  utc_UIFW_elm_scrolled_entry_select_all_func \
		  utc_UIFW_elm_scrolled_entry_cursor_next_func \
		  utc_UIFW_elm_scrolled_entry_cursor_prev_func \
		  utc_UIFW_elm_scrolled_entry_cursor_up_func \
		  utc_UIFW_elm_scrolled_entry_cursor_down_func \
		  utc_UIFW_elm_scrolled_entry_cursor_begin_set_func \
		  utc_UIFW_elm_scrolled_entry_cursor_end_set_func \
		  utc_UIFW_elm_scrolled_entry_cursor_line_begin_set_func \
		  utc_UIFW_elm_scrolled_entry_cursor_line_end_set_func


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
