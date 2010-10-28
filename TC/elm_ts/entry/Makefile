CC ?= gcc

TARGETS = utc_UIFW_elm_entry_cursor_up_func \
	  utc_UIFW_elm_entry_cursor_down_func \
	  utc_UIFW_elm_entry_cursor_is_format_get_func \
	  utc_UIFW_elm_entry_cursor_is_visible_format_get_func \
	  utc_UIFW_elm_entry_cursor_content_get_func \
	  utc_UIFW_elm_entry_context_menu_disabled_get_func \
	  utc_UIFW_elm_entry_markup_to_utf8_func \
	  utc_UIFW_elm_entry_utf8_to_markup_func \
	  utc_UIFW_elm_entry_imf_context_get_func

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
