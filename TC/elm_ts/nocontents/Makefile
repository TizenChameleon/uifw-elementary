CC ?= gcc


TARGETS = utc_UIFW_elm_nocontents_add_func \
	  utc_UIFW_elm_nocontents_custom_get_func \
	  utc_UIFW_elm_nocontents_custom_set_func \
	  utc_UIFW_elm_nocontents_label_get_func \
	  utc_UIFW_elm_nocontents_label_set_func	

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
