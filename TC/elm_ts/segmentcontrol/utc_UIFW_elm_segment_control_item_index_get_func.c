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

Evas_Object *main_win, *segment_control;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_segment_control_item_index_get_func_01(void);
static void utc_UIFW_elm_segment_control_item_index_get_func_02(void);

enum {
   POSITIVE_TC_IDX = 0x01,
   NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
   { utc_UIFW_elm_segment_control_item_index_get_func_01, POSITIVE_TC_IDX },
   { utc_UIFW_elm_segment_control_item_index_get_func_02, NEGATIVE_TC_IDX },
   { NULL, 0 }
};

static void startup(void)
{
   tet_infoline("[[ TET_MSG ]]:: ============ Startup ============ ");
   elm_init(0, NULL);
   main_win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
   evas_object_show(main_win);
   segment_control = elm_segment_control_add(main_win);
   evas_object_show(segment_control);
}

static void cleanup(void)
{
   if ( NULL != main_win ) {
      evas_object_del(main_win);
      main_win = NULL;
   }
   if ( NULL != segment_control ) {
      evas_object_del(segment_control);
      segment_control = NULL;
   }
   elm_shutdown();
   tet_infoline("[[ TET_MSG ]]:: ============ Cleanup ============ ");
}

/**
 * @brief Positive test case of elm_segment_control_item_index_get()
 */
static void utc_UIFW_elm_segment_control_item_index_get_func_01(void)
{
   Elm_Object_Item *item = NULL;
   int index = -1;
   Evas_Object *segment = NULL;
   segment = elm_segment_control_add(main_win);
   evas_object_show(segment);
   item = elm_segment_control_item_add(segment, NULL, "All");
   index = elm_segment_control_item_index_get(item);
   if (index!=0) {
      tet_result(TET_FAIL);
      return;
   }
   tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init elm_segment_control_item_index_get()
 */
static void utc_UIFW_elm_segment_control_item_index_get_func_02(void)
{
   Elm_Object_Item *item = NULL;
   int index =-1;
   Evas_Object *segment = NULL;
   segment = elm_segment_control_add(main_win);
   evas_object_show(segment);
   item = elm_segment_control_item_add(segment, NULL, "All");
   index = elm_segment_control_item_index_get(NULL);
   if (index!=-1) {
      tet_infoline("elm_segment_control_item_index_get() failed in negative test case");
      tet_result(TET_FAIL);
      return;
   }
   tet_result(TET_PASS);
}
