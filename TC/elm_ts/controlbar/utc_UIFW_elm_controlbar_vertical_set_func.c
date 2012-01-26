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

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_controlbar_vertical_set_func_01(void);
static void utc_UIFW_elm_controlbar_vertical_set_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_controlbar_vertical_set_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_controlbar_vertical_set_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 }
};

static void startup(void)
{
	char buf[255];

	tet_infoline("[[ TET_MSG ]]:: ============ Startup ============ ");
	elm_init(0, NULL);
	main_win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
	evas_object_show(main_win);

	controlbar = elm_controlbar_add(main_win);
	snprintf(buf, sizeof(buf), "%s/logo_small.png", ICON_DIR);
	elm_controlbar_tab_item_append(controlbar, buf, "Controlbar", NULL);
	elm_controlbar_tab_item_append(controlbar, buf, "Controlbar", NULL);
	elm_controlbar_tab_item_append(controlbar, buf, "Controlbar", NULL);
	elm_controlbar_tab_item_append(controlbar, buf, "Controlbar", NULL);
}

static void cleanup(void)
{
	if ( NULL != main_win ) {
		evas_object_del(main_win);
	       	main_win = NULL;
	}
	elm_shutdown();
	tet_infoline("[[ TET_MSG ]]:: ============ Cleanup ============ ");

	evas_object_del(controlbar);
}

/**
 * @brief Positive test case of elm_controlbar_vertical_set()
 */
static void utc_UIFW_elm_controlbar_vertical_set_func_01(void)
{
 	elm_controlbar_vertical_set(controlbar, EINA_TRUE);

	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init elm_controlbar_vertical_set()
 */
static void utc_UIFW_elm_controlbar_vertical_set_func_02(void)
{
	elm_controlbar_vertical_set(controlbar, NULL);

	tet_result(TET_PASS);
}
