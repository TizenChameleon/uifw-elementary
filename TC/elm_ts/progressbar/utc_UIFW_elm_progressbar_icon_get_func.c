#include <tet_api.h>
#include <Elementary.h>

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


Evas_Object *main_win, *progressbar;


static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_progressbar_icon_get_func_01(void);
static void utc_UIFW_elm_progressbar_icon_get_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_progressbar_icon_get_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_progressbar_icon_get_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 }
};

static void startup(void)
{
	tet_infoline("[[ TET_MSG ]]:: ============ Startup ============ ");
	elm_init(0, NULL);
	main_win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
	evas_object_show(main_win);

	progressbar = elm_progressbar_add(main_win);
	evas_object_show(progressbar);
	elm_win_resize_object_add(main_win, progressbar);
	evas_object_size_hint_align_set(progressbar, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
}

static void cleanup(void)
{
	if ( NULL != progressbar ) {
		evas_object_del(progressbar);
	    progressbar = NULL;
	}

	if ( NULL != main_win ) {
		evas_object_del(main_win);
	       	main_win = NULL;
	}
	elm_shutdown();
	tet_infoline("[[ TET_MSG ]]:: ============ Cleanup ============ ");
}

/**
 * @brief Positive test case of elm_progressbar_icon_get()
 */
static void utc_UIFW_elm_progressbar_icon_get_func_01(void)
{
	Evas_Object *icon, *res;

	tet_infoline("[[ DEBUG :: Positive ]]");

	icon = elm_icon_add(main_win);
	elm_icon_file_set(icon, "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item1.jpg", NULL);
	evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_HORIZONTAL, 1, 0);
	evas_object_size_hint_min_set(icon, 10, 10);
	elm_progressbar_icon_set(progressbar, icon);

	res = elm_progressbar_icon_get(progressbar);
	if (res != icon) {
		tet_infoline("elm_progressbar_icon_get() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}

	tet_result(TET_PASS);
	tet_infoline("[[ TET_MSG ]]::[ID]:TC_01, [TYPE]: Positive, [RESULT]:PASS, elm_progressbar_icon_get().");
}

/**
 * @brief Negative test case of ug_init elm_progressbar_icon_get()
 */
static void utc_UIFW_elm_progressbar_icon_get_func_02(void)
{
	Evas_Object *icon, *res;

	tet_infoline("[[ DEBUG:: Negative ]]");

	icon = elm_icon_add(main_win);
	elm_icon_file_set(icon, "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item1.jpg", NULL);
	evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_HORIZONTAL, 1, 0);
	evas_object_size_hint_min_set(icon, 10, 10);
	elm_progressbar_icon_set(progressbar, icon);

	res = elm_progressbar_icon_get(NULL);
	if (res) {
		tet_infoline("elm_progressbar_icon_get() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}

	tet_result(TET_PASS);
	tet_infoline("[[ TET_MSG ]]::[ID]:TC_02, [TYPE]: Negative, [RESULT]:PASS, elm_progressbar_icon_get().");
}
