/**
 * @defgroup Photocam Photocam
 *
 * @image html img/widget/photocam/preview-00.png
 * @image latex img/widget/photocam/preview-00.eps
 *
 * This is a widget specifically for displaying high-resolution digital
 * camera photos giving speedy feedback (fast load), low memory footprint
 * and zooming and panning as well as fitting logic. It is entirely focused
 * on jpeg images, and takes advantage of properties of the jpeg format (via
 * evas loader features in the jpeg loader).
 *
 * Signals that you can add callbacks for are:
 * @li "clicked" - This is called when a user has clicked the photo without
 *                 dragging around.
 * @li "press" - This is called when a user has pressed down on the photo.
 * @li "longpressed" - This is called when a user has pressed down on the
 *                     photo for a long time without dragging around.
 * @li "clicked,double" - This is called when a user has double-clicked the
 *                        photo.
 * @li "load" - Photo load begins.
 * @li "loaded" - This is called when the image file load is complete for the
 *                first view (low resolution blurry version).
 * @li "load,detail" - Photo detailed data load begins.
 * @li "loaded,detail" - This is called when the image file load is complete
 *                      for the detailed image data (full resolution needed).
 * @li "zoom,start" - Zoom animation started.
 * @li "zoom,stop" - Zoom animation stopped.
 * @li "zoom,change" - Zoom changed when using an auto zoom mode.
 * @li "scroll" - the content has been scrolled (moved)
 * @li "scroll,anim,start" - scrolling animation has started
 * @li "scroll,anim,stop" - scrolling animation has stopped
 * @li "scroll,drag,start" - dragging the contents around has started
 * @li "scroll,drag,stop" - dragging the contents around has stopped
 *
 * @ref tutorial_photocam shows the API in action.
 * @{
 */

/**
 * @brief Types of zoom available.
 */
typedef enum
{
   ELM_PHOTOCAM_ZOOM_MODE_MANUAL = 0, /**< Zoom controlled normally by elm_photocam_zoom_set */
   ELM_PHOTOCAM_ZOOM_MODE_AUTO_FIT, /**< Zoom until photo fits in photocam */
   ELM_PHOTOCAM_ZOOM_MODE_AUTO_FILL, /**< Zoom until photo fills photocam */
<<<<<<< HEAD
   ELM_PHOTOCAM_ZOOM_MODE_AUTO_FIT_IN, /**< Unzoom until photo fits in photocam */
=======
   ELM_PHOTOCAM_ZOOM_MODE_AUTO_FIT_IN, /**< Zoom in until photo fits in photocam */
>>>>>>> remotes/origin/upstream
   ELM_PHOTOCAM_ZOOM_MODE_LAST
} Elm_Photocam_Zoom_Mode;

/**
 * @brief Add a new Photocam object
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 */
EAPI Evas_Object           *elm_photocam_add(Evas_Object *parent);

/**
 * @brief Set the photo file to be shown
 *
 * @param obj The photocam object
 * @param file The photo file
 * @return The return error (see EVAS_LOAD_ERROR_NONE, EVAS_LOAD_ERROR_GENERIC etc.)
 *
 * This sets (and shows) the specified file (with a relative or absolute
 * path) and will return a load error (same error that
 * evas_object_image_load_error_get() will return). The image will change and
 * adjust its size at this point and begin a background load process for this
 * photo that at some time in the future will be displayed at the full
 * quality needed.
 */
EAPI Evas_Load_Error        elm_photocam_file_set(Evas_Object *obj, const char *file);

/**
 * @brief Returns the path of the current image file
 *
 * @param obj The photocam object
 * @return Returns the path
 *
 * @see elm_photocam_file_set()
 */
EAPI const char            *elm_photocam_file_get(const Evas_Object *obj);

/**
 * @brief Set the zoom level of the photo
 *
 * @param obj The photocam object
 * @param zoom The zoom level to set
 *
 * This sets the zoom level. 1 will be 1:1 pixel for pixel. 2 will be 2:1
 * (that is 2x2 photo pixels will display as 1 on-screen pixel). 4:1 will be
 * 4x4 photo pixels as 1 screen pixel, and so on. The @p zoom parameter must
<<<<<<< HEAD
 * be greater than 0. It is usggested to stick to powers of 2. (1, 2, 4, 8,
=======
 * be greater than 0. It is suggested to stick to powers of 2. (1, 2, 4, 8,
>>>>>>> remotes/origin/upstream
 * 16, 32, etc.).
 */
EAPI void                   elm_photocam_zoom_set(Evas_Object *obj, double zoom);

/**
 * @brief Get the zoom level of the photo
 *
 * @param obj The photocam object
 * @return The current zoom level
 *
 * This returns the current zoom level of the photocam object. Note that if
 * you set the fill mode to other than ELM_PHOTOCAM_ZOOM_MODE_MANUAL
 * (which is the default), the zoom level may be changed at any time by the
<<<<<<< HEAD
 * photocam object itself to account for photo size and photocam viewpoer
=======
 * photocam object itself to account for photo size and photocam viewport
>>>>>>> remotes/origin/upstream
 * size.
 *
 * @see elm_photocam_zoom_set()
 * @see elm_photocam_zoom_mode_set()
 */
EAPI double                 elm_photocam_zoom_get(const Evas_Object *obj);

/**
 * @brief Set the zoom mode
 *
 * @param obj The photocam object
 * @param mode The desired mode
 *
 * This sets the zoom mode to manual or one of several automatic levels.
 * Manual (ELM_PHOTOCAM_ZOOM_MODE_MANUAL) means that zoom is set manually by
 * elm_photocam_zoom_set() and will stay at that level until changed by code
 * or until zoom mode is changed. This is the default mode. The Automatic
 * modes will allow the photocam object to automatically adjust zoom mode
 * based on properties. ELM_PHOTOCAM_ZOOM_MODE_AUTO_FIT) will adjust zoom so
 * the photo fits EXACTLY inside the scroll frame with no pixels outside this
<<<<<<< HEAD
 * area. ELM_PHOTOCAM_ZOOM_MODE_AUTO_FILL will be similar but ensure no
=======
 * region. ELM_PHOTOCAM_ZOOM_MODE_AUTO_FILL will be similar but ensure no
>>>>>>> remotes/origin/upstream
 * pixels within the frame are left unfilled.
 */
EAPI void                   elm_photocam_zoom_mode_set(Evas_Object *obj, Elm_Photocam_Zoom_Mode mode);

/**
 * @brief Get the zoom mode
 *
 * @param obj The photocam object
 * @return The current zoom mode
 *
 * This gets the current zoom mode of the photocam object.
 *
 * @see elm_photocam_zoom_mode_set()
 */
EAPI Elm_Photocam_Zoom_Mode elm_photocam_zoom_mode_get(const Evas_Object *obj);

/**
 * @brief Get the current image pixel width and height
 *
 * @param obj The photocam object
 * @param w A pointer to the width return
 * @param h A pointer to the height return
 *
 * This gets the current photo pixel width and height (for the original).
 * The size will be returned in the integers @p w and @p h that are pointed
 * to.
 */
EAPI void                   elm_photocam_image_size_get(const Evas_Object *obj, int *w, int *h);

/**
<<<<<<< HEAD
 * @brief Get the area of the image that is currently shown
=======
 * @brief Get the region of the image that is currently shown
>>>>>>> remotes/origin/upstream
 *
 * @param obj
 * @param x A pointer to the X-coordinate of region
 * @param y A pointer to the Y-coordinate of region
 * @param w A pointer to the width
 * @param h A pointer to the height
 *
 * @see elm_photocam_image_region_show()
 * @see elm_photocam_image_region_bring_in()
 */
<<<<<<< HEAD
EAPI void                   elm_photocam_region_get(const Evas_Object *obj, int *x, int *y, int *w, int *h);

/**
 * @brief Set the viewed portion of the image
=======
EAPI void                   elm_photocam_image_region_get(const Evas_Object *obj, int *x, int *y, int *w, int *h);

/**
 * @brief Set the viewed region of the image
>>>>>>> remotes/origin/upstream
 *
 * @param obj The photocam object
 * @param x X-coordinate of region in image original pixels
 * @param y Y-coordinate of region in image original pixels
 * @param w Width of region in image original pixels
 * @param h Height of region in image original pixels
 *
 * This shows the region of the image without using animation.
 */
EAPI void                   elm_photocam_image_region_show(Evas_Object *obj, int x, int y, int w, int h);

/**
 * @brief Bring in the viewed portion of the image
 *
 * @param obj The photocam object
 * @param x X-coordinate of region in image original pixels
 * @param y Y-coordinate of region in image original pixels
 * @param w Width of region in image original pixels
 * @param h Height of region in image original pixels
 *
 * This shows the region of the image using animation.
 */
EAPI void                   elm_photocam_image_region_bring_in(Evas_Object *obj, int x, int y, int w, int h);

/**
 * @brief Set the paused state for photocam
 *
 * @param obj The photocam object
 * @param paused The pause state to set
 *
 * This sets the paused state to on(EINA_TRUE) or off (EINA_FALSE) for
 * photocam. The default is off. This will stop zooming using animation on
<<<<<<< HEAD
 * zoom levels changes and change instantly. This will stop any existing
=======
 * zoom level changes and change instantly. This will stop any existing
>>>>>>> remotes/origin/upstream
 * animations that are running.
 */
EAPI void                   elm_photocam_paused_set(Evas_Object *obj, Eina_Bool paused);

/**
 * @brief Get the paused state for photocam
 *
 * @param obj The photocam object
 * @return The current paused state
 *
 * This gets the current paused state for the photocam object.
 *
 * @see elm_photocam_paused_set()
 */
EAPI Eina_Bool              elm_photocam_paused_get(const Evas_Object *obj);

/**
 * @brief Get the internal low-res image used for photocam
 *
 * @param obj The photocam object
 * @return The internal image object handle, or NULL if none exists
 *
 * This gets the internal image object inside photocam. Do not modify it. It
 * is for inspection only, and hooking callbacks to. Nothing else. It may be
 * deleted at any time as well.
 */
EAPI Evas_Object           *elm_photocam_internal_image_get(const Evas_Object *obj);

/**
 * @brief Set the photocam scrolling bouncing.
 *
 * @param obj The photocam object
<<<<<<< HEAD
 * @param h_bounce bouncing for horizontal
 * @param v_bounce bouncing for vertical
=======
 * @param h_bounce set this to @c EINA_TRUE for horizontal bouncing
 * @param v_bounce set this to @c EINA_TRUE for vertical bouncing
>>>>>>> remotes/origin/upstream
 */
EAPI void                   elm_photocam_bounce_set(Evas_Object *obj, Eina_Bool h_bounce, Eina_Bool v_bounce);

/**
 * @brief Get the photocam scrolling bouncing.
 *
 * @param obj The photocam object
<<<<<<< HEAD
 * @param h_bounce bouncing for horizontal
 * @param v_bounce bouncing for vertical
=======
 * @param h_bounce horizontal bouncing
 * @param v_bounce vertical bouncing
>>>>>>> remotes/origin/upstream
 *
 * @see elm_photocam_bounce_set()
 */
EAPI void                   elm_photocam_bounce_get(const Evas_Object *obj, Eina_Bool *h_bounce, Eina_Bool *v_bounce);

/**
<<<<<<< HEAD
=======
 * @brief Set the gesture state for photocam.
 *
 * @param obj The photocam object
 * @param gesture The gesture state to set
 *
 * This sets the gesture state to on(EINA_TRUE) or off (EINA_FALSE) for
 * photocam. The default is off. This will start multi touch zooming.
 */
EAPI void		    elm_photocam_gesture_enabled_set(Evas_Object *obj, Eina_Bool gesture);

/**
 * @brief Get the gesture state for photocam.
 *
 * @param obj The photocam object
 * @return The current gesture state
 *
 * This gets the current gesture state for the photocam object.
 *
 * @see elm_photocam_gesture_enabled_set()
 */
EAPI Eina_Bool		    elm_photocam_gesture_enabled_get(const Evas_Object *obj);
/**
>>>>>>> remotes/origin/upstream
 * @}
 */
