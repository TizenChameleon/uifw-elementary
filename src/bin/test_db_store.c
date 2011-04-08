#include <Elementary.h>

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#ifndef ELM_LIB_QUICKLAUNCH
#ifdef HAVE_ELEMENTARY_SQLITE3
#include "sqlite3.h"
#endif

#define BUF_SIZE 1024
#define ITEM_COUNT 1000
#define BLOCK_COUNT 10
#define GROUP_MAX (ITEM_COUNT/5)

typedef struct _My_Contact My_Contact;
typedef struct _Group_Title Group_Title;

struct _My_Contact
{
   int n_id;
   char *psz_name;
   char *psz_jobtitle;
   char *psz_mobile;
};

struct _Group_Title
{
   char *group_title;
};

static int group_index = -1;
static sqlite3 *p_db = NULL;
static char *_st_store_group_custom_label_get(void *data, Elm_Store_Item * sti, const char *part);

// callbacks just to see user interacting with genlist
static void
_st_selected(void *data __UNUSED__, Evas_Object * obj __UNUSED__, void *event_info)
{
   printf("selected: %p\n", event_info);
}

static void
_st_clicked(void *data __UNUSED__, Evas_Object * obj __UNUSED__, void *event_info)
{
   printf("clicked: %p\n", event_info);
}

static void
_st_longpress(void *data __UNUSED__, Evas_Object * obj __UNUSED__, void *event_info)
{
   printf("longpress %p\n", event_info);
}

// store callbacks to handle loading/parsing/freeing of store items from src
static char *group_title[GROUP_MAX];

static Elm_Genlist_Item_Class itc1 = {
     "message_db", {NULL, NULL, NULL, NULL}
};

static Elm_Genlist_Item_Class itc2 = {
     "group_title", {NULL, NULL, NULL, NULL}
};

static const Elm_Store_Item_Mapping it1_mapping[] = {
       {
          ELM_STORE_ITEM_MAPPING_LABEL,
          "elm.title.1", ELM_STORE_ITEM_MAPPING_OFFSET (My_Contact, psz_name),
          {.empty = {
                       EINA_TRUE}}},
       {
          ELM_STORE_ITEM_MAPPING_LABEL,
          "elm.title.2", ELM_STORE_ITEM_MAPPING_OFFSET (My_Contact, psz_jobtitle),
          {.empty = {
                       EINA_TRUE}}},
       {
          ELM_STORE_ITEM_MAPPING_LABEL,
          "elm.text", ELM_STORE_ITEM_MAPPING_OFFSET (My_Contact, psz_mobile),
          {.empty = {
                       EINA_TRUE}}},
       ELM_STORE_ITEM_MAPPING_END
};

static const Elm_Store_Item_Mapping it2_mapping[] = {
       {
          ELM_STORE_ITEM_MAPPING_CUSTOM,
          "elm.text", 0,
          {.custom = {
                        (Elm_Store_Item_Mapping_Cb)_st_store_group_custom_label_get}}},
       ELM_STORE_ITEM_MAPPING_END
};

static char *
_st_store_group_custom_label_get(void *data, Elm_Store_Item * sti, const char *part)
{
   if (!strcmp(part, "elm.text"))
     return strdup("group title");

   return strdup("");
}

////// **** WARNING ***********************************************************
////   * This function runs inside a thread outside efl mainloop. Be careful! *
//     ************************************************************************
static Eina_Bool
_st_store_list(void *data __UNUSED__, Elm_Store_Item_Info * item_info)
{
   if ((item_info->index % 5) == 0)
     {
        char gtext[128];
        int index_label = item_info->index / 5;
        sprintf(gtext, "group title (%d)", index_label);
        group_title[index_label] = strdup(gtext);
        group_index = index_label;

        item_info->item_type = ELM_GENLIST_ITEM_GROUP;
        item_info->group_index = group_index;
        item_info->rec_item = EINA_FALSE;
        item_info->pre_group_index = -1;
        item_info->item_class = &itc2;
        item_info->mapping = it2_mapping;
     }
   else
     {
        item_info->item_type = ELM_GENLIST_ITEM_NONE;
        item_info->group_index = group_index;
        item_info->rec_item = EINA_FALSE;
        item_info->pre_group_index = -1;
        item_info->item_class = &itc1;
        item_info->mapping = it1_mapping;
     }

   item_info->data = NULL;
   return EINA_TRUE;
}

//     ************************************************************************
////   * End of separate thread function.                                     *
////// ************************************************************************

////// **** WARNING ***********************************************************
////   * This function runs inside a thread outside efl mainloop. Be careful! *
//     ************************************************************************

static void
_st_store_fetch(void *data __UNUSED__, Elm_Store_Item * sti, Elm_Store_Item_Info * item_info)
{
   if (item_info->item_type == ELM_GENLIST_ITEM_GROUP)
     {
        Group_Title *pGpTitle;
        pGpTitle = calloc(1, sizeof(Group_Title));
        pGpTitle->group_title = strdup(group_title[item_info->group_index]);
        elm_store_item_data_set(sti, pGpTitle);
     }
   else
     {

        My_Contact *pmyct;

        // alloc my item in memory that holds data to show in the list
#ifdef HAVE_ELEMENTARY_SQLITE3
        sqlite3_stmt* stmt = NULL;
        char szbuf[BUF_SIZE] = {0, };
        int rc = 0;

        int start_idx = elm_store_item_data_index_get(sti);
        int fetch_count = 1;
        sqlite3 *pdb = elm_store_dbsystem_db_get(sti);

        snprintf(szbuf, BUF_SIZE, "SELECT * FROM tblEmpList ORDER BY id ASC LIMIT ?,?;");

        rc = sqlite3_prepare(pdb, szbuf, strlen(szbuf), &stmt, NULL);
        rc = sqlite3_bind_int(stmt, 1, start_idx);
        rc = sqlite3_bind_int(stmt, 2, fetch_count);
        rc = sqlite3_step(stmt);

        pmyct = calloc(1, sizeof(My_Contact));
        pmyct->n_id = sqlite3_column_int(stmt, 0);
        pmyct->psz_name = strdup((const char *)sqlite3_column_text(stmt, 1));
        pmyct->psz_jobtitle= strdup((const char *)sqlite3_column_text(stmt, 2));
        pmyct->psz_mobile = strdup((const char *)sqlite3_column_text(stmt, 3));

        rc = sqlite3_finalize(stmt);
#else
        int start_idx = elm_store_item_data_index_get(sti);
        pmyct = calloc(1, sizeof(My_Contact));
        pmyct->n_id = start_idx;
        pmyct->psz_name = strdup("Name");
        pmyct->psz_jobtitle = strdup("Title");
        pmyct->psz_mobile = strdup("Mobile");
#endif
        elm_store_item_data_set(sti, pmyct);
     }
   return;
}

//     ************************************************************************
////   * End of separate thread function.                                     *
////// ************************************************************************
static void
_st_store_unfetch(void *data __UNUSED__, Elm_Store_Item * sti, Elm_Store_Item_Info * item_info)
{
   if (item_info->item_type == ELM_GENLIST_ITEM_GROUP)
     {
        Group_Title *myit = elm_store_item_data_get(sti);

        if (!myit)
          return;
        if (myit->group_title)
          free(myit->group_title);
        free(myit);

     }
   else
     {
        My_Contact *myit = elm_store_item_data_get(sti);

        if (!myit)
          return;
        if (myit->psz_name)
          free(myit->psz_name);
        if (myit->psz_jobtitle)
          free(myit->psz_jobtitle);
        if (myit->psz_mobile)
          free(myit->psz_mobile);
        free(myit);
     }
   elm_store_item_data_set(sti, NULL);
}

static void
_st_store_item_select(void *data, Evas_Object * obj, void *event_info)
{
   Elm_Genlist_Item *gli = event_info;
   Elm_Store_Item *sti = elm_genlist_item_data_get(gli);

   if (sti)
     {
        int index = elm_store_item_data_index_get(sti);
        printf("item %d is selected\n", index);
        elm_genlist_item_selected_set(gli, EINA_FALSE);
     }
}

void
test_db_store(void *data __UNUSED__, Evas_Object * obj __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Object *win, *bg, *gl, *bx;
   Elm_Store *st;

   win = elm_win_add(NULL, "db-store", ELM_WIN_BASIC);
   elm_win_title_set(win, "Store");
   elm_win_autodel_set(win, 1);

   bg = elm_bg_add(win);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   gl = elm_genlist_add(win);
   elm_genlist_homogeneous_set(gl, EINA_TRUE);
   elm_genlist_compress_mode_set(gl, EINA_TRUE);
   elm_genlist_block_count_set(gl, BLOCK_COUNT);
   elm_genlist_height_for_width_mode_set(gl, EINA_TRUE);
   evas_object_smart_callback_add(gl, "selected", _st_selected, NULL);
   evas_object_smart_callback_add(gl, "clicked", _st_clicked, NULL);
   evas_object_smart_callback_add(gl, "longpressed", _st_longpress, NULL);
   evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, gl);
   evas_object_show(gl);

   // Set the db handler to store. after open it
   // Then store will add items as you set the item count number
#ifdef HAVE_ELEMENTARY_SQLITE3
   if (!p_db)
     {
        int rc = sqlite3_open("./genlist.db", &p_db);
        if(SQLITE_OK != rc)
          {
             printf("Fail to open DB ./genlist.db\n");
          }
     }
#endif

   st = elm_store_dbsystem_new();
   elm_store_fetch_thread_set(st, EINA_TRUE);
   elm_store_item_count_set(st, ITEM_COUNT);
   elm_store_list_func_set(st, _st_store_list, NULL);
   elm_store_fetch_func_set(st, _st_store_fetch, NULL);
   elm_store_unfetch_func_set(st, _st_store_unfetch, NULL);
   elm_store_item_select_func_set(st, (Elm_Store_Item_Select_Cb)_st_store_item_select, NULL);
   elm_store_target_genlist_set(st, gl);
   elm_store_dbsystem_db_set(st, p_db);

   evas_object_resize(win, 480, 800);
   evas_object_show(win);
}
#endif
