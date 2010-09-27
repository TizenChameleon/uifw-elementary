CC ?= gcc

TARGETS = utc_UIFW_elm_multibuttonentry_add_func \
	  utc_UIFW_elm_multibuttonentry_label_get_func \
	  utc_UIFW_elm_multibuttonentry_entry_get_func \
	  utc_UIFW_elm_multibuttonentry_item_add_start_func \
	  utc_UIFW_elm_multibuttonentry_item_add_end_func \
	  utc_UIFW_elm_multibuttonentry_item_add_before_func \
	  utc_UIFW_elm_multibuttonentry_item_add_after_func \
	  utc_UIFW_elm_multibuttonentry_items_get_func \
	  utc_UIFW_elm_multibuttonentry_item_first_get_func \
	  utc_UIFW_elm_multibuttonentry_item_last_get_func \
	  utc_UIFW_elm_multibuttonentry_item_selected_get_func \
	  utc_UIFW_elm_multibuttonentry_item_del_func \
	  utc_UIFW_elm_multibuttonentry_items_del_func \
	  utc_UIFW_elm_multibuttonentry_item_label_get_func \
	  utc_UIFW_elm_multibuttonentry_item_prev_func \
	  utc_UIFW_elm_multibuttonentry_item_next_func \
	  utc_UIFW_elm_multibuttonentry_item_data_get_func

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
