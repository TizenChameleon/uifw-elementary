#include <Elementary.h>
#include <Elementary_Cursor.h>
#include "elm_priv.h"

#define ELM_STORE_MAGIC            0x3f89ea56
#define ELM_STORE_FILESYSTEM_MAGIC 0x3f89ea57
#define ELM_STORE_DBSYSTEM_MAGIC   0x3f89ea58
#define ELM_STORE_ITEM_MAGIC       0x5afe8c1d
#define CACHE_COUNT                1024

struct _Elm_Store
{
   EINA_MAGIC;
   void           (*free)(Elm_Store *store);
   struct {
      void        (*free)(Elm_Store_Item *item);
   } item;
   Evas_Object   *genlist;
   Ecore_Thread  *list_th;
   Eina_Inlist   *items;
   Eina_List     *realized;
   int            realized_count;
   int            cache_max;
   int            item_count;
   int            type;
   Eina_List     *always_fetched;
   struct {
      struct {
         Elm_Store_Item_List_Cb     func;
         void                      *data;
      } list;
      struct {
         Elm_Store_Item_Fetch_Cb    func;
         void                      *data;
      } fetch;
      struct {
         Elm_Store_Item_Unfetch_Cb  func;
         void                      *data;
      } unfetch;
      struct {
         Elm_Store_Item_Select_Cb   func;
         void                      *data;
      } item_select;
      struct {
         Elm_Store_Item_Sort_Cb     func;
         void                      *data;
      } item_sort;
      struct {
         Elm_Store_Item_Free_Cb     func;
         void                      *data;
      } item_free;
   } cb;
   Eina_Bool      sorted : 1;
   Eina_Bool      fetch_thread : 1;
   Eina_Bool      live : 1;
};

struct _Elm_Store_Item
{
   EINA_INLIST;
   EINA_MAGIC;
   Elm_Store                    *store;
   Elm_Object_Item             *item;
   Ecore_Thread                 *fetch_th;
   Ecore_Job                    *eval_job;
   const Elm_Store_Item_Mapping *mapping;
   void                         *data;
   Elm_Store_Item_Info          *item_info;
   Elm_Object_Item             *first_item;
   Elm_Object_Item             *last_item;
   Eina_Lock                     lock;
   Eina_Bool                     live : 1;
   Eina_Bool                     was_live : 1;
   Eina_Bool                     realized : 1;
   Eina_Bool                     fetched : 1;
};

struct _Elm_Store_Filesystem
{
   Elm_Store base;
   EINA_MAGIC;
   const char *dir;
};

struct _Elm_Store_Item_Filesystem
{
   Elm_Store_Item base;
   const char *path;
};

struct _Elm_Store_DBsystem
{
   Elm_Store   base;
   EINA_MAGIC;
   void       *p_db;
};

typedef enum
{
   ELM_STORE_ITEM_SORT_LOW = -1,
   ELM_STORE_ITEM_SORT_SAME = 0,
   ELM_STORE_ITEM_SORT_HIGH = 1,
   ELM_STORE_ITEM_SORT_UNKNOWN = 2,
   ELM_STORE_ITEM_SORT_LAST
} Elm_Store_Item_Sort_Type;

static Elm_Genlist_Item_Class _store_item_class;

static void _item_del(void *data, Evas_Object *obj __UNUSED__);
static void _item_realized(void *data, Evas_Object *obj __UNUSED__, void *event_info);
static void _item_unrealized(void *data, Evas_Object *obj __UNUSED__, void *event_info);
static void _genlist_del(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__);

static void
_store_cache_trim(Elm_Store *st)
{
   while ((st->realized ) &&
          (((int)eina_list_count(st->realized) - st->realized_count)
           > st->cache_max))
     {
        Elm_Store_Item *sti = st->realized->data;
        if (sti->realized)
          {
             st->realized = eina_list_remove_list(st->realized, st->realized);
             sti->realized = EINA_FALSE;
          }
        eina_lock_take(&sti->lock);
        if (!sti->fetched)
          {
             eina_lock_release(&sti->lock);
             if (sti->fetch_th)
               {
                  ecore_thread_cancel(sti->fetch_th);
                  sti->fetch_th = NULL;
               }
             eina_lock_take(&sti->lock);
          }
        sti->fetched = EINA_FALSE;
        eina_lock_release(&sti->lock);
        if (st->cb.unfetch.func)
          st->cb.unfetch.func(st->cb.unfetch.data, sti, NULL);
        eina_lock_take(&sti->lock);
        sti->data = NULL;
        eina_lock_release(&sti->lock);
     }
}

static void
_store_genlist_del(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Store *st = data;
   st->genlist = NULL;
   if (st->list_th)
     {
        ecore_thread_cancel(st->list_th);
        st->list_th = NULL;
     }
   eina_list_free(st->realized);
   while (st->items)
     {
        Elm_Store_Item *sti = (Elm_Store_Item *)st->items;
        if (sti->eval_job) ecore_job_del(sti->eval_job);
        if (sti->fetch_th)
          {
             ecore_thread_cancel(sti->fetch_th);
             sti->fetch_th = NULL;
          }
        if (sti->store->item.free) sti->store->item.free(sti);
        if (sti->data)
          {
             if (st->cb.unfetch.func)
               st->cb.unfetch.func(st->cb.unfetch.data, sti, NULL);
             sti->data = NULL;
          }
        eina_lock_free(&sti->lock);
	st->items = NULL;
        free(sti);
     }
   // FIXME: kill threads and more
}

////// **** WARNING ***********************************************************
////   * This function runs inside a thread outside efl mainloop. Be careful! *
//     ************************************************************************
/* TODO: refactor lock part into core? this does not depend on filesystm part */
static void
_store_filesystem_fetch_do(void *data, Ecore_Thread *th __UNUSED__)
{
   Elm_Store_Item *sti = data;
   eina_lock_take(&sti->lock);
   if (sti->data)
     {
        eina_lock_release(&sti->lock);
        return;
     }
   if (!sti->fetched)
     {
        eina_lock_release(&sti->lock);
        if (sti->store->cb.fetch.func)
          sti->store->cb.fetch.func(sti->store->cb.fetch.data, sti, NULL);
        eina_lock_take(&sti->lock);
        sti->fetched = EINA_TRUE;
     }
   eina_lock_release(&sti->lock);
}
//     ************************************************************************
////   * End of separate thread function.                                     *
////// ************************************************************************
/* TODO: refactor lock part into core? this does not depend on filesystm part */
static void
_store_filesystem_fetch_end(void *data, Ecore_Thread *th)
{
   Elm_Store_Item *sti = data;
   eina_lock_take(&sti->lock);
   if (sti->data) elm_genlist_item_update((Elm_Object_Item *) sti->item);
   eina_lock_release(&sti->lock);
   if (th == sti->fetch_th) sti->fetch_th = NULL;
}

/* TODO: refactor lock part into core? this does not depend on filesystm part */
static void
_store_filesystem_fetch_cancel(void *data, Ecore_Thread *th)
{
   Elm_Store_Item *sti = data;
   eina_lock_take(&sti->lock);
   if (th == sti->fetch_th) sti->fetch_th = NULL;
   if (sti->data) elm_genlist_item_update((Elm_Object_Item *) sti->item);
   eina_lock_release(&sti->lock);
}

static void
_store_item_eval(void *data)
{
   Elm_Store_Item *sti = data;
   sti->eval_job = NULL;
   if (sti->live == sti->was_live) return;
   sti->was_live = sti->live;
   if (sti->live)
     {
        _store_cache_trim(sti->store);
        if (sti->realized)
          sti->store->realized = eina_list_remove(sti->store->realized, sti);
        sti->store->realized = eina_list_append(sti->store->realized, sti);
        sti->realized = EINA_TRUE;
        if ((sti->store->fetch_thread) && (!sti->fetch_th))
          sti->fetch_th = ecore_thread_run(_store_filesystem_fetch_do,
                                           _store_filesystem_fetch_end,
                                           _store_filesystem_fetch_cancel,
                                           sti);
        else if ((!sti->store->fetch_thread))
          {
             _store_filesystem_fetch_do(sti, NULL);
             _store_filesystem_fetch_end(sti, NULL);
          }
     }
   else
     {
        if (sti->fetch_th)
          {
             ecore_thread_cancel(sti->fetch_th);
             sti->fetch_th = NULL;
          }
        _store_cache_trim(sti->store);
     }
}

static void
_store_genlist_item_realized(void *data, Evas_Object *obj __UNUSED__, void *event_info)
{
   Elm_Store *st = data;
   Elm_Object_Item *gli = event_info;
   Elm_Store_Item *sti = (Elm_Store_Item *) elm_genlist_item_data_get(gli);
   if (!sti) return;
   st->realized_count++;
   sti->live = EINA_TRUE;
   if (sti->eval_job) ecore_job_del(sti->eval_job);
   sti->eval_job = ecore_job_add(_store_item_eval, sti);
}

static void
_store_genlist_item_unrealized(void *data, Evas_Object *obj __UNUSED__, void *event_info)
{
   Elm_Store *st = data;
   Elm_Object_Item *gli = event_info;
   Elm_Store_Item *sti = (Elm_Store_Item *) elm_genlist_item_data_get(gli);
   if (!sti) return;
   st->realized_count--;
   sti->live = EINA_FALSE;
   if (sti->eval_job) ecore_job_del(sti->eval_job);
   sti->eval_job = ecore_job_add(_store_item_eval, sti);
}

static const Elm_Store_Item_Mapping *
_store_item_mapping_find(Elm_Store_Item *sti, const char *part)
{
   const Elm_Store_Item_Mapping *m;

   for (m = sti->mapping; m; m ++)
     {
        if (m->type == ELM_STORE_ITEM_MAPPING_NONE) break;
        if (!strcmp(part, m->part)) return m;
     }
   return NULL;
}

static char *
_store_item_text_get(void *data, Evas_Object *obj __UNUSED__, const char *part)
{
   Elm_Store_Item *sti = data;
   const char *s = "";
   eina_lock_take(&sti->lock);
   if (sti->data)
     {
        const Elm_Store_Item_Mapping *m = _store_item_mapping_find(sti, part);
        if (m)
          {
             switch (m->type)
               {
                case ELM_STORE_ITEM_MAPPING_LABEL:
                   s = *(char **)(((unsigned char *)sti->data) + m->offset);
                   break;
                case ELM_STORE_ITEM_MAPPING_CUSTOM:
                   if (m->details.custom.func)
                     s = m->details.custom.func(sti->data, sti, part);
                   break;
                default:
                   break;
               }
          }
     }
   eina_lock_release(&sti->lock);
   return s ? strdup(s) : NULL;
}

static Evas_Object *
_store_item_content_get(void *data, Evas_Object *obj, const char *part)
{
   Elm_Store_Item *sti = data;
   eina_lock_take(&sti->lock);
   if (sti->data)
     {
        const Elm_Store_Item_Mapping *m = _store_item_mapping_find(sti, part);
        if (m)
          {
             Evas_Object *ic = NULL;
             const char *s = NULL;

             switch (m->type)
               {
                case ELM_STORE_ITEM_MAPPING_ICON:
                   ic = elm_icon_add(obj);
                   s = *(char **)(((unsigned char *)sti->data) + m->offset);
                   elm_icon_order_lookup_set(ic, m->details.icon.lookup_order);
                   evas_object_size_hint_aspect_set(ic,
                                                    EVAS_ASPECT_CONTROL_VERTICAL,
                                                    m->details.icon.w,
                                                    m->details.icon.h);
                   elm_icon_smooth_set(ic, m->details.icon.smooth);
                   elm_icon_no_scale_set(ic, m->details.icon.no_scale);
                   elm_icon_scale_set(ic,
                                      m->details.icon.scale_up,
                                      m->details.icon.scale_down);
                   if (s)
                     {
                        if (m->details.icon.standard_name)
                          elm_icon_standard_set(ic, s);
                        else
                          elm_icon_file_set(ic, s, NULL);
                     }
                   break;
                case ELM_STORE_ITEM_MAPPING_PHOTO:
                   ic = elm_icon_add(obj);
                   s = *(char **)(((unsigned char *)sti->data) + m->offset);
                   elm_photo_size_set(ic, m->details.photo.size);
                   if (s)
                     elm_photo_file_set(ic, s);
                   break;
                case ELM_STORE_ITEM_MAPPING_CUSTOM:
                   if (m->details.custom.func)
                     ic = m->details.custom.func(sti->data, sti, part);
                   break;
                default:
                   break;
               }
             eina_lock_release(&sti->lock);
             return ic;
          }
     }
   eina_lock_release(&sti->lock);
   return NULL;
}

static void
_store_item_del(void *data __UNUSED__, Evas_Object *obj __UNUSED__)
{
}

////// **** WARNING ***********************************************************
////   * This function runs inside a thread outside efl mainloop. Be careful! *
//     ************************************************************************
static int
_store_filesystem_sort_cb(void *d1, void *d2)
{
   Elm_Store_Item_Info *info1 = d1, *info2 = d2;
   if ((!info1->sort_id) || (!info2->sort_id)) return 0;
   return strcoll(info1->sort_id, info2->sort_id);
}

static void
_store_filesystem_list_do(void *data, Ecore_Thread *th __UNUSED__)
{
   Elm_Store_Filesystem *st = data;
   Eina_Iterator *it;
   const Eina_File_Direct_Info *finf;
   Eina_List *sorted = NULL;
   Elm_Store_Item_Info_Filesystem *info;

   // FIXME: need a way to abstract the open, list, feed items from list
   // and maybe get initial sortable key vals etc.
   it = eina_file_stat_ls(st->dir);
   if (!it) return;
   EINA_ITERATOR_FOREACH(it, finf)
     {
        Eina_Bool ok;
        size_t pathsz = finf->path_length + 1;

	if (finf->path[finf->name_start] == '.') continue ;

        info = calloc(1, sizeof(Elm_Store_Item_Info_Filesystem) + pathsz);
        if (!info) continue;
        info->path = ((char *)info) + sizeof(Elm_Store_Item_Info_Filesystem);
        memcpy(info->path, finf->path, pathsz);
        ok = EINA_TRUE;
        if (st->base.cb.list.func)
          ok = st->base.cb.list.func(st->base.cb.list.data, &info->base);
        if (ok)
          {
             if (!st->base.sorted) ecore_thread_feedback(th, info);
             else sorted = eina_list_append(sorted, info);
          }
        else
          {
             if (info->base.sort_id) free(info->base.sort_id);
             free(info);
          }
        if (ecore_thread_check(th)) break;
     }
   eina_iterator_free(it);
   if (sorted)
     {
        sorted = eina_list_sort(sorted, 0,
                                EINA_COMPARE_CB(_store_filesystem_sort_cb));
        EINA_LIST_FREE(sorted, info)
          {
             if (!ecore_thread_check(th)) ecore_thread_feedback(th, info);
          }
     }
}
//     ************************************************************************
////   * End of separate thread function.                                     *
////// ************************************************************************

static void
_store_filesystem_list_end(void *data, Ecore_Thread *th)
{
   Elm_Store *st = data;
   if (th == st->list_th) st->list_th = NULL;
}

static void
_store_filesystem_list_cancel(void *data, Ecore_Thread *th)
{
   Elm_Store *st = data;
   if (th == st->list_th) st->list_th = NULL;
}

static void
_store_filesystem_list_update(void *data, Ecore_Thread *th __UNUSED__, void *msg)
{
   Elm_Store *st = data;
   Elm_Store_Item_Filesystem *sti;
   Elm_Genlist_Item_Class *itc;
   Elm_Store_Item_Info_Filesystem *info = msg;

   sti = calloc(1, sizeof(Elm_Store_Item_Filesystem));
   if (!sti) goto done;
   eina_lock_new(&sti->base.lock);
   EINA_MAGIC_SET(&(sti->base), ELM_STORE_ITEM_MAGIC);
   sti->base.store = st;
   sti->base.data = info->base.data;
   sti->base.mapping = info->base.mapping;
   sti->path = eina_stringshare_add(info->path);

   itc = info->base.item_class;
   if (!itc) itc = &_store_item_class;
   else
     {
        itc->func.text_get = (GenlistItemTextGetFunc)_store_item_text_get;
        itc->func.content_get  = (GenlistItemContentGetFunc)_store_item_content_get;
        itc->func.state_get = NULL; // FIXME: support state gets later
        itc->func.del = (GenlistItemDelFunc)_store_item_del;
     }

   // FIXME: handle being a parent (tree)
   sti->base.item = elm_genlist_item_append(st->genlist, itc,
                                            sti/* item data */,
                                            NULL/* parent */,
                                            ELM_GENLIST_ITEM_NONE,
                                            NULL/* func */,
                                            NULL/* func data */);
   st->items = eina_inlist_append(st->items, (Eina_Inlist *)sti);
done:
   if (info->base.sort_id) free(info->base.sort_id);
   free(info);
}

// public api calls
static Elm_Store *
_elm_store_new(size_t size)
{
   Elm_Store *st = calloc(1, size);
   EINA_SAFETY_ON_NULL_RETURN_VAL(st, NULL);

   // TODO: BEGIN - move to elm_store_init()
   eina_magic_string_set(ELM_STORE_MAGIC, "Elm_Store");
   eina_magic_string_set(ELM_STORE_FILESYSTEM_MAGIC, "Elm_Store_Filesystem");
   eina_magic_string_set(ELM_STORE_ITEM_MAGIC, "Elm_Store_Item");
   // setup default item class (always the same) if list cb doesnt provide one
   _store_item_class.item_style = "default";
   _store_item_class.func.text_get = (GenlistItemTextGetFunc)_store_item_text_get;
   _store_item_class.func.content_get  = (GenlistItemContentGetFunc)_store_item_content_get;
   _store_item_class.func.state_get = NULL; // FIXME: support state gets later
   _store_item_class.func.del       = (GenlistItemDelFunc)_store_item_del;
   // TODO: END - move to elm_store_init()

   EINA_MAGIC_SET(st, ELM_STORE_MAGIC);
   st->cache_max = 128;
   st->fetch_thread = EINA_TRUE;
   st->type = 0;
   return st;
}
#define elm_store_new(type) (type*)_elm_store_new(sizeof(type))

static void
_elm_store_filesystem_free(Elm_Store *store)
{
   Elm_Store_Filesystem *st = (Elm_Store_Filesystem *)store;
   eina_stringshare_del(st->dir);
}

static void
_elm_store_filesystem_item_free(Elm_Store_Item *item)
{
   Elm_Store_Item_Filesystem *sti = (Elm_Store_Item_Filesystem *)item;
   eina_stringshare_del(sti->path);
}

EAPI Elm_Store *
elm_store_filesystem_new(void)
{
   Elm_Store_Filesystem *st = elm_store_new(Elm_Store_Filesystem);
   EINA_SAFETY_ON_NULL_RETURN_VAL(st, NULL);

   EINA_MAGIC_SET(st, ELM_STORE_FILESYSTEM_MAGIC);
   st->base.free = _elm_store_filesystem_free;
   st->base.item.free = _elm_store_filesystem_item_free;

   return &st->base;
}

EAPI void
elm_store_free(Elm_Store *st)
{
   void (*item_free)(Elm_Store_Item *);
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   if (st->list_th)
     {
        ecore_thread_cancel(st->list_th);
        st->list_th = NULL;
     }

   if (!st->type)
     {
        eina_list_free(st->realized);
        item_free = st->item.free;
        while (st->items)
          {
             Elm_Store_Item *sti = (Elm_Store_Item *)st->items;
             if (sti->eval_job) ecore_job_del(sti->eval_job);
             if (sti->fetch_th)
               {
                  ecore_thread_cancel(sti->fetch_th);
                  sti->fetch_th = NULL;
               }
             if (item_free) item_free(sti);
             if (sti->data)
               {
                  if (st->cb.unfetch.func)
                  st->cb.unfetch.func(st->cb.unfetch.data, sti, NULL);
                  sti->data = NULL;
               }
             eina_lock_free(&sti->lock);
             free(sti);
          }
        if (st->genlist)
          {
             evas_object_event_callback_del_full(st->genlist, EVAS_CALLBACK_DEL, _store_genlist_del, st);
             evas_object_smart_callback_del(st->genlist, "realized", _store_genlist_item_realized);
             evas_object_smart_callback_del(st->genlist, "unrealized", _store_genlist_item_unrealized);
             elm_genlist_clear(st->genlist);
             st->genlist = NULL;
          }
        if (st->free) st->free(st);
     }
   else
     {
        if (st->genlist)
          {
             evas_object_event_callback_del_full(st->genlist, EVAS_CALLBACK_DEL, _genlist_del, st);
             evas_object_smart_callback_del(st->genlist, "realized", _item_realized);
             evas_object_smart_callback_del(st->genlist, "unrealized", _item_unrealized);
             elm_genlist_clear(st->genlist);
             st->genlist = NULL;
          }
        while (st->always_fetched)
          {
             Elm_Store_Item *sti = eina_list_data_get(st->always_fetched);
             Eina_List *find = NULL;
             find = eina_list_data_find_list(st->always_fetched, sti);
             if (find)
               {
                  st->always_fetched = eina_list_remove_list(st->always_fetched, find);
                  _item_del(sti,NULL);
               }
          }
        st->always_fetched = eina_list_free(st->always_fetched);
        st->realized = eina_list_free(st->realized);
        if (st->free) st->free(st);
        st->live = EINA_FALSE;
     }
   free(st);
}

EAPI void
elm_store_target_genlist_set(Elm_Store *st, Evas_Object *obj)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   if (st->genlist == obj) return;
   if (st->genlist)
     {
        if (!st->type)
          {
             evas_object_event_callback_del_full(st->genlist, EVAS_CALLBACK_DEL, _store_genlist_del, st);
             evas_object_smart_callback_del(st->genlist, "realized", _store_genlist_item_realized);
             evas_object_smart_callback_del(st->genlist, "unrealized", _store_genlist_item_unrealized);
          }
        else
          {
             evas_object_event_callback_del_full(st->genlist, EVAS_CALLBACK_DEL, _genlist_del, st);
             evas_object_smart_callback_del(st->genlist, "realized", _item_realized);
             evas_object_smart_callback_del(st->genlist, "unrealized", _item_unrealized);
          }
        elm_genlist_clear(st->genlist);
     }
   st->genlist = obj;
   if (!st->genlist) return;
   if (!st->type)
     {
        evas_object_smart_callback_add(st->genlist, "realized", _store_genlist_item_realized, st);
        evas_object_smart_callback_add(st->genlist, "unrealized", _store_genlist_item_unrealized, st);
        evas_object_event_callback_add(st->genlist, EVAS_CALLBACK_DEL, _store_genlist_del, st);
     }
   else
     {
        evas_object_smart_callback_add(st->genlist, "realized", _item_realized, st);
        evas_object_smart_callback_add(st->genlist, "unrealized", _item_unrealized, st);
        evas_object_event_callback_add(st->genlist, EVAS_CALLBACK_DEL, _genlist_del, st);
     }
   elm_genlist_clear(st->genlist);
}

EAPI void
elm_store_filesystem_directory_set(Elm_Store *store, const char *dir)
{
   Elm_Store_Filesystem *st = (Elm_Store_Filesystem *)store;
   if (!EINA_MAGIC_CHECK(store, ELM_STORE_MAGIC)) return;
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_FILESYSTEM_MAGIC)) return;
   if (store->list_th)
     {
        ecore_thread_cancel(store->list_th);
        store->list_th = NULL;
     }
   if (!eina_stringshare_replace(&st->dir, dir)) return;
   store->list_th = ecore_thread_feedback_run(_store_filesystem_list_do,
                                              _store_filesystem_list_update,
                                              _store_filesystem_list_end,
                                              _store_filesystem_list_cancel,
                                              st, EINA_TRUE);
}

EAPI const char *
elm_store_filesystem_directory_get(const Elm_Store *store)
{
   const Elm_Store_Filesystem *st = (const Elm_Store_Filesystem *)store;
   if (!EINA_MAGIC_CHECK(store, ELM_STORE_MAGIC)) return NULL;
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_FILESYSTEM_MAGIC)) return NULL;
   return st->dir;
}

EAPI void
elm_store_cache_set(Elm_Store *st, int max)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   if (max < 0) max = 0;
   st->cache_max = max;
   if(!st->type) _store_cache_trim(st);
}

EAPI int
elm_store_cache_get(const Elm_Store *st)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return 0;
   return st->cache_max;
}

EAPI void
elm_store_list_func_set(Elm_Store *st, Elm_Store_Item_List_Cb func, const void *data)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   st->cb.list.func = func;
   st->cb.list.data = (void *)data;
}

EAPI void
elm_store_fetch_func_set(Elm_Store *st, Elm_Store_Item_Fetch_Cb func, const void *data)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   st->cb.fetch.func = func;
   st->cb.fetch.data = (void *)data;
}

EAPI void
elm_store_fetch_thread_set(Elm_Store *st, Eina_Bool use_thread)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   st->fetch_thread = !!use_thread;
}

EAPI Eina_Bool
elm_store_fetch_thread_get(const Elm_Store *st)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return EINA_FALSE;
   return st->fetch_thread;
}

EAPI void
elm_store_unfetch_func_set(Elm_Store *st, Elm_Store_Item_Unfetch_Cb func, const void *data)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   st->cb.unfetch.func = func;
   st->cb.unfetch.data = (void *)data;
}

EAPI void
elm_store_sorted_set(Elm_Store *st, Eina_Bool sorted)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   st->sorted = sorted;
}

EAPI Eina_Bool
elm_store_sorted_get(const Elm_Store *st)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return EINA_FALSE;
   return st->sorted;
}

EAPI void
elm_store_item_data_set(Elm_Store_Item *sti, void *data)
{
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return;
   eina_lock_take(&sti->lock);
   sti->data = data;
   eina_lock_release(&sti->lock);
}

EAPI void *
elm_store_item_data_get(Elm_Store_Item *sti)
{
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return NULL;
   void *d;
   eina_lock_take(&sti->lock);
   d = sti->data;
   eina_lock_release(&sti->lock);
   return d;
}

EAPI const Elm_Store *
elm_store_item_store_get(const Elm_Store_Item *sti)
{
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return NULL;
   // dont need lock
   return sti->store;
}

EAPI const Elm_Object_Item *
elm_store_item_genlist_item_get(const Elm_Store_Item *sti)
{
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return NULL;
   // dont need lock
   return sti->item;
}

EAPI const char *
elm_store_item_filesystem_path_get(const Elm_Store_Item *item)
{
   Elm_Store_Item_Filesystem *sti = (Elm_Store_Item_Filesystem *)item;
   Elm_Store_Filesystem *st;
   if (!EINA_MAGIC_CHECK(item, ELM_STORE_ITEM_MAGIC)) return NULL;
   if (!EINA_MAGIC_CHECK(item->store, ELM_STORE_MAGIC)) return NULL;
   /* ensure we're dealing with filesystem item */
   st = (Elm_Store_Filesystem *)item->store;
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_FILESYSTEM_MAGIC)) return NULL;
   // dont need lock
   return sti->path;
}

// TODO: BEGIN -DBsystem store

static const Elm_Store_Item_Mapping *
_item_mapping_find(Elm_Store_Item *sti, const char *part)
{
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return NULL;
   const Elm_Store_Item_Mapping *m;

   if (!sti->item_info) return NULL;

   for (m = sti->item_info->mapping; m; m++)
     {
        if (m->type == ELM_STORE_ITEM_MAPPING_NONE) break;
        if (!strcmp(part, m->part)) return m;
     }
   return NULL;
}

static char *
_item_text_get(void *data, Evas_Object *obj __UNUSED__, const char *part)
{
   Elm_Store_Item *sti = data;
   Elm_Store *st = sti->store;
   if (st->live)
     {
        eina_lock_take(&sti->lock);
        if (sti->data)
          {
             const char *s = "";
             const Elm_Store_Item_Mapping *m = _item_mapping_find(sti, part);
             if (m)
               {
                  switch (m->type)
                    {
                     case ELM_STORE_ITEM_MAPPING_LABEL:
                        eina_lock_release(&sti->lock);
                        s = *(char **)(((unsigned char *)sti->data) + m->offset);
                        eina_lock_take(&sti->lock);
                        break;

                     case ELM_STORE_ITEM_MAPPING_CUSTOM:
                        if (m->details.custom.func)
                          {
                             eina_lock_release(&sti->lock);
                             s = m->details.custom.func(sti->data, sti, part);
                             eina_lock_take(&sti->lock);
                          }
                        break;

                     default:
                        break;
                    }
                  if (s)
                    {
                       eina_lock_release(&sti->lock);
                       return strdup(s);
                    }
                  else
                    {
                       eina_lock_release(&sti->lock);
                       return NULL;
                    }
               }
          }
        else
          {
             const char *s = "";
             const Elm_Store_Item_Mapping *m = _item_mapping_find(sti, part);
             if (m->type == ELM_STORE_ITEM_MAPPING_CUSTOM)
               {
                  if (m->details.custom.func)
                    {
                       eina_lock_release(&sti->lock);
                       s = m->details.custom.func(sti->item_info, sti, part);
                       eina_lock_take(&sti->lock);
                    }

                  if (s)
                    {
                       eina_lock_release(&sti->lock);
                       return strdup(s);
                    }
                  else
                    {
                       eina_lock_release(&sti->lock);
                       return NULL;
                    }
               }
             eina_lock_release(&sti->lock);
             return NULL;
             /*
                if (!strcmp(part, "elm.text.1"))
                {
                eina_lock_release(&sti->lock);
             //             	   	   elm_genlist_item_display_only_set(sti->item, EINA_TRUE);
             return strdup("Loading..");
             }
              */
          }
        eina_lock_release(&sti->lock);
     }
   return NULL;
}

static Evas_Object *
_item_content_get(void *data, Evas_Object *obj, const char *part)
{
   Elm_Store_Item *sti = data;
   Elm_Store *st = sti->store;

   if (st->live && sti->item)
     {
        eina_lock_take(&sti->lock);
        if (sti->data)
          {
             const Elm_Store_Item_Mapping *m = _item_mapping_find(sti, part);
             if (m)
               {
                  Evas_Object *ic = NULL;
                  const char *s = NULL;

                  switch (m->type)
                    {
                     case ELM_STORE_ITEM_MAPPING_ICON:
                        ic = elm_icon_add(obj);
                        s = *(char **)(((unsigned char *)sti->data) + m->offset);
                        elm_icon_order_lookup_set(ic, m->details.icon.lookup_order);
                        evas_object_size_hint_aspect_set(ic,
                                                         EVAS_ASPECT_CONTROL_VERTICAL,
                                                         m->details.icon.w,
                                                         m->details.icon.h);
                        elm_icon_smooth_set(ic, m->details.icon.smooth);
                        elm_icon_no_scale_set(ic, m->details.icon.no_scale);
                        elm_icon_scale_set(ic,
                                           m->details.icon.scale_up,
                                           m->details.icon.scale_down);

                        if (s)
                          {
                             if (m->details.icon.standard_name)
                               elm_icon_standard_set(ic, s);
                             else
                               elm_icon_file_set(ic, s, NULL);
                          }
                        break;

                     case ELM_STORE_ITEM_MAPPING_PHOTO:
                        ic = elm_icon_add(obj);
                        s = *(char **)(((unsigned char *)sti->data) + m->offset);
                        elm_photo_size_set(ic, m->details.photo.size);
                        if (s)
                          elm_photo_file_set(ic, s);
                        break;

                     case ELM_STORE_ITEM_MAPPING_CUSTOM:
                        if (m->details.custom.func)
                          ic = m->details.custom.func(sti->data, sti, part);
                        break;

                     default:
                        break;
                    }
                  eina_lock_release(&sti->lock);
                  return ic;
               }
          }
        eina_lock_release(&sti->lock);
     }
   return NULL;
}

static Elm_Store *
_store_init(size_t size)
{
   Elm_Store *st = calloc(1, size);
   EINA_SAFETY_ON_NULL_RETURN_VAL(st, NULL);

   eina_magic_string_set(ELM_STORE_MAGIC, "Elm_Store");
   eina_magic_string_set(ELM_STORE_ITEM_MAGIC, "Elm_Store_Item");
   eina_magic_string_set(ELM_STORE_DBSYSTEM_MAGIC, "Elm_Store_DBsystem");

   _store_item_class.item_style = "default";
   _store_item_class.func.text_get = (GenlistItemTextGetFunc)_item_text_get;
   _store_item_class.func.content_get = (GenlistItemContentGetFunc)_item_content_get;
   _store_item_class.func.state_get = NULL;
   _store_item_class.func.del = NULL;

   EINA_MAGIC_SET(st, ELM_STORE_MAGIC);
   st->cache_max = CACHE_COUNT;
   st->live = EINA_TRUE;
   st->fetch_thread = EINA_FALSE;
   st->type = 1;
   return st;
}

#define _store_new(type) (type *)_store_init(sizeof(type))

static void
_genlist_del(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Store *st = data;
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;

   st->genlist = NULL;
   if (st->list_th)
     {
        ecore_thread_cancel(st->list_th);
        st->list_th = NULL;
     }
   st->realized = eina_list_free(st->realized);
}

static void
_store_fetch_do(void *data, Ecore_Thread *th __UNUSED__)
{
   Elm_Store_Item *sti = data;

   eina_lock_take(&sti->lock);
   if (sti->data)
     {
        eina_lock_release(&sti->lock);
        return;
     }
   if (!sti->fetched)
     {
        if (sti->item_info != NULL)
          {
             eina_lock_release(&sti->lock);
             if (sti->store->cb.fetch.func)
               sti->store->cb.fetch.func(sti->store->cb.fetch.data, sti, sti->item_info);
             eina_lock_take(&sti->lock);
             sti->fetched = EINA_TRUE;
          }
     }
   eina_lock_release(&sti->lock);
}

static void
_store_fetch_end(void *data, Ecore_Thread *th)
{
   Elm_Store_Item *sti = data;
   eina_lock_take(&sti->lock);
   if (sti->data) elm_genlist_item_update((Elm_Object_Item *) sti->item);
   eina_lock_release(&sti->lock);
   if (th == sti->fetch_th) sti->fetch_th = NULL;
}

static void
_store_fetch_cancel(void *data, Ecore_Thread *th)
{
   Elm_Store_Item *sti = data;
   eina_lock_take(&sti->lock);
   if (th == sti->fetch_th) sti->fetch_th = NULL;
   //   if(sti->data) elm_genlist_item_update(sti->item);
   eina_lock_release(&sti->lock);
}

static void
_item_eval(void *data)
{
   Elm_Store_Item *sti = data;
   if (!sti) return;
   Elm_Store *st = sti->store;

   if (sti->fetched == EINA_FALSE)
     {
        if (st->fetch_thread && !sti->fetch_th)
          {
             sti->fetch_th = ecore_thread_run(_store_fetch_do, _store_fetch_end, _store_fetch_cancel, sti);
          }
        else if (!st->fetch_thread)
          {
             _store_fetch_do(sti,NULL);
             _store_fetch_end(sti,NULL);
          }
     }
   else
     {
        eina_lock_take(&sti->lock);
        if (!sti->fetched)
          {
             eina_lock_release(&sti->lock);
             if (sti->fetch_th)
               {
                  ecore_thread_cancel(sti->fetch_th);
                  sti->fetch_th = NULL;
               }
             eina_lock_take(&sti->lock);
          }
        sti->fetched = EINA_FALSE;
        eina_lock_release(&sti->lock);
        if (st->cb.unfetch.func)
          st->cb.unfetch.func(st->cb.unfetch.data, sti, sti->item_info);
        eina_lock_take(&sti->lock);
        sti->data = NULL;
        eina_lock_release(&sti->lock);
     }
}

static void
_item_realize(void *data)
{
   Elm_Store_Item *sti = data;
   Elm_Store *st = sti->store;
   sti->eval_job = NULL;
   if (st->live)
     {
        Eina_List *find = NULL;
        find = eina_list_data_find_list(st->always_fetched, sti);
        if (find) return;

        find = eina_list_data_find_list(st->realized,sti);
        if (find)
          {
             Elm_Store_Item *realized_sti = NULL;
             realized_sti = eina_list_data_get(find);
             st->realized = eina_list_remove_list(st->realized, find);
             _item_eval(realized_sti);
          }
        if (st->realized)
          {
             if ((int)eina_list_count(st->realized) == st->cache_max)
               {
                  Elm_Store_Item *realized_sti = NULL;
                  Eina_List *last = eina_list_last(st->realized);
                  realized_sti = eina_list_data_get(last);
                  st->realized = eina_list_remove_list(st->realized, last);
                  _item_eval(realized_sti);
               }
          }
        st->realized = eina_list_prepend(st->realized, sti);
        _item_eval(sti);
     }
}

static void
_item_job_add(Elm_Store_Item *sti)
{
   if (sti->eval_job) ecore_job_del(sti->eval_job);
   sti->eval_job = ecore_job_add(_item_realize, sti);
}

static void
_item_fetch(Elm_Store_Item *sti)
{
   Elm_Store *st = sti->store;

   if (st->live)
     {
        eina_lock_take(&sti->lock);
        if (!sti->fetched)
          {
             eina_lock_release(&sti->lock);
             if (sti->fetch_th)
               {
                  ecore_thread_cancel(sti->fetch_th);
                  sti->fetch_th = NULL;
               }
             eina_lock_take(&sti->lock);
          }
        if (sti->item_info != NULL)
          {
             eina_lock_release(&sti->lock);
             if (sti->store->cb.fetch.func)
               sti->store->cb.fetch.func(sti->store->cb.fetch.data, sti, sti->item_info);
             eina_lock_take(&sti->lock);
             sti->fetched = EINA_TRUE;
          }
        eina_lock_release(&sti->lock);
     }
}

static void
_item_unfetch(Elm_Store_Item *sti)
{
   EINA_SAFETY_ON_NULL_RETURN(sti);
   Elm_Store *st = sti->store;

   if (st->live)
     {
        eina_lock_take(&sti->lock);
        if (!sti->fetched)
          {
             eina_lock_release(&sti->lock);
             if (sti->fetch_th)
               {
                  ecore_thread_cancel(sti->fetch_th);
                  sti->fetch_th = NULL;
               }
             eina_lock_take(&sti->lock);
          }
        sti->fetched = EINA_FALSE;
        eina_lock_release(&sti->lock);
        if (st->cb.unfetch.func)
          st->cb.unfetch.func(st->cb.unfetch.data, sti, sti->item_info);
        eina_lock_take(&sti->lock);
        sti->data = NULL;
        eina_lock_release(&sti->lock);
     }
}

static void
_item_realized(void *data , Evas_Object *obj __UNUSED__, void *event_info)
{
   Elm_Store *st = data;
   if (!st) return;
   Elm_Object_Item *gli = event_info;
   Elm_Store_Item *sti = elm_genlist_item_data_get(gli);
   if (!sti) return;

   if (st->live)
     {
        if (!sti->data) _item_job_add(sti);
     }
   // TODO:
}

static void
_item_unrealized(void *data , Evas_Object *obj __UNUSED__, void *event_info)
{
   Elm_Store *st = data;
   if (!st) return;
   Elm_Object_Item *gli = event_info;
   Elm_Store_Item *sti = elm_genlist_item_data_get(gli);
   if (!sti) return;

   if (st->live)
     {
        if (sti->eval_job)
          {
             ecore_job_del(sti->eval_job);
             sti->eval_job = NULL;
          }
     }
}

static void
_item_free(Elm_Store_Item *sti)
{
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return;
   Elm_Store *st = sti->store;
   eina_lock_take(&sti->lock);
   if (st->live && st->cb.item_free.func && sti->item_info)
     {
        eina_lock_release(&sti->lock);
        st->cb.item_free.func(st->cb.item_free.data, sti->item_info);
        eina_lock_take(&sti->lock);
        sti->item_info = NULL;
     }
   eina_lock_release(&sti->lock);
   eina_lock_take(&sti->lock);
   free(sti);
}

static void
_item_del(void *data, Evas_Object *obj __UNUSED__)
{
   Elm_Store_Item *sti = data;
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return;
   Elm_Store *st = sti->store;

   if (sti->eval_job)
     {
        ecore_job_del(sti->eval_job);
        sti->eval_job = NULL;
     }

   Eina_List *find = NULL;
   find = eina_list_data_find_list(st->always_fetched, sti);
   if (find) return;

   find = eina_list_data_find_list(st->realized,sti);
   if (find)
     {
        Elm_Store_Item *realized_sti = NULL;
        realized_sti = eina_list_data_get(find);
        st->realized = eina_list_remove_list(st->realized, find);
     }
   if (sti->data) _item_unfetch(sti);
   if (st->item.free) st->item.free(sti);
}

static void
_list_do(void *data, Ecore_Thread *th)
{
   Elm_Store *st = data;
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;

   Elm_Store_Item_Info *item_info;
   Eina_Bool ok = EINA_FALSE;
   int loop;
   for (loop = 0; loop < st->item_count; loop++)
     {
        item_info = calloc(1, sizeof(Elm_Store_Item_Info));
        if (!item_info) return;
        item_info->index = loop;

        if (st->cb.list.func) ok = st->cb.list.func(st->cb.list.data, item_info);
        if (ok) ecore_thread_feedback(th, item_info);
        else free(item_info);
        if (ecore_thread_check(th)) break;
     }
}

static void
_list_update(void *data, Ecore_Thread *th __UNUSED__, void *msg)
{
   Elm_Store *st = data;
   Elm_Store_Item_Info *info = msg;
   elm_store_item_add(st, info);
}

static void
_list_end(void *data, Ecore_Thread *th)
{
   Elm_Store *st = data;
   if (th == st->list_th) st->list_th = NULL;
}

static void
_list_cancel(void *data, Ecore_Thread *th)
{
   Elm_Store *st = data;
   if (th == st->list_th) st->list_th = NULL;
}

static void
_group_item_append(Elm_Store_Item *sti, Elm_Genlist_Item_Class *itc)
{
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return;
   Elm_Store *st = sti->store;
   if (st->live)
     {
        if (st->always_fetched)
          {
             Eina_Bool group_existed = EINA_FALSE;
             const Eina_List *l = st->always_fetched;
             Elm_Store_Item *group_sti = eina_list_data_get(l);
             while (!group_existed && group_sti)
               {
                  if (group_sti->item_info->group_index == sti->item_info->group_index)
                    {
                       group_existed = EINA_TRUE;
                       break;
                    }
                  else
                    {
                       l = eina_list_next(l);
                       group_sti = eina_list_data_get(l);
                    }
               }
             if (group_existed) return; //Already existed the group item
          }
        st->always_fetched = eina_list_append(st->always_fetched, sti);
        sti->realized = EINA_FALSE;
        if (sti->data) _item_unfetch(sti);
        _item_fetch(sti);

        if (sti->item_info->group_index == -1)
          {
             sti->item = elm_genlist_item_append(st->genlist,
                                                 itc,
                                                 sti,
                                                 NULL,
                                                 ELM_GENLIST_ITEM_NONE,
                                                 (Evas_Smart_Cb)sti->store->cb.item_select.func,
                                                 (void *)sti->store->cb.item_select.data);
             return;
          }
     }
}

static void
_normal_item_append(Elm_Store_Item *sti, Elm_Genlist_Item_Class *itc)
{
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return;
   Elm_Store *st = sti->store;
   if (st->live && sti->item_info)
     {
        Eina_Bool need_update = EINA_FALSE;
        Eina_Bool group_existed = EINA_FALSE;
        const Eina_List *l;

        if (st->always_fetched)
          {
             if (sti->item_info->rec_item == EINA_TRUE)
               {
                  if (sti->item_info->group_index != sti->item_info->pre_group_index)
                    {
                       if (sti->item_info->group_index < sti->item_info->pre_group_index)
                         {
                            Eina_Bool pre_group_existed = EINA_FALSE;
                            l = st->always_fetched;
                            Elm_Store_Item *pre_group_sti = eina_list_data_get(l);
                            while (!pre_group_existed && pre_group_sti)
                              {
                                 if (pre_group_sti->item_info->pre_group_index == sti->item_info->pre_group_index)
                                   {
                                      pre_group_existed = EINA_TRUE;
                                      break;
                                   }
                                 else
                                   {
                                      l = eina_list_next(l);
                                      pre_group_sti = eina_list_data_get(l);
                                   }
                              }
                            if (pre_group_sti && pre_group_sti->realized) // already added the header item to the genlist
                              {
                                 Eina_Bool deleted = EINA_FALSE;
                                 Eina_Bool last_item = EINA_FALSE;
                                 Elm_Object_Item *comp_item = pre_group_sti->first_item;
                                 while (!deleted && comp_item)
                                   {
                                      Elm_Store_Item *comp_sti = elm_genlist_item_data_get(comp_item);
                                      if (comp_sti)
                                        {
                                           int sort = st->cb.item_sort.func(st->cb.item_sort.data, sti->item_info, comp_sti->item_info);
                                           if(sort == ELM_STORE_ITEM_SORT_SAME)
                                             {
                                                elm_store_item_del(comp_sti);
                                                deleted = EINA_TRUE;
                                                break;
                                             }
                                        }
                                      if (last_item) break;
                                      else comp_item = elm_genlist_item_next_get(comp_item);

                                      if (comp_item == pre_group_sti->last_item) last_item = EINA_TRUE;
                                   }
                                 if (!deleted) printf(" The item does not existed in the pre group of genlist \n");
                              }
                            else //Pre group item does not existed in the always fetched list or the genlist
                              {
                                 Eina_Bool deleted = EINA_FALSE;
                                 Elm_Object_Item *comp_item = elm_genlist_first_item_get(st->genlist);
                                 while (!deleted && comp_item)
                                   {
                                      Elm_Store_Item *comp_sti = elm_genlist_item_data_get(comp_item);
                                      if (comp_sti && sti->item_info->item_type == comp_sti->item_info->item_type)
                                        {
                                           int sort = st->cb.item_sort.func(st->cb.item_sort.data, sti->item_info, comp_sti->item_info);
                                           if (sort == ELM_STORE_ITEM_SORT_SAME)
                                             {
                                                elm_store_item_del(comp_sti);
                                                deleted = EINA_TRUE;
                                                break;
                                             }
                                        }
                                      comp_item = elm_genlist_item_next_get(comp_item);
                                   }
                                 if (!deleted) printf(" The item does not existed in the genlist \n");
                              }
                         }
                       else
                         {
                            sti->item_info->group_index = sti->item_info->pre_group_index;
                            need_update = EINA_TRUE;
                         }
                    }
                  else 	need_update = EINA_TRUE;
               }
             l = st->always_fetched;
             Elm_Store_Item *group_sti = eina_list_data_get(l);
             while (!group_existed && group_sti) // Search the group item of a normal item in the always_fetched list
               {
                  if (group_sti->item_info->group_index == sti->item_info->group_index)
                    {
                       group_existed = EINA_TRUE;
                       break;
                    }
                  else
                    {
                       l = eina_list_next(l);
                       group_sti = eina_list_data_get(l);
                    }
               }
             if (group_sti)
               {
                  if (group_sti->realized) // already added the header item to the genlist
                    {
                       Eina_Bool added = EINA_FALSE;
                       Elm_Object_Item *comp_item = group_sti->first_item;
                       while (!added && comp_item)
                         {
                            Elm_Store_Item *comp_sti = elm_genlist_item_data_get(comp_item);
                            if (comp_sti)
                              {
                                 int sort = st->cb.item_sort.func(st->cb.item_sort.data, sti->item_info, comp_sti->item_info);
                                 if (sort == ELM_STORE_ITEM_SORT_SAME)
                                   {
                                      if (need_update == EINA_TRUE) elm_store_item_update(comp_sti);
                                      else added = EINA_TRUE;
                                      break;
                                   }
                                 else if (sort == ELM_STORE_ITEM_SORT_LOW)
                                   {
                                      sti->item = elm_genlist_item_insert_before(st->genlist,
                                                                                 itc,
                                                                                 sti,
                                                                                 group_sti->item,
                                                                                 comp_item,
                                                                                 ELM_GENLIST_ITEM_NONE,
                                                                                 (Evas_Smart_Cb)sti->store->cb.item_select.func,
                                                                                 (void *)sti->store->cb.item_select.data);

                                      if (comp_item == group_sti->first_item) group_sti->first_item = sti->item;
                                      added = EINA_TRUE;
                                      break;
                                   }

                                 if (comp_item == group_sti->last_item) //To add the item in to the last of its group
                                   {
                                      need_update = EINA_FALSE;
                                      break;
                                   }
                              }
                            comp_item = elm_genlist_item_next_get(comp_item);
                         }
                       if (!added && !need_update)
                         {
                            Elm_Object_Item *last_item = group_sti->last_item;
                            if (last_item)
                              {
                                 sti->item = elm_genlist_item_insert_after(st->genlist,
                                                                           itc,
                                                                           sti,
                                                                           group_sti->item,
                                                                           last_item,
                                                                           ELM_GENLIST_ITEM_NONE,
                                                                           (Evas_Smart_Cb)sti->store->cb.item_select.func,
                                                                           (void *)sti->store->cb.item_select.data);
                                 group_sti->last_item = sti->item;
                              }
                            else printf(" Group item have no last item. so can not add a item to the genlist \n");
                         }
                    }
                  else // To add the header item in genlist, and compare with other header items along with callback func
                    {
                       Eina_Bool added = EINA_FALSE;
                       l = st->always_fetched;
                       Elm_Store_Item *comp_group_sti = eina_list_data_get(l);
                       while (!added && comp_group_sti)
                         {
                            if (comp_group_sti != group_sti && comp_group_sti->realized)
                              {
                                 // Compare with group items
                                 int sort = st->cb.item_sort.func(st->cb.item_sort.data, group_sti->item_info, comp_group_sti->item_info);
                                 if(sort == ELM_STORE_ITEM_SORT_LOW)
                                   {
                                      group_sti->item = elm_genlist_item_insert_before(st->genlist,
                                                                                       group_sti->item_info->item_class,
                                                                                       group_sti,
                                                                                       NULL,
                                                                                       comp_group_sti->item,
                                                                                       ELM_GENLIST_ITEM_GROUP,
                                                                                       NULL, NULL);

                                      group_sti->realized = EINA_TRUE;
                                      sti->item = elm_genlist_item_insert_after(st->genlist,
                                                                                itc,
                                                                                sti,
                                                                                group_sti->item,
                                                                                group_sti->item,
                                                                                ELM_GENLIST_ITEM_NONE,
                                                                                (Evas_Smart_Cb)sti->store->cb.item_select.func,
                                                                                (void *)sti->store->cb.item_select.data);

                                      group_sti->first_item = sti->item;
                                      group_sti->last_item = sti->item;
                                      added = EINA_TRUE;
                                      break;
                                   }
                              }
                            l = eina_list_next(l);
                            comp_group_sti = eina_list_data_get(l);
                         }
                       if (!comp_group_sti) // First item append in the genlist
                         {
                            group_sti->item = elm_genlist_item_append(st->genlist,
                                                                      group_sti->item_info->item_class,
                                                                      group_sti,
                                                                      NULL,
                                                                      ELM_GENLIST_ITEM_GROUP,
                                                                      NULL, NULL);

                            group_sti->realized = EINA_TRUE;
                            sti->item = elm_genlist_item_append(st->genlist,
                                                                itc,
                                                                sti,
                                                                group_sti->item,
                                                                ELM_GENLIST_ITEM_NONE,
                                                                (Evas_Smart_Cb)sti->store->cb.item_select.func,
                                                                (void *)sti->store->cb.item_select.data);

                            group_sti->first_item = sti->item;
                            group_sti->last_item = sti->item;
                         }
                    }
               }
          }
        if (!group_existed) // No exist the group item of normal item, so it added without group.
          {
             Eina_Bool added = EINA_FALSE;
             Elm_Object_Item *comp_item = elm_genlist_first_item_get(st->genlist);
             while (!added && comp_item)
               {
                  Elm_Store_Item *comp_sti = elm_genlist_item_data_get(comp_item);
                  if (comp_sti)
                    {
                       if (sti->item_info->item_type == comp_sti->item_info->item_type)
                         {
                            int sort = st->cb.item_sort.func(st->cb.item_sort.data, sti->item_info, comp_sti->item_info);
                            if (sort == ELM_STORE_ITEM_SORT_SAME)
                              {
                                 if (sti->item_info->rec_item == EINA_TRUE)
                                   {
                                      elm_store_item_update(comp_sti);
                                      need_update = EINA_TRUE;
                                   }
                                 else added = EINA_TRUE;
                                 break;
                              }
                            else if (sort == ELM_STORE_ITEM_SORT_LOW)
                              {
                                 sti->item = elm_genlist_item_insert_before(st->genlist,
                                                                            itc,
                                                                            sti,
                                                                            NULL,
                                                                            comp_item,
                                                                            ELM_GENLIST_ITEM_NONE,
                                                                            (Evas_Smart_Cb)sti->store->cb.item_select.func,
                                                                            (void *)sti->store->cb.item_select.data);

                                 added = EINA_TRUE;
                                 break;
                              }
                         }
                    }
                  comp_item = elm_genlist_item_next_get(comp_item);
                  if (comp_item == NULL) need_update = EINA_FALSE;
               }
             if (!added && !need_update)
               {
                  sti->item = elm_genlist_item_append(st->genlist,
                                                      itc,
                                                      sti,
                                                      NULL,
                                                      ELM_GENLIST_ITEM_NONE,
                                                      (Evas_Smart_Cb)sti->store->cb.item_select.func,
                                                      (void *)sti->store->cb.item_select.data);
               }
          }
     }
}

static void
_store_free(Elm_Store *st)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   Elm_Store_DBsystem *std = (Elm_Store_DBsystem *)st;
   if(std->p_db) eina_stringshare_del(std->p_db);
}

/**
 * Add a new dbsystem Store object
 *
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Store
 */
EAPI Elm_Store *
elm_store_dbsystem_new(void)
{
   Elm_Store_DBsystem *std = _store_new(Elm_Store_DBsystem);
   EINA_SAFETY_ON_NULL_RETURN_VAL(std, NULL);

   EINA_MAGIC_SET(std, ELM_STORE_DBSYSTEM_MAGIC);
   std->base.free = _store_free;
   std->base.item.free = _item_free;
   return &std->base;
}

/**
 * Sets the item count of a store
 *
 * @param st The store object
 * @param count The item count of an store
 *
 * @ingroup Store
 */
EAPI void
elm_store_item_count_set(Elm_Store *st, int count)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   st->item_count = count;
}


/**
 * Set the select func that select the state of a list item whether true or false
 *
 * @param st The store object
 * @param func The select cb function of an store
 * @param data The new data pointer to set
 *
 * @ingroup Store
 */
EAPI void
elm_store_item_select_func_set(Elm_Store *st, Elm_Store_Item_Select_Cb func, const void *data)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   st->cb.item_select.func = func;
   st->cb.item_select.data = (void *)data;
}

/**
 * Sets the sort func that sort the item with a next in the list
 *
 * @param st The store object
 * @param func The sort cb function of an store
 * @param data The new data pointer to set
 *
 * @ingroup Store
 */
EAPI void
elm_store_item_sort_func_set(Elm_Store *st, Elm_Store_Item_Sort_Cb func, const void *data)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   st->cb.item_sort.func = func;
   st->cb.item_sort.data = (void *)data;
}

/**
 * Set the store item free func
 *
 * @param st The store object
 * @param func The free cb function of an store
 * @param data The new data pointer to set
 *
 * @ingroup Store
 */
EAPI void
elm_store_item_free_func_set(Elm_Store *st, Elm_Store_Item_Free_Cb func, const void *data)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   st->cb.item_free.func = func;
   st->cb.item_free.data = (void *)data;
}

/**
 * Get the item index that included header items
 *
 * @param sti The store item object
 * @return The item index in genlist
 *
 * @ingroup Store
 */
EAPI int
elm_store_item_index_get(const Elm_Store_Item *sti)
{
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return -1;
   Elm_Store *st = sti->store;

   if (st->live)
     {
        int index = 0;
        if (st->genlist)
          {
             Elm_Object_Item *gen_item = elm_genlist_first_item_get(st->genlist);
             while (gen_item)
               {
                  Elm_Store_Item *item = elm_genlist_item_data_get(gen_item);
                  if (item == sti) return index;
                  gen_item = elm_genlist_item_next_get(gen_item);
                  index++;
               }
          }
     }
   return -1;
}

/**
 * Get the item index of real data that don't included header items
 *
 * @param sti The store item object
 * @return The real item index
 *
 * @ingroup Store
 */
EAPI int
elm_store_item_data_index_get(const Elm_Store_Item *sti)
{
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return -1;
   Elm_Store *st = sti->store;

   if (st->live)
     {
        int index = 0;
        Elm_Object_Item *gen_item = elm_genlist_first_item_get(st->genlist);
        while (gen_item)
          {
             Elm_Store_Item *item = elm_genlist_item_data_get(gen_item);
             if (item && item->item_info->item_type != ELM_GENLIST_ITEM_GROUP)
               {
                  if(item == sti) return index;
                  index++;
               }
             gen_item = elm_genlist_item_next_get(gen_item);
          }
     }
   return -1;
}

/**
 * Get the DB pointer of an item
 *
 * @param sti The store item object
 * @return The DB pointer of item
 *
 * @ingroup Store
 */
EAPI void *
elm_store_dbsystem_db_get(const Elm_Store_Item *sti)
{
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return NULL;

   const Elm_Store_DBsystem *std = (const Elm_Store_DBsystem *)sti->store;
   if (!EINA_MAGIC_CHECK(sti->store, ELM_STORE_MAGIC)) return NULL;
   if (!EINA_MAGIC_CHECK(std, ELM_STORE_DBSYSTEM_MAGIC)) return NULL;
   return std->p_db;
}

/**
 * Set the DB pointer of an item
 *
 * @param sti The store item object
 * @parm p_db The DB pointer of item
 *
 * @ingroup Store
 */
EAPI void
elm_store_dbsystem_db_set(Elm_Store *store, void *p_db)
{
   Elm_Store_DBsystem *std = (Elm_Store_DBsystem *)store;
   if (!EINA_MAGIC_CHECK(store, ELM_STORE_MAGIC)) return;
   if (!EINA_MAGIC_CHECK(std, ELM_STORE_DBSYSTEM_MAGIC)) return;
   if (store->list_th)
     {
        ecore_thread_cancel(store->list_th);
        store->list_th = NULL;
     }
   std->p_db = p_db;
   store->list_th = ecore_thread_feedback_run(_list_do,
                                              _list_update,
                                              _list_end,
                                              _list_cancel,
                                              store, EINA_TRUE);
}

/**
 * Append the item to the genlist
 *
 * @param st The store object
 * @param info The store item info dbsystem object
 * @return The item of store
 *
 * @ingroup Store
 */
EAPI Elm_Store_Item *
elm_store_item_add(Elm_Store *st, Elm_Store_Item_Info *info)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return NULL;
   EINA_SAFETY_ON_NULL_RETURN_VAL(info, NULL);
   Elm_Store_Item *sti;
   Elm_Genlist_Item_Class *itc;

   sti = calloc(1, sizeof(Elm_Store_Item));
   if (!sti) return NULL;
   eina_lock_new(&sti->lock);
   EINA_MAGIC_SET(sti, ELM_STORE_ITEM_MAGIC);
   sti->store = st;
   sti->item_info = info;
   sti->fetched = EINA_FALSE;

   itc = info->item_class;
   if (!itc) itc = &_store_item_class;
   else
     {
        itc->func.text_get = (GenlistItemTextGetFunc)_item_text_get;
        itc->func.content_get = (GenlistItemContentGetFunc)_item_content_get;
        itc->func.state_get = NULL;
        itc->func.del = (GenlistItemDelFunc)_item_del;
     }

   if (st->live)
     {
        if (sti->item_info->item_type == ELM_GENLIST_ITEM_GROUP) _group_item_append(sti, itc);
        else _normal_item_append(sti, itc);
        return sti;
     }
   else
     {
        free(sti);
        return NULL;
     }
}

/**
 * Realize the visible items to the screen
 *
 * @param st The store object
 *
 * @ingroup Store
 */
EAPI void
elm_store_visible_items_update(Elm_Store *st)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;

   Eina_List *realized_list = elm_genlist_realized_items_get(st->genlist);
   Elm_Object_Item *item = eina_list_data_get(realized_list);
   while (item)
     {
        Elm_Store_Item *realized_sti = elm_genlist_item_data_get(item);
        elm_store_item_update(realized_sti);
        realized_list = eina_list_next(realized_list);
        item = eina_list_data_get(realized_list);
     }
}

/**
 * Realize the item to the screen
 *
 * @param sti The store item object
 *
 * @ingroup Store
 */
EAPI void
elm_store_item_update(Elm_Store_Item *sti)
{
   Elm_Store *st = sti->store;

   Eina_List *find = NULL;
   find = eina_list_data_find_list(st->always_fetched, sti);
   if (find)
     {
        if (sti->data) _item_unfetch(sti);
        _item_fetch(sti);
        if (sti->realized) elm_genlist_item_update(sti->item);
     }
   else
     {
        find = eina_list_data_find_list(st->realized,sti);
        if (find) 	_item_realize(sti);
     }
}

/**
 * Delete the item of genlist
 *
 * @param sti The store item object
 *
 * @ingroup Store
 */
EAPI void
elm_store_item_del(Elm_Store_Item *sti)
{
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return;
   Elm_Store *st = sti->store;

   if (st->live)
     {
        Eina_List *find = NULL;
        find = eina_list_data_find_list(st->always_fetched, sti);
        if (find) st->always_fetched = eina_list_remove_list(st->always_fetched, find);

        Eina_Bool deleted = EINA_FALSE;
        Elm_Object_Item *comp_item = elm_genlist_first_item_get(st->genlist);
        while (!deleted && comp_item)
          {
             Elm_Store_Item *comp_sti = elm_genlist_item_data_get(comp_item);
             if (comp_sti && (sti == comp_sti))
               {
                  Elm_Object_Item *group_item = elm_genlist_item_parent_get(comp_item);
                  if (group_item)
                    {
                       Elm_Store_Item *group_sti = elm_genlist_item_data_get(group_item);
                       if (group_sti)
                         {
                            if ((group_sti->first_item == comp_item) && (group_sti->last_item == comp_item))
                              {
                                 group_sti->realized = EINA_FALSE;
                                 group_sti->first_item = NULL;
                                 group_sti->last_item = NULL;
                                 elm_genlist_item_del(group_item);
                              }
                            else if ((group_sti->first_item == comp_item) && (group_sti->last_item != comp_item))
                              {
                                 Elm_Object_Item *next_item = elm_genlist_item_next_get(comp_item);
                                 group_sti->first_item = next_item;
                              }
                            else if ((group_sti->first_item != comp_item) && (group_sti->last_item == comp_item))
                              {
                                 Elm_Object_Item *prev_item = elm_genlist_item_prev_get(comp_item);
                                 group_sti->last_item = prev_item;
                              }
                         }
                    }
                  elm_genlist_item_del(comp_sti->item);
                  deleted = EINA_TRUE;
                  break;
               }
             comp_item = elm_genlist_item_next_get(comp_item);
          }

        if(!deleted) printf(" Not deleted because it does not existed in the genlist \n");
     }
}

// TODO: END -DBsystem store

