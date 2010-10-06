CC ?= gcc

TARGETS = utc_UIFW_elm_dialoguegroup_add_func \
	  utc_UIFW_elm_dialoguegroup_prepend_func \
	  utc_UIFW_elm_dialoguegroup_append_func \
	  utc_UIFW_elm_dialoguegroup_insert_after_func \
	  utc_UIFW_elm_dialoguegroup_insert_before_func \
	  utc_UIFW_elm_dialoguegroup_remove_func \
	  utc_UIFW_elm_dialoguegroup_remove_all_func \
	  utc_UIFW_elm_dialoguegroup_title_set_func \
	  utc_UIFW_elm_dialoguegroup_title_get_func \
	  utc_UIFW_elm_dialoguegroup_press_effect_get_func \
	  utc_UIFW_elm_dialoguegroup_press_effect_set_func \
	  utc_UIFW_elm_dialoguegroup_item_content_get_func \
	  utc_UIFW_elm_dialoguegroup_item_style_set_func \
	  utc_UIFW_elm_dialoguegroup_item_style_get_func

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
