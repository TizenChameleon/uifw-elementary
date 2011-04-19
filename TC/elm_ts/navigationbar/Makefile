CC ?= gcc

TARGETS = utc_UIFW_elm_navigationbar_add_func \
	  utc_UIFW_elm_navigationbar_push_func \
	  utc_UIFW_elm_navigationbar_pop_func \
	  utc_UIFW_elm_navigationbar_to_content_pop_func \
	  utc_UIFW_elm_navigationbar_title_label_set_func \
	  utc_UIFW_elm_navigationbar_title_label_get_func \
	  utc_UIFW_elm_navigationbar_title_object_add_func \
	  utc_UIFW_elm_navigationbar_title_object_list_get_func \
	  utc_UIFW_elm_navigationbar_content_top_get_func \
	  utc_UIFW_elm_navigationbar_content_bottom_get_func \
	  utc_UIFW_elm_navigationbar_hidden_set_func \
	  utc_UIFW_elm_navigationbar_title_button_set_func \
	  utc_UIFW_elm_navigationbar_title_button_get_func \
	  utc_UIFW_elm_navigationbar_subtitle_label_get_func \
	  utc_UIFW_elm_navigationbar_subtitle_label_set_func \
	  utc_UIFW_elm_navigationbar_animation_disabled_set_func \
	  utc_UIFW_elm_navigationbar_title_object_list_unset_func 

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