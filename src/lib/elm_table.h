/**
 * @defgroup Table Table
 *
 * A container widget to arrange other widgets in a table where items can
 * also span multiple columns or rows - even overlap (and then be raised or
 * lowered accordingly to adjust stacking if they do overlap).
 *
 * For a Table widget the row/column count is not fixed.
 * The table widget adjusts itself when subobjects are added to it dynamically.
 *
 * The followin are examples of how to use a table:
 * @li @ref tutorial_table_01
 * @li @ref tutorial_table_02
 *
 * @{
 */

/**
 * @brief Add a new table to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 */
EAPI Evas_Object *elm_table_add(Evas_Object *parent) EINA_ARG_NONNULL(1);

/**
 * @brief Set the homogeneous layout in the table
 *
 * @param obj The layout object
 * @param homogeneous A boolean to set if the layout is homogeneous in the
 * table (EINA_TRUE = homogeneous,  EINA_FALSE = no homogeneous)
 */
EAPI void      elm_table_homogeneous_set(Evas_Object *obj, Eina_Bool homogeneous) EINA_ARG_NONNULL(1);

/**
 * @brief Get the current table homogeneous mode.
 *
 * @param obj The table object
 * @return A boolean to indicating if the layout is homogeneous in the table
 * (EINA_TRUE = homogeneous,  EINA_FALSE = no homogeneous)
 */
EAPI Eina_Bool elm_table_homogeneous_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

/**
 * @brief Set padding between cells.
 *
 * @param obj The layout object.
 * @param horizontal set the horizontal padding.
 * @param vertical set the vertical padding.
 *
 * Default value is 0.
 */
EAPI void      elm_table_padding_set(Evas_Object *obj, Evas_Coord horizontal, Evas_Coord vertical) EINA_ARG_NONNULL(1);

/**
 * @brief Get padding between cells.
 *
 * @param obj The layout object.
 * @param horizontal set the horizontal padding.
 * @param vertical set the vertical padding.
 */
EAPI void      elm_table_padding_get(const Evas_Object *obj, Evas_Coord *horizontal, Evas_Coord *vertical) EINA_ARG_NONNULL(1);

/**
 * @brief Add a subobject on the table with the coordinates passed
 *
 * @param obj The table object
 * @param subobj The subobject to be added to the table
 * @param x Row number
 * @param y Column number
 * @param w rowspan
 * @param h colspan
 *
 * @note All positioning inside the table is relative to rows and columns, so
 * a value of 0 for x and y, means the top left cell of the table, and a
 * value of 1 for w and h means @p subobj only takes that 1 cell.
 */
EAPI void      elm_table_pack(Evas_Object *obj, Evas_Object *subobj, int x, int y, int w, int h) EINA_ARG_NONNULL(1);

/**
 * @brief Remove child from table.
 *
 * @param obj The table object
 * @param subobj The subobject
 */
EAPI void      elm_table_unpack(Evas_Object *obj, Evas_Object *subobj) EINA_ARG_NONNULL(1);

/**
 * @brief Faster way to remove all child objects from a table object.
 *
 * @param obj The table object
 * @param clear If true, will delete children, else just remove from table.
 */
EAPI void      elm_table_clear(Evas_Object *obj, Eina_Bool clear) EINA_ARG_NONNULL(1);

/**
 * @brief Set the packing location of an existing child of the table
 *
 * @param subobj The subobject to be modified in the table
 * @param x Row number
 * @param y Column number
 * @param w rowspan
 * @param h colspan
 *
 * Modifies the position of an object already in the table.
 *
 * @note All positioning inside the table is relative to rows and columns, so
 * a value of 0 for x and y, means the top left cell of the table, and a
 * value of 1 for w and h means @p subobj only takes that 1 cell.
 */
EAPI void      elm_table_pack_set(Evas_Object *subobj, int x, int y, int w, int h) EINA_ARG_NONNULL(1);

/**
 * @brief Get the packing location of an existing child of the table
 *
 * @param subobj The subobject to be modified in the table
 * @param x Row number
 * @param y Column number
 * @param w rowspan
 * @param h colspan
 *
 * @see elm_table_pack_set()
 */
EAPI void      elm_table_pack_get(Evas_Object *subobj, int *x, int *y, int *w, int *h) EINA_ARG_NONNULL(1);

/**
 * @}
 */
