#include <tet_api.h>
#include <Elementary.h>
#define ICON_DIR "usr/share/elementary/images"

// Definitions
// For checking the result of the positive test case.
#define TET_CHECK_PASS(x1, y...) \
{ \
	Evas_Object *err = y; \
	if (err == (x1)) \
		{ \
			tet_printf("[TET_CHECK_PASS]:: %s[%d] : Test has failed..", __FILE__,__LINE__); \
			tet_result(TET_FAIL); \
			return; \
		} \
}

// For checking the result of the negative test case.
#define TET_CHECK_FAIL(x1, y...) \
{ \
	Evas_Object *err = y; \
	if (err != (x1)) \
		{ \
			tet_printf("[TET_CHECK_FAIL]:: %s[%d] : Test has failed..", __FILE__,__LINE__); \
			tet_result(TET_FAIL); \
			return; \
		} \
}


Evas_Object *main_win;
Evas_Object *controlbar;
Evas_Object *view;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_controlbar_tab_item_prepend_func_01(void);
static void utc_UIFW_elm_controlbar_tab_item_prepend_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_controlbar_tab_item_prepend_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_controlbar_tab_item_prepend_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 }
};

static void startup(void)
{
	tet_infoline("[[ TET_MSG ]]:: ============ Startup ============ ");
	elm_init(0, NULL);
	main_win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
	evas_object_show(main_win);

	controlbar = elm_controlbar_add(main_win);

	view = elm_layout_add(controlbar);
}

static void cleanup(void)
{
	if ( NULL != main_win ) {
		evas_object_del(main_win);
	       	main_win = NULL;
	}
	if ( NULL != controlbar ) {
		evas_object_del(controlbar);
	       	controlbar = NULL;
	}
	if ( NULL != view ) {
		evas_object_del(view);
	       	view = NULL;
	}

	elm_shutdown();
	tet_infoline("[[ TET_MSG ]]:: ============ Cleanup ============ ");
}

/**
 * @brief Positive test case of elm_controlbar_tab_item_prepend()
 */
static void utc_UIFW_elm_controlbar_tab_item_prepend_func_01(void)
{
	char buf[255];

	Elm_Controlbar_Item *item = NULL;
	snprintf(buf, sizeof(buf), "%s/logo_small.png", ICON_DIR);
   	item = elm_controlbar_tab_item_prepend(controlbar, buf, "Songs", view);

	if (!item) {
		tet_infoline("elm_controlbar_tab_item_prepend() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init elm_controlbar_tab_item_prepend()
 */
static void utc_UIFW_elm_controlbar_tab_item_prepend_func_02(void)
{
	char buf[255];

	Elm_Controlbar_Item *item = NULL;
	snprintf(buf, sizeof(buf), "%s/logo_small.png", ICON_DIR);
   	item = elm_controlbar_tab_item_prepend(NULL, buf, "Songs", view);

	if (item) {
		tet_infoline("elm_controlbar_tab_item_prepend() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}