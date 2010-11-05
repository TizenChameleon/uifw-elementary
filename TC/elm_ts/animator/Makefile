CC ?= gcc

TARGETS = utc_UIFW_elm_check_add_func \
		utc_UIFW_elm_animator_add_func \
		utc_UIFW_elm_animator_animate_func \
		utc_UIFW_elm_animator_auto_reverse_get_func \
		utc_UIFW_elm_animator_auto_reverse_set_func \
		utc_UIFW_elm_animator_completion_callback_set_func \
		utc_UIFW_elm_animator_curve_style_set_func \
		utc_UIFW_elm_animator_del_func \
		utc_UIFW_elm_animator_duration_set_func \
		utc_UIFW_elm_animator_operating_get_func \
		utc_UIFW_elm_animator_operation_callback_set_func \
		utc_UIFW_elm_animator_pause_func \
		utc_UIFW_elm_animator_repeat_get_func \
		utc_UIFW_elm_animator_repeat_set_func \
		utc_UIFW_elm_animator_resume_func \
		utc_UIFW_elm_animator_stop_func 



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
