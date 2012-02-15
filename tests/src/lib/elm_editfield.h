   /* editfield */
   EAPI Evas_Object *elm_editfield_add(Evas_Object *parent);
   EAPI void         elm_editfield_label_set(Evas_Object *obj, const char *label);
   EAPI const char  *elm_editfield_label_get(Evas_Object *obj);
   EAPI void         elm_editfield_guide_text_set(Evas_Object *obj, const char *text);
   EAPI const char  *elm_editfield_guide_text_get(Evas_Object *obj);
   EAPI Evas_Object *elm_editfield_entry_get(Evas_Object *obj);
   EAPI void         elm_editfield_entry_single_line_set(Evas_Object *obj, Eina_Bool single_line);
   EAPI Eina_Bool    elm_editfield_entry_single_line_get(Evas_Object *obj);
   EAPI void         elm_editfield_eraser_set(Evas_Object *obj, Eina_Bool visible);
   EAPI Eina_Bool    elm_editfield_eraser_get(Evas_Object *obj);
   /* smart callbacks called:
    * "clicked" - when an editfield is clicked
    * "unfocused" - when an editfield is unfocused
    */

