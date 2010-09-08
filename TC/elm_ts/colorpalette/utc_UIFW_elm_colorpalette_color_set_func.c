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
Evas_Object *colorpalette;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_colorpalette_color_set_func_01(void);
static void utc_UIFW_elm_colorpalette_color_set_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_colorpalette_color_set_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_colorpalette_color_set_func_02, NEGATIVE_TC_IDX },
};

static void startup(void)
{
	tet_infoline("[[ TET_MSG ]]:: ============ Startup ============ ");
	elm_init(0, NULL);
	main_win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
	evas_object_show(main_win);	
	colorpalette = elm_colorpalette_add(main_win);
	evas_object_show(colorpalette);
}

static void cleanup(void)
{
	if ( NULL != main_win ) {
		evas_object_del(main_win);
	       	main_win = NULL;
	}
	else if ( NULL != colorpalette ) {
		evas_object_del(colorpalette);
	       	colorpalette = NULL;
	}
	elm_shutdown();
	tet_infoline("[[ TET_MSG ]]:: ============ Cleanup ============ ");
}

/**
 * @brief Positive test case of elm_colorpalette_color_set()
 */
static void utc_UIFW_elm_colorpalette_color_set_func_01(void)
{
	Elm_Colorpalette_Color color_set[5];
	
	color_set[ 0 ].r = 255;
	color_set[ 0 ].g = 90;
	color_set[ 0 ].b = 18;

	color_set[ 1 ].r = 255;
	color_set[ 1 ].g = 213;
	color_set[ 1 ].b = 0;

	color_set[ 2 ].r = 146;
	color_set[ 2 ].g = 255;
	color_set[ 2 ].b = 11;

	color_set[ 3 ].r = 9;
	color_set[ 3 ].g = 186;
	color_set[ 3 ].b = 10;

	color_set[ 4 ].r = 86;
	color_set[ 4 ].g = 201;
	color_set[ 4 ].b = 242;

   	elm_colorpalette_color_set(colorpalette, 5, color_set);

	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init elm_colorpalette_color_set()
 */
static void utc_UIFW_elm_colorpalette_color_set_func_02(void)
{
	Elm_Colorpalette_Color color_set[5];
	
	color_set[ 0 ].r = 255;
	color_set[ 0 ].g = 90;
	color_set[ 0 ].b = 18;

	color_set[ 1 ].r = 255;
	color_set[ 1 ].g = 213;
	color_set[ 1 ].b = 0;

	color_set[ 2 ].r = 146;
	color_set[ 2 ].g = 255;
	color_set[ 2 ].b = 11;

	color_set[ 3 ].r = 9;
	color_set[ 3 ].g = 186;
	color_set[ 3 ].b = 10;

	color_set[ 4 ].r = 86;
	color_set[ 4 ].g = 201;
	color_set[ 4 ].b = 242;

   	elm_colorpalette_color_set(NULL, 5, color_set);
	
	tet_result(TET_PASS);
}
