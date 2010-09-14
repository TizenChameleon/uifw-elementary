CC ?= gcc

TARGETS = utc_UIFW_elm_datefield_add_func \
	  utc_UIFW_elm_datefield_layout_set_func \
	  utc_UIFW_elm_datefield_layout_get_func \
	  utc_UIFW_elm_datefield_date_set_func \
	  utc_UIFW_elm_datefield_date_get_func \
	  utc_UIFW_elm_datefield_time_mode_set_func \
	  utc_UIFW_elm_datefield_time_mode_get_func \
	  utc_UIFW_elm_datefield_date_format_set_func \
	  utc_UIFW_elm_datefield_date_format_get_func \
	  utc_UIFW_elm_datefield_input_panel_state_callback_add_func \
	  utc_UIFW_elm_datefield_input_panel_state_callback_del_func

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
