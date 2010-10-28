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

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_scrolled_entry_cursor_up_func_01(void);
static void utc_UIFW_elm_scrolled_entry_cursor_up_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_scrolled_entry_cursor_up_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_scrolled_entry_cursor_up_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 }
};

static void startup(void)
{
	tet_infoline("[[ TET_MSG ]]:: ============ Startup ============ ");
	elm_init(0, NULL);
	main_win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
	evas_object_show(main_win);	
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
 * @brief Positive test case of elm_scrolled_entry_cursor_up()
 */
static void utc_UIFW_elm_scrolled_entry_cursor_up_func_01(void)
{
	Eina_Bool result;

	Evas_Object *test_en = elm_scrolled_entry_add(main_win);
	elm_scrolled_entry_entry_set(test_en, "This is<br>test scrolled entry");

	// Current return type of this API is "void"
	result = elm_scrolled_entry_cursor_up(test_en);

	if (result == EINA_TRUE)
	{
		tet_result(TET_PASS);
		tet_infoline("[[ TET_MSG ]]::[ID]:TC_01, [TYPE]: Positive, [RESULT]:PASS, Moving the cursor one line up within the entry had succeed.");
	} else {
		tet_result(TET_FAIL);
		tet_infoline("[[ TET_MSG ]]::[ID]:TC_01, [TYPE]: Positive, [RESULT]:FAIL, Moving the cursor one line up within the entry had failed.");
	}
}

/**
 * @brief Negative test case of ug_init elm_scrolled_entry_cursor_up()
 */
static void utc_UIFW_elm_scrolled_entry_cursor_up_func_02(void)
{
	Eina_Bool result;
	// Current return type of this API is "void"
	result = elm_scrolled_entry_cursor_up(NULL);

	if (result == EINA_FALSE)
	{
		tet_result(TET_PASS);
		tet_infoline("[[ TET_MSG ]]::[ID]:TC_02, [TYPE]: Negative, [RESULT]:PASS, Moving the cursor one line up within the entry had failed.");
	} else {
		tet_result(TET_FAIL);
		tet_infoline("[[ TET_MSG ]]::[ID]:TC_02, [TYPE]: Negative, [RESULT]:FAIL, Error occurs while moving the cursor one line up within the entry.");

	}
}