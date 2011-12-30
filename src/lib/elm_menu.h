   /**
    * @defgroup Menu Menu
    *
    * @image html img/widget/menu/preview-00.png
    * @image latex img/widget/menu/preview-00.eps
    *
    * A menu is a list of items displayed above its parent. When the menu is
    * showing its parent is darkened. Each item can have a sub-menu. The menu
    * object can be used to display a menu on a right click event, in a toolbar,
    * anywhere.
    *
    * Signals that you can add callbacks for are:
    * @li "clicked" - the user clicked the empty space in the menu to dismiss.
    *
    * Default contents parts of the menu items that you can use for are:
    * @li "default" - A main content of the menu item
    *
    * Default text parts of the menu items that you can use for are:
    * @li "default" - label in the menu item
    *
    * @see @ref tutorial_menu
    * @{
    */

   /**
    * @brief Add a new menu to the parent
    *
    * @param parent The parent object.
    * @return The new object or NULL if it cannot be created.
    */
   EAPI Evas_Object       *elm_menu_add(Evas_Object *parent) EINA_ARG_NONNULL(1);

   /**
    * @brief Set the parent for the given menu widget
    *
    * @param obj The menu object.
    * @param parent The new parent.
    */
   EAPI void               elm_menu_parent_set(Evas_Object *obj, Evas_Object *parent) EINA_ARG_NONNULL(1);

   /**
    * @brief Get the parent for the given menu widget
    *
    * @param obj The menu object.
    * @return The parent.
    *
    * @see elm_menu_parent_set()
    */
   EAPI Evas_Object       *elm_menu_parent_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

   /**
    * @brief Move the menu to a new position
    *
    * @param obj The menu object.
    * @param x The new position.
    * @param y The new position.
    *
    * Sets the top-left position of the menu to (@p x,@p y).
    *
    * @note @p x and @p y coordinates are relative to parent.
    */
   EAPI void               elm_menu_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y) EINA_ARG_NONNULL(1);

   /**
    * @brief Close a opened menu
    *
    * @param obj the menu object
    * @return void
    *
    * Hides the menu and all it's sub-menus.
    */
   EAPI void               elm_menu_close(Evas_Object *obj) EINA_ARG_NONNULL(1);

   /**
    * @brief Returns a list of @p item's items.
    *
    * @param obj The menu object
    * @return An Eina_List* of @p item's items
    */
   EAPI const Eina_List   *elm_menu_items_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

   /**
    * @brief Get the Evas_Object of an Elm_Object_Item
    *
    * @param it The menu item object.
    * @return The edje object containing the swallowed content
    *
    * @warning Don't manipulate this object!
    *
    */
   EAPI Evas_Object       *elm_menu_item_object_get(const Elm_Object_Item *it) EINA_ARG_NONNULL(1);

   /**
    * @brief Add an item at the end of the given menu widget
    *
    * @param obj The menu object.
    * @param parent The parent menu item (optional)
    * @param icon An icon display on the item. The icon will be destryed by the menu.
    * @param label The label of the item.
    * @param func Function called when the user select the item.
    * @param data Data sent by the callback.
    * @return Returns the new item.
    */
   EAPI Elm_Object_Item     *elm_menu_item_add(Evas_Object *obj, Elm_Object_Item *parent, const char *icon, const char *label, Evas_Smart_Cb func, const void *data) EINA_ARG_NONNULL(1);

   /**
    * @brief Add an object swallowed in an item at the end of the given menu
    * widget
    *
    * @param obj The menu object.
    * @param parent The parent menu item (optional)
    * @param subobj The object to swallow
    * @param func Function called when the user select the item.
    * @param data Data sent by the callback.
    * @return Returns the new item.
    *
    * Add an evas object as an item to the menu.
    */
   EAPI Elm_Object_Item     *elm_menu_item_add_object(Evas_Object *obj, Elm_Object_Item *parent, Evas_Object *subobj, Evas_Smart_Cb func, const void *data) EINA_ARG_NONNULL(1);

   /**
    * @brief Set the label of a menu item
    *
    * @param it The menu item object.
    * @param label The label to set for @p item
    *
    * @warning Don't use this funcion on items created with
    * elm_menu_item_add_object() or elm_menu_item_separator_add().
    *
    * @deprecated Use elm_object_item_text_set() instead
    */
   EINA_DEPRECATED EAPI void               elm_menu_item_label_set(Elm_Object_Item *it, const char *label) EINA_ARG_NONNULL(1);

   /**
    * @brief Get the label of a menu item
    *
    * @param it The menu item object.
    * @return The label of @p item
	 * @deprecated Use elm_object_item_text_get() instead
    */
   EINA_DEPRECATED EAPI const char        *elm_menu_item_label_get(const Elm_Object_Item *it) EINA_ARG_NONNULL(1);

   /**
    * @brief Set the icon of a menu item to the standard icon with name @p icon
    *
    * @param it The menu item object.
    * @param icon The icon object to set for the content of @p item
    *
    * Once this icon is set, any previously set icon will be deleted.
    */
   EAPI void               elm_menu_item_object_icon_name_set(Elm_Object_Item *it, const char *icon) EINA_ARG_NONNULL(1, 2);

   /**
    * @brief Get the string representation from the icon of a menu item
    *
    * @param it The menu item object.
    * @return The string representation of @p item's icon or NULL
    *
    * @see elm_menu_item_object_icon_name_set()
    */
   EAPI const char        *elm_menu_item_object_icon_name_get(const Elm_Object_Item *it) EINA_ARG_NONNULL(1);

   /**
    * @brief Set the content object of a menu item
    *
    * @param it The menu item object
    * @param The content object or NULL
    * @return EINA_TRUE on success, else EINA_FALSE
    *
    * Use this function to change the object swallowed by a menu item, deleting
    * any previously swallowed object.
    *
    * @deprecated Use elm_object_item_content_set() instead
    */
   EINA_DEPRECATED EAPI Eina_Bool          elm_menu_item_object_content_set(Elm_Object_Item *it, Evas_Object *obj) EINA_ARG_NONNULL(1);

   /**
    * @brief Get the content object of a menu item
    *
    * @param it The menu item object
    * @return The content object or NULL
    * @note If @p item was added with elm_menu_item_add_object, this
    * function will return the object passed, else it will return the
    * icon object.
    *
    * @see elm_menu_item_object_content_set()
    *
    * @deprecated Use elm_object_item_content_get() instead
    */
   EINA_DEPRECATED EAPI Evas_Object *elm_menu_item_object_content_get(const Elm_Object_Item *it) EINA_ARG_NONNULL(1);

   /**
    * @brief Set the selected state of @p item.
    *
    * @param it The menu item object.
    * @param selected The selected/unselected state of the item
    */
   EAPI void               elm_menu_item_selected_set(Elm_Object_Item *it, Eina_Bool selected) EINA_ARG_NONNULL(1);

   /**
    * @brief Get the selected state of @p item.
    *
    * @param it The menu item object.
    * @return The selected/unselected state of the item
    *
    * @see elm_menu_item_selected_set()
    */
   EAPI Eina_Bool          elm_menu_item_selected_get(const Elm_Object_Item *it) EINA_ARG_NONNULL(1);

   /**
    * @brief Set the disabled state of @p item.
    *
    * @param it The menu item object.
    * @param disabled The enabled/disabled state of the item
    * @deprecated Use elm_object_item_disabled_set() instead
    */
   EINA_DEPRECATED EAPI void               elm_menu_item_disabled_set(Elm_Object_Item *it, Eina_Bool disabled) EINA_ARG_NONNULL(1);

   /**
    * @brief Get the disabled state of @p item.
    *
    * @param it The menu item object.
    * @return The enabled/disabled state of the item
    *
    * @see elm_menu_item_disabled_set()
    * @deprecated Use elm_object_item_disabled_get() instead
    */
   EINA_DEPRECATED EAPI Eina_Bool          elm_menu_item_disabled_get(const Elm_Object_Item *it) EINA_ARG_NONNULL(1);

   /**
    * @brief Add a separator item to menu @p obj under @p parent.
    *
    * @param obj The menu object
    * @param parent The item to add the separator under
    * @return The created item or NULL on failure
    *
    * This is item is a @ref Separator.
    */
   EAPI Elm_Object_Item     *elm_menu_item_separator_add(Evas_Object *obj, Elm_Object_Item *parent) EINA_ARG_NONNULL(1);

   /**
    * @brief Returns whether @p item is a separator.
    *
    * @param it The item to check
    * @return If true, @p item is a separator
    *
    * @see elm_menu_item_separator_add()
    */
   EAPI Eina_Bool          elm_menu_item_is_separator(Elm_Object_Item *it) EINA_ARG_NONNULL(1);

   /**
    * @brief Deletes an item from the menu.
    *
    * @param it The item to delete.
    *
    * @see elm_menu_item_add()
    */
   EAPI void               elm_menu_item_del(Elm_Object_Item *it) EINA_ARG_NONNULL(1);

   /**
    * @brief Set the function called when a menu item is deleted.
    *
    * @param it The item to set the callback on
    * @param func The function called
    *
    * @see elm_menu_item_add()
    * @see elm_menu_item_del()
    */
   EAPI void               elm_menu_item_del_cb_set(Elm_Object_Item *it, Evas_Smart_Cb func) EINA_ARG_NONNULL(1);

   /**
    * @brief Returns the data associated with menu item @p item.
    *
    * @param it The item
    * @return The data associated with @p item or NULL if none was set.
    *
    * This is the data set with elm_menu_add() or elm_menu_item_data_set().
    *
    * @deprecated Use elm_object_item_data_get() instead
    */
   EINA_DEPRECATED EAPI void              *elm_menu_item_data_get(const Elm_Object_Item *it) EINA_ARG_NONNULL(1);

   /**
    * @brief Sets the data to be associated with menu item @p item.
    *
    * @param it The item
    * @param data The data to be associated with @p item
    *
    * @deprecated Use elm_object_item_data_set() instead
    */
   EINA_DEPRECATED EAPI void               elm_menu_item_data_set(Elm_Object_Item *it, const void *data) EINA_ARG_NONNULL(1);

   /**
    * @brief Returns a list of @p item's subitems.
    *
    * @param it The item
    * @return An Eina_List* of @p item's subitems
    *
    * @see elm_menu_add()
    */
   EAPI const Eina_List   *elm_menu_item_subitems_get(const Elm_Object_Item *it) EINA_ARG_NONNULL(1);

   /**
    * @brief Get the position of a menu item
    *
    * @param it The menu item
    * @return The item's index
    *
    * This function returns the index position of a menu item in a menu.
    * For a sub-menu, this number is relative to the first item in the sub-menu.
    *
    * @note Index values begin with 0
    */
   EAPI unsigned int       elm_menu_item_index_get(const Elm_Object_Item *it) EINA_ARG_NONNULL(1) EINA_PURE;

   /**
    * @brief @brief Return a menu item's owner menu
    *
    * @param it The menu item
    * @return The menu object owning @p item, or NULL on failure
    *
    * Use this function to get the menu object owning an item.
    */
   EAPI Evas_Object       *elm_menu_item_menu_get(const Elm_Object_Item *it) EINA_ARG_NONNULL(1) EINA_PURE;

   /**
    * @brief Get the selected item in the menu
    *
    * @param obj The menu object
    * @return The selected item, or NULL if none
    *
    * @see elm_menu_item_selected_get()
    * @see elm_menu_item_selected_set()
    */
   EAPI Elm_Object_Item *elm_menu_selected_item_get(const Evas_Object * obj) EINA_ARG_NONNULL(1);

   /**
    * @brief Get the last item in the menu
    *
    * @param obj The menu object
    * @return The last item, or NULL if none
    */
   EAPI Elm_Object_Item *elm_menu_last_item_get(const Evas_Object * obj) EINA_ARG_NONNULL(1);

   /**
    * @brief Get the first item in the menu
    *
    * @param obj The menu object
    * @return The first item, or NULL if none
    */
   EAPI Elm_Object_Item *elm_menu_first_item_get(const Evas_Object * obj) EINA_ARG_NONNULL(1);

   /**
    * @brief Get the next item in the menu.
    *
    * @param it The menu item object.
    * @return The item after it, or NULL if none
    */
   EAPI Elm_Object_Item *elm_menu_item_next_get(const Elm_Object_Item *it) EINA_ARG_NONNULL(1);

   /**
    * @brief Get the previous item in the menu.
    *
    * @param it The menu item object.
    * @return The item before it, or NULL if none
    */
   EAPI Elm_Object_Item *elm_menu_item_prev_get(const Elm_Object_Item *it) EINA_ARG_NONNULL(1);

   /**
    * @}
    */

