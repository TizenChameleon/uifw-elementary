CC ?= gcc

TARGETS = utc_UIFW_elm_searchbar_add_func.c \
	utc_UIFW_elm_searchbar_boundary_rect_set_func.c \
	utc_UIFW_elm_searchbar_cancel_button_animation_set_func.c \
	utc_UIFW_elm_searchbar_cancel_button_set_func.c \
	utc_UIFW_elm_searchbar_clear_func.c \
	utc_UIFW_elm_searchbar_entry_get_func.c \
	utc_UIFW_elm_searchbar_text_get_func.c 

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
	rm -f *~ *.o tet_captured tet_lock $(TARGETS)
