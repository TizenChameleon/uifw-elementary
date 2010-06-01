typedef enum _Elm_Very_Smart_Scroller_Policy
{
	ELM_VERY_SMART_SCROLLER_POLICY_AUTO,
	ELM_VERY_SMART_SCROLLER_POLICY_ON,
	ELM_VERY_SMART_SCROLLER_POLICY_OFF
}

Elm_Very_Smart_Scroller_Policy;
Evas_Object *hor_elm_smart_scroller_add             (Evas *evas);
void hor_elm_smart_scroller_child_set               (Evas_Object *obj, Evas_Object *child);
void hor_elm_smart_scroller_extern_pan_set          (Evas_Object *obj, Evas_Object *pan, void (*pan_set) (Evas_Object *obj, Evas_Coord x, Evas_Coord y), void (*pan_get) (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y), void (*pan_max_get) (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y), void (*pan_child_size_get) (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y));
void hor_elm_smart_scroller_custom_edje_file_set    (Evas_Object *obj, char *file, char *group);
void hor_elm_smart_scroller_child_pos_set           (Evas_Object *obj, Evas_Coord x, Evas_Coord y);
void hor_elm_smart_scroller_child_pos_get           (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
void hor_elm_smart_scroller_child_region_show       (Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h);
void hor_elm_smart_scroller_child_viewport_size_get (Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);
void hor_elm_smart_scroller_step_size_set           (Evas_Object *obj, Evas_Coord x, Evas_Coord y);
void hor_elm_smart_scroller_step_size_get           (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
void hor_elm_smart_scroller_page_size_set           (Evas_Object *obj, Evas_Coord x, Evas_Coord y);
void hor_elm_smart_scroller_page_size_get           (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
void hor_elm_smart_scroller_policy_set              (Evas_Object *obj, Elm_Very_Smart_Scroller_Policy hbar, Elm_Very_Smart_Scroller_Policy vbar);
void hor_elm_smart_scroller_policy_get              (Evas_Object *obj, Elm_Very_Smart_Scroller_Policy *hbar, Elm_Very_Smart_Scroller_Policy *vbar);
Evas_Object *hor_elm_smart_scroller_edje_object_get (Evas_Object *obj);
void hor_elm_smart_scroller_single_dir_set          (Evas_Object *obj, Eina_Bool single_dir);
Eina_Bool hor_elm_smart_scroller_single_dir_get     (Evas_Object *obj);
void hor_elm_smart_scroller_theme_set               (Evas_Object *parent, Evas_Object *obj, const char *clas, const char *group, const char *style);
void hor_elm_smart_scroller_hold_set                (Evas_Object *obj, Eina_Bool hold);
void hor_elm_smart_scroller_freeze_set              (Evas_Object *obj, Eina_Bool freeze);
void hor_elm_smart_scroller_bounce_allow_set        (Evas_Object *obj, Eina_Bool horiz, Eina_Bool vert);
void hor_elm_smart_scroller_paging_set              (Evas_Object *obj, double pagerel_h, double pagerel_v, Evas_Coord pagesize_h, Evas_Coord pagesize_v);
void hor_elm_smart_scroller_region_bring_in         (Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h);
  
