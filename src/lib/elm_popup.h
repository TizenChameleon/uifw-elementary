   /* popup */
   typedef enum _Elm_Popup_Response
     {
        ELM_POPUP_RESPONSE_NONE = -1,
        ELM_POPUP_RESPONSE_TIMEOUT = -2,
        ELM_POPUP_RESPONSE_OK = -3,
        ELM_POPUP_RESPONSE_CANCEL = -4,
        ELM_POPUP_RESPONSE_CLOSE = -5
     } Elm_Popup_Response;

   typedef enum _Elm_Popup_Mode
     {
        ELM_POPUP_TYPE_NONE = 0,
        ELM_POPUP_TYPE_ALERT = (1 << 0)
     } Elm_Popup_Mode;

   typedef enum _Elm_Popup_Orient
     {
        ELM_POPUP_ORIENT_TOP,
        ELM_POPUP_ORIENT_CENTER,
        ELM_POPUP_ORIENT_BOTTOM,
        ELM_POPUP_ORIENT_LEFT,
        ELM_POPUP_ORIENT_RIGHT,
        ELM_POPUP_ORIENT_TOP_LEFT,
        ELM_POPUP_ORIENT_TOP_RIGHT,
        ELM_POPUP_ORIENT_BOTTOM_LEFT,
        ELM_POPUP_ORIENT_BOTTOM_RIGHT
     } Elm_Popup_Orient;

   /* smart callbacks called:
    * "response" - when ever popup is closed, this signal is sent with appropriate response id.".
    */

   EAPI Evas_Object *elm_popup_add(Evas_Object *parent);
   EAPI void         elm_popup_repeat_events_set(Evas_Object *obj, Eina_Bool repeat);
   EAPI Eina_Bool    elm_popup_repeat_events_get(Evas_Object *obj);
   EAPI void         elm_popup_desc_set(Evas_Object *obj, const char *text);
   EAPI const char  *elm_popup_desc_get(Evas_Object *obj);
   EAPI void         elm_popup_title_label_set(Evas_Object *obj, const char *text);
   EAPI const char  *elm_popup_title_label_get(Evas_Object *obj);
   EAPI void         elm_popup_title_icon_set(Evas_Object *obj, Evas_Object *icon);
   EAPI Evas_Object *elm_popup_title_icon_get(Evas_Object *obj);
   EAPI void         elm_popup_content_set(Evas_Object *obj, Evas_Object *content);
   EAPI Evas_Object *elm_popup_content_get(Evas_Object *obj);
   EAPI void         elm_popup_buttons_add(Evas_Object *obj,int no_of_buttons, const char *first_button_text,  ...);
   EAPI Evas_Object *elm_popup_with_buttons_add(Evas_Object *parent, const char *title, const char *desc_text,int no_of_buttons, const char *first_button_text, ... );
   EAPI void         elm_popup_timeout_set(Evas_Object *obj, double timeout);
   EAPI void         elm_popup_mode_set(Evas_Object *obj, Elm_Popup_Mode mode);
   EAPI void         elm_popup_response(Evas_Object *obj, int  response_id);
   EAPI void         elm_popup_orient_set(Evas_Object *obj, Elm_Popup_Orient orient);
   EAPI int          elm_popup_run(Evas_Object *obj);
