/**
 * @defgroup File_Selector_Button File Selector Button
 *
 * @image html img/widget/fileselector_button/preview-00.png
 * @image latex img/widget/fileselector_button/preview-00.eps
 * @image html img/widget/fileselector_button/preview-01.png
 * @image latex img/widget/fileselector_button/preview-01.eps
 * @image html img/widget/fileselector_button/preview-02.png
 * @image latex img/widget/fileselector_button/preview-02.eps
 *
 * This is a button that, when clicked, creates an Elementary
 * window (or inner window) <b> with a @ref Fileselector "file
 * selector widget" within</b>. When a file is chosen, the (inner)
 * window is closed and the button emits a signal having the
 * selected file as it's @c event_info.
 *
 * This widget encapsulates operations on its internal file
 * selector on its own API. There is less control over its file
 * selector than that one would have instatiating one directly.
 *
 * The following styles are available for this button:
 * @li @c "default"
 * @li @c "anchor"
 * @li @c "hoversel_vertical"
 * @li @c "hoversel_vertical_entry"
 *
 * Smart callbacks one can register to:
 * - @c "file,chosen" - the user has selected a path, whose string
 *   pointer comes as the @c event_info data (a stringshared
 *   string)
 *
 * Here is an example on its usage:
 * @li @ref fileselector_button_example
 *
 * @see @ref File_Selector_Entry for a similar widget.
 * @{
 */

/**
 * Add a new file selector button widget to the given parent
 * Elementary (container) object
 *
 * @param parent The parent object
 * @return a new file selector button widget handle or @c NULL, on
 * errors
 */
EAPI Evas_Object                *elm_fileselector_button_add(Evas_Object *parent) EINA_ARG_NONNULL(1);

/**
 * Set the icon on a given file selector button widget
 *
 * @param obj The file selector button widget
 * @param icon The icon object for the button
 *
 * Once the icon object is set, a previously set one will be
 * deleted. If you want to keep the latter, use the
 * elm_fileselector_button_icon_unset() function.
 *
 * @see elm_fileselector_button_icon_get()
 */
EAPI void                        elm_fileselector_button_icon_set(Evas_Object *obj, Evas_Object *icon) EINA_ARG_NONNULL(1);

/**
 * Get the icon set for a given file selector button widget
 *
 * @param obj The file selector button widget
 * @return The icon object currently set on @p obj or @c NULL, if
 * none is
 *
 * @see elm_fileselector_button_icon_set()
 */
EAPI Evas_Object                *elm_fileselector_button_icon_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Unset the icon used in a given file selector button widget
 *
 * @param obj The file selector button widget
 * @return The icon object that was being used on @p obj or @c
 * NULL, on errors
 *
 * Unparent and return the icon object which was set for this
 * widget.
 *
 * @see elm_fileselector_button_icon_set()
 */
EAPI Evas_Object                *elm_fileselector_button_icon_unset(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Set the title for a given file selector button widget's window
 *
 * @param obj The file selector button widget
 * @param title The title string
 *
 * This will change the window's title, when the file selector pops
 * out after a click on the button. Those windows have the default
 * (unlocalized) value of @c "Select a file" as titles.
 *
 * @note It will only take any effect if the file selector
 * button widget is @b not under "inwin mode".
 *
 * @see elm_fileselector_button_window_title_get()
 */
EAPI void                        elm_fileselector_button_window_title_set(Evas_Object *obj, const char *title) EINA_ARG_NONNULL(1);

/**
 * Get the title set for a given file selector button widget's
 * window
 *
 * @param obj The file selector button widget
 * @return Title of the file selector button's window
 *
 * @see elm_fileselector_button_window_title_get() for more details
 */
EAPI const char                 *elm_fileselector_button_window_title_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Set the size of a given file selector button widget's window,
 * holding the file selector itself.
 *
 * @param obj The file selector button widget
 * @param width The window's width
 * @param height The window's height
 *
 * @note it will only take any effect if the file selector button
 * widget is @b not under "inwin mode". The default size for the
 * window (when applicable) is 400x400 pixels.
 *
 * @see elm_fileselector_button_window_size_get()
 */
EAPI void                        elm_fileselector_button_window_size_set(Evas_Object *obj, Evas_Coord width, Evas_Coord height) EINA_ARG_NONNULL(1);

/**
 * Get the size of a given file selector button widget's window,
 * holding the file selector itself.
 *
 * @param obj The file selector button widget
 * @param width Pointer into which to store the width value
 * @param height Pointer into which to store the height value
 *
 * @note Use @c NULL pointers on the size values you're not
 * interested in: they'll be ignored by the function.
 *
 * @see elm_fileselector_button_window_size_set(), for more details
 */
EAPI void                        elm_fileselector_button_window_size_get(const Evas_Object *obj, Evas_Coord *width, Evas_Coord *height) EINA_ARG_NONNULL(1);

/**
 * Set the initial file system path for a given file selector
 * button widget
 *
 * @param obj The file selector button widget
 * @param path The path string
 *
 * It must be a <b>directory</b> path, which will have the contents
 * displayed initially in the file selector's view, when invoked
 * from @p obj. The default initial path is the @c "HOME"
 * environment variable's value.
 *
 * @see elm_fileselector_button_path_get()
 */
EAPI void                        elm_fileselector_button_path_set(Evas_Object *obj, const char *path) EINA_ARG_NONNULL(1);

/**
 * Get the initial file system path set for a given file selector
 * button widget
 *
 * @param obj The file selector button widget
 * @return path The path string
 *
 * @see elm_fileselector_button_path_set() for more details
 */
EAPI const char                 *elm_fileselector_button_path_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Enable/disable a tree view in the given file selector button
 * widget's internal file selector
 *
 * @param obj The file selector button widget
 * @param value @c EINA_TRUE to enable tree view, @c EINA_FALSE to
 * disable
 *
 * This has the same effect as elm_fileselector_expandable_set(),
 * but now applied to a file selector button's internal file
 * selector.
 *
 * @note There's no way to put a file selector button's internal
 * file selector in "grid mode", as one may do with "pure" file
 * selectors.
 *
 * @see elm_fileselector_expandable_get()
 */
EAPI void                        elm_fileselector_button_expandable_set(Evas_Object *obj, Eina_Bool value) EINA_ARG_NONNULL(1);

/**
 * Get whether tree view is enabled for the given file selector
 * button widget's internal file selector
 *
 * @param obj The file selector button widget
 * @return @c EINA_TRUE if @p obj widget's internal file selector
 * is in tree view, @c EINA_FALSE otherwise (and or errors)
 *
 * @see elm_fileselector_expandable_set() for more details
 */
EAPI Eina_Bool                   elm_fileselector_button_expandable_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Set whether a given file selector button widget's internal file
 * selector is to display folders only or the directory contents,
 * as well.
 *
 * @param obj The file selector button widget
 * @param value @c EINA_TRUE to make @p obj widget's internal file
 * selector only display directories, @c EINA_FALSE to make files
 * to be displayed in it too
 *
 * This has the same effect as elm_fileselector_folder_only_set(),
 * but now applied to a file selector button's internal file
 * selector.
 *
 * @see elm_fileselector_folder_only_get()
 */
EAPI void                        elm_fileselector_button_folder_only_set(Evas_Object *obj, Eina_Bool value) EINA_ARG_NONNULL(1);

/**
 * Get whether a given file selector button widget's internal file
 * selector is displaying folders only or the directory contents,
 * as well.
 *
 * @param obj The file selector button widget
 * @return @c EINA_TRUE if @p obj widget's internal file
 * selector is only displaying directories, @c EINA_FALSE if files
 * are being displayed in it too (and on errors)
 *
 * @see elm_fileselector_button_folder_only_set() for more details
 */
EAPI Eina_Bool                   elm_fileselector_button_folder_only_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Enable/disable the file name entry box where the user can type
 * in a name for a file, in a given file selector button widget's
 * internal file selector.
 *
 * @param obj The file selector button widget
 * @param value @c EINA_TRUE to make @p obj widget's internal
 * file selector a "saving dialog", @c EINA_FALSE otherwise
 *
 * This has the same effect as elm_fileselector_is_save_set(),
 * but now applied to a file selector button's internal file
 * selector.
 *
 * @see elm_fileselector_is_save_get()
 */
EAPI void                        elm_fileselector_button_is_save_set(Evas_Object *obj, Eina_Bool value) EINA_ARG_NONNULL(1);

/**
 * Get whether the given file selector button widget's internal
 * file selector is in "saving dialog" mode
 *
 * @param obj The file selector button widget
 * @return @c EINA_TRUE, if @p obj widget's internal file selector
 * is in "saving dialog" mode, @c EINA_FALSE otherwise (and on
 * errors)
 *
 * @see elm_fileselector_button_is_save_set() for more details
 */
EAPI Eina_Bool                   elm_fileselector_button_is_save_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Set whether a given file selector button widget's internal file
 * selector will raise an Elementary "inner window", instead of a
 * dedicated Elementary window. By default, it won't.
 *
 * @param obj The file selector button widget
 * @param value @c EINA_TRUE to make it use an inner window, @c
 * EINA_TRUE to make it use a dedicated window
 *
 * @see elm_win_inwin_add() for more information on inner windows
 * @see elm_fileselector_button_inwin_mode_get()
 */
EAPI void                        elm_fileselector_button_inwin_mode_set(Evas_Object *obj, Eina_Bool value) EINA_ARG_NONNULL(1);

/**
 * Get whether a given file selector button widget's internal file
 * selector will raise an Elementary "inner window", instead of a
 * dedicated Elementary window.
 *
 * @param obj The file selector button widget
 * @return @c EINA_TRUE if will use an inner window, @c EINA_TRUE
 * if it will use a dedicated window
 *
 * @see elm_fileselector_button_inwin_mode_set() for more details
 */
EAPI Eina_Bool                   elm_fileselector_button_inwin_mode_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * @}
 */
