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


static Evas_Object *main_win;
static Evas_Object *datefield;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_datefield_format_set_func_01(void);
static void utc_UIFW_elm_datefield_format_set_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_datefield_format_set_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_datefield_format_set_func_02, NEGATIVE_TC_IDX },
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
	if ( NULL != datefield ) {
		evas_object_del(datefield);
		datefield = NULL;
	}
	if ( NULL != main_win ) {
		evas_object_del(main_win);
		main_win = NULL;
	}
	elm_shutdown();
	tet_infoline("[[ TET_MSG ]]:: ============ Cleanup ============ ");
}

/**
 * @brief Positive test case of elm_datefield_format_set()
 */
static void utc_UIFW_elm_datefield_format_set_func_01(void)
{
	char *fmt = "%B %d, %Y";

	datefield = elm_datefield_add(main_win);
	elm_datefield_format_set(datefield, fmt);

	if (strncmp(elm_datefield_format_get(datefield), fmt, strlen(fmt))) {
		tet_infoline("elm_datefield_format_set() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}

	evas_object_show(datefield);

	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init elm_datefield_format_set()
 */
static void utc_UIFW_elm_datefield_format_set_func_02(void)
{
	char *fmt1, *fmt2;

	datefield = elm_datefield_add(main_win);
	fmt1 = elm_datefield_format_get(datefield);
	elm_datefield_format_set(datefield, NULL);
	fmt2 = elm_datefield_format_get(datefield);

	if (strncmp(fmt1, fmt2, strlen(fmt1))) {
		tet_infoline("elm_datefield_format_set() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}

	evas_object_show(datefield);

	tet_result(TET_PASS);
}
