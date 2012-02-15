   /* Dayselector */
   typedef enum
     {
        ELM_DAYSELECTOR_SUN,
        ELM_DAYSELECTOR_MON,
        ELM_DAYSELECTOR_TUE,
        ELM_DAYSELECTOR_WED,
        ELM_DAYSELECTOR_THU,
        ELM_DAYSELECTOR_FRI,
        ELM_DAYSELECTOR_SAT
     } Elm_DaySelector_Day;

   EAPI Evas_Object *elm_dayselector_add(Evas_Object *parent);
   EAPI Eina_Bool    elm_dayselector_check_state_get(Evas_Object *obj, Elm_DaySelector_Day day);
   EAPI void         elm_dayselector_check_state_set(Evas_Object *obj, Elm_DaySelector_Day day, Eina_Bool checked);

