   /**
    * @defgroup Searchbar Searchbar
    * @addtogroup Searchbar
    * @{
    * @ingroup Elementary
    *
    * This is Searchbar.
    * It can contain a simple entry and button object.
    */

   /**
    * Add a new searchbar to the parent
    * @param parent The parent object
    * @return The new object or NULL if it cannot be created
    */
   EAPI Evas_Object *elm_searchbar_add(Evas_Object *parent);
   /**
    * set the text of entry
    *
    * @param obj The searchbar object
    * @return void
    */
   EAPI void         elm_searchbar_text_set(Evas_Object *obj, const char *entry);
   /**
    * get the text of entry
    *
    * @param obj The searchbar object
    * @return string pointer of entry
    */
   EAPI const char  *elm_searchbar_text_get(Evas_Object *obj);
   /**
    * get the pointer of entry
    *
    * @param obj The searchbar object
    * @return the entry object
    */
   EAPI Evas_Object *elm_searchbar_entry_get(Evas_Object *obj);
   /**
    * get the pointer of editfield
    *
    * @param obj The searchbar object
    * @return the editfield object
    */
   EAPI Evas_Object *elm_searchbar_editfield_get(Evas_Object *obj);
   /**
    * set the cancel button animation flag
    *
    * @param obj The searchbar object
    * @param cancel_btn_ani_flag The flag of animating cancen button or not
    * @return void
    */
   EAPI void         elm_searchbar_cancel_button_animation_set(Evas_Object *obj, Eina_Bool cancel_btn_ani_flag);
   /**
    * set the cancel button show mode
    *
    * @param obj The searchbar object
    * @param visible The flag of cancen button show or not
    * @return void
    */
   EAPI void         elm_searchbar_cancel_button_set(Evas_Object *obj, Eina_Bool visible);
   /**
    * clear searchbar status
    *
    * @param obj The searchbar object
    * @return void
    */
   EAPI void         elm_searchbar_clear(Evas_Object *obj);
   /**
    * set the searchbar boundary rect mode(with bg rect) set
    *
    * @param obj The searchbar object
    * @param boundary The present flag of boundary rect or not
    * @return void
    */
   EAPI void         elm_searchbar_boundary_rect_set(Evas_Object *obj, Eina_Bool boundary);
   /**
    * @}
    */

