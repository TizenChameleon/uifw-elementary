   /**
    * @defgroup TickerNoti TickerNoti
    * @ingroup Elementary
    *
    * This is a notification widget which can be used to display some short information.
    *
    * Signals that you can add callback for are:
    * @li "clicked" - tickernoti object has been clicked, except at the
    * swallow/button region
    * @li "hide" - tickernoti is completely hidden. In case of
    * any hide animation, this signal is emitted after the animation.
    *
    * Default contents parts of a tickernoti object that you can use for are:
    * @li "icon" - The icon in tickernoti object
    * @li "button" - The button in tickernoti object
    *
    * Default text parts of the tickernoti object that you can use for are:
    * @li "default" - textual content in the tickernoti object
    *
    * Supported elm_object common APIs.
    * @li elm_object_text_set
    * @li elm_object_part_text_set
    * @li elm_object_part_content_set
    *
    */

   /**
    * @addtogroup Tickernoti
    * @{
    */
   typedef enum
     {
        ELM_TICKERNOTI_ORIENT_TOP = 0,
        ELM_TICKERNOTI_ORIENT_BOTTOM,
        ELM_TICKERNOTI_ORIENT_LAST
     }  Elm_Tickernoti_Orient;

   /**
    * Add a tickernoti object to @p parent
    *
    * @param parent The parent object
    *
    * @return The tickernoti object, or NULL upon failure
    */
   EAPI Evas_Object              *elm_tickernoti_add (Evas_Object *parent);
   /**
    * Set the orientation of the tickernoti object
    *
    * @param obj The tickernoti object
    * @param orient The orientation of tickernoti object
    */
   EAPI void                      elm_tickernoti_orient_set (Evas_Object *obj, Elm_Tickernoti_Orient orient) EINA_ARG_NONNULL(1);
   /**
    * Get the orientation of the tickernoti object
    *
    * @param obj The tickernotil object
    * @return The orientation of tickernotil object
    */
   EAPI Elm_Tickernoti_Orient     elm_tickernoti_orient_get (const Evas_Object *obj) EINA_ARG_NONNULL(1);
   /**
    * Get the rotation of tickernoti object
    *
    * @param obj The tickernotil object
    * @return The rotation angle
    */
   EAPI int                       elm_tickernoti_rotation_get (const Evas_Object *obj) EINA_ARG_NONNULL(1);
   /**
    * Set the rotation angle for the tickernoti object
    *
    * @param obj The tickernoti object
    * @param angle The rotation angle(in degree) will be used on the tickernoti object
    */
   EAPI void                      elm_tickernoti_rotation_set (Evas_Object *obj, int angle) EINA_ARG_NONNULL(1);
   /**
    * Get the view window(elm_win) on the tickernoti object
    *
    * @param obj The tickernotil object
    * @return internal view window(elm_win) object
    */
   EAPI Evas_Object              *elm_tickernoti_win_get (const Evas_Object *obj) EINA_ARG_NONNULL(1);
   /* #### Below APIs and data structures are going to be deprecated, announcment will be made soon ####*/
   /**
    * @deprecated
    */
   typedef enum
    {
       ELM_TICKERNOTI_DEFAULT,
       ELM_TICKERNOTI_DETAILVIEW
    } Elm_Tickernoti_Mode;
   /**
    * Set the detail label on the tickernoti object
    *
    * @param obj The tickernoti object
    * @param label The label will be used on the tickernoti object
    * @deprecated use elm_object_text_set() instead
    */
   EINA_DEPRECATED  EAPI void                      elm_tickernoti_detailview_label_set (Evas_Object *obj, const char *label) EINA_ARG_NONNULL(1);
   /**
    * Get the detail label used on the tickernoti object
    *
    * @param obj The tickernotil object
    * @return The string inside the label
    * @deprecated use elm_object_text_get() instead
    */
   EINA_DEPRECATED  EAPI const char               *elm_tickernoti_detailview_label_get (const Evas_Object *obj)EINA_ARG_NONNULL(1);
   /**
    * Set the button object used on the tickernoti object
    *
    * @param obj The tickernotil object
    * @param button The button object will be used on the tickernoti object
    * @deprecated use elm_object_part_content_set() instead with "button" as part name
    */
   EINA_DEPRECATED  EAPI void                      elm_tickernoti_detailview_button_set (Evas_Object *obj, Evas_Object *button) EINA_ARG_NONNULL(2);
   /**
    * Get the button object used on the tickernoti object
    *
    * @param obj The tickernotil object
    * @return The button object inside the tickernoti
    * @deprecated use elm_object_part_content_get() instead with "button" as part name
    */
   EINA_DEPRECATED  EAPI Evas_Object              *elm_tickernoti_detailview_button_get (const Evas_Object *obj) EINA_ARG_NONNULL(1);
   /**
    * Set the detail icon object used on the tickernoti object
    *
    * @param obj The tickernotil object
    * @param icon The icon object will be used on the tickernoti object
    * @deprecated use elm_object_part_content_set() instead with "icon" as part name
    */
   EINA_DEPRECATED  EAPI void                      elm_tickernoti_detailview_icon_set (Evas_Object *obj, Evas_Object *icon) EINA_ARG_NONNULL(1);
   /**
    * Get the detail icon object used on the tickernoti object
    *
    * @param obj The tickernotil object
    * @return The icon object inside the tickernoti
    * @deprecated use elm_object_part_content_get() instead with "icon" as part name
    */
   EINA_DEPRECATED  EAPI Evas_Object              *elm_tickernoti_detailview_icon_get (const Evas_Object *obj) EINA_ARG_NONNULL(1);
   /**
    * Get the view mode on the tickernoti object
    *
    * @param obj The tickernotil object
    * @return The internal window used
    * @deprecated use elm_tickernoti_win_get instead when internal window object is needed
    */
   EINA_DEPRECATED  EAPI Evas_Object              *elm_tickernoti_detailview_get (const Evas_Object *obj) EINA_ARG_NONNULL(1);
   /**
    * Set the view mode used on the tickernoti object
    *
    * @param obj The tickernotil object
    * @param mode The view mode will be used on the tickernoti object
    * @deprecated removed as now styles are used. Use elm_object_style_set instead.
    */
   EINA_DEPRECATED  EAPI void                      elm_tickernoti_mode_set (Evas_Object *obj, Elm_Tickernoti_Mode mode) EINA_ARG_NONNULL(1);
   /**
    * Get the current mode of the tickernoti object
    *
    * @param obj The tickernotil object
    * @return the mode of the object. Can be ELM_TICKERNOTI_DEFAULT/ELM_TICKERNOTI_DETAILVIEW
    */
   EINA_DEPRECATED  EAPI Elm_Tickernoti_Mode       elm_tickernoti_mode_get (const Evas_Object *obj) EINA_ARG_NONNULL(1);
   /**
    * Set the orientation of the tickernoti object
    *
    * @param obj The tickernoti object
    * @param orient The orientation of tickernoti object
    * @deprecated use elm_tickernoti_orient_set() instead
    */
   EINA_DEPRECATED  EAPI void                      elm_tickernoti_orientation_set (Evas_Object *obj, Elm_Tickernoti_Orient orient) EINA_ARG_NONNULL(1);
   /**
    * Get the orientation of the tickernoti object
    *
    * @param obj The tickernotil object
    * @return The orientation of tickernotil object
    * @deprecated use elm_tickernoti_orient_get() instead
    */
   EINA_DEPRECATED  EAPI Elm_Tickernoti_Orient     elm_tickernoti_orientation_get (const Evas_Object *obj) EINA_ARG_NONNULL(1);
   /**
    * Set the label on the tickernoti object
    *
    * @param obj The tickernoti object
    * @param label The label will be used on the tickernoti object
    * @deprecated use elm_object_text_set()
    */
   EINA_DEPRECATED  EAPI void                      elm_tickernoti_label_set (Evas_Object *obj, const char *label) EINA_ARG_NONNULL(1);
   /**
    * Get the label used on the tickernoti object
    *
    * @param obj The tickernotil object
    * @return The string inside the label
    * @deprecated use elm_object_text_get() instead
    */
   EINA_DEPRECATED  EAPI const char               *elm_tickernoti_label_get (const Evas_Object *obj) EINA_ARG_NONNULL(1);
   /**
    * Set the icon object of the tickernoti object
    *
    * @param obj The tickernotil object
    * @param icon The icon object will be used on the tickernoti object
    * @deprecated use elm_object_part_content_set() instead with "icon" as part name
    */
   EINA_DEPRECATED  EAPI void                      elm_tickernoti_icon_set (Evas_Object *obj, Evas_Object *icon) EINA_ARG_NONNULL(1);
   /**
    * Get the icon object of the tickernoti object
    *
    * @param obj The tickernotil object
    * @return The icon object inside the tickernoti
    * @deprecated use elm_object_part_content_get() instead with "icon" as part name
    */
   EINA_DEPRECATED  EAPI Evas_Object              *elm_tickernoti_icon_get (const Evas_Object *obj) EINA_ARG_NONNULL(1);
   /**
    * Set the action button object used on the tickernoti object
    *
    * @param obj The tickernotil object
    * @param button The button object will be used on the tickernoti object
    * @deprecated use elm_object_part_content_set() instead with "button" as part name
    */
   EINA_DEPRECATED  EAPI void                      elm_tickernoti_button_set (Evas_Object *obj, Evas_Object *button) EINA_ARG_NONNULL(1);
   /**
    * Get the action button object used on the tickernoti object
    *
    * @param obj The tickernotil object
    * @return The button object inside the tickernoti
    * @deprecated use elm_object_part_content_get() instead with "button" as part name
    */
   EINA_DEPRECATED  EAPI Evas_Object              *elm_tickernoti_button_get (const Evas_Object *obj) EINA_ARG_NONNULL(1);
   /**
    * @}
    */

