typedef struct _Elm_Store                      Elm_Store;
typedef struct _Elm_Store_DBsystem             Elm_Store_DBsystem;
typedef struct _Elm_Store_Filesystem           Elm_Store_Filesystem;
typedef struct _Elm_Store_Item                 Elm_Store_Item;
typedef struct _Elm_Store_Item_DBsystem        Elm_Store_Item_DBsystem;
typedef struct _Elm_Store_Item_Filesystem      Elm_Store_Item_Filesystem;
typedef struct _Elm_Store_Item_Info            Elm_Store_Item_Info;
typedef struct _Elm_Store_Item_Info_Filesystem Elm_Store_Item_Info_Filesystem;
typedef struct _Elm_Store_Item_Mapping         Elm_Store_Item_Mapping;
typedef struct _Elm_Store_Item_Mapping_Empty   Elm_Store_Item_Mapping_Empty;
typedef struct _Elm_Store_Item_Mapping_Icon    Elm_Store_Item_Mapping_Icon;
typedef struct _Elm_Store_Item_Mapping_Photo   Elm_Store_Item_Mapping_Photo;
typedef struct _Elm_Store_Item_Mapping_Custom  Elm_Store_Item_Mapping_Custom;

typedef Eina_Bool (*Elm_Store_Item_List_Cb) (void *data, Elm_Store_Item_Info *info);
typedef void      (*Elm_Store_Item_Fetch_Cb) (void *data, Elm_Store_Item *sti, Elm_Store_Item_Info *info);
typedef void      (*Elm_Store_Item_Unfetch_Cb) (void *data, Elm_Store_Item *sti, Elm_Store_Item_Info *info);
typedef void      (*Elm_Store_Item_Select_Cb) (void *data, Elm_Store_Item *sti);
typedef int       (*Elm_Store_Item_Sort_Cb) (void *data, Elm_Store_Item_Info *info1, Elm_Store_Item_Info *info2);
typedef void      (*Elm_Store_Item_Free_Cb) (void *data, Elm_Store_Item_Info *info);
typedef void     *(*Elm_Store_Item_Mapping_Cb) (void *data, Elm_Store_Item *sti, const char *part);

typedef enum
{
   ELM_STORE_ITEM_MAPPING_NONE = 0,
   ELM_STORE_ITEM_MAPPING_LABEL, // const char * -> label
   ELM_STORE_ITEM_MAPPING_STATE, // Eina_Bool -> state
   ELM_STORE_ITEM_MAPPING_ICON, // char * -> icon path
   ELM_STORE_ITEM_MAPPING_PHOTO, // char * -> photo path
   ELM_STORE_ITEM_MAPPING_CUSTOM, // item->custom(it->data, it, part) -> void * (-> any)
   // can add more here as needed by common apps
   ELM_STORE_ITEM_MAPPING_LAST
} Elm_Store_Item_Mapping_Type;

struct _Elm_Store_Item_Mapping_Icon
{
   // FIXME: allow edje file icons
   int                   w, h;
   Elm_Icon_Lookup_Order lookup_order;
   Eina_Bool             standard_name : 1;
   Eina_Bool             no_scale : 1;
   Eina_Bool             smooth : 1;
   Eina_Bool             scale_up : 1;
   Eina_Bool             scale_down : 1;
};

struct _Elm_Store_Item_Mapping_Empty
{
   Eina_Bool dummy;
};

struct _Elm_Store_Item_Mapping_Photo
{
   int size;
};

struct _Elm_Store_Item_Mapping_Custom
{
   Elm_Store_Item_Mapping_Cb func;
};

struct _Elm_Store_Item_Mapping
{
   Elm_Store_Item_Mapping_Type type;
   const char                 *part;
   int                         offset;
   union
   {
      Elm_Store_Item_Mapping_Empty  empty;
      Elm_Store_Item_Mapping_Icon   icon;
      Elm_Store_Item_Mapping_Photo  photo;
      Elm_Store_Item_Mapping_Custom custom;
      // add more types here
   } details;
};

struct _Elm_Store_Item_Info
{
   int                           index;
   int                           item_type;
   int                           group_index;
   Eina_Bool                     rec_item;
   int                           pre_group_index;

   Elm_Genlist_Item_Class       *item_class;
   const Elm_Store_Item_Mapping *mapping;
   void                         *data;
   char                         *sort_id;
};


struct _Elm_Store_Item_Info_Filesystem
{
   Elm_Store_Item_Info base;
   char               *path;
};

#define ELM_STORE_ITEM_MAPPING_END { ELM_STORE_ITEM_MAPPING_NONE, NULL, 0, { .empty = { EINA_TRUE } } }
#define ELM_STORE_ITEM_MAPPING_OFFSET(st, it) offsetof(st, it)

EAPI void                    elm_store_free(Elm_Store *st);

EAPI Elm_Store              *elm_store_filesystem_new(void);
EAPI void                    elm_store_filesystem_directory_set(Elm_Store *st, const char *dir);
EAPI const char             *elm_store_filesystem_directory_get(const Elm_Store *st);
EAPI const char             *elm_store_item_filesystem_path_get(const Elm_Store_Item *sti);

EAPI void                    elm_store_target_genlist_set(Elm_Store *st, Evas_Object *obj);

EAPI void                    elm_store_cache_set(Elm_Store *st, int max);
EAPI int                     elm_store_cache_get(const Elm_Store *st);
EAPI void                    elm_store_list_func_set(Elm_Store *st, Elm_Store_Item_List_Cb func, const void *data);
EAPI void                    elm_store_fetch_func_set(Elm_Store *st, Elm_Store_Item_Fetch_Cb func, const void *data);
EAPI void                    elm_store_fetch_thread_set(Elm_Store *st, Eina_Bool use_thread);
EAPI Eina_Bool               elm_store_fetch_thread_get(const Elm_Store *st);

EAPI void                    elm_store_unfetch_func_set(Elm_Store *st, Elm_Store_Item_Unfetch_Cb func, const void *data);
EAPI void                    elm_store_sorted_set(Elm_Store *st, Eina_Bool sorted);
EAPI Eina_Bool               elm_store_sorted_get(const Elm_Store *st);
EAPI void                    elm_store_item_data_set(Elm_Store_Item *sti, void *data);
EAPI void                   *elm_store_item_data_get(Elm_Store_Item *sti);
EAPI const Elm_Store        *elm_store_item_store_get(const Elm_Store_Item *sti);
EAPI const Elm_Object_Item *elm_store_item_genlist_item_get(const Elm_Store_Item *sti);

/**
 * dbsystem Store object
 *
 * @addtogroup DBStore
 * @{
 *
 * @return The new object or NULL if it cannot be created
 */
EAPI Elm_Store              *elm_store_dbsystem_new(void);
/**
 * Sets the item count of a store
 *
 * @param st The store object
 * @param count The item count of an store
 */
EAPI void                    elm_store_item_count_set(Elm_Store *st, int count) EINA_ARG_NONNULL(1);
/**
 * Set the select func that select the state of a list item whether true or false
 *
 * @param st The store object
 * @param func The select cb function of an store
 * @param data The new data pointer to set
 */
EAPI void                    elm_store_item_select_func_set(Elm_Store *st, Elm_Store_Item_Select_Cb func, const void *data) EINA_ARG_NONNULL(1);
/**
 * Sets the sort func that sort the item with a next in the list
 *
 * @param st The store object
 * @param func The sort cb function of an store
 * @param data The new data pointer to set
 */
EAPI void                    elm_store_item_sort_func_set(Elm_Store *st, Elm_Store_Item_Sort_Cb func, const void *data) EINA_ARG_NONNULL(1);
/**
 * Set the store item free func
 *
 * @param st The store object
 * @param func The free cb function of an store
 * @param data The new data pointer to set
 */
EAPI void                    elm_store_item_free_func_set(Elm_Store *st, Elm_Store_Item_Free_Cb func, const void *data) EINA_ARG_NONNULL(1);
/**
 * Get the item index that included header items
 *
 * @param sti The store item object
 * @return The item index in genlist
 */
EAPI int                     elm_store_item_data_index_get(const Elm_Store_Item *sti) EINA_ARG_NONNULL(1);
/**
 * Get the DB pointer of an item
 *
 * @param sti The store item object
 * @return The DB pointer of item
 */
EAPI void                   *elm_store_dbsystem_db_get(const Elm_Store_Item *sti) EINA_ARG_NONNULL(1);
/**
 * Set the DB pointer of an item
 *
 * @param sti The store item object
 * @parm p_db The DB pointer of item
 */
EAPI void                    elm_store_dbsystem_db_set(Elm_Store *store, void *pDB) EINA_ARG_NONNULL(1);
/**
 */
EAPI int                     elm_store_item_index_get(const Elm_Store_Item *sti) EINA_ARG_NONNULL(1);
/**
 * Append the item to the genlist
 *
 * @param st The store object
 * @param info The store item info dbsystem object
 * @return The item of store
 */
EAPI Elm_Store_Item         *elm_store_item_add(Elm_Store *st, Elm_Store_Item_Info *info) EINA_ARG_NONNULL(1);
/**
 * Realize the visible items to the screen
 *
 * @param st The store object
 */
EAPI void                    elm_store_item_update(Elm_Store_Item *sti) EINA_ARG_NONNULL(1);
/**
 * Realize the item to the screen
 *
 * @param sti The store item object
 */
EAPI void                    elm_store_visible_items_update(Elm_Store *st) EINA_ARG_NONNULL(1);
/**
 * Delete the item of genlist
 *
 * @param sti The store item object
 */
EAPI void                    elm_store_item_del(Elm_Store_Item *sti) EINA_ARG_NONNULL(1);
EAPI void                    elm_store_free(Elm_Store *st);
EAPI Elm_Store              *elm_store_filesystem_new(void);
EAPI void                    elm_store_filesystem_directory_set(Elm_Store *st, const char *dir) EINA_ARG_NONNULL(1);
EAPI const char             *elm_store_filesystem_directory_get(const Elm_Store *st) EINA_ARG_NONNULL(1);
EAPI const char             *elm_store_item_filesystem_path_get(const Elm_Store_Item *sti) EINA_ARG_NONNULL(1);

EAPI void                    elm_store_target_genlist_set(Elm_Store *st, Evas_Object *obj) EINA_ARG_NONNULL(1);

EAPI void                    elm_store_cache_set(Elm_Store *st, int max) EINA_ARG_NONNULL(1);
EAPI int                     elm_store_cache_get(const Elm_Store *st) EINA_ARG_NONNULL(1);
EAPI void                    elm_store_list_func_set(Elm_Store *st, Elm_Store_Item_List_Cb func, const void *data) EINA_ARG_NONNULL(1, 2);
EAPI void                    elm_store_fetch_func_set(Elm_Store *st, Elm_Store_Item_Fetch_Cb func, const void *data) EINA_ARG_NONNULL(1, 2);
EAPI void                    elm_store_fetch_thread_set(Elm_Store *st, Eina_Bool use_thread) EINA_ARG_NONNULL(1);
EAPI Eina_Bool               elm_store_fetch_thread_get(const Elm_Store *st) EINA_ARG_NONNULL(1);

EAPI void                    elm_store_unfetch_func_set(Elm_Store *st, Elm_Store_Item_Unfetch_Cb func, const void *data) EINA_ARG_NONNULL(1, 2);
EAPI void                    elm_store_sorted_set(Elm_Store *st, Eina_Bool sorted) EINA_ARG_NONNULL(1);
EAPI Eina_Bool               elm_store_sorted_get(const Elm_Store *st) EINA_ARG_NONNULL(1);
EAPI void                    elm_store_item_data_set(Elm_Store_Item *sti, void *data) EINA_ARG_NONNULL(1);
EAPI void                   *elm_store_item_data_get(Elm_Store_Item *sti) EINA_ARG_NONNULL(1);
EAPI const Elm_Store        *elm_store_item_store_get(const Elm_Store_Item *sti) EINA_ARG_NONNULL(1);
