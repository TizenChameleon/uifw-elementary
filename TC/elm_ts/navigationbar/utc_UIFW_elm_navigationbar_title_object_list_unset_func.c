#include <tet_api.h>
#include <Elementary.h>
#define ICON_DIR "usr/share/beat_winset_test/icon"


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


Evas_Object *main_win, *navibar;
char buf[PATH_MAX];


static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_navigationbar_title_object_list_unset_func_01(void);
static void utc_UIFW_elm_navigationbar_title_object_list_unset_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_navigationbar_title_object_list_unset_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_navigationbar_title_object_list_unset_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 }
};

static void startup(void)
{
	tet_infoline("[[ TET_MSG ]]:: ============ Startup ============ ");
	elm_init(0, NULL);
	main_win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
	evas_object_show(main_win);
	navibar = elm_navigationbar_add(main_win);
	evas_object_show(navibar);
	elm_win_resize_object_add(main_win, navibar);
}

static void cleanup(void)
{
	if ( NULL != navibar ) {
		evas_object_del(navibar);
	       	navibar = NULL;
	}
	if ( NULL != main_win ) {
		evas_object_del(main_win);
	       	main_win = NULL;
	}
	elm_shutdown();
	tet_infoline("[[ TET_MSG ]]:: ============ Cleanup ============ ");
}

/**
 * @brief Positive test case of elm_navigationbar_title_object_list_unset()
 */
static void utc_UIFW_elm_navigationbar_title_object_list_unset_func_01(void)
{
	Eina_List *list = NULL;
	Evas_Object *content = elm_icon_add(navibar);
	snprintf(buf, sizeof(buf), "%s/00_volume_icon.png", ICON_DIR);
	elm_icon_file_set(content, buf, NULL);
	evas_object_size_hint_aspect_set(content, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
	elm_icon_scale_set(content, 1, 1);

	Evas_Object *title = elm_button_add(navibar);
	evas_object_show(title);

	elm_navigationbar_push(navibar, "title", NULL, NULL, NULL, content);
	elm_navigationbar_title_object_add(navibar, content, title);
	elm_navigationbar_title_object_list_unset(navibar, content, &list);
	if (!list) {
		tet_infoline("elm_navigationbar_title_object_list_unset() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init elm_navigationbar_title_object_list_unset()
 */
static void utc_UIFW_elm_navigationbar_title_object_list_unset_func_02(void)
{
	Eina_List *list = NULL;
	Evas_Object *content = elm_icon_add(navibar);
	snprintf(buf, sizeof(buf), "%s/00_volume_icon.png", ICON_DIR);
	elm_icon_file_set(content, buf, NULL);
	evas_object_size_hint_aspect_set(content, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
	elm_icon_scale_set(content, 1, 1);

	Evas_Object *title = elm_button_add(navibar);
	evas_object_show(title);

	elm_navigationbar_push(navibar, "title", NULL, NULL, NULL, content);
	elm_navigationbar_title_object_add(navibar, content, title);
	elm_navigationbar_title_object_list_unset(NULL, content, &list);
	if (list) {
		tet_infoline("elm_navigationbar_title_object_list_unset() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}
