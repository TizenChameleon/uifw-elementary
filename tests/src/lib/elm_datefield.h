   /* datefield */

   typedef enum _Elm_Datefield_ItemType
     {
        ELM_DATEFIELD_YEAR = 0,
        ELM_DATEFIELD_MONTH,
        ELM_DATEFIELD_DATE,
        ELM_DATEFIELD_HOUR,
        ELM_DATEFIELD_MINUTE,
        ELM_DATEFIELD_AMPM
     } Elm_Datefield_ItemType;

   EAPI Evas_Object *elm_datefield_add(Evas_Object *parent);
   EAPI void         elm_datefield_format_set(Evas_Object *obj, const char *fmt);
   EAPI char        *elm_datefield_format_get(const Evas_Object *obj);
   EAPI void         elm_datefield_item_enabled_set(Evas_Object *obj, Elm_Datefield_ItemType itemtype, Eina_Bool enable);
   EAPI Eina_Bool    elm_datefield_item_enabled_get(const Evas_Object *obj, Elm_Datefield_ItemType itemtype);
   EAPI void         elm_datefield_item_value_set(Evas_Object *obj, Elm_Datefield_ItemType itemtype, int value);
   EAPI int          elm_datefield_item_value_get(const Evas_Object *obj, Elm_Datefield_ItemType itemtype);
   EAPI void         elm_datefield_item_min_set(Evas_Object *obj, Elm_Datefield_ItemType itemtype, int value, Eina_Bool abs_limit);
   EAPI int          elm_datefield_item_min_get(const Evas_Object *obj, Elm_Datefield_ItemType itemtype);
   EAPI Eina_Bool    elm_datefield_item_min_is_absolute(const Evas_Object *obj, Elm_Datefield_ItemType itemtype);
   EAPI void         elm_datefield_item_max_set(Evas_Object *obj, Elm_Datefield_ItemType itemtype, int value, Eina_Bool abs_limit);
   EAPI int          elm_datefield_item_max_get(const Evas_Object *obj, Elm_Datefield_ItemType itemtype);
   EAPI Eina_Bool    elm_datefield_item_max_is_absolute(const Evas_Object *obj, Elm_Datefield_ItemType itemtype);

   /* smart callbacks called:
   * "changed" - when datefield value is changed, this signal is sent.
   */

