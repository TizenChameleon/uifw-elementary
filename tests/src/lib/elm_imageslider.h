   /**
    * @defgroup Imageslider Imageslider
    * @ingroup Elementary
    * @addtogroup Imageslider
    * @{
    *
    * By flicking images on the screen,
    * you can see the images in specific path.
    */
   typedef struct _Imageslider_Item Elm_Imageslider_Item;
   typedef void (*Elm_Imageslider_Cb)(void *data, Evas_Object *obj, void *event_info);

   /**
    * Add an Image Slider widget
    *
    * @param        parent  The parent object
    * @return       The new Image slider object or NULL if it cannot be created
    */
   EAPI Evas_Object           *elm_imageslider_add(Evas_Object *parent) EINA_ARG_NONNULL(1);
   /**
    * Append an Image Slider item
    *
    * @param        obj          The Image Slider object
    * @param        photo_file   photo file path
    * @param        func         callback function
    * @param        data         callback data
    * @return       The Image Slider item handle or NULL
    */
   EAPI Elm_Imageslider_Item  *elm_imageslider_item_append(Evas_Object *obj, const char *photo_file, Elm_Imageslider_Cb func, void *data) EINA_ARG_NONNULL(1);
   /**
    * Insert an Image Slider item into the Image Slider Widget by using the given index.
    *
    * @param        obj                     The Image Slider object
    * @param        photo_file      photo file path
    * @param        func            callback function
    * @param        index           required position
    * @param        data            callback data
    * @return       The Image Slider item handle or NULL
    */
   EAPI Elm_Imageslider_Item  *elm_imageslider_item_append_relative(Evas_Object *obj, const char *photo_file, Elm_Imageslider_Cb func, unsigned int index, void *data) EINA_ARG_NONNULL(1);
   /**
    * Prepend Image Slider item
    *
    * @param        obj          The Image Slider object
    * @param        photo_file   photo file path
    * @param        func         callback function
    * @param        data         callback data
    * @return       The imageslider item handle or NULL
    */
   EAPI Elm_Imageslider_Item  *elm_imageslider_item_prepend(Evas_Object *obj, const char *photo_file, Elm_Imageslider_Cb func, void *data) EINA_ARG_NONNULL(1);
   /**
    * Delete the selected Image Slider item
    *
    * @param it             The selected Image Slider item handle
    */
   EAPI void                   elm_imageslider_item_del(Elm_Imageslider_Item *it) EINA_ARG_NONNULL(1);
   /**
    * Get the selected Image Slider item
    *
    * @param obj            The Image Slider object
    * @return The selected Image Slider item or NULL
    */
   EAPI Elm_Imageslider_Item  *elm_imageslider_selected_item_get(Evas_Object *obj) EINA_ARG_NONNULL(1);
   /**
    * Get whether an Image Slider item is selected or not
    *
    * @param it              the selected Image Slider item
    * @return EINA_TRUE or EINA_FALSE
    */
   EAPI Eina_Bool              elm_imageslider_item_selected_get(Elm_Imageslider_Item *it) EINA_ARG_NONNULL(1);
   /**
    * Set the selected Image Slider item
    *
    * @param it             The Imaga Slider item
    */
   EAPI void                   elm_imageslider_item_selected_set(Elm_Imageslider_Item *it) EINA_ARG_NONNULL(1);
   /**
    * Get the photo file path of given Image Slider item
    *
    * @param it             The Image Slider item
    * @return The photo file path or NULL;
    */
   EAPI const char            *elm_imageslider_item_photo_file_get(Elm_Imageslider_Item *it) EINA_ARG_NONNULL(1);
   /**
    * Sets the photo file path of given Image Slider item
    *
    * @param it         The Image Slider item
    * @param photo_file The photo file path or NULL;
    */
   EAPI Elm_Imageslider_Item  *elm_imageslider_item_prev(Elm_Imageslider_Item *it) EINA_ARG_NONNULL(1);
   /**
    * Get the previous Image Slider item
    *
    * @param it             The Image Slider item
    * @return The previous Image Slider item or NULL
    */
   EAPI Elm_Imageslider_Item  *elm_imageslider_item_next(Elm_Imageslider_Item *it) EINA_ARG_NONNULL(1);
   /**
    * Get the next Image Slider item
    *
    * @param it             The Image Slider item
    * @return The next Image Slider item or NULL
    */
   EAPI void                   elm_imageslider_prev(Evas_Object *obj) EINA_ARG_NONNULL(1);
   /**
    * Move to the previous Image Slider item
    *
    * @param obj    The Image Slider object
    */
   EAPI void                   elm_imageslider_next(Evas_Object *obj) EINA_ARG_NONNULL(1);
   /**
    * Move to the next Image Slider item
    *
    * @param obj The Image Slider object
    */
   EAPI void                   elm_imageslider_item_photo_file_set(Elm_Imageslider_Item *it, const char *photo_file) EINA_ARG_NONNULL(1,2);
   /**
    * Updates an Image Slider item
    *
    * @param it The Image Slider item
    */
   EAPI void                   elm_imageslider_item_update(Elm_Imageslider_Item *it) EINA_ARG_NONNULL(1);
   /**
    * @}
    */

