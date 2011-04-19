CC ?= gcc

TARGETS = utc_UIFW_elm_navigationbar_ex_add_func \
	  utc_UIFW_elm_navigationbar_ex_item_push_func \
	  utc_UIFW_elm_navigationbar_ex_item_pop_func \
	  utc_UIFW_elm_navigationbar_ex_item_promote_func \
	  utc_UIFW_elm_navigationbar_ex_to_item_pop_func \
	  utc_UIFW_elm_navigationbar_ex_item_title_label_set_func \
	  utc_UIFW_elm_navigationbar_ex_item_title_label_get_func \
	  utc_UIFW_elm_navigationbar_ex_item_top_get_func \
	  utc_UIFW_elm_navigationbar_ex_item_bottom_get_func \
	  utc_UIFW_elm_navigationbar_ex_item_title_button_set_func \
	  utc_UIFW_elm_navigationbar_ex_item_title_button_get_func \
	  utc_UIFW_elm_navigationbar_ex_item_title_object_set_func \
	  utc_UIFW_elm_navigationbar_ex_item_title_object_unset_func \
	  utc_UIFW_elm_navigationbar_ex_item_title_hidden_set_func \
	  utc_UIFW_elm_navigationbar_ex_item_title_object_get_func \
	  utc_UIFW_elm_navigationbar_ex_item_subtitle_label_get_func \
	  utc_UIFW_elm_navigationbar_ex_item_subtitle_label_set_func \
	  utc_UIFW_elm_navigationbar_ex_item_style_set_func \
	  utc_UIFW_elm_navigationbar_ex_item_style_get_func \
	  utc_UIFW_elm_navigationbar_ex_item_content_unset_func \
	  utc_UIFW_elm_navigationbar_ex_item_content_get_func \
	  utc_UIFW_elm_navigationbar_ex_delete_on_pop_set_func 

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
