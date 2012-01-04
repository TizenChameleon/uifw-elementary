/**
 * @defgroup Frame Frame
 *
 * @image html img/widget/frame/preview-00.png
 * @image latex img/widget/frame/preview-00.eps
 *
 * @brief Frame is a widget that holds some content and has a title.
 *
 * The default look is a frame with a title, but Frame supports multple
 * styles:
 * @li default
 * @li pad_small
 * @li pad_medium
 * @li pad_large
 * @li pad_huge
 * @li outdent_top
 * @li outdent_bottom
 *
 * Of all this styles only default shows the title.
 *
 * Smart callbacks one can listen to:
 * - @c "clicked" - The user has clicked the frame's label
 *
 * Default contents parts of the frame widget that you can use for are:
 * @li "default" - A content of the frame
 *
 * Default text parts of the frame widget that you can use for are:
 * @li "elm.text" - Label of the frame
 *
 * For a detailed example see the @ref tutorial_frame.
 *
 * @{
 */

/**
 * @brief Add a new frame to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 */
EAPI Evas_Object                 *elm_frame_add(Evas_Object *parent);

/**
 * @brief Toggle autocollapsing of a frame
 * @param obj The frame
 * @param enable Whether to enable autocollapse
 *
 * When @p enable is EINA_TRUE, clicking a frame's label will collapse the frame
 * vertically, shrinking it to the height of the label.
 * By default, this is DISABLED.
 */
EAPI void elm_frame_autocollapse_set(Evas_Object *obj, Eina_Bool enable);

/**
 * @brief Determine autocollapsing of a frame
 * @param obj The frame
 * @return Whether autocollapse is enabled
 *
 * When this returns EINA_TRUE, clicking a frame's label will collapse the frame
 * vertically, shrinking it to the height of the label.
 * By default, this is DISABLED.
 */
EAPI Eina_Bool elm_frame_autocollapse_get(Evas_Object *obj);

/**
 * @brief Manually collapse a frame
 * @param obj The frame
 * @param enable true to collapse, false to expand
 *
 * Use this to toggle the collapsed state of a frame.
 */
EAPI void elm_frame_collapse_set(Evas_Object *obj, Eina_Bool enable);

/**
 * @brief Determine the collapse state of a frame
 * @param obj The frame
 * @return true if collapsed, false otherwise
 *
 * Use this to determine the collapse state of a frame.
 */
EAPI Eina_Bool elm_frame_collapse_get(Evas_Object *obj);
/**
 * @}
 */
