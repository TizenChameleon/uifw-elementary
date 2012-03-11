   /**
    * @defgroup Controlbar Controlbar
    * @ingroup Elementary
    * @addtogroup Controlbar
    * @{
    *
    * This is a Controlbar. It can contain label and icon objects.
    * In edit mode, you can change the location of items.
    */

   /* Control Bar */

   typedef enum _Elm_Controlbar_Mode_Type
     {
        ELM_CONTROLBAR_MODE_DEFAULT = 0,
        ELM_CONTROLBAR_MODE_TRANSLUCENCE,
        ELM_CONTROLBAR_MODE_TRANSPARENCY,
        ELM_CONTROLBAR_MODE_LARGE,
        ELM_CONTROLBAR_MODE_SMALL,
        ELM_CONTROLBAR_MODE_LEFT,
        ELM_CONTROLBAR_MODE_RIGHT
     } Elm_Controlbar_Mode_Type;

   typedef struct _Elm_Controlbar_Item Elm_Controlbar_Item;
   /**
    * Add a new controlbar object
    *
    * @param parent The parent object
    * @return The new object or NULL if it cannot be created
    */
   EAPI Evas_Object *elm_controlbar_add(Evas_Object *parent);
   /**
    * Append new tab item
    *
    * @param	obj The controlbar object
    * @param	icon_path The icon path of item
    * @param	label The label of item
    * @param	view The view of item
    * @return	The item of controlbar
    */
   EAPI Elm_Controlbar_Item *elm_controlbar_tab_item_append(Evas_Object *obj, const char *icon_path, const char *label, Evas_Object *view);
   /**
    * Prepend new tab item
    *
    * @param	obj The controlbar object
    * @param	icon_path The icon path of item
    * @param	label The label of item
    * @param	view The view of item
    * @return	The item of controlbar
    */
   EAPI Elm_Controlbar_Item *elm_controlbar_tab_item_prepend(Evas_Object *obj, const char *icon_path, const char *label, Evas_Object *view);
   /**
    * Insert new tab item before given item
    *
    * @param	obj The controlbar object
    * @param	before The given item
    * @param	icon_path The icon path of item
    * @param	label The label of item
    * @param	view The view of item
    * @return	The item of controlbar
    */
   EAPI Elm_Controlbar_Item *elm_controlbar_tab_item_insert_before(Evas_Object *obj, Elm_Controlbar_Item *before, const char *icon_path, const char *label, Evas_Object *view);
   /**
    * Insert new tab item after given item
    *
    * @param	obj The controlbar object
    * @param	after The given item
    * @param	icon_path The icon path of item
    * @param	label The label of item
    * @param	view The view of item
    * @return	The item of controlbar
    */
   EAPI Elm_Controlbar_Item *elm_controlbar_tab_item_insert_after(Evas_Object *obj, Elm_Controlbar_Item *after, const char *icon_path, const char *label, Evas_Object *view);
   /**
    * Append new tool item
    *
    * @param	obj The controlbar object
    * @param	icon_path The icon path of item
    * @param	label The label of item
    * @param	func Callback function of item
    * @param	data The data of callback function
    * @return	The item of controlbar
    */
   EAPI Elm_Controlbar_Item *elm_controlbar_tool_item_append(Evas_Object *obj, const char *icon_path, const char *label, void (*func) (void *data, Evas_Object *obj, void *event_info), void *data);
   /**
    * Prepend new tool item
    *
    * @param	obj The controlbar object
    * @param	icon_path The icon path of item
    * @param	label The label of item
    * @param	func Callback function of item
    * @param	data The data of callback function
    * @return	The item of controlbar
    */
   EAPI Elm_Controlbar_Item *elm_controlbar_tool_item_prepend(Evas_Object *obj, const char *icon_path, const char *label, void (*func) (void *data, Evas_Object *obj, void *event_info), void *data);
   /**
    * Insert new tool item before given item
    *
    * @param	obj The controlbar object
    * @param	before The given item
    * @param	icon_path The icon path of item
    * @param	label The label of item
    * @param	func Callback function of item
    * @param	data The data of callback function
    * @return	The item of controlbar
    */
   EAPI Elm_Controlbar_Item *elm_controlbar_tool_item_insert_before(Evas_Object *obj, Elm_Controlbar_Item *before, const char *icon_path, const char *label, void (*func) (void *data, Evas_Object *obj, void *event_info), void *data);
   /**
    * Insert new tool item after given item
    *
    * @param	obj The controlbar object
    * @param	after The given item
    * @param	icon_path The icon path of item
    * @param	label The label of item
    * @param	func Callback function of item
    * @param	data The data of callback function
    * @return	The item of controlbar
    */
   EAPI Elm_Controlbar_Item *elm_controlbar_tool_item_insert_after(Evas_Object *obj, Elm_Controlbar_Item *after, const char *icon_path, const char *label, void (*func) (void *data, Evas_Object *obj, void *event_info), void *data);
   /**
    * Append new object item
    *
    * @param	obj The controlbar object
    * @param	obj_item The object of item
    * @param	sel The number of sel occupied
    * @return  The item of controlbar
    */
   EAPI Elm_Controlbar_Item *elm_controlbar_object_item_append(Evas_Object *obj, Evas_Object *obj_item, const int sel);
   /**
    * Prepend new object item
    *
    * @param	obj The controlbar object
    * @param	obj_item The object of item
    * @param	sel The number of sel occupied
    * @return  The item of controlbar
    */
   EAPI Elm_Controlbar_Item *elm_controlbar_object_item_prepend(Evas_Object *obj, Evas_Object *obj_item, const int sel);
   /**
    * Insert new object item before given item
    *
    * @param	obj The controlbar object
    * @param	before The given item
    * @param	obj_item The object of item
    * @param	sel The number of sel occupied
    * @return  The item of controlbar
    */
   EAPI Elm_Controlbar_Item *elm_controlbar_object_item_insert_before(Evas_Object *obj, Elm_Controlbar_Item *before, Evas_Object *obj_item, const int sel);
   /**
    * Insert new object item after given item
    *
    * @param	obj The controlbar object
    * @param	after The given item
    * @param	obj_item The object of item
    * @param	sel The number of sel occupied
    * @return  The item of controlbar
    */
   EAPI Elm_Controlbar_Item *elm_controlbar_object_item_insert_after(Evas_Object *obj, Elm_Controlbar_Item *after, Evas_Object *obj_item, const int sel);
   /**
    * Get the object of the object item
    *
    * @param       it The item of controlbar
    * @return      The object of the object item
    */
   EAPI Evas_Object *elm_controlbar_object_item_object_get(const Elm_Controlbar_Item *it);
   /**
    * Delete item from controlbar
    *
    * @param	it The item of controlbar
    */
   EAPI void         elm_controlbar_item_del(Elm_Controlbar_Item *it);
   /**
    * Select item in controlbar
    *
    * @param	it The item of controlbar
    */
   EAPI void         elm_controlbar_item_select(Elm_Controlbar_Item *it);
   /**
    * Set the visible status of item in bar
    *
    * @param	it The item of controlbar
    * @param	bar EINA_TRUE or EINA_FALSE
    */
   EAPI void         elm_controlbar_item_visible_set(Elm_Controlbar_Item *it, Eina_Bool bar);
   /**
    * Get the result which or not item is visible in bar
    *
    * @param	it The item of controlbar
    * @return	EINA_TRUE or EINA_FALSE
    */
   EAPI Eina_Bool    elm_controlbar_item_visible_get(const Elm_Controlbar_Item * it);
   /**
    * Set item disable
    *
    * @param	it The item of controlbar
    * @param	bar EINA_TRUE or EINA_FALSE
    */
   EAPI void         elm_controlbar_item_disabled_set(Elm_Controlbar_Item * it, Eina_Bool disabled);
   /**
    * Get item disable
    *
    * @param	it The item of controlbar
    * @return 	EINA_TRUE or EINA_FALSE
    */
   EAPI Eina_Bool    elm_controlbar_item_disabled_get(const Elm_Controlbar_Item * it);
   /**
    * Set the icon of item
    *
    * @param	it The item of controlbar
    * @param	icon_path The icon path of the item
    * @return	The icon object
    */
   EAPI void         elm_controlbar_item_icon_set(Elm_Controlbar_Item *it, const char *icon_path);
   /**
    * Get the icon of item
    *
    * @param	it The item of controlbar
    * @return	The icon object
    */
   EAPI Evas_Object *elm_controlbar_item_icon_get(const Elm_Controlbar_Item *it);
   /**
    * Set the label of item
    *
    * @param	it The item of controlbar
    * @param	label The label of item
    */
   EAPI void         elm_controlbar_item_label_set(Elm_Controlbar_Item *it, const char *label);
   /**
    * Get the label of item
    *
    * @param	it The item of controlbar
    * @return The label of item
    */
   EAPI const char  *elm_controlbar_item_label_get(const Elm_Controlbar_Item *it);
   /**
    * Get the selected item
    *
    * @param	obj The controlbar object
    * @return		The item of controlbar
    */
   EAPI Elm_Controlbar_Item *elm_controlbar_selected_item_get(const Evas_Object *obj);
   /**
    * Get the first item
    *
    * @param	obj The controlbar object
    * @return		The item of controlbar
    */
   EAPI Elm_Controlbar_Item *elm_controlbar_first_item_get(const Evas_Object *obj);
   /**
    * Get the last item
    *
    * @param	obj The controlbar object
    * @return		The item of controlbar
    */
   EAPI Elm_Controlbar_Item *elm_controlbar_last_item_get(const Evas_Object *obj);
   /**
    * Get the items
    *
    * @param	obj The controlbar object
    * @return	The list of the items
    */
   EAPI const Eina_List   *elm_controlbar_items_get(const Evas_Object *obj);
   /**
    * Get the previous item
    *
    * @param	it The item of controlbar
    * @return	The previous item of the parameter item
    */
   EAPI Elm_Controlbar_Item *elm_controlbar_item_prev(Elm_Controlbar_Item *it);
   /**
    * Get the next item
    *
    * @param	obj The controlbar object
    * @return	The next item of the parameter item
    */
   EAPI Elm_Controlbar_Item *elm_controlbar_item_next(Elm_Controlbar_Item *it);
   /**
    * Set the view of the item
    *
    * @param	it The item of controlbar
    * @param	view The view for the item
    */
   EAPI void         elm_controlbar_item_view_set(Elm_Controlbar_Item *it, Evas_Object * view);
   /**
    * Get the view of the item
    *
    * @param	it The item of controlbar
    * @return	The view for the item
    */
   EAPI Evas_Object *elm_controlbar_item_view_get(const Elm_Controlbar_Item *it);
   /**
    * Unset the view of the item
    *
    * @param	it The item of controlbar
    * @return	The view for the item
    */
   EAPI Evas_Object *elm_controlbar_item_view_unset(Elm_Controlbar_Item *it);
   /**
    * Set the vertical mode of the controlbar
    *
    * @param	obj The object of the controlbar
    * @param	vertical The vertical mode of the controlbar (TRUE = vertical, FALSE = horizontal)
    */
   EAPI Evas_Object *elm_controlbar_item_button_get(const Elm_Controlbar_Item *it);
   /**
    * Set the mode of the controlbar
    *
    * @param	obj The object of the controlbar
    * @param	mode The mode of the controlbar
    */
   EAPI void         elm_controlbar_mode_set(Evas_Object *obj, int mode);
   /**
    * Set the alpha of the controlbar
    *
    * @param	obj The object of the controlbar
    * @param	alpha The alpha value of the controlbar (0-100)
    */
   EAPI void         elm_controlbar_alpha_set(Evas_Object *obj, int alpha);
   /**
    * Set auto-align mode of the controlbar(It's not prepared yet)
    * If you set the auto-align and add items more than 5,
    * the "more" item will be made and the items more than 5 will be unvisible.
    *
    * @param	obj The object of the controlbar
    * @param	auto_align The dicision that the controlbar use the auto-align
    */
   EAPI void         elm_controlbar_item_auto_align_set(Evas_Object *obj, Eina_Bool auto_align);
   /**
    * Get the button object of the item
    *
    * @param	it The item of controlbar
    * @return  button object of the item
    */
   EAPI void         elm_controlbar_vertical_set(Evas_Object *obj, Eina_Bool vertical);
   /**
    * @}
    */

