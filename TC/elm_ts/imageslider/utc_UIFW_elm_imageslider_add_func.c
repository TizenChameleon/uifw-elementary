#include <tet_api.h>


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

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_imageslider_add_func_01(void);
static void utc_UIFW_elm_imageslider_add_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_imageslider_add_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_imageslider_add_func_02, NEGATIVE_TC_IDX },
};

static void startup(void)
{
}

static void cleanup(void)
{
}

/**
 * @brief Positive test case of elm_imageslider_add()
 */
static void utc_UIFW_elm_imageslider_add_func_01(void)
{
	int r = 0;

/*
   	r = elm_imageslider_add(...);
*/
	if (r) {
		tet_infoline("elm_imageslider_add() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init elm_imageslider_add()
 */
static void utc_UIFW_elm_imageslider_add_func_02(void)
{
	int r = 0;

/*
   	r = elm_imageslider_add(...);
*/
	if (r) {
		tet_infoline("elm_imageslider_add() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}
