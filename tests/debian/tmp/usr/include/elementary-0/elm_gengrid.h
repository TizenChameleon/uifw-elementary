// XXX: elm_gengrid - make sure api's match 1:1 with genlist api's
// for eventual move to back-end of gengrid == genlist (in grid mode)
/**
 * @defgroup Gengrid Gengrid (Generic grid)
 *
 * This widget aims to position objects in a grid layout while
 * actually creating and rendering only the visible ones, using the
 * same idea as the @ref Genlist "genlist": the user defines a @b
 * class for each item, specifying functions that will be called at
 * object creation, deletion, etc. When those items are selected by
 * the user, a callback function is issued. Users may interact with
 * a gengrid via the mouse (by clicking on items to select them and
 * clicking on the grid's viewport and swiping to pan the whole
 * view) or via the keyboard, navigating through item with the
 * arrow keys.
 *
 * @section Gengrid_Layouts Gengrid layouts
 *
 * Gengrid may layout its items in one of two possible layouts:
 * - horizontal or
 * - vertical.
 *
 * When in "horizontal mode", items will be placed in @b columns,
 * from top to bottom and, when the space for a column is filled,
 * another one is started on the right, thus expanding the grid
 * horizontally, making for horizontal scrolling. When in "vertical
 * mode" , though, items will be placed in @b rows, from left to
 * right and, when the space for a row is filled, another one is
 * started below, thus expanding the grid vertically (and making
 * for vertical scrolling).
 *
 * @section Gengrid_Items Gengrid items
 *
 * An item in a gengrid can have 0 or more texts (they can be
 * regular text or textblock Evas objects - that's up to the style
 * to determine), 0 or more contents (which are simply objects
 * swallowed into the gengrid item's theming Edje object) and 0 or
 * more <b>boolean states</b>, which have the behavior left to the
 * user to define. The Edje part names for each of these properties
 * will be looked up, in the theme file for the gengrid, under the
 * Edje (string) data items named @c "texts", @c "contents" and @c
 * "states", respectively. For each of those properties, if more
 * than one part is provided, they must have names listed separated
 * by spaces in the data fields. For the default gengrid item
 * theme, we have @b one text part (@c "elm.text"), @b two content
 * parts (@c "elm.swalllow.icon" and @c "elm.swallow.end") and @b
 * no state parts.
 *
 * A gengrid item may be at one of several styles. Elementary
 * provides one by default - "default", but this can be extended by
 * system or application custom themes/overlays/extensions (see
 * @ref Theme "themes" for more details).
 *
 * @section Gengrid_Item_Class Gengrid item classes
 *
 * In order to have the ability to add and delete items on the fly,
 * gengrid implements a class (callback) system where the
 * application provides a structure with information about that
 * type of item (gengrid may contain multiple different items with
 * different classes, states and styles). Gengrid will call the
 * functions in this struct (methods) when an item is "realized"
 * (i.e., created dynamically, while the user is scrolling the
 * grid). All objects will simply be deleted when no longer needed
 * with evas_object_del(). The #Elm_GenGrid_Item_Class structure
 * contains the following members:
 * - @c item_style - This is a constant string and simply defines
 * the name of the item style. It @b must be specified and the
 * default should be @c "default".
 * - @c func.text_get - This function is called when an item
 * object is actually created. The @c data parameter will point to
 * the same data passed to elm_gengrid_item_append() and related
 * item creation functions. The @c obj parameter is the gengrid
 * object itself, while the @c part one is the name string of one
 * of the existing text parts in the Edje group implementing the
 * item's theme. This function @b must return a strdup'()ed string,
 * as the caller will free() it when done. See
 * #Elm_Gengrid_Item_Text_Get_Cb.
 * - @c func.content_get - This function is called when an item object
 * is actually created. The @c data parameter will point to the
 * same data passed to elm_gengrid_item_append() and related item
 * creation functions. The @c obj parameter is the gengrid object
 * itself, while the @c part one is the name string of one of the
 * existing (content) swallow parts in the Edje group implementing the
 * item's theme. It must return @c NULL, when no content is desired,
 * or a valid object handle, otherwise. The object will be deleted
 * by the gengrid on its deletion or when the item is "unrealized".
 * See #Elm_Gengrid_Item_Content_Get_Cb.
 * - @c func.state_get - This function is called when an item
 * object is actually created. The @c data parameter will point to
 * the same data passed to elm_gengrid_item_append() and related
 * item creation functions. The @c obj parameter is the gengrid
 * object itself, while the @c part one is the name string of one
 * of the state parts in the Edje group implementing the item's
 * theme. Return @c EINA_FALSE for false/off or @c EINA_TRUE for
 * true/on. Gengrids will emit a signal to its theming Edje object
 * with @c "elm,state,XXX,active" and @c "elm" as "emission" and
 * "source" arguments, respectively, when the state is true (the
 * default is false), where @c XXX is the name of the (state) part.
 * See #Elm_Gengrid_Item_State_Get_Cb.
 * - @c func.del - This is called when elm_gengrid_item_del() is
 * called on an item or elm_gengrid_clear() is called on the
 * gengrid. This is intended for use when gengrid items are
 * deleted, so any data attached to the item (e.g. its data
 * parameter on creation) can be deleted. See #Elm_Gengrid_Item_Del_Cb.
 *
 * @section Gengrid_Usage_Hints Usage hints
 *
 * If the user wants to have multiple items selected at the same
 * time, elm_gengrid_multi_select_set() will permit it. If the
 * gengrid is single-selection only (the default), then
 * elm_gengrid_select_item_get() will return the selected item or
 * @c NULL, if none is selected. If the gengrid is under
 * multi-selection, then elm_gengrid_selected_items_get() will
 * return a list (that is only valid as long as no items are
 * modified (added, deleted, selected or unselected) of child items
 * on a gengrid.
 *
 * If an item changes (internal (boolean) state, text or content
 * changes), then use elm_gengrid_item_update() to have gengrid
 * update the item with the new state. A gengrid will re-"realize"
 * the item, thus calling the functions in the
 * #Elm_Gengrid_Item_Class set for that item.
 *
 * To programmatically (un)select an item, use
 * elm_gengrid_item_selected_set(). To get its selected state use
 * elm_gengrid_item_selected_get(). To make an item disabled
 * (unable to be selected and appear differently) use
 * elm_gengrid_item_disabled_set() to set this and
 * elm_gengrid_item_disabled_get() to get the disabled state.
 *
 * Grid cells will only have their selection smart callbacks called
 * when firstly getting selected. Any further clicks will do
 * nothing, unless you enable the "always select mode", with
 * elm_gengrid_always_select_mode_set(), thus making every click to
 * issue selection callbacks. elm_gengrid_no_select_mode_set() will
 * turn off the ability to select items entirely in the widget and
 * they will neither appear selected nor call the selection smart
 * callbacks.
 *
 * Remember that you can create new styles and add your own theme
 * augmentation per application with elm_theme_extension_add(). If
 * you absolutely must have a specific style that overrides any
 * theme the user or system sets up you can use
 * elm_theme_overlay_add() to add such a file.
 *
 * @section Gengrid_Smart_Events Gengrid smart events
 *
 * Smart events that you can add callbacks for are:
 * - @c "activated" - The user has double-clicked or pressed
 *   (enter|return|spacebar) on an item. The @c event_info parameter
 *   is the gengrid item that was activated.
 * - @c "clicked,double" - The user has double-clicked an item.
 *   The @c event_info parameter is the gengrid item that was double-clicked.
 * - @c "longpressed" - This is called when the item is pressed for a certain
 *   amount of time. By default it's 1 second.
 * - @c "selected" - The user has made an item selected. The
 *   @c event_info parameter is the gengrid item that was selected.
 * - @c "unselected" - The user has made an item unselected. The
 *   @c event_info parameter is the gengrid item that was unselected.
 * - @c "realized" - This is called when the item in the gengrid
 *   has its implementing Evas object instantiated, de facto. @c
 *   event_info is the gengrid item that was created. The object
 *   may be deleted at any time, so it is highly advised to the
 *   caller @b not to use the object pointer returned from
 *   elm_gengrid_item_object_get(), because it may point to freed
 *   objects.
 * - @c "unrealized" - This is called when the implementing Evas
 *   object for this item is deleted. @c event_info is the gengrid
 *   item that was deleted.
 * - @c "changed" - Called when an item is added, removed, resized
 *   or moved and when the gengrid is resized or gets "horizontal"
 *   property changes.
 * - @c "scroll,anim,start" - This is called when scrolling animation has
 *   started.
 * - @c "scroll,anim,stop" - This is called when scrolling animation has
 *   stopped.
 * - @c "drag,start,up" - Called when the item in the gengrid has
 *   been dragged (not scrolled) up.
 * - @c "drag,start,down" - Called when the item in the gengrid has
 *   been dragged (not scrolled) down.
 * - @c "drag,start,left" - Called when the item in the gengrid has
 *   been dragged (not scrolled) left.
 * - @c "drag,start,right" - Called when the item in the gengrid has
 *   been dragged (not scrolled) right.
 * - @c "drag,stop" - Called when the item in the gengrid has
 *   stopped being dragged.
 * - @c "drag" - Called when the item in the gengrid is being
 *   dragged.
 * - @c "scroll" - called when the content has been scrolled
 *   (moved).
 * - @c "scroll,drag,start" - called when dragging the content has
 *   started.
 * - @c "scroll,drag,stop" - called when dragging the content has
 *   stopped.
 * - @c "edge,top" - This is called when the gengrid is scrolled until
 *   the top edge.
 * - @c "edge,bottom" - This is called when the gengrid is scrolled
 *   until the bottom edge.
 * - @c "edge,left" - This is called when the gengrid is scrolled
 *   until the left edge.
 * - @c "edge,right" - This is called when the gengrid is scrolled
 *   until the right edge.
 *
 * List of gengrid examples:
 * @li @ref gengrid_example
 */

/**
 * @addtogroup Gengrid
 * @{
 */

typedef struct _Elm_Gengrid_Item_Class      Elm_Gengrid_Item_Class; /**< Gengrid item class definition structs */
#define Elm_Gengrid_Item_Class Elm_Gen_Item_Class
typedef struct _Elm_Gengrid_Item_Class_Func Elm_Gengrid_Item_Class_Func;    /**< Class functions for gengrid item classes. */

/**
 * Text fetching class function for Elm_Gen_Item_Class.
 * @param data The data passed in the item creation function
 * @param obj The base widget object
 * @param part The part name of the swallow
 * @return The allocated (NOT stringshared) string to set as the text
 */
typedef char *(*Elm_Gengrid_Item_Text_Get_Cb)(void *data, Evas_Object *obj, const char *part);

/**
 * Content (swallowed object) fetching class function for Elm_Gen_Item_Class.
 * @param data The data passed in the item creation function
 * @param obj The base widget object
 * @param part The part name of the swallow
 * @return The content object to swallow
 */
typedef Evas_Object *(*Elm_Gengrid_Item_Content_Get_Cb)(void *data, Evas_Object *obj, const char *part);

/**
 * State fetching class function for Elm_Gen_Item_Class.
 * @param data The data passed in the item creation function
 * @param obj The base widget object
 * @param part The part name of the swallow
 * @return The hell if I know
 */
typedef Eina_Bool (*Elm_Gengrid_Item_State_Get_Cb)(void *data, Evas_Object *obj, const char *part);

/**
 * Deletion class function for Elm_Gen_Item_Class.
 * @param data The data passed in the item creation function
 * @param obj The base widget object
 */
typedef void (*Elm_Gengrid_Item_Del_Cb)(void *data, Evas_Object *obj);

/**
 * @struct _Elm_Gengrid_Item_Class
 *
 * Gengrid item class definition. See @ref Gengrid_Item_Class for
 * field details.
 */
struct _Elm_Gengrid_Item_Class
{
   const char *item_style;
   struct _Elm_Gengrid_Item_Class_Func
   {
      Elm_Gengrid_Item_Text_Get_Cb    text_get; /**< Text fetching class function for gengrid item classes.*/
      Elm_Gengrid_Item_Content_Get_Cb content_get; /**< Content fetching class function for gengrid item classes. */
      Elm_Gengrid_Item_State_Get_Cb   state_get; /**< State fetching class function for gengrid item classes. */
      Elm_Gengrid_Item_Del_Cb         del; /**< Deletion class function for gengrid item classes. */
   } func;
};   /**< #Elm_Gengrid_Item_Class member definitions */
#define Elm_Gengrid_Item_Class_Func Elm_Gen_Item_Class_Func

/**
 * Add a new gengrid widget to the given parent Elementary
 * (container) object
 *
 * @param parent The parent object
 * @return a new gengrid widget handle or @c NULL, on errors
 *
 * This function inserts a new gengrid widget on the canvas.
 *
 * @see elm_gengrid_item_size_set()
 * @see elm_gengrid_group_item_size_set()
 * @see elm_gengrid_horizontal_set()
 * @see elm_gengrid_item_append()
 * @see elm_gengrid_item_del()
 * @see elm_gengrid_clear()
 *
 * @ingroup Gengrid
 */
EAPI Evas_Object                  *elm_gengrid_add(Evas_Object *parent);

/**
 * Set the size for the items of a given gengrid widget
 *
 * @param obj The gengrid object.
 * @param w The items' width.
 * @param h The items' height;
 *
 * A gengrid, after creation, has still no information on the size
 * to give to each of its cells. So, you most probably will end up
 * with squares one @ref Fingers "finger" wide, the default
 * size. Use this function to force a custom size for you items,
 * making them as big as you wish.
 *
 * @see elm_gengrid_item_size_get()
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_item_size_set(Evas_Object *obj, Evas_Coord w, Evas_Coord h);

/**
 * Get the size set for the items of a given gengrid widget
 *
 * @param obj The gengrid object.
 * @param w Pointer to a variable where to store the items' width.
 * @param h Pointer to a variable where to store the items' height.
 *
 * @note Use @c NULL pointers on the size values you're not
 * interested in: they'll be ignored by the function.
 *
 * @see elm_gengrid_item_size_get() for more details
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_item_size_get(const Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);

/**
 * Set the size for the group items of a given gengrid widget
 *
 * @param obj The gengrid object.
 * @param w The group items' width.
 * @param h The group items' height;
 *
 * A gengrid, after creation, has still no information on the size
 * to give to each of its cells. So, you most probably will end up
 * with squares one @ref Fingers "finger" wide, the default
 * size. Use this function to force a custom size for you group items,
 * making them as big as you wish.
 *
 * @see elm_gengrid_group_item_size_get()
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_group_item_size_set(Evas_Object *obj, Evas_Coord w, Evas_Coord h);

/**
 * Get the size set for the group items of a given gengrid widget
 *
 * @param obj The gengrid object.
 * @param w Pointer to a variable where to store the group items' width.
 * @param h Pointer to a variable where to store the group items' height.
 *
 * @note Use @c NULL pointers on the size values you're not
 * interested in: they'll be ignored by the function.
 *
 * @see elm_gengrid_group_item_size_get() for more details
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_group_item_size_get(const Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);

/**
 * Set the items grid's alignment within a given gengrid widget
 *
 * @param obj The gengrid object.
 * @param align_x Alignment in the horizontal axis (0 <= align_x <= 1).
 * @param align_y Alignment in the vertical axis (0 <= align_y <= 1).
 *
 * This sets the alignment of the whole grid of items of a gengrid
 * within its given viewport. By default, those values are both
 * 0.5, meaning that the gengrid will have its items grid placed
 * exactly in the middle of its viewport.
 *
 * @note If given alignment values are out of the cited ranges,
 * they'll be changed to the nearest boundary values on the valid
 * ranges.
 *
 * @see elm_gengrid_align_get()
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_align_set(Evas_Object *obj, double align_x, double align_y);

/**
 * Get the items grid's alignment values within a given gengrid
 * widget
 *
 * @param obj The gengrid object.
 * @param align_x Pointer to a variable where to store the
 * horizontal alignment.
 * @param align_y Pointer to a variable where to store the vertical
 * alignment.
 *
 * @note Use @c NULL pointers on the alignment values you're not
 * interested in: they'll be ignored by the function.
 *
 * @see elm_gengrid_align_set() for more details
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_align_get(const Evas_Object *obj, double *align_x, double *align_y);

/**
 * Set whether a given gengrid widget is or not able have items
 * @b reordered
 *
 * @param obj The gengrid object
 * @param reorder_mode Use @c EINA_TRUE to turn reoderding on,
 * @c EINA_FALSE to turn it off
 *
 * If a gengrid is set to allow reordering, a click held for more
 * than 0.5 over a given item will highlight it specially,
 * signalling the gengrid has entered the reordering state. From
 * that time on, the user will be able to, while still holding the
 * mouse button down, move the item freely in the gengrid's
 * viewport, replacing to said item to the locations it goes to.
 * The replacements will be animated and, whenever the user
 * releases the mouse button, the item being replaced gets a new
 * definitive place in the grid.
 *
 * @see elm_gengrid_reorder_mode_get()
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_reorder_mode_set(Evas_Object *obj, Eina_Bool reorder_mode);

/**
 * Get whether a given gengrid widget is or not able have items
 * @b reordered
 *
 * @param obj The gengrid object
 * @return @c EINA_TRUE, if reoderding is on, @c EINA_FALSE if it's
 * off
 *
 * @see elm_gengrid_reorder_mode_set() for more details
 *
 * @ingroup Gengrid
 */
EAPI Eina_Bool                     elm_gengrid_reorder_mode_get(const Evas_Object *obj);

/**
 * Append a new item in a given gengrid widget.
 *
 * @param obj The gengrid object.
 * @param gic The item class for the item.
 * @param data The item data.
 * @param func Convenience function called when the item is
 * selected.
 * @param func_data Data to be passed to @p func.
 * @return A handle to the item added or @c NULL, on errors.
 *
 * This adds an item to the beginning of the gengrid.
 *
 * @see elm_gengrid_item_prepend()
 * @see elm_gengrid_item_insert_before()
 * @see elm_gengrid_item_insert_after()
 * @see elm_gengrid_item_del()
 *
 * @ingroup Gengrid
 */
EAPI Elm_Object_Item             *elm_gengrid_item_append(Evas_Object *obj, const Elm_Gengrid_Item_Class *gic, const void *data, Evas_Smart_Cb func, const void *func_data);

/**
 * Prepend a new item in a given gengrid widget.
 *
 * @param obj The gengrid object.
 * @param gic The item class for the item.
 * @param data The item data.
 * @param func Convenience function called when the item is
 * selected.
 * @param func_data Data to be passed to @p func.
 * @return A handle to the item added or @c NULL, on errors.
 *
 * This adds an item to the end of the gengrid.
 *
 * @see elm_gengrid_item_append()
 * @see elm_gengrid_item_insert_before()
 * @see elm_gengrid_item_insert_after()
 * @see elm_gengrid_item_del()
 *
 * @ingroup Gengrid
 */
EAPI Elm_Object_Item             *elm_gengrid_item_prepend(Evas_Object *obj, const Elm_Gengrid_Item_Class *gic, const void *data, Evas_Smart_Cb func, const void *func_data);

/**
 * Insert an item before another in a gengrid widget
 *
 * @param obj The gengrid object.
 * @param gic The item class for the item.
 * @param data The item data.
 * @param relative The item to place this new one before.
 * @param func Convenience function called when the item is
 * selected.
 * @param func_data Data to be passed to @p func.
 * @return A handle to the item added or @c NULL, on errors.
 *
 * This inserts an item before another in the gengrid.
 *
 * @see elm_gengrid_item_append()
 * @see elm_gengrid_item_prepend()
 * @see elm_gengrid_item_insert_after()
 * @see elm_gengrid_item_del()
 *
 * @ingroup Gengrid
 */
EAPI Elm_Object_Item             *elm_gengrid_item_insert_before(Evas_Object *obj, const Elm_Gengrid_Item_Class *gic, const void *data, Elm_Object_Item *relative, Evas_Smart_Cb func, const void *func_data);

/**
 * Insert an item after another in a gengrid widget
 *
 * @param obj The gengrid object.
 * @param gic The item class for the item.
 * @param data The item data.
 * @param relative The item to place this new one after.
 * @param func Convenience function called when the item is
 * selected.
 * @param func_data Data to be passed to @p func.
 * @return A handle to the item added or @c NULL, on errors.
 *
 * This inserts an item after another in the gengrid.
 *
 * @see elm_gengrid_item_append()
 * @see elm_gengrid_item_prepend()
 * @see elm_gengrid_item_insert_after()
 * @see elm_gengrid_item_del()
 *
 * @ingroup Gengrid
 */
EAPI Elm_Object_Item             *elm_gengrid_item_insert_after(Evas_Object *obj, const Elm_Gengrid_Item_Class *gic, const void *data, Elm_Object_Item *relative, Evas_Smart_Cb func, const void *func_data);

/**
 * Insert an item in a gengrid widget using a user-defined sort function.
 *
 * @param obj The gengrid object.
 * @param gic The item class for the item.
 * @param data The item data.
 * @param comp User defined comparison function that defines the sort order
 *             based on Elm_Gen_Item and its data param.
 * @param func Convenience function called when the item is selected.
 * @param func_data Data to be passed to @p func.
 * @return A handle to the item added or @c NULL, on errors.
 *
 * This inserts an item in the gengrid based on user defined comparison function.
 *
 * @see elm_gengrid_item_append()
 * @see elm_gengrid_item_prepend()
 * @see elm_gengrid_item_insert_after()
 * @see elm_gengrid_item_del()
 * @see elm_gengrid_item_direct_sorted_insert()
 *
 * @ingroup Gengrid
 */
EAPI Elm_Object_Item             *elm_gengrid_item_sorted_insert(Evas_Object *obj, const Elm_Gengrid_Item_Class *gic, const void *data, Eina_Compare_Cb comp, Evas_Smart_Cb func, const void *func_data);

/**
 * Insert an item in a gengrid widget using a user-defined sort function.
 *
 * @param obj The gengrid object.
 * @param gic The item class for the item.
 * @param data The item data.
 * @param comp User defined comparison function that defines the sort order
 *             based on Elm_Gen_Item.
 * @param func Convenience function called when the item is selected.
 * @param func_data Data to be passed to @p func.
 * @return A handle to the item added or @c NULL, on errors.
 *
 * This inserts an item in the gengrid based on user defined comparison function.
 *
 * @see elm_gengrid_item_append()
 * @see elm_gengrid_item_prepend()
 * @see elm_gengrid_item_insert_after()
 * @see elm_gengrid_item_del()
 * @see elm_gengrid_item_sorted_insert()
 *
 * @ingroup Gengrid
 */
EAPI Elm_Object_Item             *elm_gengrid_item_direct_sorted_insert(Evas_Object *obj, const Elm_Gengrid_Item_Class *gic, const void *data, Eina_Compare_Cb comp, Evas_Smart_Cb func, const void *func_data);

/**
 * Set whether items on a given gengrid widget are to get their
 * selection callbacks issued for @b every subsequent selection
 * click on them or just for the first click.
 *
 * @param obj The gengrid object
 * @param always_select @c EINA_TRUE to make items "always
 * selected", @c EINA_FALSE, otherwise
 *
 * By default, grid items will only call their selection callback
 * function when firstly getting selected, any subsequent further
 * clicks will do nothing. With this call, you make those
 * subsequent clicks also to issue the selection callbacks.
 *
 * @note <b>Double clicks</b> will @b always be reported on items.
 *
 * @see elm_gengrid_always_select_mode_get()
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_always_select_mode_set(Evas_Object *obj, Eina_Bool always_select);

/**
 * Get whether items on a given gengrid widget have their selection
 * callbacks issued for @b every subsequent selection click on them
 * or just for the first click.
 *
 * @param obj The gengrid object.
 * @return @c EINA_TRUE if the gengrid items are "always selected",
 * @c EINA_FALSE, otherwise
 *
 * @see elm_gengrid_always_select_mode_set() for more details
 *
 * @ingroup Gengrid
 */
EAPI Eina_Bool                     elm_gengrid_always_select_mode_get(const Evas_Object *obj);

/**
 * Set whether items on a given gengrid widget can be selected or not.
 *
 * @param obj The gengrid object
 * @param no_select @c EINA_TRUE to make items selectable,
 * @c EINA_FALSE otherwise
 *
 * This will make items in @p obj selectable or not. In the latter
 * case, any user interaction on the gengrid items will neither make
 * them appear selected nor them call their selection callback
 * functions.
 *
 * @see elm_gengrid_no_select_mode_get()
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_no_select_mode_set(Evas_Object *obj, Eina_Bool no_select);

/**
 * Get whether items on a given gengrid widget can be selected or
 * not.
 *
 * @param obj The gengrid object
 * @return @c EINA_TRUE, if items are selectable, @c EINA_FALSE
 * otherwise
 *
 * @see elm_gengrid_no_select_mode_set() for more details
 *
 * @ingroup Gengrid
 */
EAPI Eina_Bool                     elm_gengrid_no_select_mode_get(const Evas_Object *obj);

/**
 * Enable or disable multi-selection in a given gengrid widget
 *
 * @param obj The gengrid object.
 * @param multi @c EINA_TRUE, to enable multi-selection,
 * @c EINA_FALSE to disable it.
 *
 * Multi-selection is the ability to have @b more than one
 * item selected, on a given gengrid, simultaneously. When it is
 * enabled, a sequence of clicks on different items will make them
 * all selected, progressively. A click on an already selected item
 * will unselect it. If interacting via the keyboard,
 * multi-selection is enabled while holding the "Shift" key.
 *
 * @note By default, multi-selection is @b disabled on gengrids
 *
 * @see elm_gengrid_multi_select_get()
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_multi_select_set(Evas_Object *obj, Eina_Bool multi);

/**
 * Get whether multi-selection is enabled or disabled for a given
 * gengrid widget
 *
 * @param obj The gengrid object.
 * @return @c EINA_TRUE, if multi-selection is enabled, @c
 * EINA_FALSE otherwise
 *
 * @see elm_gengrid_multi_select_set() for more details
 *
 * @ingroup Gengrid
 */
EAPI Eina_Bool                     elm_gengrid_multi_select_get(const Evas_Object *obj);

/**
 * Enable or disable bouncing effect for a given gengrid widget
 *
 * @param obj The gengrid object
 * @param h_bounce @c EINA_TRUE, to enable @b horizontal bouncing,
 * @c EINA_FALSE to disable it
 * @param v_bounce @c EINA_TRUE, to enable @b vertical bouncing,
 * @c EINA_FALSE to disable it
 *
 * The bouncing effect occurs whenever one reaches the gengrid's
 * edge's while panning it -- it will scroll past its limits a
 * little bit and return to the edge again, in a animated for,
 * automatically.
 *
 * @note By default, gengrids have bouncing enabled on both axis
 *
 * @see elm_gengrid_bounce_get()
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_bounce_set(Evas_Object *obj, Eina_Bool h_bounce, Eina_Bool v_bounce);

/**
 * Get whether bouncing effects are enabled or disabled, for a
 * given gengrid widget, on each axis
 *
 * @param obj The gengrid object
 * @param h_bounce Pointer to a variable where to store the
 * horizontal bouncing flag.
 * @param v_bounce Pointer to a variable where to store the
 * vertical bouncing flag.
 *
 * @see elm_gengrid_bounce_set() for more details
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_bounce_get(const Evas_Object *obj, Eina_Bool *h_bounce, Eina_Bool *v_bounce);

/**
 * Set a given gengrid widget's scrolling page size, relative to
 * its viewport size.
 *
 * @param obj The gengrid object
 * @param h_pagerel The horizontal page (relative) size
 * @param v_pagerel The vertical page (relative) size
 *
 * The gengrid's scroller is capable of binding scrolling by the
 * user to "pages". It means that, while scrolling and, specially
 * after releasing the mouse button, the grid will @b snap to the
 * nearest displaying page's area. When page sizes are set, the
 * grid's continuous content area is split into (equal) page sized
 * pieces.
 *
 * This function sets the size of a page <b>relatively to the
 * viewport dimensions</b> of the gengrid, for each axis. A value
 * @c 1.0 means "the exact viewport's size", in that axis, while @c
 * 0.0 turns paging off in that axis. Likewise, @c 0.5 means "half
 * a viewport". Sane usable values are, than, between @c 0.0 and @c
 * 1.0. Values beyond those will make it behave behave
 * inconsistently. If you only want one axis to snap to pages, use
 * the value @c 0.0 for the other one.
 *
 * There is a function setting page size values in @b absolute
 * values, too -- elm_gengrid_page_size_set(). Naturally, its use
 * is mutually exclusive to this one.
 *
 * @see elm_gengrid_page_relative_get()
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_page_relative_set(Evas_Object *obj, double h_pagerel, double v_pagerel);

/**
 * Get a given gengrid widget's scrolling page size, relative to
 * its viewport size.
 *
 * @param obj The gengrid object
 * @param h_pagerel Pointer to a variable where to store the
 * horizontal page (relative) size
 * @param v_pagerel Pointer to a variable where to store the
 * vertical page (relative) size
 *
 * @see elm_gengrid_page_relative_set() for more details
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_page_relative_get(const Evas_Object *obj, double *h_pagerel, double *v_pagerel);

/**
 * Set a given gengrid widget's scrolling page size
 *
 * @param obj The gengrid object
 * @param h_pagerel The horizontal page size, in pixels
 * @param v_pagerel The vertical page size, in pixels
 *
 * The gengrid's scroller is capable of binding scrolling by the
 * user to "pages". It means that, while scrolling and, specially
 * after releasing the mouse button, the grid will @b snap to the
 * nearest displaying page's area. When page sizes are set, the
 * grid's continuous content area is split into (equal) page sized
 * pieces.
 *
 * This function sets the size of a page of the gengrid, in pixels,
 * for each axis. Sane usable values are, between @c 0 and the
 * dimensions of @p obj, for each axis. Values beyond those will
 * make it behave behave inconsistently. If you only want one axis
 * to snap to pages, use the value @c 0 for the other one.
 *
 * There is a function setting page size values in @b relative
 * values, too -- elm_gengrid_page_relative_set(). Naturally, its
 * use is mutually exclusive to this one.
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_page_size_set(Evas_Object *obj, Evas_Coord h_pagesize, Evas_Coord v_pagesize);

/**
 * @brief Get gengrid current page number.
 *
 * @param obj The gengrid object
 * @param h_pagenumber The horizontal page number
 * @param v_pagenumber The vertical page number
 *
 * The page number starts from 0. 0 is the first page.
 * Current page means the page which meet the top-left of the viewport.
 * If there are two or more pages in the viewport, it returns the number of page
 * which meet the top-left of the viewport.
 *
 * @see elm_gengrid_last_page_get()
 * @see elm_gengrid_page_show()
 * @see elm_gengrid_page_brint_in()
 */
EAPI void                          elm_gengrid_current_page_get(const Evas_Object *obj, int *h_pagenumber, int *v_pagenumber);

/**
 * @brief Get scroll last page number.
 *
 * @param obj The gengrid object
 * @param h_pagenumber The horizontal page number
 * @param v_pagenumber The vertical page number
 *
 * The page number starts from 0. 0 is the first page.
 * This returns the last page number among the pages.
 *
 * @see elm_gengrid_current_page_get()
 * @see elm_gengrid_page_show()
 * @see elm_gengrid_page_brint_in()
 */
EAPI void                          elm_gengrid_last_page_get(const Evas_Object *obj, int *h_pagenumber, int *v_pagenumber);

/**
 * Show a specific virtual region within the gengrid content object by page number.
 *
 * @param obj The gengrid object
 * @param h_pagenumber The horizontal page number
 * @param v_pagenumber The vertical page number
 *
 * 0, 0 of the indicated page is located at the top-left of the viewport.
 * This will jump to the page directly without animation.
 *
 * Example of usage:
 *
 * @code
 * sc = elm_gengrid_add(win);
 * elm_gengrid_content_set(sc, content);
 * elm_gengrid_page_relative_set(sc, 1, 0);
 * elm_gengrid_current_page_get(sc, &h_page, &v_page);
 * elm_gengrid_page_show(sc, h_page + 1, v_page);
 * @endcode
 *
 * @see elm_gengrid_page_bring_in()
 */
EAPI void                          elm_gengrid_page_show(const Evas_Object *obj, int h_pagenumber, int v_pagenumber);

/**
 * Show a specific virtual region within the gengrid content object by page number.
 *
 * @param obj The gengrid object
 * @param h_pagenumber The horizontal page number
 * @param v_pagenumber The vertical page number
 *
 * 0, 0 of the indicated page is located at the top-left of the viewport.
 * This will slide to the page with animation.
 *
 * Example of usage:
 *
 * @code
 * sc = elm_gengrid_add(win);
 * elm_gengrid_content_set(sc, content);
 * elm_gengrid_page_relative_set(sc, 1, 0);
 * elm_gengrid_last_page_get(sc, &h_page, &v_page);
 * elm_gengrid_page_bring_in(sc, h_page, v_page);
 * @endcode
 *
 * @see elm_gengrid_page_show()
 */
EAPI void                          elm_gengrid_page_bring_in(const Evas_Object *obj, int h_pagenumber, int v_pagenumber);

/**
 * Set the direction in which a given gengrid widget will expand while
 * placing its items.
 *
 * @param obj The gengrid object.
 * @param setting @c EINA_TRUE to make the gengrid expand
 * horizontally, @c EINA_FALSE to expand vertically.
 *
 * When in "horizontal mode" (@c EINA_TRUE), items will be placed
 * in @b columns, from top to bottom and, when the space for a
 * column is filled, another one is started on the right, thus
 * expanding the grid horizontally. When in "vertical mode"
 * (@c EINA_FALSE), though, items will be placed in @b rows, from left
 * to right and, when the space for a row is filled, another one is
 * started below, thus expanding the grid vertically.
 *
 * @see elm_gengrid_horizontal_get()
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_horizontal_set(Evas_Object *obj, Eina_Bool setting);

/**
 * Get for what direction a given gengrid widget will expand while
 * placing its items.
 *
 * @param obj The gengrid object.
 * @return @c EINA_TRUE, if @p obj is set to expand horizontally,
 * @c EINA_FALSE if it's set to expand vertically.
 *
 * @see elm_gengrid_horizontal_set() for more detais
 *
 * @ingroup Gengrid
 */
EAPI Eina_Bool                     elm_gengrid_horizontal_get(const Evas_Object *obj);

/**
 * Get the first item in a given gengrid widget
 *
 * @param obj The gengrid object
 * @return The first item's handle or @c NULL, if there are no
 * items in @p obj (and on errors)
 *
 * This returns the first item in the @p obj's internal list of
 * items.
 *
 * @see elm_gengrid_last_item_get()
 *
 * @ingroup Gengrid
 */
EAPI Elm_Object_Item             *elm_gengrid_first_item_get(const Evas_Object *obj);

/**
 * Get the last item in a given gengrid widget
 *
 * @param obj The gengrid object
 * @return The last item's handle or @c NULL, if there are no
 * items in @p obj (and on errors)
 *
 * This returns the last item in the @p obj's internal list of
 * items.
 *
 * @see elm_gengrid_first_item_get()
 *
 * @ingroup Gengrid
 */
EAPI Elm_Object_Item             *elm_gengrid_last_item_get(const Evas_Object *obj);

/**
 * Get the @b next item in a gengrid widget's internal list of items,
 * given a handle to one of those items.
 *
 * @param it The gengrid item to fetch next from
 * @return The item after @p item, or @c NULL if there's none (and
 * on errors)
 *
 * This returns the item placed after the @p item, on the container
 * gengrid.
 *
 * @see elm_gengrid_item_prev_get()
 *
 * @ingroup Gengrid
 */
EAPI Elm_Object_Item             *elm_gengrid_item_next_get(const Elm_Object_Item *it);

/**
 * Get the @b previous item in a gengrid widget's internal list of items,
 * given a handle to one of those items.
 *
 * @param it The gengrid item to fetch previous from
 * @return The item before @p item, or @c NULL if there's none (and
 * on errors)
 *
 * This returns the item placed before the @p item, on the container
 * gengrid.
 *
 * @see elm_gengrid_item_next_get()
 *
 * @ingroup Gengrid
 */
EAPI Elm_Object_Item             *elm_gengrid_item_prev_get(const Elm_Object_Item *it);

/**
 * Get the gengrid object's handle which contains a given gengrid item
 *
 * @param it The item to fetch the container from
 * @return The gengrid (parent) object
 *
 * This returns the gengrid object itself that an item belongs to.
 *
 * @ingroup Gengrid
 */
EAPI Evas_Object                  *elm_gengrid_item_gengrid_get(const Elm_Object_Item *it);

/**
 * Remove a gengrid item from its parent, deleting it.
 *
 * @param it The item to be removed.
 * @return @c EINA_TRUE on success or @c EINA_FALSE, otherwise.
 *
 * @see elm_gengrid_clear(), to remove all items in a gengrid at
 * once.
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_item_del(Elm_Object_Item *it);

/**
 * Update the contents of a given gengrid item
 *
 * @param it The gengrid item
 *
 * This updates an item by calling all the item class functions
 * again to get the contents, texts and states. Use this when the
 * original item data has changed and you want the changes to be
 * reflected.
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_item_update(Elm_Object_Item *it);

/**
 * Get the Gengrid Item class for the given Gengrid Item.
 *
 * @param it The gengrid item
 *
 * This returns the Gengrid_Item_Class for the given item. It can be used to examine
 * the function pointers and item_style.
 *
 * @ingroup Gengrid
 */
EAPI const Elm_Gengrid_Item_Class *elm_gengrid_item_item_class_get(const Elm_Object_Item *it);

/**
 * Get the Gengrid Item class for the given Gengrid Item.
 *
 * This sets the Gengrid_Item_Class for the given item. It can be used to examine
 * the function pointers and item_style.
 *
 * @param it The gengrid item
 * @param gic The gengrid item class describing the function pointers and the item style.
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_item_item_class_set(Elm_Object_Item *it, const Elm_Gengrid_Item_Class *gic);

/**
 * Return the data associated to a given gengrid item
 *
 * @param it The gengrid item.
 * @return the data associated with this item.
 *
 * This returns the @c data value passed on the
 * elm_gengrid_item_append() and related item addition calls.
 *
 * @see elm_gengrid_item_append()
 * @see elm_gengrid_item_data_set()
 *
 * @ingroup Gengrid
 */
EAPI void                         *elm_gengrid_item_data_get(const Elm_Object_Item *it);

/**
 * Set the data associated with a given gengrid item
 *
 * @param it The gengrid item
 * @param data The data pointer to set on it
 *
 * This @b overrides the @c data value passed on the
 * elm_gengrid_item_append() and related item addition calls. This
 * function @b won't call elm_gengrid_item_update() automatically,
 * so you'd issue it afterwards if you want to have the item
 * updated to reflect the new data.
 *
 * @see elm_gengrid_item_data_get()
 * @see elm_gengrid_item_update()
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_item_data_set(Elm_Object_Item *it, const void *data);

/**
 * Get a given gengrid item's position, relative to the whole
 * gengrid's grid area.
 *
 * @param it The Gengrid item.
 * @param x Pointer to variable to store the item's <b>row number</b>.
 * @param y Pointer to variable to store the item's <b>column number</b>.
 *
 * This returns the "logical" position of the item within the
 * gengrid. For example, @c (0, 1) would stand for first row,
 * second column.
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_item_pos_get(const Elm_Object_Item *it, unsigned int *x, unsigned int *y);

/**
 * Set whether a given gengrid item is selected or not
 *
 * @param it The gengrid item
 * @param selected Use @c EINA_TRUE, to make it selected, @c
 * EINA_FALSE to make it unselected
 *
 * This sets the selected state of an item. If multi-selection is
 * not enabled on the containing gengrid and @p selected is @c
 * EINA_TRUE, any other previously selected items will get
 * unselected in favor of this new one.
 *
 * @see elm_gengrid_item_selected_get()
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_item_selected_set(Elm_Object_Item *it, Eina_Bool selected);

/**
 * Get whether a given gengrid item is selected or not
 *
 * @param it The gengrid item
 * @return @c EINA_TRUE, if it's selected, @c EINA_FALSE otherwise
 *
 * This API returns EINA_TRUE for all the items selected in multi-select mode as well.
 *
 * @see elm_gengrid_item_selected_set() for more details
 *
 * @ingroup Gengrid
 */
EAPI Eina_Bool                     elm_gengrid_item_selected_get(const Elm_Object_Item *it);

/**
 * Get the real Evas object created to implement the view of a
 * given gengrid item
 *
 * @param it The gengrid item.
 * @return the Evas object implementing this item's view.
 *
 * This returns the actual Evas object used to implement the
 * specified gengrid item's view. This may be @c NULL, as it may
 * not have been created or may have been deleted, at any time, by
 * the gengrid. <b>Do not modify this object</b> (move, resize,
 * show, hide, etc.), as the gengrid is controlling it. This
 * function is for querying, emitting custom signals or hooking
 * lower level callbacks for events on that object. Do not delete
 * this object under any circumstances.
 *
 * @see elm_gengrid_item_data_get()
 *
 * @ingroup Gengrid
 */
EAPI const Evas_Object            *elm_gengrid_item_object_get(const Elm_Object_Item *it);

/**
 * Show the portion of a gengrid's internal grid containing a given
 * item, @b immediately.
 *
 * @param it The item to display
 *
 * This causes gengrid to @b redraw its viewport's contents to the
 * region contining the given @p item item, if it is not fully
 * visible.
 *
 * @see elm_gengrid_item_bring_in()
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_item_show(Elm_Object_Item *it);

/**
 * Animatedly bring in, to the visible area of a gengrid, a given
 * item on it.
 *
 * @param it The gengrid item to display
 *
 * This causes gengrid to jump to the given @p item and show
 * it (by scrolling), if it is not fully visible. This will use
 * animation to do so and take a period of time to complete.
 *
 * @see elm_gengrid_item_show()
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_item_bring_in(Elm_Object_Item *it);

/**
 * Set whether a given gengrid item is disabled or not.
 *
 * @param it The gengrid item
 * @param disabled Use @c EINA_TRUE, true disable it, @c EINA_FALSE
 * to enable it back.
 *
 * A disabled item cannot be selected or unselected. It will also
 * change its appearance, to signal the user it's disabled.
 *
 * @see elm_gengrid_item_disabled_get()
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_item_disabled_set(Elm_Object_Item *it, Eina_Bool disabled);

/**
 * Get whether a given gengrid item is disabled or not.
 *
 * @param it The gengrid item
 * @return @c EINA_TRUE, if it's disabled, @c EINA_FALSE otherwise
 * (and on errors).
 *
 * @see elm_gengrid_item_disabled_set() for more details
 *
 * @ingroup Gengrid
 */
EAPI Eina_Bool                     elm_gengrid_item_disabled_get(const Elm_Object_Item *it);

/**
 * Set the text to be shown in a given gengrid item's tooltips.
 *
 * @param it The gengrid item
 * @param text The text to set in the content
 *
 * This call will setup the text to be used as tooltip to that item
 * (analogous to elm_object_tooltip_text_set(), but being item
 * tooltips with higher precedence than object tooltips). It can
 * have only one tooltip at a time, so any previous tooltip data
 * will get removed.
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_item_tooltip_text_set(Elm_Object_Item *it, const char *text);

/**
 * Set the content to be shown in a given gengrid item's tooltip
 *
 * @param it The gengrid item.
 * @param func The function returning the tooltip contents.
 * @param data What to provide to @a func as callback data/context.
 * @param del_cb Called when data is not needed anymore, either when
 *        another callback replaces @p func, the tooltip is unset with
 *        elm_gengrid_item_tooltip_unset() or the owner @p item
 *        dies. This callback receives as its first parameter the
 *        given @p data, being @c event_info the item handle.
 *
 * This call will setup the tooltip's contents to @p item
 * (analogous to elm_object_tooltip_content_cb_set(), but being
 * item tooltips with higher precedence than object tooltips). It
 * can have only one tooltip at a time, so any previous tooltip
 * content will get removed. @p func (with @p data) will be called
 * every time Elementary needs to show the tooltip and it should
 * return a valid Evas object, which will be fully managed by the
 * tooltip system, getting deleted when the tooltip is gone.
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_item_tooltip_content_cb_set(Elm_Object_Item *it, Elm_Tooltip_Item_Content_Cb func, const void *data, Evas_Smart_Cb del_cb);

/**
 * Unset a tooltip from a given gengrid item
 *
 * @param it gengrid item to remove a previously set tooltip from.
 *
 * This call removes any tooltip set on @p item. The callback
 * provided as @c del_cb to
 * elm_gengrid_item_tooltip_content_cb_set() will be called to
 * notify it is not used anymore (and have resources cleaned, if
 * need be).
 *
 * @see elm_gengrid_item_tooltip_content_cb_set()
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_item_tooltip_unset(Elm_Object_Item *it);

/**
 * Set a different @b style for a given gengrid item's tooltip.
 *
 * @param it gengrid item with tooltip set
 * @param style the <b>theme style</b> to use on tooltips (e.g. @c
 * "default", @c "transparent", etc)
 *
 * Tooltips can have <b>alternate styles</b> to be displayed on,
 * which are defined by the theme set on Elementary. This function
 * works analogously as elm_object_tooltip_style_set(), but here
 * applied only to gengrid item objects. The default style for
 * tooltips is @c "default".
 *
 * @note before you set a style you should define a tooltip with
 *       elm_gengrid_item_tooltip_content_cb_set() or
 *       elm_gengrid_item_tooltip_text_set()
 *
 * @see elm_gengrid_item_tooltip_style_get()
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_item_tooltip_style_set(Elm_Object_Item *it, const char *style);

/**
 * Get the style set a given gengrid item's tooltip.
 *
 * @param it gengrid item with tooltip already set on.
 * @return style the theme style in use, which defaults to
 *         "default". If the object does not have a tooltip set,
 *         then @c NULL is returned.
 *
 * @see elm_gengrid_item_tooltip_style_set() for more details
 *
 * @ingroup Gengrid
 */
EAPI const char                   *elm_gengrid_item_tooltip_style_get(const Elm_Object_Item *it);

/**
 * @brief Disable size restrictions on an object's tooltip
 * @param it The tooltip's anchor object
 * @param disable If EINA_TRUE, size restrictions are disabled
 * @return EINA_FALSE on failure, EINA_TRUE on success
 *
 * This function allows a tooltip to expand beyond its parant window's canvas.
 * It will instead be limited only by the size of the display.
 */
EAPI Eina_Bool                     elm_gengrid_item_tooltip_window_mode_set(Elm_Object_Item *it, Eina_Bool disable);

/**
 * @brief Retrieve size restriction state of an object's tooltip
 * @param it The tooltip's anchor object
 * @return If EINA_TRUE, size restrictions are disabled
 *
 * This function returns whether a tooltip is allowed to expand beyond
 * its parant window's canvas.
 * It will instead be limited only by the size of the display.
 */
EAPI Eina_Bool                     elm_gengrid_item_tooltip_window_mode_get(const Elm_Object_Item *it);

/**
 * Set the type of mouse pointer/cursor decoration to be shown,
 * when the mouse pointer is over the given gengrid widget item
 *
 * @param it gengrid item to customize cursor on
 * @param cursor the cursor type's name
 *
 * This function works analogously as elm_object_cursor_set(), but
 * here the cursor's changing area is restricted to the item's
 * area, and not the whole widget's. Note that that item cursors
 * have precedence over widget cursors, so that a mouse over @p
 * item will always show cursor @p type.
 *
 * If this function is called twice for an object, a previously set
 * cursor will be unset on the second call.
 *
 * @see elm_object_cursor_set()
 * @see elm_gengrid_item_cursor_get()
 * @see elm_gengrid_item_cursor_unset()
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_item_cursor_set(Elm_Object_Item *it, const char *cursor);

/**
 * Get the type of mouse pointer/cursor decoration set to be shown,
 * when the mouse pointer is over the given gengrid widget item
 *
 * @param it gengrid item with custom cursor set
 * @return the cursor type's name or @c NULL, if no custom cursors
 * were set to @p item (and on errors)
 *
 * @see elm_object_cursor_get()
 * @see elm_gengrid_item_cursor_set() for more details
 * @see elm_gengrid_item_cursor_unset()
 *
 * @ingroup Gengrid
 */
EAPI const char                   *elm_gengrid_item_cursor_get(const Elm_Object_Item *it);

/**
 * Unset any custom mouse pointer/cursor decoration set to be
 * shown, when the mouse pointer is over the given gengrid widget
 * item, thus making it show the @b default cursor again.
 *
 * @param it a gengrid item
 *
 * Use this call to undo any custom settings on this item's cursor
 * decoration, bringing it back to defaults (no custom style set).
 *
 * @see elm_object_cursor_unset()
 * @see elm_gengrid_item_cursor_set() for more details
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_item_cursor_unset(Elm_Object_Item *it);

/**
 * Set a different @b style for a given custom cursor set for a
 * gengrid item.
 *
 * @param it gengrid item with custom cursor set
 * @param style the <b>theme style</b> to use (e.g. @c "default",
 * @c "transparent", etc)
 *
 * This function only makes sense when one is using custom mouse
 * cursor decorations <b>defined in a theme file</b> , which can
 * have, given a cursor name/type, <b>alternate styles</b> on
 * it. It works analogously as elm_object_cursor_style_set(), but
 * here applied only to gengrid item objects.
 *
 * @warning Before you set a cursor style you should have defined a
 *       custom cursor previously on the item, with
 *       elm_gengrid_item_cursor_set()
 *
 * @see elm_gengrid_item_cursor_engine_only_set()
 * @see elm_gengrid_item_cursor_style_get()
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_item_cursor_style_set(Elm_Object_Item *it, const char *style);

/**
 * Get the current @b style set for a given gengrid item's custom
 * cursor
 *
 * @param it gengrid item with custom cursor set.
 * @return style the cursor style in use. If the object does not
 *         have a cursor set, then @c NULL is returned.
 *
 * @see elm_gengrid_item_cursor_style_set() for more details
 *
 * @ingroup Gengrid
 */
EAPI const char                   *elm_gengrid_item_cursor_style_get(const Elm_Object_Item *it);

/**
 * Set if the (custom) cursor for a given gengrid item should be
 * searched in its theme, also, or should only rely on the
 * rendering engine.
 *
 * @param it item with custom (custom) cursor already set on
 * @param engine_only Use @c EINA_TRUE to have cursors looked for
 * only on those provided by the rendering engine, @c EINA_FALSE to
 * have them searched on the widget's theme, as well.
 *
 * @note This call is of use only if you've set a custom cursor
 * for gengrid items, with elm_gengrid_item_cursor_set().
 *
 * @note By default, cursors will only be looked for between those
 * provided by the rendering engine.
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_item_cursor_engine_only_set(Elm_Object_Item *it, Eina_Bool engine_only);

/**
 * Get if the (custom) cursor for a given gengrid item is being
 * searched in its theme, also, or is only relying on the rendering
 * engine.
 *
 * @param it a gengrid item
 * @return @c EINA_TRUE, if cursors are being looked for only on
 * those provided by the rendering engine, @c EINA_FALSE if they
 * are being searched on the widget's theme, as well.
 *
 * @see elm_gengrid_item_cursor_engine_only_set(), for more details
 *
 * @ingroup Gengrid
 */
EAPI Eina_Bool                     elm_gengrid_item_cursor_engine_only_get(const Elm_Object_Item *it);

/**
 * Remove all items from a given gengrid widget
 *
 * @param obj The gengrid object.
 *
 * This removes (and deletes) all items in @p obj, leaving it
 * empty.
 *
 * @see elm_gengrid_item_del(), to remove just one item.
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_clear(Evas_Object *obj);

/**
 * Get the selected item in a given gengrid widget
 *
 * @param obj The gengrid object.
 * @return The selected item's handleor @c NULL, if none is
 * selected at the moment (and on errors)
 *
 * This returns the selected item in @p obj. If multi selection is
 * enabled on @p obj (@see elm_gengrid_multi_select_set()), only
 * the first item in the list is selected, which might not be very
 * useful. For that case, see elm_gengrid_selected_items_get().
 *
 * @ingroup Gengrid
 */
EAPI Elm_Object_Item             *elm_gengrid_selected_item_get(const Evas_Object *obj);

/**
 * Get <b>a list</b> of selected items in a given gengrid
 *
 * @param obj The gengrid object.
 * @return The list of selected items or @c NULL, if none is
 * selected at the moment (and on errors)
 *
 * This returns a list of the selected items, in the order that
 * they appear in the grid. This list is only valid as long as no
 * more items are selected or unselected (or unselected implictly
 * by deletion). The list contains #Gengrid item pointers as
 * data, naturally.
 *
 * @see elm_gengrid_selected_item_get()
 *
 * @ingroup Gengrid
 */
EAPI const Eina_List              *elm_gengrid_selected_items_get(const Evas_Object *obj);

/**
 * Set how the items grid's filled within a given gengrid widget
 *
 * @param obj The gengrid object.
 * @param fill Filled if True
 *
 * This sets the fill state of the whole grid of items of a gengrid
 * within its given viewport. By default, this value is False, meaning
 * that if the first line of items grid's aren't filled
 *
 * Set how the items grid's filled within a given gengrid widget.
 * This sets the fill state of the whole grid of items of a gengrid
 * within its given viewport. By default, this value is false, meaning
 * that if the first line of items grid's isn't filled, the items are
 * centered with the alignment
 *
 * @see elm_gengrid_filled_get()
 *
 * @ingroup Gengrid
 */
EAPI void                          elm_gengrid_filled_set(Evas_Object *obj, Eina_Bool fill);


/**
 * Get how the items grid's filled within a given gengrid widget
 *
 * @param obj The gengrid object.
 * @return @c EINA_TRUE, if filled is on, @c EINA_FALSE if it's
 * off
 *
 * @note Use @c NULL pointers on the alignment values you're not
 * interested in: they'll be ignored by the function.
 *
 * @see elm_gengrid_align_set() for more details
 *
 * @ingroup Gengrid
 */
EAPI Eina_Bool                     elm_gengrid_filled_get(const Evas_Object *obj);

/**
 * @}
 */
