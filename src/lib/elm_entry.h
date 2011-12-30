typedef enum
{
   ELM_TEXT_FORMAT_PLAIN_UTF8,
   ELM_TEXT_FORMAT_MARKUP_UTF8
} Elm_Text_Format;

/**
 * Line wrapping types.
 */
typedef enum
{
   ELM_WRAP_NONE = 0, /**< No wrap - value is zero */
   ELM_WRAP_CHAR, /**< Char wrap - wrap between characters */
   ELM_WRAP_WORD, /**< Word wrap - wrap in allowed wrapping points (as defined in the unicode standard) */
   ELM_WRAP_MIXED, /**< Mixed wrap - Word wrap, and if that fails, char wrap. */
   ELM_WRAP_LAST
} Elm_Wrap_Type; /**< Type of word or character wrapping to use */

typedef enum
{
   ELM_INPUT_PANEL_LAYOUT_NORMAL, /**< Default layout */
   ELM_INPUT_PANEL_LAYOUT_NUMBER, /**< Number layout */
   ELM_INPUT_PANEL_LAYOUT_EMAIL, /**< Email layout */
   ELM_INPUT_PANEL_LAYOUT_URL, /**< URL layout */
   ELM_INPUT_PANEL_LAYOUT_PHONENUMBER, /**< Phone Number layout */
   ELM_INPUT_PANEL_LAYOUT_IP, /**< IP layout */
   ELM_INPUT_PANEL_LAYOUT_MONTH, /**< Month layout */
   ELM_INPUT_PANEL_LAYOUT_NUMBERONLY, /**< Number Only layout */
   ELM_INPUT_PANEL_LAYOUT_INVALID // XXX: remove this so we can expand
} Elm_Input_Panel_Layout; /**< Type of input panel (virtual keyboard) to use */

typedef enum
{
   ELM_AUTOCAPITAL_TYPE_NONE, /**< No auto-capitalization when typing */
   ELM_AUTOCAPITAL_TYPE_WORD, /**< Autocapitalize each word typed */
   ELM_AUTOCAPITAL_TYPE_SENTENCE, /**< Autocapitalize the start of each sentence */
   ELM_AUTOCAPITAL_TYPE_ALLCHARACTER, /**< Autocapitalize all letters */
} Elm_Autocapital_Type; /**< Choose method of auto-capitalization */

/**
 * @defgroup Entry Entry
 *
 * @image html img/widget/entry/preview-00.png
 * @image latex img/widget/entry/preview-00.eps width=\textwidth
 * @image html img/widget/entry/preview-01.png
 * @image latex img/widget/entry/preview-01.eps width=\textwidth
 * @image html img/widget/entry/preview-02.png
 * @image latex img/widget/entry/preview-02.eps width=\textwidth
 * @image html img/widget/entry/preview-03.png
 * @image latex img/widget/entry/preview-03.eps width=\textwidth
 *
 * An entry is a convenience widget which shows a box that the user can
 * enter text into. Entries by default don't scroll, so they grow to
 * accomodate the entire text, resizing the parent window as needed. This
 * can be changed with the elm_entry_scrollable_set() function.
 *
 * They can also be single line or multi line (the default) and when set
 * to multi line mode they support text wrapping in any of the modes
 * indicated by Elm_Wrap_Type.
 *
 * Other features include password mode, filtering of inserted text with
 * elm_entry_text_filter_append() and related functions, inline "items" and
 * formatted markup text.
 *
 * @section entry-markup Formatted text
 *
 * The markup tags supported by the Entry are defined by the theme, but
 * even when writing new themes or extensions it's a good idea to stick to
 * a sane default, to maintain coherency and avoid application breakages.
 * Currently defined by the default theme are the following tags:
 * @li \<br\>: Inserts a line break.
 * @li \<ps\>: Inserts a paragraph separator. This is preferred over line
 * breaks.
 * @li \<tab\>: Inserts a tab.
 * @li \<em\>...\</em\>: Emphasis. Sets the @em oblique style for the
 * enclosed text.
 * @li \<b\>...\</b\>: Sets the @b bold style for the enclosed text.
 * @li \<link\>...\</link\>: Underlines the enclosed text.
 * @li \<hilight\>...\</hilight\>: Hilights the enclosed text.
 *
 * @section entry-special Special markups
 *
 * Besides those used to format text, entries support two special markup
 * tags used to insert clickable portions of text or items inlined within
 * the text.
 *
 * @subsection entry-anchors Anchors
 *
 * Anchors are similar to HTML anchors. Text can be surrounded by \<a\> and
 * \</a\> tags and an event will be generated when this text is clicked,
 * like this:
 *
 * @code
 * This text is outside <a href=anc-01>but this one is an anchor</a>
 * @endcode
 *
 * The @c href attribute in the opening tag gives the name that will be
 * used to identify the anchor and it can be any valid utf8 string.
 *
 * When an anchor is clicked, an @c "anchor,clicked" signal is emitted with
 * an #Elm_Entry_Anchor_Info in the @c event_info parameter for the
 * callback function. The same applies for "anchor,in" (mouse in), "anchor,out"
 * (mouse out), "anchor,down" (mouse down), and "anchor,up" (mouse up) events on
 * an anchor.
 *
 * @subsection entry-items Items
 *
 * Inlined in the text, any other @c Evas_Object can be inserted by using
 * \<item\> tags this way:
 *
 * @code
 * <item size=16x16 vsize=full href=emoticon/haha></item>
 * @endcode
 *
 * Just like with anchors, the @c href identifies each item, but these need,
 * in addition, to indicate their size, which is done using any one of
 * @c size, @c absize or @c relsize attributes. These attributes take their
 * value in the WxH format, where W is the width and H the height of the
 * item.
 *
 * @li absize: Absolute pixel size for the item. Whatever value is set will
 * be the item's size regardless of any scale value the object may have
 * been set to. The final line height will be adjusted to fit larger items.
 * @li size: Similar to @c absize, but it's adjusted to the scale value set
 * for the object.
 * @li relsize: Size is adjusted for the item to fit within the current
 * line height.
 *
 * Besides their size, items are specificed a @c vsize value that affects
 * how their final size and position are calculated. The possible values
 * are:
 * @li ascent: Item will be placed within the line's baseline and its
 * ascent. That is, the height between the line where all characters are
 * positioned and the highest point in the line. For @c size and @c absize
 * items, the descent value will be added to the total line height to make
 * them fit. @c relsize items will be adjusted to fit within this space.
 * @li full: Items will be placed between the descent and ascent, or the
 * lowest point in the line and its highest.
 *
 * The next image shows different configurations of items and how
 * the previously mentioned options affect their sizes. In all cases,
 * the green line indicates the ascent, blue for the baseline and red for
 * the descent.
 *
 * @image html entry_item.png
 * @image latex entry_item.eps width=\textwidth
 *
 * And another one to show how size differs from absize. In the first one,
 * the scale value is set to 1.0, while the second one is using one of 2.0.
 *
 * @image html entry_item_scale.png
 * @image latex entry_item_scale.eps width=\textwidth
 *
 * After the size for an item is calculated, the entry will request an
 * object to place in its space. For this, the functions set with
 * elm_entry_item_provider_append() and related functions will be called
 * in order until one of them returns a @c non-NULL value. If no providers
 * are available, or all of them return @c NULL, then the entry falls back
 * to one of the internal defaults, provided the name matches with one of
 * them.
 *
 * All of the following are currently supported:
 *
 * - emoticon/angry
 * - emoticon/angry-shout
 * - emoticon/crazy-laugh
 * - emoticon/evil-laugh
 * - emoticon/evil
 * - emoticon/goggle-smile
 * - emoticon/grumpy
 * - emoticon/grumpy-smile
 * - emoticon/guilty
 * - emoticon/guilty-smile
 * - emoticon/haha
 * - emoticon/half-smile
 * - emoticon/happy-panting
 * - emoticon/happy
 * - emoticon/indifferent
 * - emoticon/kiss
 * - emoticon/knowing-grin
 * - emoticon/laugh
 * - emoticon/little-bit-sorry
 * - emoticon/love-lots
 * - emoticon/love
 * - emoticon/minimal-smile
 * - emoticon/not-happy
 * - emoticon/not-impressed
 * - emoticon/omg
 * - emoticon/opensmile
 * - emoticon/smile
 * - emoticon/sorry
 * - emoticon/squint-laugh
 * - emoticon/surprised
 * - emoticon/suspicious
 * - emoticon/tongue-dangling
 * - emoticon/tongue-poke
 * - emoticon/uh
 * - emoticon/unhappy
 * - emoticon/very-sorry
 * - emoticon/what
 * - emoticon/wink
 * - emoticon/worried
 * - emoticon/wtf
 *
 * Alternatively, an item may reference an image by its path, using
 * the URI form @c file:///path/to/an/image.png and the entry will then
 * use that image for the item.
 *
 * @section entry-files Loading and saving files
 *
 * Entries have convinience functions to load text from a file and save
 * changes back to it after a short delay. The automatic saving is enabled
 * by default, but can be disabled with elm_entry_autosave_set() and files
 * can be loaded directly as plain text or have any markup in them
 * recognized. See elm_entry_file_set() for more details.
 *
 * @section entry-signals Emitted signals
 *
 * This widget emits the following signals:
 *
 * @li "changed": The text within the entry was changed.
 * @li "changed,user": The text within the entry was changed because of user interaction.
 * @li "activated": The enter key was pressed on a single line entry.
 * @li "press": A mouse button has been pressed on the entry.
 * @li "longpressed": A mouse button has been pressed and held for a couple
 * seconds.
 * @li "clicked": The entry has been clicked (mouse press and release).
 * @li "clicked,double": The entry has been double clicked.
 * @li "clicked,triple": The entry has been triple clicked.
 * @li "focused": The entry has received focus.
 * @li "unfocused": The entry has lost focus.
 * @li "selection,paste": A paste of the clipboard contents was requested.
 * @li "selection,copy": A copy of the selected text into the clipboard was
 * requested.
 * @li "selection,cut": A cut of the selected text into the clipboard was
 * requested.
 * @li "selection,start": A selection has begun and no previous selection
 * existed.
 * @li "selection,changed": The current selection has changed.
 * @li "selection,cleared": The current selection has been cleared.
 * @li "cursor,changed": The cursor has changed position.
 * @li "anchor,clicked": An anchor has been clicked. The event_info
 * parameter for the callback will be an #Elm_Entry_Anchor_Info.
 * @li "anchor,in": Mouse cursor has moved into an anchor. The event_info
 * parameter for the callback will be an #Elm_Entry_Anchor_Info.
 * @li "anchor,out": Mouse cursor has moved out of an anchor. The event_info
 * parameter for the callback will be an #Elm_Entry_Anchor_Info.
 * @li "anchor,up": Mouse button has been unpressed on an anchor. The event_info
 * parameter for the callback will be an #Elm_Entry_Anchor_Info.
 * @li "anchor,down": Mouse button has been pressed on an anchor. The event_info
 * parameter for the callback will be an #Elm_Entry_Anchor_Info.
 * @li "preedit,changed": The preedit string has changed.
 * @li "language,changed": Program language changed.
 *
 * @section entry-examples
 *
 * An overview of the Entry API can be seen in @ref entry_example_01
 *
 * @{
 */

/**
 * @typedef Elm_Entry_Anchor_Info
 *
 * The info sent in the callback for the "anchor,clicked" signals emitted
 * by entries.
 */
typedef struct _Elm_Entry_Anchor_Info Elm_Entry_Anchor_Info;

/**
 * @struct _Elm_Entry_Anchor_Info
 *
 * The info sent in the callback for the "anchor,clicked" signals emitted
 * by entries.
 */
struct _Elm_Entry_Anchor_Info
{
   const char *name; /**< The name of the anchor, as stated in its href */
   int         button; /**< The mouse button used to click on it */
   Evas_Coord  x, /**< Anchor geometry, relative to canvas */
               y, /**< Anchor geometry, relative to canvas */
               w, /**< Anchor geometry, relative to canvas */
               h; /**< Anchor geometry, relative to canvas */
};

/**
 * @typedef Elm_Entry_Filter_Cb
 * This callback type is used by entry filters to modify text.
 * @param data The data specified as the last param when adding the filter
 * @param entry The entry object
 * @param text A pointer to the location of the text being filtered. This data can be modified,
 * but any additional allocations must be managed by the user.
 * @see elm_entry_text_filter_append
 * @see elm_entry_text_filter_prepend
 */
typedef void (*Elm_Entry_Filter_Cb)(void *data, Evas_Object *entry, char **text);

/**
 * @typedef Elm_Entry_Change_Info
 * This corresponds to Edje_Entry_Change_Info. Includes information about
 * a change in the entry.
 */
typedef Edje_Entry_Change_Info Elm_Entry_Change_Info;

/**
 * This adds an entry to @p parent object.
 *
 * By default, entries are:
 * @li not scrolled
 * @li multi-line
 * @li word wrapped
 * @li autosave is enabled
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 */
EAPI Evas_Object       *elm_entry_add(Evas_Object *parent) EINA_ARG_NONNULL(1);

/**
 * Sets the entry to single line mode.
 *
 * In single line mode, entries don't ever wrap when the text reaches the
 * edge, and instead they keep growing horizontally. Pressing the @c Enter
 * key will generate an @c "activate" event instead of adding a new line.
 *
 * When @p single_line is @c EINA_FALSE, line wrapping takes effect again
 * and pressing enter will break the text into a different line
 * without generating any events.
 *
 * @param obj The entry object
 * @param single_line If true, the text in the entry
 * will be on a single line.
 */
EAPI void               elm_entry_single_line_set(Evas_Object *obj, Eina_Bool single_line) EINA_ARG_NONNULL(1);

/**
 * Gets whether the entry is set to be single line.
 *
 * @param obj The entry object
 * @return single_line If true, the text in the entry is set to display
 * on a single line.
 *
 * @see elm_entry_single_line_set()
 */
EAPI Eina_Bool          elm_entry_single_line_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Sets the entry to password mode.
 *
 * In password mode, entries are implicitly single line and the display of
 * any text in them is replaced with asterisks (*).
 *
 * @param obj The entry object
 * @param password If true, password mode is enabled.
 */
EAPI void               elm_entry_password_set(Evas_Object *obj, Eina_Bool password) EINA_ARG_NONNULL(1);

/**
 * Gets whether the entry is set to password mode.
 *
 * @param obj The entry object
 * @return If true, the entry is set to display all characters
 * as asterisks (*).
 *
 * @see elm_entry_password_set()
 */
EAPI Eina_Bool          elm_entry_password_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Appends @p entry to the text of the entry.
 *
 * Adds the text in @p entry to the end of any text already present in the
 * widget.
 *
 * The appended text is subject to any filters set for the widget.
 *
 * @param obj The entry object
 * @param entry The text to be displayed
 *
 * @see elm_entry_text_filter_append()
 */
EAPI void               elm_entry_entry_append(Evas_Object *obj, const char *entry) EINA_ARG_NONNULL(1);

/**
 * Gets whether the entry is empty.
 *
 * Empty means no text at all. If there are any markup tags, like an item
 * tag for which no provider finds anything, and no text is displayed, this
 * function still returns EINA_FALSE.
 *
 * @param obj The entry object
 * @return EINA_TRUE if the entry is empty, EINA_FALSE otherwise.
 */
EAPI Eina_Bool          elm_entry_is_empty(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Gets any selected text within the entry.
 *
 * If there's any selected text in the entry, this function returns it as
 * a string in markup format. NULL is returned if no selection exists or
 * if an error occurred.
 *
 * The returned value points to an internal string and should not be freed
 * or modified in any way. If the @p entry object is deleted or its
 * contents are changed, the returned pointer should be considered invalid.
 *
 * @param obj The entry object
 * @return The selected text within the entry or NULL on failure
 */
EAPI const char        *elm_entry_selection_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Returns the actual textblock object of the entry.
 *
 * This function exposes the internal textblock object that actually
 * contains and draws the text. This should be used for low-level
 * manipulations that are otherwise not possible.
 *
 * Changing the textblock directly from here will not notify edje/elm to
 * recalculate the textblock size automatically, so any modifications
 * done to the textblock returned by this function should be followed by
 * a call to elm_entry_calc_force().
 *
 * The return value is marked as const as an additional warning.
 * One should not use the returned object with any of the generic evas
 * functions (geometry_get/resize/move and etc), but only with the textblock
 * functions; The former will either not work at all, or break the correct
 * functionality.
 *
 * IMPORTANT: Many functions may change (i.e delete and create a new one)
 * the internal textblock object. Do NOT cache the returned object, and try
 * not to mix calls on this object with regular elm_entry calls (which may
 * change the internal textblock object). This applies to all cursors
 * returned from textblock calls, and all the other derivative values.
 *
 * @param obj The entry object
 * @return The textblock object.
 */
EAPI const Evas_Object *elm_entry_textblock_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Forces calculation of the entry size and text layouting.
 *
 * This should be used after modifying the textblock object directly. See
 * elm_entry_textblock_get() for more information.
 *
 * @param obj The entry object
 *
 * @see elm_entry_textblock_get()
 */
EAPI void               elm_entry_calc_force(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Inserts the given text into the entry at the current cursor position.
 *
 * This inserts text at the cursor position as if it was typed
 * by the user (note that this also allows markup which a user
 * can't just "type" as it would be converted to escaped text, so this
 * call can be used to insert things like emoticon items or bold push/pop
 * tags, other font and color change tags etc.)
 *
 * If any selection exists, it will be replaced by the inserted text.
 *
 * The inserted text is subject to any filters set for the widget.
 *
 * @param obj The entry object
 * @param entry The text to insert
 *
 * @see elm_entry_text_filter_append()
 */
EAPI void               elm_entry_entry_insert(Evas_Object *obj, const char *entry) EINA_ARG_NONNULL(1);

/**
 * Set the line wrap type to use on multi-line entries.
 *
 * Sets the wrap type used by the entry to any of the specified in
 * Elm_Wrap_Type. This tells how the text will be implicitly cut into a new
 * line (without inserting a line break or paragraph separator) when it
 * reaches the far edge of the widget.
 *
 * Note that this only makes sense for multi-line entries. A widget set
 * to be single line will never wrap.
 *
 * @param obj The entry object
 * @param wrap The wrap mode to use. See Elm_Wrap_Type for details on them
 */
EAPI void               elm_entry_line_wrap_set(Evas_Object *obj, Elm_Wrap_Type wrap) EINA_ARG_NONNULL(1);

/**
 * Gets the wrap mode the entry was set to use.
 *
 * @param obj The entry object
 * @return Wrap type
 *
 * @see also elm_entry_line_wrap_set()
 */
EAPI Elm_Wrap_Type      elm_entry_line_wrap_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Sets if the entry is to be editable or not.
 *
 * By default, entries are editable and when focused, any text input by the
 * user will be inserted at the current cursor position. But calling this
 * function with @p editable as EINA_FALSE will prevent the user from
 * inputting text into the entry.
 *
 * The only way to change the text of a non-editable entry is to use
 * elm_object_text_set(), elm_entry_entry_insert() and other related
 * functions.
 *
 * @param obj The entry object
 * @param editable If EINA_TRUE, user input will be inserted in the entry,
 * if not, the entry is read-only and no user input is allowed.
 */
EAPI void               elm_entry_editable_set(Evas_Object *obj, Eina_Bool editable) EINA_ARG_NONNULL(1);

/**
 * Gets whether the entry is editable or not.
 *
 * @param obj The entry object
 * @return If true, the entry is editable by the user.
 * If false, it is not editable by the user
 *
 * @see elm_entry_editable_set()
 */
EAPI Eina_Bool          elm_entry_editable_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * This drops any existing text selection within the entry.
 *
 * @param obj The entry object
 */
EAPI void               elm_entry_select_none(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * This selects all text within the entry.
 *
 * @param obj The entry object
 */
EAPI void               elm_entry_select_all(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * This moves the cursor one place to the right within the entry.
 *
 * @param obj The entry object
 * @return EINA_TRUE upon success, EINA_FALSE upon failure
 */
EAPI Eina_Bool          elm_entry_cursor_next(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * This moves the cursor one place to the left within the entry.
 *
 * @param obj The entry object
 * @return EINA_TRUE upon success, EINA_FALSE upon failure
 */
EAPI Eina_Bool          elm_entry_cursor_prev(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * This moves the cursor one line up within the entry.
 *
 * @param obj The entry object
 * @return EINA_TRUE upon success, EINA_FALSE upon failure
 */
EAPI Eina_Bool          elm_entry_cursor_up(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * This moves the cursor one line down within the entry.
 *
 * @param obj The entry object
 * @return EINA_TRUE upon success, EINA_FALSE upon failure
 */
EAPI Eina_Bool          elm_entry_cursor_down(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * This moves the cursor to the beginning of the entry.
 *
 * @param obj The entry object
 */
EAPI void               elm_entry_cursor_begin_set(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * This moves the cursor to the end of the entry.
 *
 * @param obj The entry object
 */
EAPI void               elm_entry_cursor_end_set(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * This moves the cursor to the beginning of the current line.
 *
 * @param obj The entry object
 */
EAPI void               elm_entry_cursor_line_begin_set(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * This moves the cursor to the end of the current line.
 *
 * @param obj The entry object
 */
EAPI void               elm_entry_cursor_line_end_set(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * This begins a selection within the entry as though
 * the user were holding down the mouse button to make a selection.
 *
 * @param obj The entry object
 */
EAPI void               elm_entry_cursor_selection_begin(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * This ends a selection within the entry as though
 * the user had just released the mouse button while making a selection.
 *
 * @param obj The entry object
 */
EAPI void               elm_entry_cursor_selection_end(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Gets whether a format node exists at the current cursor position.
 *
 * A format node is anything that defines how the text is rendered. It can
 * be a visible format node, such as a line break or a paragraph separator,
 * or an invisible one, such as bold begin or end tag.
 * This function returns whether any format node exists at the current
 * cursor position.
 *
 * @param obj The entry object
 * @return EINA_TRUE if the current cursor position contains a format node,
 * EINA_FALSE otherwise.
 *
 * @see elm_entry_cursor_is_visible_format_get()
 */
EAPI Eina_Bool          elm_entry_cursor_is_format_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Gets if the current cursor position holds a visible format node.
 *
 * @param obj The entry object
 * @return EINA_TRUE if the current cursor is a visible format, EINA_FALSE
 * if it's an invisible one or no format exists.
 *
 * @see elm_entry_cursor_is_format_get()
 */
EAPI Eina_Bool          elm_entry_cursor_is_visible_format_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Gets the character pointed by the cursor at its current position.
 *
 * This function returns a string with the utf8 character stored at the
 * current cursor position.
 * Only the text is returned, any format that may exist will not be part
 * of the return value.
 *
 * @param obj The entry object
 * @return The text pointed by the cursors.
 */
EAPI const char        *elm_entry_cursor_content_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * This function returns the geometry of the cursor.
 *
 * It's useful if you want to draw something on the cursor (or where it is),
 * or for example in the case of scrolled entry where you want to show the
 * cursor.
 *
 * @param obj The entry object
 * @param x returned geometry
 * @param y returned geometry
 * @param w returned geometry
 * @param h returned geometry
 * @return EINA_TRUE upon success, EINA_FALSE upon failure
 */
EAPI Eina_Bool          elm_entry_cursor_geometry_get(const Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h) EINA_ARG_NONNULL(1);

/**
 * Sets the cursor position in the entry to the given value
 *
 * The value in @p pos is the index of the character position within the
 * contents of the string as returned by elm_entry_cursor_pos_get().
 *
 * @param obj The entry object
 * @param pos The position of the cursor
 */
EAPI void               elm_entry_cursor_pos_set(Evas_Object *obj, int pos) EINA_ARG_NONNULL(1);

/**
 * Retrieves the current position of the cursor in the entry
 *
 * @param obj The entry object
 * @return The cursor position
 */
EAPI int                elm_entry_cursor_pos_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * This executes a "cut" action on the selected text in the entry.
 *
 * @param obj The entry object
 */
EAPI void               elm_entry_selection_cut(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * This executes a "copy" action on the selected text in the entry.
 *
 * @param obj The entry object
 */
EAPI void               elm_entry_selection_copy(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * This executes a "paste" action in the entry.
 *
 * @param obj The entry object
 */
EAPI void               elm_entry_selection_paste(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * This clears and frees the items in a entry's contextual (longpress)
 * menu.
 *
 * @param obj The entry object
 *
 * @see elm_entry_context_menu_item_add()
 */
EAPI void               elm_entry_context_menu_clear(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * This adds an item to the entry's contextual menu.
 *
 * A longpress on an entry will make the contextual menu show up, if this
 * hasn't been disabled with elm_entry_context_menu_disabled_set().
 * By default, this menu provides a few options like enabling selection mode,
 * which is useful on embedded devices that need to be explicit about it,
 * and when a selection exists it also shows the copy and cut actions.
 *
 * With this function, developers can add other options to this menu to
 * perform any action they deem necessary.
 *
 * @param obj The entry object
 * @param label The item's text label
 * @param icon_file The item's icon file
 * @param icon_type The item's icon type
 * @param func The callback to execute when the item is clicked
 * @param data The data to associate with the item for related functions
 */
EAPI void               elm_entry_context_menu_item_add(Evas_Object *obj, const char *label, const char *icon_file, Elm_Icon_Type icon_type, Evas_Smart_Cb func, const void *data) EINA_ARG_NONNULL(1);

/**
 * This disables the entry's contextual (longpress) menu.
 *
 * @param obj The entry object
 * @param disabled If true, the menu is disabled
 */
EAPI void               elm_entry_context_menu_disabled_set(Evas_Object *obj, Eina_Bool disabled) EINA_ARG_NONNULL(1);

/**
 * This returns whether the entry's contextual (longpress) menu is
 * disabled.
 *
 * @param obj The entry object
 * @return If true, the menu is disabled
 */
EAPI Eina_Bool          elm_entry_context_menu_disabled_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * This appends a custom item provider to the list for that entry
 *
 * This appends the given callback. The list is walked from beginning to end
 * with each function called given the item href string in the text. If the
 * function returns an object handle other than NULL (it should create an
 * object to do this), then this object is used to replace that item. If
 * not the next provider is called until one provides an item object, or the
 * default provider in entry does.
 *
 * @param obj The entry object
 * @param func The function called to provide the item object
 * @param data The data passed to @p func
 *
 * @see @ref entry-items
 */
EAPI void               elm_entry_item_provider_append(Evas_Object *obj, Evas_Object * (*func)(void *data, Evas_Object * entry, const char *item), void *data) EINA_ARG_NONNULL(1, 2);

/**
 * This prepends a custom item provider to the list for that entry
 *
 * This prepends the given callback. See elm_entry_item_provider_append() for
 * more information
 *
 * @param obj The entry object
 * @param func The function called to provide the item object
 * @param data The data passed to @p func
 */
EAPI void               elm_entry_item_provider_prepend(Evas_Object *obj, Evas_Object * (*func)(void *data, Evas_Object * entry, const char *item), void *data) EINA_ARG_NONNULL(1, 2);

/**
 * This removes a custom item provider to the list for that entry
 *
 * This removes the given callback. See elm_entry_item_provider_append() for
 * more information
 *
 * @param obj The entry object
 * @param func The function called to provide the item object
 * @param data The data passed to @p func
 */
EAPI void               elm_entry_item_provider_remove(Evas_Object *obj, Evas_Object * (*func)(void *data, Evas_Object * entry, const char *item), void *data) EINA_ARG_NONNULL(1, 2);

/**
 * Append a filter function for text inserted in the entry
 *
 * Append the given callback to the list. This functions will be called
 * whenever any text is inserted into the entry, with the text to be inserted
 * as a parameter. The callback function is free to alter the text in any way
 * it wants, but it must remember to free the given pointer and update it.
 * If the new text is to be discarded, the function can free it and set its
 * text parameter to NULL. This will also prevent any following filters from
 * being called.
 *
 * @param obj The entry object
 * @param func The function to use as text filter
 * @param data User data to pass to @p func
 */
EAPI void               elm_entry_text_filter_append(Evas_Object *obj, Elm_Entry_Filter_Cb func, void *data) EINA_ARG_NONNULL(1, 2);

/**
 * Prepend a filter function for text insdrted in the entry
 *
 * Prepend the given callback to the list. See elm_entry_text_filter_append()
 * for more information
 *
 * @param obj The entry object
 * @param func The function to use as text filter
 * @param data User data to pass to @p func
 */
EAPI void               elm_entry_text_filter_prepend(Evas_Object *obj, Elm_Entry_Filter_Cb func, void *data) EINA_ARG_NONNULL(1, 2);

/**
 * Remove a filter from the list
 *
 * Removes the given callback from the filter list. See
 * elm_entry_text_filter_append() for more information.
 *
 * @param obj The entry object
 * @param func The filter function to remove
 * @param data The user data passed when adding the function
 */
EAPI void               elm_entry_text_filter_remove(Evas_Object *obj, Elm_Entry_Filter_Cb func, void *data) EINA_ARG_NONNULL(1, 2);

/**
 * This converts a markup (HTML-like) string into UTF-8.
 *
 * The returned string is a malloc'ed buffer and it should be freed when
 * not needed anymore.
 *
 * @param s The string (in markup) to be converted
 * @return The converted string (in UTF-8). It should be freed.
 */
EAPI char              *elm_entry_markup_to_utf8(const char *s)
EINA_MALLOC EINA_WARN_UNUSED_RESULT;

/**
 * This converts a UTF-8 string into markup (HTML-like).
 *
 * The returned string is a malloc'ed buffer and it should be freed when
 * not needed anymore.
 *
 * @param s The string (in UTF-8) to be converted
 * @return The converted string (in markup). It should be freed.
 */
EAPI char              *elm_entry_utf8_to_markup(const char *s)
EINA_MALLOC EINA_WARN_UNUSED_RESULT;

/**
 * This sets the file (and implicitly loads it) for the text to display and
 * then edit. All changes are written back to the file after a short delay if
 * the entry object is set to autosave (which is the default).
 *
 * If the entry had any other file set previously, any changes made to it
 * will be saved if the autosave feature is enabled, otherwise, the file
 * will be silently discarded and any non-saved changes will be lost.
 *
 * @param obj The entry object
 * @param file The path to the file to load and save
 * @param format The file format
 */
EAPI void               elm_entry_file_set(Evas_Object *obj, const char *file, Elm_Text_Format format) EINA_ARG_NONNULL(1);

/**
 * Gets the file being edited by the entry.
 *
 * This function can be used to retrieve any file set on the entry for
 * edition, along with the format used to load and save it.
 *
 * @param obj The entry object
 * @param file The path to the file to load and save
 * @param format The file format
 */
EAPI void               elm_entry_file_get(const Evas_Object *obj, const char **file, Elm_Text_Format *format) EINA_ARG_NONNULL(1);

/**
 * This function writes any changes made to the file set with
 * elm_entry_file_set()
 *
 * @param obj The entry object
 */
EAPI void               elm_entry_file_save(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * This sets the entry object to 'autosave' the loaded text file or not.
 *
 * @param obj The entry object
 * @param autosave Autosave the loaded file or not
 *
 * @see elm_entry_file_set()
 */
EAPI void               elm_entry_autosave_set(Evas_Object *obj, Eina_Bool autosave) EINA_ARG_NONNULL(1);

/**
 * This gets the entry object's 'autosave' status.
 *
 * @param obj The entry object
 * @return Autosave the loaded file or not
 *
 * @see elm_entry_file_set()
 */
EAPI Eina_Bool          elm_entry_autosave_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Control pasting of text and images for the widget.
 *
 * Normally the entry allows both text and images to be pasted.  By setting
 * textonly to be true, this prevents images from being pasted.
 *
 * Note this only changes the behaviour of text.
 *
 * @param obj The entry object
 * @param textonly paste mode - EINA_TRUE is text only, EINA_FALSE is
 * text+image+other.
 */
EAPI void               elm_entry_cnp_textonly_set(Evas_Object *obj, Eina_Bool textonly) EINA_ARG_NONNULL(1);

/**
 * Getting elm_entry text paste/drop mode.
 *
 * In textonly mode, only text may be pasted or dropped into the widget.
 *
 * @param obj The entry object
 * @return If the widget only accepts text from pastes.
 */
EAPI Eina_Bool          elm_entry_cnp_textonly_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Enable or disable scrolling in entry
 *
 * Normally the entry is not scrollable unless you enable it with this call.
 *
 * @param obj The entry object
 * @param scroll EINA_TRUE if it is to be scrollable, EINA_FALSE otherwise
 */
EAPI void               elm_entry_scrollable_set(Evas_Object *obj, Eina_Bool scroll);

/**
 * Get the scrollable state of the entry
 *
 * Normally the entry is not scrollable. This gets the scrollable state
 * of the entry. See elm_entry_scrollable_set() for more information.
 *
 * @param obj The entry object
 * @return The scrollable state
 */
EAPI Eina_Bool          elm_entry_scrollable_get(const Evas_Object *obj);

/**
 * This sets a widget to be displayed to the left of a scrolled entry.
 *
 * @param obj The scrolled entry object
 * @param icon The widget to display on the left side of the scrolled
 * entry.
 *
 * @note A previously set widget will be destroyed.
 * @note If the object being set does not have minimum size hints set,
 * it won't get properly displayed.
 *
 * @see elm_entry_end_set()
 */
EAPI void               elm_entry_icon_set(Evas_Object *obj, Evas_Object *icon);

/**
 * Gets the leftmost widget of the scrolled entry. This object is
 * owned by the scrolled entry and should not be modified.
 *
 * @param obj The scrolled entry object
 * @return the left widget inside the scroller
 */
EAPI Evas_Object       *elm_entry_icon_get(const Evas_Object *obj);

/**
 * Unset the leftmost widget of the scrolled entry, unparenting and
 * returning it.
 *
 * @param obj The scrolled entry object
 * @return the previously set icon sub-object of this entry, on
 * success.
 *
 * @see elm_entry_icon_set()
 */
EAPI Evas_Object       *elm_entry_icon_unset(Evas_Object *obj);

/**
 * Sets the visibility of the left-side widget of the scrolled entry,
 * set by elm_entry_icon_set().
 *
 * @param obj The scrolled entry object
 * @param setting EINA_TRUE if the object should be displayed,
 * EINA_FALSE if not.
 */
EAPI void               elm_entry_icon_visible_set(Evas_Object *obj, Eina_Bool setting);

/**
 * This sets a widget to be displayed to the end of a scrolled entry.
 *
 * @param obj The scrolled entry object
 * @param end The widget to display on the right side of the scrolled
 * entry.
 *
 * @note A previously set widget will be destroyed.
 * @note If the object being set does not have minimum size hints set,
 * it won't get properly displayed.
 *
 * @see elm_entry_icon_set
 */
EAPI void               elm_entry_end_set(Evas_Object *obj, Evas_Object *end);

/**
 * Gets the endmost widget of the scrolled entry. This object is owned
 * by the scrolled entry and should not be modified.
 *
 * @param obj The scrolled entry object
 * @return the right widget inside the scroller
 */
EAPI Evas_Object       *elm_entry_end_get(const Evas_Object *obj);

/**
 * Unset the endmost widget of the scrolled entry, unparenting and
 * returning it.
 *
 * @param obj The scrolled entry object
 * @return the previously set icon sub-object of this entry, on
 * success.
 *
 * @see elm_entry_icon_set()
 */
EAPI Evas_Object       *elm_entry_end_unset(Evas_Object *obj);

/**
 * Sets the visibility of the end widget of the scrolled entry, set by
 * elm_entry_end_set().
 *
 * @param obj The scrolled entry object
 * @param setting EINA_TRUE if the object should be displayed,
 * EINA_FALSE if not.
 */
EAPI void               elm_entry_end_visible_set(Evas_Object *obj, Eina_Bool setting);

/**
 * This sets the scrolled entry's scrollbar policy (ie. enabling/disabling
 * them).
 *
 * Setting an entry to single-line mode with elm_entry_single_line_set()
 * will automatically disable the display of scrollbars when the entry
 * moves inside its scroller.
 *
 * @param obj The scrolled entry object
 * @param h The horizontal scrollbar policy to apply
 * @param v The vertical scrollbar policy to apply
 */
EAPI void               elm_entry_scrollbar_policy_set(Evas_Object *obj, Elm_Scroller_Policy h, Elm_Scroller_Policy v);

/**
 * This enables/disables bouncing within the entry.
 *
 * This function sets whether the entry will bounce when scrolling reaches
 * the end of the contained entry.
 *
 * @param obj The scrolled entry object
 * @param h_bounce The horizontal bounce state
 * @param v_bounce The vertical bounce state
 */
EAPI void               elm_entry_bounce_set(Evas_Object *obj, Eina_Bool h_bounce, Eina_Bool v_bounce);

/**
 * Get the bounce mode
 *
 * @param obj The Entry object
 * @param h_bounce Allow bounce horizontally
 * @param v_bounce Allow bounce vertically
 */
EAPI void               elm_entry_bounce_get(const Evas_Object *obj, Eina_Bool *h_bounce, Eina_Bool *v_bounce);

/* pre-made filters for entries */
/**
 * @typedef Elm_Entry_Filter_Limit_Size
 *
 * Data for the elm_entry_filter_limit_size() entry filter.
 */
typedef struct _Elm_Entry_Filter_Limit_Size Elm_Entry_Filter_Limit_Size;

/**
 * @struct _Elm_Entry_Filter_Limit_Size
 *
 * Data for the elm_entry_filter_limit_size() entry filter.
 */
struct _Elm_Entry_Filter_Limit_Size
{
   int max_char_count;      /**< The maximum number of characters allowed. */
   int max_byte_count;      /**< The maximum number of bytes allowed*/
};

/**
 * Filter inserted text based on user defined character and byte limits
 *
 * Add this filter to an entry to limit the characters that it will accept
 * based the the contents of the provided #Elm_Entry_Filter_Limit_Size.
 * The funtion works on the UTF-8 representation of the string, converting
 * it from the set markup, thus not accounting for any format in it.
 *
 * The user must create an #Elm_Entry_Filter_Limit_Size structure and pass
 * it as data when setting the filter. In it, it's possible to set limits
 * by character count or bytes (any of them is disabled if 0), and both can
 * be set at the same time. In that case, it first checks for characters,
 * then bytes.
 *
 * The function will cut the inserted text in order to allow only the first
 * number of characters that are still allowed. The cut is made in
 * characters, even when limiting by bytes, in order to always contain
 * valid ones and avoid half unicode characters making it in.
 *
 * This filter, like any others, does not apply when setting the entry text
 * directly with elm_object_text_set().
 */
EAPI void elm_entry_filter_limit_size(void *data, Evas_Object *entry, char **text) EINA_ARG_NONNULL(1, 2, 3);

/**
 * @typedef Elm_Entry_Filter_Accept_Set
 *
 * Data for the elm_entry_filter_accept_set() entry filter.
 */
typedef struct _Elm_Entry_Filter_Accept_Set Elm_Entry_Filter_Accept_Set;

/**
 * @struct _Elm_Entry_Filter_Accept_Set
 *
 * Data for the elm_entry_filter_accept_set() entry filter.
 */
struct _Elm_Entry_Filter_Accept_Set
{
   const char *accepted;      /**< Set of characters accepted in the entry. */
   const char *rejected;      /**< Set of characters rejected from the entry. */
};

/**
 * Filter inserted text based on accepted or rejected sets of characters
 *
 * Add this filter to an entry to restrict the set of accepted characters
 * based on the sets in the provided #Elm_Entry_Filter_Accept_Set.
 * This structure contains both accepted and rejected sets, but they are
 * mutually exclusive.
 *
 * The @c accepted set takes preference, so if it is set, the filter will
 * only work based on the accepted characters, ignoring anything in the
 * @c rejected value. If @c accepted is @c NULL, then @c rejected is used.
 *
 * In both cases, the function filters by matching utf8 characters to the
 * raw markup text, so it can be used to remove formatting tags.
 *
 * This filter, like any others, does not apply when setting the entry text
 * directly with elm_object_text_set()
 */
EAPI void                   elm_entry_filter_accept_set(void *data, Evas_Object *entry, char **text) EINA_ARG_NONNULL(1, 3);
/**
 * Set the input panel layout of the entry
 *
 * @param obj The entry object
 * @param layout layout type
 */
EAPI void                   elm_entry_input_panel_layout_set(Evas_Object *obj, Elm_Input_Panel_Layout layout) EINA_ARG_NONNULL(1);

/**
 * Get the input panel layout of the entry
 *
 * @param obj The entry object
 * @return layout type
 *
 * @see elm_entry_input_panel_layout_set
 */
EAPI Elm_Input_Panel_Layout elm_entry_input_panel_layout_get(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Set the autocapitalization type on the immodule.
 *
 * @param obj The entry object
 * @param autocapital_type The type of autocapitalization
 */
EAPI void                   elm_entry_autocapital_type_set(Evas_Object *obj, Elm_Autocapital_Type autocapital_type) EINA_ARG_NONNULL(1);

/**
 * Retrieve the autocapitalization type on the immodule.
 *
 * @param obj The entry object
 * @return autocapitalization type
 */
EAPI Elm_Autocapital_Type   elm_entry_autocapital_type_get(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * Sets the attribute to show the input panel automatically.
 *
 * @param obj The entry object
 * @param enabled If true, the input panel is appeared when entry is clicked or has a focus
 */
EAPI void                   elm_entry_input_panel_enabled_set(Evas_Object *obj, Eina_Bool enabled) EINA_ARG_NONNULL(1);

/**
 * Retrieve the attribute to show the input panel automatically.
 *
 * @param obj The entry object
 * @return EINA_TRUE if input panel will be appeared when the entry is clicked or has a focus, EINA_FALSE otherwise
 */
EAPI Eina_Bool              elm_entry_input_panel_enabled_get(Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * @}
 */
