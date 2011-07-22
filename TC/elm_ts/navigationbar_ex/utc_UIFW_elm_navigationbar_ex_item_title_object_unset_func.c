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


Evas_Object *main_win, *navi_ex;
Elm_Navigationbar_ex_Item* item;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_navigationbar_ex_item_title_object_unset_func_01(void);
static void utc_UIFW_elm_navigationbar_ex_item_title_object_unset_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_navigationbar_ex_item_title_object_unset_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_navigationbar_ex_item_title_object_unset_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 }
};

static void startup(void)
{
	tet_infoline("[[ TET_MSG ]]:: ============ Startup ============ ");
	elm_init(0, NULL);
	main_win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
	evas_object_show(main_win);
	navi_ex = elm_navigationbar_ex_add(main_win);
	evas_object_show(navi_ex);
	Evas_Object *btn = elm_button_add(navi_ex);
	evas_object_show(btn);
	item = elm_navigationbar_ex_item_push(navi_ex, btn, "topbar_seg_full");
	Evas_Object *title_obj = elm_segment_control_add(navi_ex);
	elm_segment_control_item_add(title_obj, NULL, "Sg1" );
	elm_segment_control_item_add(title_obj, NULL, "Sg2" );
	elm_segment_control_item_add(title_obj, NULL, "Sg3" );
	evas_object_show(title_obj);
	elm_navigationbar_ex_item_title_object_set(item, title_obj);
	elm_win_resize_object_add(main_win, navi_ex);
}

static void cleanup(void)
{
	if ( NULL != main_win ) {
		evas_object_del(main_win);
	       	main_win = NULL;
	}
	elm_shutdown();
	tet_infoline("[[ TET_MSG ]]:: ============ Cleanup ============ ");
}

/**
 * @brief Positive test case of elm_navigationbar_ex_item_title_object_unset()
 */
static void utc_UIFW_elm_navigationbar_ex_item_title_object_unset_func_01(void)
{
	Evas_Object *title_obj = NULL;

   	title_obj = elm_navigationbar_ex_item_title_object_unset(item);

	if (!title_obj) {
		tet_infoline("elm_navigationbar_ex_item_title_object_unset() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init elm_navigationbar_ex_item_title_object_unset()
 */
static void utc_UIFW_elm_navigationbar_ex_item_title_object_unset_func_02(void)
{
	Evas_Object *title_obj = NULL;
   	title_obj = elm_navigationbar_ex_item_title_object_unset(NULL);
	if (title_obj) {
		tet_infoline("elm_navigationbar_ex_item_title_object_unset() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}
