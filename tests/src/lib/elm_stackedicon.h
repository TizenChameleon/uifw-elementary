   /**
    * @defgroup Stackedicon Stackedicon
    * @ingroup Elementary
    * @addtogroup Stackedicon
    * @{
    *
    * This is a Stackedicon.
    * smart callback called:
    * "expanded" - This signal is emitted when a stackedicon is expanded.
    * "clicked" - This signal is emitted when a stackedicon is clicked.
    *
    * available styles:
    * default
    */
   typedef struct _Stackedicon_Item Elm_Stackedicon_Item;
   /**
    * Add a new stackedicon to the parent
    *
    * @param parent The parent object
    * @return The new object or NULL if it cannot be created
    */
   EAPI Evas_Object          *elm_stackedicon_add(Evas_Object *parent);
   /**
    * This appends a path to the stackedicon
    *
    * @param    obj   The stackedicon object
    * @param    path   The image full path
    * @return   The new item or NULL if it cannot be created
    */
   EAPI Elm_Stackedicon_Item *elm_stackedicon_item_append(Evas_Object *obj, const char *path);
   /**
    * This prepends a path to the stackedicon
    *
    * @param    obj   The stackedicon object
    * @param    path   The image full path
    * @return   The new item or NULL if it cannot be created
    */
   EAPI Elm_Stackedicon_Item *elm_stackedicon_item_prepend(Evas_Object *obj, const char *path);
   /**
    * This delete a path at the stackedicon
    *
    * @param    Elm_Stackedicon_Item   The delete item
    */
   EAPI void                  elm_stackedicon_item_del(Elm_Stackedicon_Item *it);
   /**
    * Get item list from the stackedicon
    *
    * @param    obj   The stackedicon object
    * @return   The item list or NULL if it cannot be created
    */
   EAPI Eina_List            *elm_stackedicon_item_list_get(Evas_Object *obj);
   /**
    * @}
    */

