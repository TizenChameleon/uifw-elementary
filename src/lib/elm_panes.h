   /**
    * @defgroup Panes Panes
    * @ingroup Elementary
    *
    * @image html img/widget/panes/preview-00.png
    * @image latex img/widget/panes/preview-00.eps width=\textwidth
    *
    * @image html img/panes.png
    * @image latex img/panes.eps width=\textwidth
    *
    * The panes adds a dragable bar between two contents. When dragged
    * this bar will resize contents size.
    *
    * Panes can be displayed vertically or horizontally, and contents
    * size proportion can be customized (homogeneous by default).
    *
    * Smart callbacks one can listen to:
    * - "press" - The panes has been pressed (button wasn't released yet).
    * - "unpressed" - The panes was released after being pressed.
    * - "clicked" - The panes has been clicked>
    * - "clicked,double" - The panes has been double clicked
    *
    * Available styles for it:
    * - @c "default"
    *
    * Default contents parts of the panes widget that you can use for are:
    * @li "left" - A leftside content of the panes
    * @li "right" - A rightside content of the panes
    *
    * If panes is displayed vertically, left content will be displayed at
    * top.
    *
    * Here is an example on its usage:
    * @li @ref panes_example
    */

   /**
    * @addtogroup Panes
    * @{
    */

   /**
    * Add a new panes widget to the given parent Elementary
    * (container) object.
    *
    * @param parent The parent object.
    * @return a new panes widget handle or @c NULL, on errors.
    *
    * This function inserts a new panes widget on the canvas.
    *
    * @ingroup Panes
    */
   EAPI Evas_Object          *elm_panes_add(Evas_Object *parent) EINA_ARG_NONNULL(1);

   /**
    * Set the left content of the panes widget.
    *
    * @param obj The panes object.
    * @param content The new left content object.
    *
    * Once the content object is set, a previously set one will be deleted.
    * If you want to keep that old content object, use the
    * elm_panes_content_left_unset() function.
    *
    * If panes is displayed vertically, left content will be displayed at
    * top.
    *
    * @see elm_panes_content_left_get()
    * @see elm_panes_content_right_set() to set content on the other side.
    *
    * @deprecated use elm_object_part_content_set() instead
    *
    * @ingroup Panes
    */
   EINA_DEPRECATED EAPI void                  elm_panes_content_left_set(Evas_Object *obj, Evas_Object *content) EINA_ARG_NONNULL(1);

   /**
    * Set the right content of the panes widget.
    *
    * @param obj The panes object.
    * @param content The new right content object.
    *
    * Once the content object is set, a previously set one will be deleted.
    * If you want to keep that old content object, use the
    * elm_panes_content_right_unset() function.
    *
    * If panes is displayed vertically, left content will be displayed at
    * bottom.
    *
    * @see elm_panes_content_right_get()
    * @see elm_panes_content_left_set() to set content on the other side.
    *
    * @deprecated use elm_object_part_content_set() instead
    *
    * @ingroup Panes
    */
   EINA_DEPRECATED EAPI void                  elm_panes_content_right_set(Evas_Object *obj, Evas_Object *content) EINA_ARG_NONNULL(1);

   /**
    * Get the left content of the panes.
    *
    * @param obj The panes object.
    * @return The left content object that is being used.
    *
    * Return the left content object which is set for this widget.
    *
    * @see elm_panes_content_left_set() for details.
    *
    * @deprecated use elm_object_part_content_get() instead
    *
    * @ingroup Panes
    */
   EINA_DEPRECATED EAPI Evas_Object          *elm_panes_content_left_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

   /**
    * Get the right content of the panes.
    *
    * @param obj The panes object
    * @return The right content object that is being used
    *
    * Return the right content object which is set for this widget.
    *
    * @see elm_panes_content_right_set() for details.
    *
    * @deprecated use elm_object_part_content_get() instead
    *
    * @ingroup Panes
    */
   EINA_DEPRECATED EAPI Evas_Object          *elm_panes_content_right_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

   /**
    * Unset the left content used for the panes.
    *
    * @param obj The panes object.
    * @return The left content object that was being used.
    *
    * Unparent and return the left content object which was set for this widget.
    *
    * @see elm_panes_content_left_set() for details.
    * @see elm_panes_content_left_get().
    *
    * @deprecated use elm_object_part_content_unset() instead
    *
    * @ingroup Panes
    */
   EINA_DEPRECATED EAPI Evas_Object          *elm_panes_content_left_unset(Evas_Object *obj) EINA_ARG_NONNULL(1);

   /**
    * Unset the right content used for the panes.
    *
    * @param obj The panes object.
    * @return The right content object that was being used.
    *
    * Unparent and return the right content object which was set for this
    * widget.
    *
    * @see elm_panes_content_right_set() for details.
    * @see elm_panes_content_right_get().
    *
    * @deprecated use elm_object_part_content_unset() instead
    *
    * @ingroup Panes
    */
   EINA_DEPRECATED EAPI Evas_Object          *elm_panes_content_right_unset(Evas_Object *obj) EINA_ARG_NONNULL(1);

   /**
    * Get the size proportion of panes widget's left side.
    *
    * @param obj The panes object.
    * @return float value between 0.0 and 1.0 representing size proportion
    * of left side.
    *
    * @see elm_panes_content_left_size_set() for more details.
    *
    * @ingroup Panes
    */
   EAPI double                elm_panes_content_left_size_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

   /**
    * Set the size proportion of panes widget's left side.
    *
    * @param obj The panes object.
    * @param size Value between 0.0 and 1.0 representing size proportion
    * of left side.
    *
    * By default it's homogeneous, i.e., both sides have the same size.
    *
    * If something different is required, it can be set with this function.
    * For example, if the left content should be displayed over
    * 75% of the panes size, @p size should be passed as @c 0.75.
    * This way, right content will be resized to 25% of panes size.
    *
    * If displayed vertically, left content is displayed at top, and
    * right content at bottom.
    *
    * @note This proportion will change when user drags the panes bar.
    *
    * @see elm_panes_content_left_size_get()
    *
    * @ingroup Panes
    */
   EAPI void                  elm_panes_content_left_size_set(Evas_Object *obj, double size) EINA_ARG_NONNULL(1);

  /**
   * Set the orientation of a given panes widget.
   *
   * @param obj The panes object.
   * @param horizontal Use @c EINA_TRUE to make @p obj to be
   * @b horizontal, @c EINA_FALSE to make it @b vertical.
   *
   * Use this function to change how your panes is to be
   * disposed: vertically or horizontally.
   *
   * By default it's displayed horizontally.
   *
   * @see elm_panes_horizontal_get()
   *
   * @ingroup Panes
   */
   EAPI void                  elm_panes_horizontal_set(Evas_Object *obj, Eina_Bool horizontal) EINA_ARG_NONNULL(1);

   /**
    * Retrieve the orientation of a given panes widget.
    *
    * @param obj The panes object.
    * @return @c EINA_TRUE, if @p obj is set to be @b horizontal,
    * @c EINA_FALSE if it's @b vertical (and on errors).
    *
    * @see elm_panes_horizontal_set() for more details.
    *
    * @ingroup Panes
    */
   EAPI Eina_Bool             elm_panes_horizontal_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);
   EAPI void                  elm_panes_fixed_set(Evas_Object *obj, Eina_Bool fixed) EINA_ARG_NONNULL(1);
   EAPI Eina_Bool             elm_panes_fixed_get(const Evas_Object *obj) EINA_ARG_NONNULL(1);

   /**
    * @}
    */

