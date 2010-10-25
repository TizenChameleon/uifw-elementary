CC ?= gcc

TARGETS = utc_UIFW_elm_editfield_add_func \
  	  utc_UIFW_elm_editfield_label_set_func\
  	  utc_UIFW_elm_editfield_label_get_func\
  	  utc_UIFW_elm_editfield_guide_text_set_func\
  	  utc_UIFW_elm_editfield_guide_text_get_func\
  	  utc_UIFW_elm_editfield_entry_get_func\
  	  utc_UIFW_elm_editfield_right_icon_set_func\
  	  utc_UIFW_elm_editfield_right_icon_get_func\
  	  utc_UIFW_elm_editfield_left_icon_set_func\
  	  utc_UIFW_elm_editfield_left_icon_get_func\
  	  utc_UIFW_elm_editfield_entry_single_line_set_func\
  	  utc_UIFW_elm_editfield_entry_single_line_get_func\
  	  utc_UIFW_elm_editfield_entry_eraser_set_func\
  	  utc_UIFW_elm_editfield_entry_eraser_get_func

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
