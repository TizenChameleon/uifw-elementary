/**
 * @defgroup Video Video
 *
 * @addtogroup Video
 * @{
 *
 * Elementary comes with two object that help design application that need
<<<<<<< HEAD
 * to display video. The main one, Elm_Video, display a video by using Emotion.
 * It does embedded the video inside an Edje object, so you can do some
 * animation depending on the video state change. It does also implement a
 * ressource management policy to remove this burden from the application writer.
 *
 * The second one, Elm_Player is a video player that need to be linked with and Elm_Video.
 * It take care of updating its content according to Emotion event and provide a
 * way to theme itself. It also does automatically raise the priority of the
 * linked Elm_Video so it will use the video decoder if available. It also does
 * activate the remember function on the linked Elm_Video object.
=======
 * to display video. 
 *
 * The first one, Elm_Video, display a video by using Emotion.
 * It embeds the video inside an Edje object, so you can do some
 * animation depending on the video state change. It also implements a
 * resource management policy to remove this burden from the application.
 *
 * The second one, 
 * Elm_Player is a video player that need to be linked with an Elm_Video. 
 * It take care of updating its content according to Emotion event and provide a
 * way to theme itself. It also automatically raises the priority of the
 * linked Elm_Video so it will use the video decoder, if available. It also
 * activates the "remember" function on the linked Elm_Video object.
>>>>>>> remotes/origin/upstream
 *
 * Signals that you can add callback for are :
 *
 * "forward,clicked" - the user clicked the forward button.
 * "info,clicked" - the user clicked the info button.
 * "next,clicked" - the user clicked the next button.
 * "pause,clicked" - the user clicked the pause button.
 * "play,clicked" - the user clicked the play button.
 * "prev,clicked" - the user clicked the prev button.
 * "rewind,clicked" - the user clicked the rewind button.
 * "stop,clicked" - the user clicked the stop button.
 *
<<<<<<< HEAD
 * Default contents parts of the player widget that you can use for are:
=======
 * Default content parts of the player widget that you can use for are:
>>>>>>> remotes/origin/upstream
 * @li "video" - A video of the player
 *
 */

/**
 * @brief Add a new Elm_Player object to the given parent Elementary (container) object.
 *
 * @param parent The parent object
 * @return a new player widget handle or @c NULL, on errors.
 *
 * This function inserts a new player widget on the canvas.
 *
 * @see elm_object_part_content_set()
 *
 * @ingroup Video
 */
EAPI Evas_Object         *elm_player_add(Evas_Object *parent);

/**
 * @brief Add a new Elm_Video object to the given parent Elementary (container) object.
 *
 * @param parent The parent object
 * @return a new video widget handle or @c NULL, on errors.
 *
 * This function inserts a new video widget on the canvas.
 *
 * @see elm_video_file_set()
<<<<<<< HEAD
 * @see elm_video_uri_set()
=======
>>>>>>> remotes/origin/upstream
 *
 * @ingroup Video
 */
EAPI Evas_Object         *elm_video_add(Evas_Object *parent);

/**
<<<<<<< HEAD
 * @brief Define the file that will be the video source.
 *
 * @param video The video object to define the file for.
 * @param filename The file to target.
 *
 * This function will explicitly define a filename as a source
 * for the video of the Elm_Video object.
 *
 * @see elm_video_uri_set()
 * @see elm_video_add()
 * @see elm_player_add()
 *
 * @ingroup Video
 */
EAPI void                 elm_video_file_set(Evas_Object *video, const char *filename);

/**
 * @brief Define the uri that will be the video source.
 *
 * @param video The video object to define the file for.
 * @param uri The uri to target.
 *
 * This function will define an uri as a source for the video of the
 * Elm_Video object. URI could be remote source of video, like http:// or local source
 * like for example WebCam who are most of the time v4l2:// (but that depend and
 * you should use Emotion API to request and list the available Webcam on your system).
 *
 * @see elm_video_file_set()
=======
 * @brief Define the file or URI that will be the video source.
 *
 * @param video The video object to define the file or URI for the video
 * of the Elm_Video object. 
 *
 * @param filename The file or URI to target.
 * Local files can be specified using file:// or by using full file paths.
 * URI could be remote source of video, like http:// or local source like 
 * WebCam (v4l2://). (You can use Emotion API to request and list 
 * the available Webcam on your system).
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise
 *
 * This function will explicitly define a file or URI as a source
 * for the video of the Elm_Video object.
 *
>>>>>>> remotes/origin/upstream
 * @see elm_video_add()
 * @see elm_player_add()
 *
 * @ingroup Video
 */
<<<<<<< HEAD
EAPI void                 elm_video_uri_set(Evas_Object *video, const char *uri);
=======
EAPI Eina_Bool            elm_video_file_set(Evas_Object *video, const char *filename);
>>>>>>> remotes/origin/upstream

/**
 * @brief Get the underlying Emotion object.
 *
 * @param video The video object to proceed the request on.
 * @return the underlying Emotion object.
 *
 * @ingroup Video
 */
EAPI Evas_Object         *elm_video_emotion_get(const Evas_Object *video);

/**
 * @brief Start to play the video
 *
 * @param video The video object to proceed the request on.
 *
 * Start to play the video and cancel all suspend state.
 *
 * @ingroup Video
 */
EAPI void                 elm_video_play(Evas_Object *video);

/**
 * @brief Pause the video
 *
 * @param video The video object to proceed the request on.
 *
 * Pause the video and start a timer to trigger suspend mode.
 *
 * @ingroup Video
 */
EAPI void                 elm_video_pause(Evas_Object *video);

/**
 * @brief Stop the video
 *
 * @param video The video object to proceed the request on.
 *
 * Stop the video and put the emotion in deep sleep mode.
 *
 * @ingroup Video
 */
EAPI void                 elm_video_stop(Evas_Object *video);

/**
 * @brief Is the video actually playing.
 *
 * @param video The video object to proceed the request on.
 * @return EINA_TRUE if the video is actually playing.
 *
 * You should consider watching event on the object instead of polling
 * the object state.
 *
 * @ingroup Video
 */
<<<<<<< HEAD
EAPI Eina_Bool            elm_video_is_playing(const Evas_Object *video);
=======
EAPI Eina_Bool            elm_video_is_playing_get(const Evas_Object *video);
>>>>>>> remotes/origin/upstream

/**
 * @brief Is it possible to seek inside the video.
 *
 * @param video The video object to proceed the request on.
 * @return EINA_TRUE if is possible to seek inside the video.
 *
 * @ingroup Video
 */
<<<<<<< HEAD
EAPI Eina_Bool            elm_video_is_seekable(const Evas_Object *video);
=======
EAPI Eina_Bool            elm_video_is_seekable_get(const Evas_Object *video);
>>>>>>> remotes/origin/upstream

/**
 * @brief Is the audio muted.
 *
 * @param video The video object to proceed the request on.
 * @return EINA_TRUE if the audio is muted.
 *
 * @ingroup Video
 */
EAPI Eina_Bool            elm_video_audio_mute_get(const Evas_Object *video);

/**
 * @brief Change the mute state of the Elm_Video object.
 *
 * @param video The video object to proceed the request on.
 * @param mute The new mute state.
 *
 * @ingroup Video
 */
EAPI void                 elm_video_audio_mute_set(Evas_Object *video, Eina_Bool mute);

/**
 * @brief Get the audio level of the current video.
 *
 * @param video The video object to proceed the request on.
 * @return the current audio level.
 *
 * @ingroup Video
 */
EAPI double               elm_video_audio_level_get(const Evas_Object *video);

/**
<<<<<<< HEAD
 * @brief Set the audio level of anElm_Video object.
=======
 * @brief Set the audio level of an Elm_Video object.
>>>>>>> remotes/origin/upstream
 *
 * @param video The video object to proceed the request on.
 * @param volume The new audio volume.
 *
 * @ingroup Video
 */
EAPI void                 elm_video_audio_level_set(Evas_Object *video, double volume);

<<<<<<< HEAD
EAPI double               elm_video_play_position_get(const Evas_Object *video);
EAPI void                 elm_video_play_position_set(Evas_Object *video, double position);
EAPI double               elm_video_play_length_get(const Evas_Object *video);
EAPI void                 elm_video_remember_position_set(Evas_Object *video, Eina_Bool remember);
EAPI Eina_Bool            elm_video_remember_position_get(const Evas_Object *video);
=======
/**
 * @brief Get the current position (in seconds) being played in the 
 * Elm_Video object.
 *
 * @param video The video object.
 * @return the time(in seconds) since the beginning of the media file.
 *
 * @ingroup Video
 */
EAPI double               elm_video_play_position_get(const Evas_Object *video);

/**
 * @brief Set the current position (in seconds) to be played in the 
 * Elm_Video object.
 *
 * @param video The video object.
 * @param position the time(in seconds) since the beginning of the media file.
 *
 * @ingroup Video
 */
EAPI void                 elm_video_play_position_set(Evas_Object *video, double position);
/**
 * @brief Get the total playing time (in seconds) of the Elm_Video object.
 *
 * @param video The video object.
 * @return the total duration(in seconds) of the media file.
 *
 * @ingroup Video
 */
EAPI double               elm_video_play_length_get(const Evas_Object *video);

/**
 * @brief Set whether the object can remember the last played position.
 *
 * @param video The video object.
 * @param remember the last played position of the Elm_Video object.
 *
 * @note This API only serves as indication. System support is required.
 *
 * @ingroup Video
 */
EAPI void                 elm_video_remember_position_set(Evas_Object *video, Eina_Bool remember);

/**
 * @brief Set whether the object can remember the last played position.
 *
 * @param video The video object.
 * @return whether the object remembers the last played position (EINA_TRUE) 
 * or not.
 *
 * @note This API only serves as indication. System support is required.
 *
 * @ingroup Video
 */
EAPI Eina_Bool            elm_video_remember_position_get(const Evas_Object *video);

/**
 * @brief Get the title (for ex. DVD title) from this emotion object.
 *
 * @param video The Elm_Video object.
 * @return A string containing the title.
 *
 * This function is only useful when playing a DVD.
 *
 * @note Don't change or free the string returned by this function.
 *
 * @ingroup Video
 */
>>>>>>> remotes/origin/upstream
EAPI const char          *elm_video_title_get(const Evas_Object *video);
/**
 * @}
 */
