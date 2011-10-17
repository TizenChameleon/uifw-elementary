CC ?= gcc

TARGETS = utc_UIFW_elm_datefield_add_func \
	  utc_UIFW_elm_datefield_format_set_func \
	  utc_UIFW_elm_datefield_format_get_func \
	  utc_UIFW_elm_datefield_item_enabled_set_func \
	  utc_UIFW_elm_datefield_item_enabled_get_func \
	  utc_UIFW_elm_datefield_item_max_set_func \
	  utc_UIFW_elm_datefield_item_max_get_func \
	  utc_UIFW_elm_datefield_item_max_is_abs_func \
	  utc_UIFW_elm_datefield_item_min_set_func \
	  utc_UIFW_elm_datefield_item_min_get_func \
	  utc_UIFW_elm_datefield_item_min_is_abs_func \
	  utc_UIFW_elm_datefield_item_value_set_func \
	  utc_UIFW_elm_datefield_item_value_get_func

PKGS = elementary evas

LDFLAGS = `pkg-config --libs $(PKGS)`
LDFLAGS += $(TET_ROOT)/lib/tet3/tcm_s.o
LDFLAGS += -L$(TET_ROOT)/lib/tet3 -ltcm_s
LDFLAGS += -L$(TET_ROOT)/lib/tet3 -lapi_s

CFLAGS = -I. `pkg-config --cflags $(PKGS)`
CFLAGS += -I$(TET_ROOT)/inc/tet3
CFLAGS += -Wall

all: $(TARGETS)

$(TARGETS): %: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGETS)
	rm -f tet_captured
	rm -f tet_lock
	rm -f *~
