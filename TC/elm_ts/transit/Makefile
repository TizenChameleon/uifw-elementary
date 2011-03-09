CC ?= gcc

TARGETS = uts_elm_transit_add_func \
	  uts_elm_transit_auto_reverse_set_func \
	  uts_elm_transit_completion_callback_set_func \
	  uts_elm_transit_curve_style_set_func \
	  uts_elm_transit_del_func \
	  uts_elm_transit_event_block_disabled_get_func \
	  uts_elm_transit_event_block_disabled_set_func \
	  uts_elm_transit_fx_clear_func \
	  uts_elm_transit_fx_insert_func \
	  uts_elm_transit_pause_func \
	  uts_elm_transit_repeat_set_func \
	  uts_elm_transit_resume_func \
	  uts_elm_transit_run_func \
	  uts_elm_transit_stop_func \
	  uts_elm_transit_fx_blend_add_func \
     uts_elm_transit_fx_color_add_func \
     uts_elm_transit_fx_fade_add_func \
	  uts_elm_transit_fx_flip_add_func \
	  uts_elm_transit_fx_image_animation_add_func \
     uts_elm_transit_fx_resizable_flip_add_func \
     uts_elm_transit_fx_resizing_add_func \
	  uts_elm_transit_fx_rotation3d_add_func \
     uts_elm_transit_fx_roation_add_func \
	  uts_elm_transit_fx_translation_add_func \
	  uts_elm_transit_fx_wipe_add_func \
	  uts_elm_transit_fx_zoom_add_func \

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
