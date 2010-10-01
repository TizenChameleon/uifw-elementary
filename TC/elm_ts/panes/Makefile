CC ?= gcc

TARGETS = utc_UIFW_elm_panes_add_func \
	  utc_UIFW_elm_panes_content_left_set_func \
	  utc_UIFW_elm_panes_content_right_set_func \
	  utc_UIFW_elm_panes_content_left_get_func \
	  utc_UIFW_elm_panes_content_right_get_func \
	  utc_UIFW_elm_panes_content_left_unset_func \
	  utc_UIFW_elm_panes_content_right_unset_func \
	  utc_UIFW_elm_panes_content_left_size_set_func \
	  utc_UIFW_elm_panes_content_left_size_get_func \
	  utc_UIFW_elm_panes_horizontal_set_func \
	  utc_UIFW_elm_panes_horizontal_is_func \
	  utc_UIFW_elm_panes_fixed_set_func \
	  utc_UIFW_elm_panes_fixed_is_func
	  
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
