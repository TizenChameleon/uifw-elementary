<<<<<<< HEAD
#include <Elementary.h>
#include <Elementary_Cursor.h>
#include "elm_priv.h"

#define ELM_GEN_ITEM_FROM_INLIST(it) \
   ((it) ? EINA_INLIST_CONTAINER_GET(it, Elm_Gen_Item) : NULL)

typedef struct Elm_Gen_Item_Type Elm_Gen_Item_Type;
typedef struct Elm_Gen_Item_Tooltip Elm_Gen_Item_Tooltip;
typedef struct _Widget_Data Widget_Data;
typedef struct _Pan Pan;

struct Elm_Gen_Item_Tooltip
{
   const void                 *data;
   Elm_Tooltip_Item_Content_Cb content_cb;
   Evas_Smart_Cb               del_cb;
   const char                 *style;
   Eina_Bool                   free_size : 1;
};

struct _Pan
{
   Evas_Object_Smart_Clipped_Data __clipped_data;
   Widget_Data                   *wd;
   Ecore_Job                     *resize_job;
};

struct Elm_Gen_Item
{
   ELM_WIDGET_ITEM;
   EINA_INLIST;
   Widget_Data                  *wd;
   Elm_Gen_Item_Type            *item;
   const Elm_Gen_Item_Class     *itc;
   Evas_Coord                    x, y, dx, dy;
   Evas_Object                  *spacer;
   Elm_Gen_Item                 *parent;
   Eina_List                    *labels, *contents, *states, *content_objs;
   Ecore_Timer                  *long_timer;
   int                           relcount;
   int                           walking;
   const char                   *mouse_cursor;

   struct
   {
      Evas_Smart_Cb func;
      const void   *data;
   } func;

   Elm_Gen_Item_Tooltip tooltip;
   Ecore_Cb    del_cb, sel_cb, highlight_cb;
   Ecore_Cb    unsel_cb, unhighlight_cb, unrealize_cb;

   Eina_Bool   want_unrealize : 1;
   Eina_Bool   display_only : 1;
   Eina_Bool   realized : 1;
   Eina_Bool   selected : 1;
   Eina_Bool   highlighted : 1;
   Eina_Bool   disabled : 1;
   Eina_Bool   dragging : 1;
   Eina_Bool   delete_me : 1;
   Eina_Bool   down : 1;
   Eina_Bool   group : 1;
   Eina_Bool   reorder : 1;
   Eina_Bool   mode_set : 1; /* item uses style mode for highlight/select */
};

Elm_Gen_Item *
elm_gen_item_new(Widget_Data              *wd,
                 const Elm_Gen_Item_Class *itc,
                 const void               *data,
                 Elm_Gen_Item             *parent,
                 Evas_Smart_Cb             func,
                 const void               *func_data);

void
elm_gen_item_unrealize(Elm_Gen_Item *it,
                       Eina_Bool     calc);
void
elm_gen_item_del_serious(Elm_Gen_Item *it);

void
elm_gen_item_del_notserious(Elm_Gen_Item *it);
=======
typedef struct Elm_Gen_Item             Elm_Gen_Item;

/**
 * @struct Elm_Gen_Item_Class
 *
 * Gengrid or Genlist item class definition.
 * field details.
 */
typedef struct _Elm_Gen_Item_Class      Elm_Gen_Item_Class;

/**
 * Text fetching class function for Elm_Gen_Item_Class.
 * @param data The data passed in the item creation function
 * @param obj The base widget object
 * @param part The part name of the swallow
 * @return The allocated (NOT stringshared) string to set as the text
 */
typedef char                         *(*Elm_Gen_Item_Text_Get_Cb)(void *data, Evas_Object *obj, const char *part); /**< Label fetching class function for gen item classes. */

/**
 * Content (swallowed object) fetching class function for Elm_Gen_Item_Class.
 * @param data The data passed in the item creation function
 * @param obj The base widget object
 * @param part The part name of the swallow
 * @return The content object to swallow
 */
typedef Evas_Object                  *(*Elm_Gen_Item_Content_Get_Cb)(void *data, Evas_Object *obj, const char *part); /**< Content(swallowed object) fetching class function for gen item classes. */

/**
 * State fetching class function for Elm_Gen_Item_Class.
 * @param data The data passed in the item creation function
 * @param obj The base widget object
 * @param part The part name of the swallow
 * @return The hell if I know
 */
typedef Eina_Bool                     (*Elm_Gen_Item_State_Get_Cb)(void *data, Evas_Object *obj, const char *part); /**< State fetching class function for gen item classes. */

/**
 * Deletion class function for Elm_Gen_Item_Class.
 * @param data The data passed in the item creation function
 * @param obj The base widget object
 */
typedef void                          (*Elm_Gen_Item_Del_Cb)(void *data, Evas_Object *obj); /**< Deletion class function for gen item classes. */

struct _Elm_Gen_Item_Class
{
   int           version;  /**< Set by elementary if you alloc an item class using elm_gengrid_item_class_new() or elm_genlist_item_class_new(), or if you set your own class (must be const) then set it to ELM_GENGRID_ITEM_CLASS_VERSION */
   unsigned int  refcount; /**< Set it to 0 if you use your own const class, or its managed for you by class ref/unref calls */
   Eina_Bool     delete_me : 1; /**< Leave this alone - set it to 0 if you have a const class of your own */
   const char   *item_style; /**< Name of the visual style to use for this item. If you don't know use "default" */
   const char   *mode_item_style; /**< Style used if item is set to a specific mode. @see elm_genlist_item_mode_set() or NULL if you don't care. currently it's used only in genlist. */
   const char   *edit_item_style; /**< Style to use when in edit mode, or NULL if you don't care. currently it's used only in genlist. */
   struct {
      Elm_Gen_Item_Text_Get_Cb    text_get; /**< Text fetching class function for gengrid/list item classes.*/
      Elm_Gen_Item_Content_Get_Cb content_get; /**< Content fetching class function for gengrid/list item classes. */
      Elm_Gen_Item_State_Get_Cb   state_get; /**< State fetching class function for gengrid/list item classes. */
      Elm_Gen_Item_Del_Cb         del; /**< Deletion class function for gengrid/list item classes. */
   } func;
}; /**< #Elm_Gen_Item_Class member definitions */

#define ELM_GEN_ITEM_CLASS_VERSION 2
#define ELM_GEN_ITEM_CLASS_HEADER ELM_GEN_ITEM_CLASS_VERSION, 0, 0
>>>>>>> remotes/origin/upstream
