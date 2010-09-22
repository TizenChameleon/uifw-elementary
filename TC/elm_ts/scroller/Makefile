CC ?= gcc

TARGETS = utc_UIFW_elm_smart_scroller_add_func	\
	utc_UIFW_elm_smart_scroller_bounce_allow_get_func	\
	utc_UIFW_elm_smart_scroller_bounce_allow_set_func	\
	utc_UIFW_elm_smart_scroller_child_pos_get_func	\
	utc_UIFW_elm_smart_scroller_child_pos_set_func	\
	utc_UIFW_elm_smart_scroller_child_region_show_func	\
	utc_UIFW_elm_smart_scroller_child_set_func	\
	utc_UIFW_elm_smart_scroller_child_viewport_size_get_func	\
	utc_UIFW_elm_smart_scroller_edje_object_get_func	\
	utc_UIFW_elm_smart_scroller_freeze_set_func	\
	utc_UIFW_elm_smart_scroller_hold_set_func	\
	utc_UIFW_elm_smart_scroller_page_move_set_func	\
	utc_UIFW_elm_smart_scroller_page_size_get_func	\
	utc_UIFW_elm_smart_scroller_page_size_set_func	\
	utc_UIFW_elm_smart_scroller_paging_set_func	\
	utc_UIFW_elm_smart_scroller_policy_get_func	\
	utc_UIFW_elm_smart_scroller_policy_set_func	\
	utc_UIFW_elm_smart_scroller_region_bring_in_func	\
	utc_UIFW_elm_smart_scroller_single_dir_get_func	\
	utc_UIFW_elm_smart_scroller_single_dir_set_func	\
	utc_UIFW_elm_smart_scroller_step_size_get_func	\
	utc_UIFW_elm_smart_scroller_step_size_set_func	\
	utc_UIFW_elm_smart_scroller_widget_set_func

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
