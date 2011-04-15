#include <Elementary.h>
#include <Elementary_Cursor.h>
#include "elm_priv.h"

#ifndef EFL_HAVE_THREADS
# error "No thread support. Required."
#endif

#ifdef EFL_HAVE_POSIX_THREADS
# include <pthread.h>
# define LK(x)  pthread_mutex_t x
# define LKI(x) pthread_mutex_init(&(x), NULL);
# define LKD(x) pthread_mutex_destroy(&(x));
# define LKL(x) pthread_mutex_lock(&(x));
# define LKU(x) pthread_mutex_unlock(&(x));
#else /* EFL_HAVE_WIN32_THREADS */
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# undef WIN32_LEAN_AND_MEAN
# define LK(x)  HANDLE x
# define LKI(x) x = CreateMutex(NULL, FALSE, NULL)
# define LKD(x) CloseHandle(x)
# define LKL(x) WaitForSingleObject(x, INFINITE)
# define LKU(x) ReleaseMutex(x)
#endif

#define ELM_STORE_MAGIC            0x3f89ea56
#define ELM_STORE_FILESYSTEM_MAGIC 0x3f89ea57
#define ELM_STORE_DBSYSTEM_MAGIC   0x3f89ea58
#define ELM_STORE_ITEM_MAGIC       0x5afe8c1d
#define CACHE_COUNT                128
#define SCREEN_ITEM_COUNT    10

struct _Elm_Store
{
   EINA_MAGIC;
   void         (*free)(Elm_Store *store);
   struct {
        void        (*free)(Elm_Store_Item *item);
   } item;
   Evas_Object   *genlist;
   Ecore_Thread  *list_th;
   Eina_Inlist   *items;
   Eina_List     *realized;
   int            realized_count;
   int            cache_max;
   int            current_top_index;
   int            start_fetch_index;
   int            end_fetch_index;
   int            item_count;
   int            total_count;
   int            block_count;
   int            type;
   Eina_List     *header_items;
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
             Elm_Store_Item_Select_Cb func;
             void                    *data;
        } item_select;
        struct {
             Elm_Store_Item_Sort_Cb func;
             void                  *data;
        } item_sort;
   } cb;
   Eina_Bool      sorted : 1;
   Eina_Bool      fetch_thread : 1;
   Eina_Bool      multi_load : 1;
   Eina_Bool      live : 1;
};

struct _Elm_Store_Item
{
   EINA_INLIST;
   EINA_MAGIC;
   Elm_Store                    *store;
   Elm_Genlist_Item             *item;
   Ecore_Thread                 *fetch_th;
   Ecore_Job                    *eval_job;
   const Elm_Store_Item_Mapping *mapping;
   void                         *data;
   Elm_Store_Item_Info          *item_info;
   LK(lock);
   Eina_Bool                     live : 1;
   Eina_Bool                     was_live : 1;
   Eina_Bool                     realized : 1;
   Eina_Bool                     fetched : 1;
};

struct _Elm_Store_Filesystem
{
   Elm_Store   base;
   EINA_MAGIC;
   const char *dir;
};

struct _Elm_Store_Item_Filesystem
{
   Elm_Store_Item base;
   const char    *path;
};

struct _Elm_Store_DBsystem
{
   Elm_Store   base;
   EINA_MAGIC;
   void       *p_db;
};

static Elm_Genlist_Item_Class _store_item_class;

static char *_item_label_get(void *data, Evas_Object *obj __UNUSED__, const char *part);
static Evas_Object *_item_icon_get(void *data, Evas_Object *obj, const char *part);
static void _item_del(void *data __UNUSED__, Evas_Object *obj __UNUSED__);
static void _store_free(Elm_Store *st);
static void _item_free(Elm_Store_Item *sti);
static void _item_realized(void *data, Evas_Object *obj __UNUSED__, void *event_info);
static void _item_unrealized(void *data, Evas_Object *obj __UNUSED__, void *event_info);
static void _genlist_del(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__);
static Elm_Store_Item *_item_unfetch(Elm_Store *st, int index);

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
        LKL(sti->lock);
        if (!sti->fetched)
          {
             LKU(sti->lock);
             if (sti->fetch_th)
               {
                  ecore_thread_cancel(sti->fetch_th);
                  sti->fetch_th = NULL;
               }
             LKL(sti->lock);
          }
        sti->fetched = EINA_FALSE;
        LKU(sti->lock);
        if (st->cb.unfetch.func)
          st->cb.unfetch.func(st->cb.unfetch.data, sti, NULL);
        LKL(sti->lock);
        sti->data = NULL;
        LKU(sti->lock);
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
        LKD(sti->lock);
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
   LKL(sti->lock);
   if (sti->data)
     {
        LKU(sti->lock);
        return;
     }
   if (!sti->fetched)
     {
        LKU(sti->lock);
        if (sti->store->cb.fetch.func)
          sti->store->cb.fetch.func(sti->store->cb.fetch.data, sti, NULL);
        LKL(sti->lock);
        sti->fetched = EINA_TRUE;
     }
   LKU(sti->lock);
}
//     ************************************************************************
////   * End of separate thread function.                                     *
////// ************************************************************************
/* TODO: refactor lock part into core? this does not depend on filesystm part */
static void
_store_filesystem_fetch_end(void *data, Ecore_Thread *th)
{
   Elm_Store_Item *sti = data;
   LKL(sti->lock);
   if (sti->data) elm_genlist_item_update(sti->item);
   LKU(sti->lock);
   if (th == sti->fetch_th) sti->fetch_th = NULL;
}

/* TODO: refactor lock part into core? this does not depend on filesystm part */
static void
_store_filesystem_fetch_cancel(void *data, Ecore_Thread *th)
{
   Elm_Store_Item *sti = data;
   LKL(sti->lock);
   if (th == sti->fetch_th) sti->fetch_th = NULL;
   if (sti->data) elm_genlist_item_update(sti->item);
   LKU(sti->lock);
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
   Elm_Genlist_Item *gli = event_info;
   Elm_Store_Item *sti = elm_genlist_item_data_get(gli);
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
   Elm_Genlist_Item *gli = event_info;
   Elm_Store_Item *sti = elm_genlist_item_data_get(gli);
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
_store_item_label_get(void *data, Evas_Object *obj __UNUSED__, const char *part)
{
   Elm_Store_Item *sti = data;
   const char *s = "";
   LKL(sti->lock);
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
   LKU(sti->lock);
   return strdup(s);
}

static Evas_Object *
_store_item_icon_get(void *data, Evas_Object *obj, const char *part)
{
   Elm_Store_Item *sti = data;
   LKL(sti->lock);
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
             LKU(sti->lock);
             return ic;
          }
     }
   LKU(sti->lock);
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
   LKI(sti->base.lock);
   EINA_MAGIC_SET(&(sti->base), ELM_STORE_ITEM_MAGIC);
   sti->base.store = st;
   sti->base.data = info->base.data;
   sti->base.mapping = info->base.mapping;
   sti->path = eina_stringshare_add(info->path);

   itc = info->base.item_class;
   if (!itc) itc = &_store_item_class;
   else
     {
        itc->func.label_get = (GenlistItemLabelGetFunc)_store_item_label_get;
        itc->func.icon_get  = (GenlistItemIconGetFunc)_store_item_icon_get;
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
   _store_item_class.func.label_get = (GenlistItemLabelGetFunc)_store_item_label_get;
   _store_item_class.func.icon_get  = (GenlistItemIconGetFunc)_store_item_icon_get;
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
             LKD(sti->lock);
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
        Eina_List *l;
        Eina_List *header_list;

        EINA_LIST_FOREACH(st->header_items, l, header_list)
          {
             if (header_list)
               {
                  Eina_List *in_l;
                  Elm_Store_Item *sti;
                  EINA_LIST_FOREACH(header_list, in_l, sti)
                    {
                       if (sti->fetch_th)
                         {
                            ecore_thread_cancel(sti->fetch_th);
                            sti->fetch_th = NULL;
                         }
                       if (sti->data)
                         {
                            int index = elm_store_item_index_get(sti);
                            _item_unfetch(st, index);
                         }
                       LKD(sti->lock);
                       free(sti);
                    }
               }
          }
        eina_list_free(st->header_items);

        if (st->genlist)
          {
             evas_object_event_callback_del_full(st->genlist, EVAS_CALLBACK_DEL, _genlist_del, st);
             evas_object_smart_callback_del(st->genlist, "realized", _item_realized);
             evas_object_smart_callback_del(st->genlist, "unrealized", _item_unrealized);
             elm_genlist_clear(st->genlist);
             st->genlist = NULL;
          }
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
        st->block_count = elm_genlist_block_count_get(st->genlist);
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
   LKL(sti->lock);
   sti->data = data;
   LKU(sti->lock);
}

EAPI void *
elm_store_item_data_get(Elm_Store_Item *sti)
{
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return NULL;
   void *d;
   LKL(sti->lock);
   d = sti->data;
   LKU(sti->lock);
   return d;
}

EAPI const Elm_Store *
elm_store_item_store_get(const Elm_Store_Item *sti)
{
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return NULL;
   // dont need lock
   return sti->store;
}

EAPI const Elm_Genlist_Item *
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

static Elm_Store *
_store_init(size_t size)
{
   Elm_Store *st = calloc(1, size);
   if (!st) return NULL;

   eina_magic_string_set(ELM_STORE_MAGIC, "Elm_Store");
   eina_magic_string_set(ELM_STORE_FILESYSTEM_MAGIC, "Elm_Store_Filesystem");
   eina_magic_string_set(ELM_STORE_ITEM_MAGIC, "Elm_Store_Item");
   eina_magic_string_set(ELM_STORE_DBSYSTEM_MAGIC, "Elm_Store_DBsystem");

   _store_item_class.item_style = "default";
   _store_item_class.func.label_get = (GenlistItemLabelGetFunc)_item_label_get;
   _store_item_class.func.icon_get = (GenlistItemIconGetFunc)_item_icon_get;
   _store_item_class.func.state_get = NULL;
   _store_item_class.func.del = (GenlistItemDelFunc)_item_del;

   EINA_MAGIC_SET(st, ELM_STORE_MAGIC);
   st->cache_max = CACHE_COUNT;
   st->current_top_index = 0;
   st->start_fetch_index = 0;
   st->end_fetch_index = st->cache_max - 1;
   st->live = EINA_TRUE;
   st->multi_load = EINA_FALSE;
   st->total_count = 0;
   st->fetch_thread = EINA_FALSE;
   st->type = 1;
   return st;
}

#define _store_new(type) (type *)_store_init(sizeof(type))

static void
_genlist_del(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Elm_Store *st = data;
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;

   elm_store_free(st);
}

static void
_store_fetch_do(void *data, Ecore_Thread *th __UNUSED__)
{
   Elm_Store_Item *sti = data;

   LKL(sti->lock);
   if (sti->data)
     {
        LKU(sti->lock);
        return;
     }
   if (!sti->fetched)
     {
        LKU(sti->lock);

        if (sti->store->cb.fetch.func)
          {
             sti->store->cb.fetch.func(sti->store->cb.fetch.data, sti, sti->item_info);
          }
        LKL(sti->lock);
        sti->fetched = EINA_TRUE;
     }
   LKU(sti->lock);
}

static void
_store_fetch_end(void *data, Ecore_Thread *th)
{
   Elm_Store_Item *sti = data;
   LKL(sti->lock);
   if (sti->data) elm_genlist_item_update(sti->item);
   LKU(sti->lock);
   if (th == sti->fetch_th) sti->fetch_th = NULL;
}

static void
_store_fetch_cancel(void *data, Ecore_Thread *th)
{
   Elm_Store_Item *sti = data;
   LKL(sti->lock);
   if (th == sti->fetch_th) sti->fetch_th = NULL;
   if (sti->data) elm_genlist_item_update(sti->item);
   LKU(sti->lock);
}

static Elm_Store_Item *
_item_fetch(Elm_Store *st, int index)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(st,NULL);
   Elm_Store_Item *sti;

   int in_index = 0;
   Eina_List *l;
   Eina_List *header_list;

   EINA_LIST_FOREACH(st->header_items, l, header_list)
     {
        if ((in_index + (signed)eina_list_count(header_list)) > index)
          {
             sti = eina_list_nth(header_list, index - in_index);
             LKL(sti->lock);
             if ((!sti->fetched) && st->cb.fetch.func && (!sti->fetch_th))
               {
                  if (st->fetch_thread)
                    {
                       LKU(sti->lock);
                       sti->fetch_th = ecore_thread_run(_store_fetch_do,
                                                        _store_fetch_end,
                                                        _store_fetch_cancel,
                                                        sti);
                       LKL(sti->lock);
                    }
                  else
                    {
                       LKU(sti->lock);
                       st->cb.fetch.func(st->cb.fetch.data, sti, sti->item_info);
                       LKL(sti->lock);
                       sti->fetched = EINA_TRUE;
                    }
               }
             LKU(sti->lock);
             return sti;
          }
        else
          {
             in_index = in_index + eina_list_count(header_list);
          }
     }
   return NULL;
}

static Elm_Store_Item *
_item_unfetch(Elm_Store *st, int index)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(st,NULL);
   Elm_Store_Item *sti;

   int in_index = 0;
   Eina_List *l;
   Eina_List *header_list;

   EINA_LIST_FOREACH(st->header_items, l, header_list)
     {
        if ((in_index + (signed)eina_list_count(header_list)) > index)
          {
             sti = eina_list_nth(header_list, index - in_index);
             LKL(sti->lock);
             if (sti->fetched && st->cb.unfetch.func)
               {
                  if (sti->fetch_th)
                    {
                       LKU(sti->lock);
                       ecore_thread_cancel(sti->fetch_th);
                       sti->fetch_th = NULL;
                       LKL(sti->lock);
                    }
                  LKU(sti->lock);
                  st->cb.unfetch.func(st->cb.unfetch.data, sti, sti->item_info);
                  LKL(sti->lock);
                  sti->data = NULL;
                  sti->fetched = EINA_FALSE;
               }
             LKU(sti->lock);
             return sti;
          }
        else
          {
             in_index = in_index + eina_list_count(header_list);
          }
     }
   return NULL;
}

static const Elm_Store_Item_Mapping *
_item_mapping_find(Elm_Store_Item *sti, const char *part)
{
   const Elm_Store_Item_Mapping *m;

   for (m = sti->item_info->mapping; m; m++)
     {
        if (m->type == ELM_STORE_ITEM_MAPPING_NONE) break;
        if (!strcmp(part, m->part)) return m;
     }
   return NULL;
}

static char *
_item_label_get(void *data, Evas_Object *obj __UNUSED__, const char *part)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(data, strdup(""));
   Elm_Store_Item *sti = data;

   if (sti->item)
     {
        if (!sti->data)
          {
             int index = elm_store_item_index_get(sti);
             if (sti->store->start_fetch_index > index)
               {
                  int diff = sti->store->start_fetch_index - index;
                  int loop;
                  for (loop = 1; loop <= diff; loop++)
                    {
                       _item_unfetch(sti->store, sti->store->end_fetch_index);
                       sti->store->end_fetch_index--;
                       _item_fetch(sti->store, (sti->store->start_fetch_index - loop));
                    }
                  sti->store->start_fetch_index = index;
               }
             else if (index > sti->store->end_fetch_index)
               {
                  int diff = index - sti->store->end_fetch_index;
                  int loop;
                  for (loop = 1; loop <= diff; loop++)
                    {
                       _item_unfetch(sti->store, sti->store->start_fetch_index);
                       sti->store->start_fetch_index++;
                       _item_fetch(sti->store, (sti->store->end_fetch_index + loop));
                    }
                  sti->store->end_fetch_index = index;
               }
             else
               {
                  _item_fetch(sti->store, index);
               }
          }

        LKL(sti->lock);
        if (sti->data)
          {
             const char *s = "";
             const Elm_Store_Item_Mapping *m = _item_mapping_find(sti, part);
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
                  if (s)
                    {
                       LKU(sti->lock);
                       return strdup(s);
                    }
                  else
                    {
                       LKU(sti->lock);
                       return NULL;
                    }
               }
          }
        LKU(sti->lock);
     }
   return NULL;
}

static Evas_Object *
_item_icon_get(void *data, Evas_Object *obj, const char *part)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(data,NULL);
   Elm_Store_Item *sti = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(sti,NULL);

   if (sti->item)
     {
        LKL(sti->lock);
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
                  LKU(sti->lock);
                  return ic;
               }
          }
        LKU(sti->lock);
     }
   return NULL;
}

static void
_item_realized(void *data, Evas_Object *obj __UNUSED__, void *event_info)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   EINA_SAFETY_ON_NULL_RETURN(event_info);
   Elm_Store *st = data;
   Elm_Genlist_Item *gli = event_info;
   Elm_Store_Item *sti = elm_genlist_item_data_get(gli);

   EINA_SAFETY_ON_NULL_RETURN(sti);

   int index = elm_store_item_index_get(sti);

   if (st->fetch_thread)
     {
        if ((st->start_fetch_index <= index) && (index <= st->end_fetch_index))
          {
             int middle_index = sti->store->start_fetch_index + (sti->store->cache_max) / 2;

             if ((middle_index < index) && (sti->store->end_fetch_index < sti->store->total_count))
               {
                  int diff = index - middle_index;
                  int loop;
                  for (loop = 0; loop < diff; loop++)
                    {
                       _item_unfetch(st, sti->store->start_fetch_index);
                       sti->store->start_fetch_index++;
                       _item_fetch(st, (sti->store->end_fetch_index + 1));
                       sti->store->end_fetch_index++;
                    }
               }
             else if ((middle_index > index) && (sti->store->start_fetch_index > 0))
               {
                  int diff = st->current_top_index - index;
                  int loop;
                  for (loop = 0; loop < diff; loop++)
                    {
                       _item_unfetch(st, sti->store->end_fetch_index);
                       sti->store->end_fetch_index--;
                       _item_fetch(st, (sti->store->start_fetch_index - 1));
                       sti->store->start_fetch_index--;
                    }
               }
             else {
                  if ((!sti->fetched))
                    {
                       _item_fetch(st, index);
                    }
             }
          }
     }

   if ((st->current_top_index > index))
     {
        st->current_top_index = index;
     }
   else if ((st->current_top_index + SCREEN_ITEM_COUNT) < index)
     {
        st->current_top_index = st->current_top_index + (index - (st->current_top_index + SCREEN_ITEM_COUNT));
     }

   // TODO: fix the item when it disposed quickly before call the label get. as example, get_more_btn in email
}

static void
_item_unrealized(void *data, Evas_Object *obj __UNUSED__, void *event_info)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   EINA_SAFETY_ON_NULL_RETURN(event_info);
   Elm_Genlist_Item *gli = event_info;
   Elm_Store_Item *sti = elm_genlist_item_data_get(gli);
   EINA_SAFETY_ON_NULL_RETURN(sti);
}

static void
_item_del(void *data, Evas_Object *obj __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Elm_Store_Item *sti = data;
   EINA_SAFETY_ON_NULL_RETURN(sti);
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return;
   elm_store_item_del(sti);
}

static void
_list_do(void *data, Ecore_Thread *th __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   Elm_Store *st = data;
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;

   if (st->multi_load == EINA_TRUE)
     {
        Elm_Store_Item_Info *item_info;
        Eina_Bool ok = EINA_FALSE;
        int loop;
        for (loop = 0; loop < st->item_count; loop++)
          {
             item_info = calloc(1, sizeof(Elm_Store_Item_Info));
             if (!item_info) return;
             item_info->index = loop;

             if (st->cb.list.func)
               {
                  ok = st->cb.list.func(st->cb.list.data, item_info);
               }
             if (ok) ecore_thread_feedback(th, item_info);
             else free(item_info);
             if (ecore_thread_check(th)) break;
          }
     }
}

static void
_list_update(void *data, Ecore_Thread *th __UNUSED__, void *msg)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   EINA_SAFETY_ON_NULL_RETURN(msg);
   Elm_Store *st = data;
   Elm_Store_Item_Info *info = msg;

   elm_store_item_add(st, info);
}

static void
_list_end(void *data, Ecore_Thread *th)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   EINA_SAFETY_ON_NULL_RETURN(th);
   Elm_Store *st = data;

   if (th == st->list_th)
     {
        ecore_thread_cancel(st->list_th);
        st->list_th = NULL;
     }
}

static void
_list_cancel(void *data, Ecore_Thread *th)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   EINA_SAFETY_ON_NULL_RETURN(th);
   Elm_Store *st = data;

   if (th == st->list_th)
     {
        ecore_thread_cancel(st->list_th);
        st->list_th = NULL;
     }
}

static Elm_Store_Item *
_item_get(const Elm_Store *st, const int index)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(st,NULL);
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return NULL;

   if (st->live)
     {
        int in_index = 0;

        Elm_Store_Item *sti;
        Eina_List *l;
        Eina_List *header_list;

        EINA_LIST_FOREACH(st->header_items, l, header_list)
          {
             if ((in_index + (signed)eina_list_count(header_list)) > index)
               {
                  sti = eina_list_nth(header_list, index - in_index);
                  return sti;
               }
             else
               {
                  in_index = in_index + eina_list_count(header_list);
               }
          }
     }
   return NULL;
}

static void
_item_select_cb(void *data, Evas_Object *obj, void *event_info)
{
   EINA_SAFETY_ON_NULL_RETURN(event_info);

   const Elm_Genlist_Item *it = (Elm_Genlist_Item *)event_info;
   Elm_Store_Item *sti = elm_genlist_item_data_get(it);
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return;

   if (sti->store->cb.item_select.func)
     {
        sti->store->cb.item_select.func(sti->store->cb.item_select.data, sti);
     }
}

static void
_group_item_append(Elm_Store_Item *sti, Elm_Genlist_Item_Class *itc)
{
   EINA_SAFETY_ON_NULL_RETURN(sti);
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return;

   if (sti->store->header_items)
     {
        Eina_Bool header_add = EINA_TRUE;
        Eina_List *l;
        Eina_List *header_list;

        EINA_LIST_FOREACH(sti->store->header_items, l, header_list)
          {
             Elm_Store_Item *item = eina_list_nth(header_list, 0);

             if (item->item_info->group_index == sti->item_info->group_index)
               {
                  header_add = EINA_FALSE;
                  break;
               }
          }
        if (header_add)
          {
             Eina_List *new_header_list = NULL;
             sti->item_info->index = 0;
             new_header_list = eina_list_append(new_header_list, sti);

             Eina_Bool last_header = EINA_TRUE;
             Eina_List *l;
             Eina_List *header_list;

             EINA_LIST_FOREACH(sti->store->header_items, l, header_list)
               {
                  Elm_Store_Item *temp_sti = eina_list_nth(header_list, 0);

                  if (temp_sti->item_info->group_index > sti->item_info->group_index)
                    {
                       sti->store->header_items = eina_list_prepend_relative(sti->store->header_items, new_header_list, header_list);
                       sti->item = elm_genlist_item_insert_before(sti->store->genlist,
                                                                  itc,
                                                                  sti,
                                                                  NULL,
                                                                  temp_sti->item,
                                                                  ELM_GENLIST_ITEM_GROUP,
                                                                  (Evas_Smart_Cb)sti->store->cb.item_select.func,
                                                                  (void *)sti->store->cb.item_select.data);
                       elm_store_item_update(sti->store, sti);
                       last_header = EINA_FALSE;
                       break;
                    }
               }
             if (last_header)
               {
                  sti->store->header_items = eina_list_append(sti->store->header_items, new_header_list);
                  sti->item = elm_genlist_item_append(sti->store->genlist,
                                                      itc,
                                                      sti,
                                                      NULL,
                                                      ELM_GENLIST_ITEM_GROUP,
                                                      (Evas_Smart_Cb)sti->store->cb.item_select.func,
                                                      (void *)sti->store->cb.item_select.data);
                  elm_store_item_update(sti->store, sti);
               }
          }
     }
   else
     {
        Eina_List *header_list = NULL;
        sti->item_info->index = 0;
        header_list = eina_list_append(header_list, sti);
        sti->store->header_items = eina_list_append(sti->store->header_items, header_list);
        sti->item = elm_genlist_item_append(sti->store->genlist,
                                            itc,
                                            sti,
                                            NULL,
                                            ELM_GENLIST_ITEM_GROUP,
                                            (Evas_Smart_Cb)sti->store->cb.item_select.func,
                                            (void *)sti->store->cb.item_select.data);
        elm_store_item_update(sti->store, sti);
     }
}

static void
_normal_item_append(Elm_Store_Item *sti, Elm_Genlist_Item_Class *itc)
{
   EINA_SAFETY_ON_NULL_RETURN(sti);
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return;
   Elm_Store *st = sti->store;

   if (sti->item_info->rec_item == EINA_TRUE)
     {
        if (sti->item_info->group_index == sti->item_info->pre_group_index)
          {
             elm_store_item_update(st, sti);
          }
        else
          {
             Eina_List *l;
             Eina_List *l_next;
             Eina_List *header_list;

             EINA_LIST_FOREACH_SAFE(st->header_items, l, l_next, header_list)
               {
                  Elm_Store_Item *header_item = eina_list_nth(header_list, 0);

                  if (header_item->item_info->group_index == sti->item_info->pre_group_index)
                    {
                       Elm_Store_Item *last_sti = eina_list_nth(header_list, eina_list_count(header_list) - 1);

                       if (last_sti->data)
                         {
                            int index = elm_store_item_index_get(last_sti);
                            _item_unfetch(st, index);
                         }
                       Eina_List *temp_header_list = header_list;
                       header_list = eina_list_remove(header_list, last_sti);
                       if (eina_list_count(header_list) == 0)
                         {
                            st->header_items = eina_list_remove(st->header_items, temp_header_list);
                            eina_list_free(header_list);
                         }
                       elm_genlist_item_del(last_sti->item);

                       if (eina_list_count(header_list) == 1)
                         {
                            Elm_Store_Item *temp_sti = eina_list_nth(header_list, 0);
                            if (temp_sti->item_info->item_type == ELM_GENLIST_ITEM_GROUP)
                              {
                                 if (temp_sti->data)
                                   {
                                      int index = elm_store_item_index_get(temp_sti);
                                      _item_unfetch(st, index);
                                   }
                                 header_list = eina_list_remove(header_list, temp_sti);
                                 st->header_items = eina_list_remove(st->header_items, temp_header_list);
                                 eina_list_free(header_list);
                                 elm_genlist_item_del(temp_sti->item);
                              }
                         }
                    }
                  else if (header_item->item_info->group_index == sti->item_info->group_index)
                    {
                       sti->item_info->index = eina_list_count(header_list);
                       Elm_Store_Item *last_sti = eina_list_nth(header_list, eina_list_count(header_list) - 1);
                       header_list = eina_list_append(header_list, sti);
                       sti->item = elm_genlist_item_insert_after(st->genlist,
                                                                 itc,
                                                                 sti,
                                                                 header_item->item,
                                                                 last_sti->item,
                                                                 ELM_GENLIST_ITEM_NONE,
                                                                 (Evas_Smart_Cb)sti->store->cb.item_select.func,
                                                                 (void *)sti->store->cb.item_select.data);
                       elm_store_item_update(st, sti);
                    }
               }
          }
     }
   else
     {
        if (st->header_items)
          {
             Eina_Bool normal_add = EINA_TRUE;
             Eina_List *l;
             Eina_List *header_list;

             EINA_LIST_FOREACH(st->header_items, l, header_list)
               {
                  if (header_list)
                    {
                       Elm_Store_Item *header_item = eina_list_nth(header_list, 0);

                       if (header_item->item_info->group_index == sti->item_info->group_index)
                         {
                            sti->item_info->index = eina_list_count(header_list);
                            Elm_Store_Item *last_sti = eina_list_nth(header_list, eina_list_count(header_list) - 1);
                            header_list = eina_list_append(header_list, sti);
                            sti->item = elm_genlist_item_insert_after(st->genlist,
                                                                      itc,
                                                                      sti,
                                                                      header_item->item,
                                                                      last_sti->item,
                                                                      ELM_GENLIST_ITEM_NONE,
                                                                      (Evas_Smart_Cb)sti->store->cb.item_select.func,
                                                                      (void *)sti->store->cb.item_select.data);
                            elm_store_item_update(st, sti);
                            normal_add = EINA_FALSE;
                            break;
                         }
                    }
               }
             if (normal_add)
               {
                  Eina_List *new_header_list = NULL;
                  sti->item_info->index = 0;
                  new_header_list = eina_list_append(new_header_list, sti);
                  st->header_items = eina_list_append(st->header_items, new_header_list);
                  sti->item = elm_genlist_item_append(st->genlist,
                                                      itc,
                                                      sti,
                                                      NULL,
                                                      ELM_GENLIST_ITEM_NONE,
                                                      (Evas_Smart_Cb)sti->store->cb.item_select.func,
                                                      (void *)sti->store->cb.item_select.data);
                  elm_store_item_update(st, sti);
               }
          }
        else
          {
             if (st->live)
               {
                  Eina_List *header_list = NULL;
                  sti->item_info->index = 0;
                  header_list = eina_list_append(header_list, sti);
                  st->header_items = eina_list_append(st->header_items, header_list);
                  sti->item = elm_genlist_item_append(st->genlist,
                                                      itc,
                                                      sti,
                                                      NULL,
                                                      ELM_GENLIST_ITEM_NONE,
                                                      (Evas_Smart_Cb)sti->store->cb.item_select.func,
                                                      (void *)sti->store->cb.item_select.data);
                  elm_store_item_update(st, sti);
               }
          }
     }
}

static void
_item_free(Elm_Store_Item *sti)
{
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return;
   elm_store_item_del(sti);
}

static void
_store_free(Elm_Store *st)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   elm_store_free(st);
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
   EINA_SAFETY_ON_NULL_RETURN(st);
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;

   st->item_count = count;
   if (count > 0)
     {
        st->multi_load = EINA_TRUE;
     }
   else
     {
        st->multi_load = EINA_FALSE;
     }
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
   EINA_SAFETY_ON_NULL_RETURN_VAL(sti, -1);
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return -1;

   if (sti->store->live)
     {
        int index = 0;
        Eina_List *l;
        Eina_List *header_list;

        EINA_LIST_FOREACH(sti->store->header_items, l, header_list)
          {
             if (header_list)
               {
                  Elm_Store_Item *temp_sti = eina_list_nth(header_list, 0);
                  if (sti->item_info->group_index == temp_sti->item_info->group_index)
                    {
                       index = index + sti->item_info->index;
                       break;
                    }
                  else
                    {
                       index = index + eina_list_count(header_list);
                    }
               }
          }
        return index;
     }
   else
     {
        return -1;
     }
}

/**
 * Sets the select func that select the state of a list item whether true or false
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
   EINA_SAFETY_ON_NULL_RETURN(st);
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
   EINA_SAFETY_ON_NULL_RETURN(st);
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;

   st->cb.item_sort.func = func;
   st->cb.item_sort.data = (void *)data;
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
   EINA_SAFETY_ON_NULL_RETURN_VAL(sti, -1);
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return -1;

   if (sti->store->live)
     {
        if (sti->item_info->item_type == ELM_GENLIST_ITEM_NONE)
          {
             int index = 0;
             int header_count = 0;
             Eina_List *l;
             Eina_List *header_list;

             EINA_LIST_FOREACH(sti->store->header_items, l, header_list)
               {

                  if (header_list)
                    {
                       Elm_Store_Item *temp_sti = eina_list_nth(header_list, 0);

                       if (temp_sti->item_info->item_type == ELM_GENLIST_ITEM_GROUP)
                         {
                            header_count++;
                         }

                       if (sti->item_info->group_index == temp_sti->item_info->group_index)
                         {
                            index = index + sti->item_info->index - header_count;
                            break;
                         }
                       else
                         {
                            index = index + eina_list_count(header_list);
                         }
                    }
               }
             return index;
          }
        else
          {
             return -1;
          }
     }
   else
     {
        return -1;
     }
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

   std->p_db = p_db;

   if (store->list_th)
     {
        ecore_thread_cancel(store->list_th);
        store->list_th = NULL;
     }
   store->list_th = ecore_thread_feedback_run(_list_do, _list_update, _list_end, _list_cancel, store, EINA_TRUE);
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
   EINA_SAFETY_ON_NULL_RETURN_VAL(st, NULL);
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return NULL;
   EINA_SAFETY_ON_NULL_RETURN_VAL(info, NULL);
   Elm_Store_Item *sti;
   Elm_Genlist_Item_Class *itc;

   sti = calloc(1, sizeof(Elm_Store_Item));
   if (!sti) return NULL;
   if (st->fetch_thread)
     {
        LKI(sti->lock);
     }
   EINA_MAGIC_SET(sti, ELM_STORE_ITEM_MAGIC);

   sti->store = st;
   st->total_count++;

   sti->item_info = info;

   itc = info->item_class;
   if (!itc) itc = &_store_item_class;
   else
     {
        itc->func.label_get = (GenlistItemLabelGetFunc)_item_label_get;
        itc->func.icon_get = (GenlistItemIconGetFunc)_item_icon_get;
        itc->func.state_get = NULL;
        itc->func.del = NULL;
     }

   if (st->live)
     {
        if (sti->item_info->item_type == ELM_GENLIST_ITEM_GROUP)
          {
             _group_item_append(sti, itc);
          }
        else
          {
             _normal_item_append(sti, itc);
          }
        return sti;
     }
   else
     {
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
   EINA_SAFETY_ON_NULL_RETURN(st);
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;

   int loop;
   for (loop = st->current_top_index; loop < (st->current_top_index + st->block_count); loop++)
     {
        Elm_Store_Item *temp_sti = _item_get(st, loop);
        if (temp_sti)
          {
             if (temp_sti->fetched)
               {
                  _item_unfetch(st, loop);
               }
             _item_fetch(st, loop);
             if (temp_sti->data) elm_genlist_item_update(temp_sti->item);

             if (!st->fetch_thread)
               {
                  if (temp_sti->data) elm_genlist_item_update(temp_sti->item);
               }
          }
        else
          {
             break;
          }
     }
}

/**
 * Realize the item to the screen
 *
 * @param st The store object
 * @param sti The store item object
 *
 * @ingroup Store
 */
EAPI void
elm_store_item_update(Elm_Store *st, Elm_Store_Item *sti)
{
   EINA_SAFETY_ON_NULL_RETURN(st);
   EINA_SAFETY_ON_NULL_RETURN(sti);
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return;

   int index = elm_store_item_index_get(sti);

   if ((st->start_fetch_index <= index) && (index <= st->end_fetch_index))
     {
        if (sti->fetched)
          {
             _item_unfetch(st, index);
          }
        _item_fetch(st, index);
        if (sti->data) elm_genlist_item_update(sti->item);
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
   EINA_SAFETY_ON_NULL_RETURN(sti);
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return;

   sti->store->total_count--;

   Eina_List *l;
   Eina_List*l_next;
   Eina_List *header_list;

   EINA_LIST_FOREACH_SAFE(sti->store->header_items, l, l_next, header_list)
     {
        if (header_list)
          {
             Elm_Store_Item *item = eina_list_nth(header_list, 0);

             if (item->item_info->group_index == sti->item_info->group_index)
               {
                  Eina_List *in_l;
                  Eina_List *in_l_next;
                  Elm_Store_Item *temp_sti;
                  EINA_LIST_FOREACH_SAFE(header_list, in_l, in_l_next, temp_sti)
                    {
                       if (temp_sti->item_info->index == sti->item_info->index)
                         {
                            if (temp_sti->data)
                              {
                                 int index = elm_store_item_index_get(temp_sti);
                                 _item_unfetch(sti->store, index);
                              }
                            Eina_List *temp_header_list = header_list;
                            header_list = eina_list_remove(header_list, temp_sti);
                            if (eina_list_count(header_list) == 0)
                              {
                                 sti->store->header_items = eina_list_remove(sti->store->header_items, temp_header_list);
                                 eina_list_free(header_list);
                              }
                            LKD(sti->lock);
                            elm_genlist_item_del(temp_sti->item);
                            free(sti);
                         }
                    }
               }
          }
     }
}

// TODO: END -DBsystem store

