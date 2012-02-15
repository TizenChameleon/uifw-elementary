/**
 * @defgroup Separator Separator
 *
 * @brief Separator is a very thin object used to separate other objects.
 *
 * A separator can be vertical or horizontal.
 *
 * @ref tutorial_separator is a good example of how to use a separator.
 * @{
 */
/**
 * @brief Add a separator object to @p parent
 *
 * @param parent The parent object
 *
 * @return The separator object, or NULL upon failure
 */
EAPI Evas_Object *elm_separator_add(Evas_Object *parent);
/**
 * @brief Set the horizontal mode of a separator object
 *
 * @param obj The separator object
 * @param horizontal If true, the separator is horizontal
 */
EAPI void      elm_separator_horizontal_set(Evas_Object *obj, Eina_Bool horizontal);
/**
 * @brief Get the horizontal mode of a separator object
 *
 * @param obj The separator object
 * @return If true, the separator is horizontal
 *
 * @see elm_separator_horizontal_set()
 */
EAPI Eina_Bool elm_separator_horizontal_get(const Evas_Object *obj);
/**
 * @}
 */
