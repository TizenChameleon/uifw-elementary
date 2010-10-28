CC ?= gcc

TARGETS = utc_UIFW_elm_genlist_add_func \
	  utc_UIFW_elm_genlist_clear_func \
	  utc_UIFW_elm_genlist_multi_select_set_func \
	  utc_UIFW_elm_genlist_multi_select_get_func \
	  utc_UIFW_elm_genlist_horizontal_mode_set_func \
	  utc_UIFW_elm_genlist_horizontal_mode_get_func \
	  utc_UIFW_elm_genlist_always_select_mode_set_func \
	  utc_UIFW_elm_genlist_always_select_mode_get_func \
	  utc_UIFW_elm_genlist_no_select_mode_set_func \
	  utc_UIFW_elm_genlist_no_select_mode_get_func \
	  utc_UIFW_elm_genlist_compress_mode_set_func \
	  utc_UIFW_elm_genlist_compress_mode_get_func \
	  utc_UIFW_elm_genlist_bounce_set_func \
	  utc_UIFW_elm_genlist_bounce_get_func \
	  utc_UIFW_elm_genlist_homogeneous_set_func \
	  utc_UIFW_elm_genlist_homogeneous_get_func \
	  utc_UIFW_elm_genlist_block_count_set_func \
	  utc_UIFW_elm_genlist_block_count_get_func \
	  utc_UIFW_elm_genlist_longpress_timeout_set_func \
	  utc_UIFW_elm_genlist_longpress_timeout_get_func \
	  utc_UIFW_elm_genlist_effect_set_func \
	  utc_UIFW_elm_genlist_pinch_zoom_set_func \
	  utc_UIFW_elm_genlist_pinch_zoom_mode_set_func \
	  utc_UIFW_elm_genlist_pinch_zoom_mode_get_func

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
