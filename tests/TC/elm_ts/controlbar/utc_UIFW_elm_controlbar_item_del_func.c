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


Evas_Object *main_win;
Evas_Object *controlbar;
Evas_Object *button;
Elm_Controlbar_Item *item1;
Elm_Controlbar_Item *item2;
Elm_Controlbar_Item *item3;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_controlbar_item_del_func_01(void);
static void utc_UIFW_elm_controlbar_item_del_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_controlbar_item_del_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_controlbar_item_del_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 }
};

static void startup(void)
{
	tet_infoline("[[ TET_MSG ]]:: ============ Startup ============ ");
	elm_init(0, NULL);
	main_win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
	evas_object_show(main_win);

	controlbar = elm_controlbar_add(main_win);
	item1 = elm_controlbar_tab_item_append(controlbar, NULL, "Controlbar", NULL);
	item2 = elm_controlbar_tool_item_append(controlbar, NULL, "Controlbar", NULL, NULL);
	button = elm_button_add(controlbar);
	elm_button_label_set(button, "button");
	item3 = elm_controlbar_object_item_append(controlbar, button, 3);
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
	evas_object_del(button);
}

/**
 * @brief Positive test case of elm_controlbar_item_del()
 */
static void utc_UIFW_elm_controlbar_item_del_func_01(void)
{
   	elm_controlbar_item_del(item1);
   	elm_controlbar_item_del(item2);
   	elm_controlbar_item_del(item3);

	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init elm_controlbar_item_del()
 */
static void utc_UIFW_elm_controlbar_item_del_func_02(void)
{
   	elm_controlbar_item_del(NULL);
	if (!item1 || !item2 || !item3) {
		tet_infoline("elm_controlbar_item_del() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}
