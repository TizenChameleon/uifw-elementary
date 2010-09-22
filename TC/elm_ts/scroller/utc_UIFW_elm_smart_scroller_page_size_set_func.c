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

static void utc_UIFW_elm_smart_scroller_page_size_set_func_01(void);
static void utc_UIFW_elm_smart_scroller_page_size_set_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_smart_scroller_page_size_set_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_smart_scroller_page_size_set_func_02, NEGATIVE_TC_IDX },
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
 * @brief Positive test case of elm_smart_scroller_page_size_set()
 */
static void utc_UIFW_elm_smart_scroller_page_size_set_func_01(void)
{
	Evas_Object *scroll = NULL;
	Evas_Object *entry = NULL;	
    Evas_Coord x,y,sx=450,sy=750,px,py;
    Evas *e;
	e = evas_object_evas_get(main_win);
   	scroll= elm_smart_scroller_add(e);
	entry = elm_entry_add(main_win);
	evas_object_show(scroll);	
	evas_object_show(entry);
	evas_object_resize(scroll,240,400);
	evas_object_resize(entry,240,400);
    elm_smart_scroller_child_set( scroll,entry);
	elm_entry_entry_set(entry, "This is a multi-line entry ");	
	elm_smart_scroller_page_size_get(scroll,&x,&y);
	elm_smart_scroller_page_size_set(scroll,sx,sy);
	elm_smart_scroller_page_size_get(scroll,&px,&py);
	if ((px!=sx)&&(py!=sy)) {
		tet_infoline("elm_smart_scroller_page_size_set() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init elm_smart_scroller_page_size_set()
 */
static void utc_UIFW_elm_smart_scroller_page_size_set_func_02(void)
{
	Evas_Object *scroll = NULL;
	Evas_Object *entry = NULL;	
    Evas_Coord x,y,sx=450,sy=750,px,py;
    Evas *e;
	e = evas_object_evas_get(main_win);
   	scroll= elm_smart_scroller_add(e);
	entry = elm_entry_add(main_win);
	evas_object_show(scroll);	
	evas_object_show(entry);
	evas_object_resize(scroll,240,400);
	evas_object_resize(entry,240,400);
    elm_smart_scroller_child_set( scroll,entry);
	elm_entry_entry_set(entry, "This is a multi-line entry ");	
	elm_smart_scroller_page_size_get(scroll,&x,&y);
	elm_smart_scroller_page_size_set(NULL,sx,sy);
	elm_smart_scroller_page_size_get(scroll,&px,&py);
	if ((px!=x)&&(py!=y)) {	
		tet_infoline("elm_smart_scroller_page_size_set() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}
