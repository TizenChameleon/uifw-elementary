/**
 * @defgroup Photo Photo
 *
<<<<<<< HEAD
 * For displaying the photo of a person (contact). Simple, yet
=======
 * For displaying a photo, for ex., a person (contact). Simple, yet
>>>>>>> remotes/origin/upstream
 * with a very specific purpose.
 *
 * Signals that you can add callbacks for are:
 *
 * "clicked" - This is called when a user has clicked the photo
<<<<<<< HEAD
 * "drag,start" - Someone started dragging the image out of the object
 * "drag,end" - Dragged item was dropped (somewhere)
=======
 * "drag,start" - dragging the image out of the photo object
 * "drag,end" - Drop the dragged item  
>>>>>>> remotes/origin/upstream
 *
 * @{
 */

/**
 * Add a new photo to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Photo
 */
EAPI Evas_Object *elm_photo_add(Evas_Object *parent);

/**
 * Set the file that will be used as photo
 *
 * @param obj The photo object
 * @param file The path to file that will be used as photo
 *
 * @return (1 = success, 0 = error)
 *
 * @ingroup Photo
 */
EAPI Eina_Bool elm_photo_file_set(Evas_Object *obj, const char *file);

/**
 * Set the file that will be used as thumbnail in the photo.
 *
 * @param obj The photo object.
<<<<<<< HEAD
 * @param file The path to file that will be used as thumb.
=======
 * @param file The path to file that will be used as thumbnail.
>>>>>>> remotes/origin/upstream
 * @param group The key used in case of an EET file.
 *
 * @ingroup Photo
 */
EAPI void      elm_photo_thumb_set(const Evas_Object *obj, const char *file, const char *group);

/**
 * Set the size that will be used on the photo
 *
 * @param obj The photo object
<<<<<<< HEAD
 * @param size The size that the photo will be
=======
 * @param size The size of the photo
>>>>>>> remotes/origin/upstream
 *
 * @ingroup Photo
 */
EAPI void      elm_photo_size_set(Evas_Object *obj, int size);

/**
 * Set if the photo should be completely visible or not.
 *
 * @param obj The photo object
 * @param fill if true the photo will be completely visible
 *
 * @ingroup Photo
 */
EAPI void      elm_photo_fill_inside_set(Evas_Object *obj, Eina_Bool fill);

/**
 * Set editability of the photo.
 *
 * An editable photo can be dragged to or from, and can be cut or
 * pasted too.  Note that pasting an image or dropping an item on
 * the image will delete the existing content.
 *
 * @param obj The photo object.
<<<<<<< HEAD
 * @param set To set of clear editablity.
=======
 * @param set To set of clear editability.
>>>>>>> remotes/origin/upstream
 */
EAPI void      elm_photo_editable_set(Evas_Object *obj, Eina_Bool set);

/**
 * Set whether the original aspect ratio of the photo should be kept on resize.
 *
 * @param obj The photo object.
 * @param fixed @c EINA_TRUE if the photo should fix the aspect,
 * @c EINA_FALSE otherwise.
 *
 * The original aspect ratio (width / height) of the photo is usually
 * distorted to match the object's size. Enabling this option will fix
 * this original aspect, and the way that the photo is fit into
 * the object's area
 *
 * @see elm_photo_aspect_fixed_get()
 *
 * @ingroup Photo
 */
EAPI void             elm_photo_aspect_fixed_set(Evas_Object *obj, Eina_Bool fixed);

/**
<<<<<<< HEAD
 * Get if the object fixes the original aspect ratio.
=======
 * Get if the object keeps the original aspect ratio.
>>>>>>> remotes/origin/upstream
 *
 * @param obj The photo object.
 * @return @c EINA_TRUE if the object keeps the original aspect, @c EINA_FALSE
 * otherwise.
 *
 * @ingroup Photo
 */
EAPI Eina_Bool        elm_photo_aspect_fixed_get(const Evas_Object *obj);

/**
 * @}
 */
