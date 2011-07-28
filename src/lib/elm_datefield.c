#include <Elementary.h>
#include "elm_priv.h"
#include <langinfo.h>

/**
 * @defgroup Datefield Datefield
 * @ingroup Elementary
 *
 * This is a date editfield. it is used to input date and time using keypad
 */

typedef struct _Widget_Data Widget_Data;

enum
{
  DATE_FORMAT_YYMMDD,
  DATE_FORMAT_YYDDMM,
  DATE_FORMAT_MMYYDD,
  DATE_FORMAT_MMDDYY,
  DATE_FORMAT_DDYYMM,
  DATE_FORMAT_DDMMYY,
  DATE_FORMAT_MAX
};

enum
{
  ENTRY_YEAR,
  ENTRY_MON,
  ENTRY_DAY,
  ENTRY_HOUR,
  ENTRY_MIN
};

#define MONTH_MAXIMUM 12
#define HOUR_24H_MAXIMUM 23
#define HOUR_12H_MAXIMUM 12
#define MIN_MAXIMUM 59
#define YEAR_MAX_LENGTH 4

struct _Widget_Data
{
   Evas_Object *base;
   Evas_Object *time_ampm;
   Evas_Object *ctxpopup;
   Evas_Object *diskselector;
   unsigned int layout;
   int year, month, day, hour, min;
   int y_max, m_max, d_max;
   int y_min, m_min, d_min;
   int date_format,date_focusedpart;
   Eina_Bool pm:1;
   Eina_Bool time_mode:1;
   Eina_Bool format_exists:1;
   Eina_Bool ctxpopup_show:1;
};

static const char *widtype = NULL;

static void _del_hook(Evas_Object *obj);
static void _on_focus_hook(void *data __UNUSED__, Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static void _datefield_resize_cb(void *data, Evas *e __UNUSED__,
                                 Evas_Object *obj, void *event_info __UNUSED__);
static void _ampm_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _signal_rect_mouse_down(void *data, Evas_Object *obj __UNUSED__,
                                    const char *emission __UNUSED__,
                                    const char *source);
static void _diskselector_cb(void *data, Evas_Object *obj __UNUSED__,
                             void *event_info);
static void _datefield_focus_set(Evas_Object *data);
static int _maximum_day_get(int year, int month);
static int _check_date_boundary(Evas_Object *obj, int num, int flag);
static char* _get_i18n_string(Evas_Object *obj, nl_item item);
static void _date_update(Evas_Object *obj);

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return ;
   free(wd);
}

static void
_on_focus_hook(void *data __UNUSED__, Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd || !wd->base) return ;
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   char sig[32] = {0,};
   char buf[1024];

   if (!wd || !wd->base) return;

   if (wd->layout == ELM_DATEFIELD_LAYOUT_DATEANDTIME)
      _elm_theme_object_set(obj, wd->base, "datefield", "dateandtime",
                            elm_widget_style_get(obj));
   else if (wd->layout == ELM_DATEFIELD_LAYOUT_DATE)
      _elm_theme_object_set(obj, wd->base, "datefield", "date",
                            elm_widget_style_get(obj));
   else if (wd->layout == ELM_DATEFIELD_LAYOUT_TIME)
      _elm_theme_object_set(obj, wd->base, "datefield", "time",
                            elm_widget_style_get(obj));

   if (wd->time_ampm)
     {
        edje_object_part_unswallow(wd->base,wd->time_ampm);
        evas_object_del(wd->time_ampm);
        wd->time_ampm = NULL;
   }
   if ((wd->layout == ELM_DATEFIELD_LAYOUT_DATEANDTIME
                  || wd->layout == ELM_DATEFIELD_LAYOUT_TIME) && wd->time_mode)
     {
        wd->time_ampm = elm_button_add(obj);
        elm_widget_sub_object_add(obj, wd->time_ampm);
        edje_object_part_swallow(wd->base, "elm.swallow.time.ampm",
                                 wd->time_ampm);
        snprintf(buf,sizeof(buf),"datefield.ampm/%s",elm_widget_style_get(obj));
        elm_object_style_set(wd->time_ampm, buf);
        evas_object_size_hint_weight_set(wd->time_ampm, EVAS_HINT_EXPAND,
                                         EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(wd->time_ampm, EVAS_HINT_FILL,
                                        EVAS_HINT_FILL);
        evas_object_smart_callback_add(wd->time_ampm, "clicked",
                                       _ampm_clicked_cb, obj);
     }

   edje_object_scale_set(wd->base,elm_widget_scale_get(obj)*_elm_config->scale);

   //set date format
   if (wd->format_exists)
     sprintf(sig, "elm,state,format,%s", elm_datefield_date_format_get(obj));
   else
     {
        char *str = _get_i18n_string(obj, D_FMT);
        if (str)
          {
             if (!strcmp(str, "yymmdd")) wd->date_format = DATE_FORMAT_YYMMDD;
             else if (!strcmp(str, "yyddmm"))
                wd->date_format = DATE_FORMAT_YYDDMM;
             else if (!strcmp(str, "mmyydd"))
                wd->date_format = DATE_FORMAT_MMYYDD;
             else if (!strcmp(str, "mmddyy"))
                wd->date_format = DATE_FORMAT_MMDDYY;
             else if (!strcmp(str, "ddyymm"))
                wd->date_format = DATE_FORMAT_DDYYMM;
             else if (!strcmp(str, "ddmmyy"))
                wd->date_format = DATE_FORMAT_DDMMYY;
             sprintf(sig, "elm,state,format,%s",str);
             free(str);
          }
     }
   edje_object_signal_emit(wd->base, sig, "elm");

   _date_update(obj);
   _sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1;

   edje_object_size_min_calc(wd->base, &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, -1, -1);
}

static void
_datefield_resize_cb(void *data,Evas *e __UNUSED__, Evas_Object *obj __UNUSED__,
                     void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd || !wd->base) return ;

   Evas_Object *disk = elm_ctxpopup_content_unset(wd->ctxpopup);
   if (disk) evas_object_del(disk);
   if (wd->ctxpopup_show)
     wd->ctxpopup_show = EINA_FALSE;
   evas_object_hide(wd->ctxpopup);
}

static void
_ctxpopup_dismissed_cb(void *data, Evas_Object *obj __UNUSED__,
                       void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd || !wd->base) return ;

   Evas_Object *disk = elm_ctxpopup_content_unset(wd->ctxpopup);
   if (disk) evas_object_del(disk);
   if (wd->ctxpopup_show)
     wd->ctxpopup_show = EINA_FALSE;

   switch (wd->date_focusedpart)
     {
      case ENTRY_YEAR:
        edje_object_signal_emit(wd->base, "elm,state,year,focus,out", "elm");
        break;
      case ENTRY_MON:
        edje_object_signal_emit(wd->base, "elm,state,month,focus,out", "elm");
        break;
      case ENTRY_DAY:
        edje_object_signal_emit(wd->base, "elm,state,day,focus,out", "elm");
        break;
      case ENTRY_HOUR:
        edje_object_signal_emit(wd->base, "elm,state,hour,focus,out", "elm");
        break;
      case ENTRY_MIN:
        edje_object_signal_emit(wd->base, "elm,state,min,focus,out", "elm");
        break;
     }
}

static void
_ampm_clicked_cb(void *data, Evas_Object *obj __UNUSED__,
                 void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   char *str;

   if (!wd || !wd->base) return ;

   wd->pm = !wd->pm;
   if (wd->pm)
     {
   str = _get_i18n_string(data, PM_STR);
   if (str)
     {
         elm_object_text_set(wd->time_ampm, str);
        free(str);
     }
        wd->hour += HOUR_12H_MAXIMUM;
     }
   else
     {
   str = _get_i18n_string(data, AM_STR);
   if (str)
     {
         elm_object_text_set(wd->time_ampm, str);
        free(str);
     }
        wd->hour -= HOUR_12H_MAXIMUM;
     }
   evas_object_smart_callback_call(data, "changed", NULL);
}

static void
_signal_rect_mouse_down(void *data, Evas_Object *obj __UNUSED__,
                        const char *emission __UNUSED__, const char *source)
{
   Widget_Data *wd = elm_widget_data_get(data);

   if (!wd) return;

   if (!strcmp(source, "elm.rect.date.year.over"))
      wd->date_focusedpart = ENTRY_YEAR;
   else if (!strcmp(source, "elm.rect.date.month.over"))
      wd->date_focusedpart = ENTRY_MON;
   else if (!strcmp(source, "elm.rect.date.day.over"))
      wd->date_focusedpart = ENTRY_DAY;
   else if (!strcmp(source, "elm.rect.time.hour.over"))
      wd->date_focusedpart = ENTRY_HOUR;
   else if (!strcmp(source, "elm.rect.time.min.over"))
     wd->date_focusedpart = ENTRY_MIN;

   _datefield_focus_set(data);
}

static void
_diskselector_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info)
{
    const char *label = elm_diskselector_item_label_get(
                        (Elm_Diskselector_Item *) event_info);
    Widget_Data *wd = elm_widget_data_get(data);
    int i=0, mon = 0, hour =0;

    if (!wd || !wd->base) return;

    if (label)
      {
        if ((wd->date_focusedpart == ENTRY_YEAR) && (wd->year!=atoi(label)))
          {
             wd->year = _check_date_boundary(data, atoi(label), ENTRY_YEAR);
             edje_object_signal_emit(wd->base,"elm,state,year,focus,out","elm");
             _date_update(data);
          }
        else if (wd->date_focusedpart == ENTRY_MON)
          {
             char *month_list[] = {
                             E_("Jan"), E_("Feb"), E_("Mar"), E_("Apr"),
                             E_("May"), E_("Jun"), E_("Jul"), E_("Aug"),
                             E_("Sep"), E_("Oct"), E_("Nov"), E_("Dec"),
             };
             for (i=0; i <12; i++)
               {
                   if (!(strcmp(month_list[i],label)))
                     mon = _check_date_boundary(data, i+1, ENTRY_MON);
               }
             if (wd->month != mon)
               {
                  wd->month = mon;
                  edje_object_signal_emit(wd->base, "elm,state,month,focus,out",
                                          "elm");
                  _date_update(data);
               }
          }
        else if ((wd->date_focusedpart == ENTRY_DAY) && (wd->day!=atoi(label)))
          {
             wd->day = _check_date_boundary(data, atoi(label), ENTRY_DAY);
             edje_object_signal_emit(wd->base,"elm,state,day,focus,out", "elm");
             _date_update(data);
          }
        else if (wd->date_focusedpart == ENTRY_HOUR)
          {
             if ((wd->hour > 12)&& (wd->time_mode)&& (wd->pm))
               hour = wd->hour - HOUR_12H_MAXIMUM;
             else
               hour = wd->hour;
             if (hour!=atoi(label))
               {
                  wd->hour = atoi(label);
                  edje_object_signal_emit(wd->base, "elm,state,hour,focus,out",
                                          "elm");
                  _date_update(data);
               }
          }
        else if ((wd->date_focusedpart == ENTRY_MIN) && (wd->min!=atoi(label)))
          {
             wd->min = atoi(label);
             edje_object_signal_emit(wd->base,"elm,state,min,focus,out", "elm");
             _date_update(data);
          }
     }
}

static void
_datefield_focus_set(Evas_Object *data)
{
   Elm_Diskselector_Item *item = NULL;
   Evas_Object *diskselector, *disk, *edj_part = NULL;
   const char *item_list[138], *value = NULL;
   int idx, count_start = 0, count_end = 0;
   Evas_Coord x, y, w, h;

   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd || !wd->base) return;

   diskselector = elm_diskselector_add(elm_widget_top_get(data));
   elm_object_style_set(diskselector, "extended/timepicker");
   evas_object_size_hint_weight_set(diskselector, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(diskselector, EVAS_HINT_FILL,EVAS_HINT_FILL);
   elm_diskselector_display_item_num_set(diskselector, 8);
   elm_object_focus_allow_set(diskselector, EINA_FALSE);
   elm_diskselector_side_label_lenght_set(diskselector, 4);

   char *month_list[] = {
               E_("Jan"), E_("Feb"), E_("Mar"), E_("Apr"), E_("May"), E_("Jun"),
               E_("Jul"), E_("Aug"), E_("Sep"), E_("Oct"), E_("Nov"), E_("Dec"),
    };

   if (wd->date_focusedpart == ENTRY_YEAR)
     {
        edje_object_signal_emit(wd->base, "elm,state,year,focus,in", "elm");
        value =(char *)edje_object_part_text_get(wd->base,"elm.text.date.year");
        edj_part = (Evas_Object *)edje_object_part_object_get(wd->base,
                                                   "elm.rect.date.year.over");
        count_start = wd->y_min;
        if (wd->y_max > wd->y_min)
           count_end = wd->y_max ;
        else
           count_end = 2037 ;
        //Maximum limit is set for making it compatible with Calendar widget.
     }
   else if (wd->date_focusedpart == ENTRY_MON)
     {
        edje_object_signal_emit(wd->base, "elm,state,month,focus,in", "elm");
        value=(char *)edje_object_part_text_get(wd->base,"elm.text.date.month");
        edj_part = (Evas_Object *)edje_object_part_object_get(wd->base,
                                                   "elm.rect.date.month.over");
        count_start = 0;
        count_end = MONTH_MAXIMUM - 1;
     }
   else if (wd->date_focusedpart == ENTRY_DAY)
     {
        edje_object_signal_emit(wd->base, "elm,state,day,focus,in", "elm");
        value = (char *)edje_object_part_text_get(wd->base,"elm.text.date.day");
        edj_part = (Evas_Object *)edje_object_part_object_get(wd->base,
                                                   "elm.rect.date.day.over");
        count_start = 1;
        count_end = _maximum_day_get(wd->year, wd->month);
     }
   else if (wd->date_focusedpart == ENTRY_HOUR)
     {
        edje_object_signal_emit(wd->base, "elm,state,hour,focus,in", "elm");
        value =(char *)edje_object_part_text_get(wd->base,"elm.text.time.hour");
        edj_part = (Evas_Object *)edje_object_part_object_get(wd->base,
                                                   "elm.rect.time.hour.over");
        if (wd->time_mode)
          {
             count_start = 1;
             count_end = HOUR_12H_MAXIMUM ;
          }
        else
          {
             count_start = 0;
             count_end = HOUR_24H_MAXIMUM ;
          }
     }
   else if (wd->date_focusedpart == ENTRY_MIN)
     {
        edje_object_signal_emit(wd->base, "elm,state,min,focus,in", "elm");
        value = (char *)edje_object_part_text_get(wd->base,"elm.text.time.min");
        edj_part = (Evas_Object *)edje_object_part_object_get(wd->base,
                                                   "elm.rect.time.min.over");
        count_start = 0;
        count_end = MIN_MAXIMUM;
     }
   if (wd->ctxpopup_show) return;
   for (idx=count_start; idx<= count_end; idx++)
     {
        char str[5];
        if (wd->date_focusedpart == ENTRY_MON)
          snprintf(str, sizeof(str), month_list[idx]);
        else
          snprintf(str, sizeof(str), "%02d", idx);
        item_list[idx] = eina_stringshare_add(str);
        if (strcmp(value, item_list[idx]) == 0)
          item = elm_diskselector_item_append(diskselector,item_list[idx],NULL,
                                              _diskselector_cb, data);
        else
          elm_diskselector_item_append(diskselector, item_list[idx], NULL,
                                       _diskselector_cb, data);
        eina_stringshare_del(item_list[idx]);
     }
   elm_diskselector_round_set(diskselector, EINA_TRUE);
   if(item != NULL) elm_diskselector_item_selected_set(item, EINA_TRUE);

   disk = elm_ctxpopup_content_unset(wd->ctxpopup);
   if (disk) evas_object_del(disk);
   elm_ctxpopup_content_set(wd->ctxpopup, diskselector);
   evas_object_show(wd->ctxpopup);
   wd->ctxpopup_show = EINA_TRUE;
   evas_object_geometry_get(edj_part, &x, &y, &w, &h);
   evas_object_move(wd->ctxpopup, (x+w/2), (y+h) );
}

static int
_maximum_day_get(int year, int month)
{
   int day_of_month = 0;
   if (year == 0 || month == 0) return 0;

   switch (month)
     {
      case 4:
      case 6:
      case 9:
      case 11:
        day_of_month = 30;
        break;
      case 2:
        {
           if ((!(year % 4) && (year % 100)) || !(year % 400))
             day_of_month = 29;
           else
             day_of_month = 28;
        }
        break;
      default:
        day_of_month = 31;
        break;
     }

   return day_of_month;
}

static int
_check_date_boundary(Evas_Object *obj, int num, int flag)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (flag == ENTRY_YEAR)
     {
        if ((num > wd->y_max)&&(wd->y_max > wd->y_min)) num = wd->y_max;
        else if (num < wd->y_min) num = wd->y_min;
        return num;
     }

   else if (flag == ENTRY_MON)
     {
        if (wd->year == wd->y_max && num > wd->m_max) num = wd->m_max;
        else if (wd->year == wd->y_min && num < wd->m_min) num = wd->m_min;
        else if (num > MONTH_MAXIMUM) num = MONTH_MAXIMUM;
        else if (num <= 0) num = 1;
        return num;
     }

   else if (flag == ENTRY_DAY)
     {
        int day_of_month = _maximum_day_get(wd->year, wd->month);
        if (wd->year == wd->y_max && wd->month == wd->m_max && num > wd->d_max)
          num = wd->d_max;
        else if (wd->year == wd->y_min && wd->month == wd->m_min
                 && num < wd->d_min) num = wd->d_min;
        else if (num > day_of_month) num = day_of_month;
        else if (num <= 0) num = 1;
        return num;
     }
   return num;
}

static char*
_get_i18n_string(Evas_Object *obj, nl_item item)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   const char *fmt;
   char *str = NULL;
   int i = 0, j = 0;

   if (!wd) return NULL;

   fmt = nl_langinfo(item);
   if (!fmt) return NULL;

   switch (item)
     {
      case D_FMT:
    str = calloc(7, sizeof(char));
    while (fmt[i])
      {
         if (fmt[i] == '%' && fmt[i+1])
      {
         i++;
         switch (fmt[i])
           {
            case 'Y': case 'M': case 'D': case 'y': case 'm': case 'd':
          str[j++] = tolower(fmt[i]);
          str[j++] = tolower(fmt[i]);
          break;
           }
      }
         i++;
      }
    return str;
      case AM_STR:
      case PM_STR:
    if (strlen(fmt) > 0)
      {
         str = calloc(strlen(fmt)+1, sizeof(char));
         strcpy(str, fmt);
      }
    else
      {
         str = calloc(3, sizeof(char));
         if (item == AM_STR) strcpy(str, "AM");
         else if (item == PM_STR) strcpy(str, "PM");
      }
    return str;
      case ABMON_1: case ABMON_2: case ABMON_3: case ABMON_4: case ABMON_5:
      case ABMON_6: case ABMON_7: case ABMON_8: case ABMON_9: case ABMON_10:
      case ABMON_11: case ABMON_12:
    str = calloc(strlen(fmt)+1, sizeof(char));
    while (fmt[i])
      {
         str[j++] = fmt[i];
         if (fmt[i] >= '1' && fmt[i] <= '9')
      {
         if (fmt[i+1] >= '1' && fmt[i+1] <= '9')
           str[j] = fmt[i+1];
         break;
      }
         i++;
      }
    return str;
     }
   return NULL;
}

static void
_date_update(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Object *diskselector;
   char str[YEAR_MAX_LENGTH+1] = {0,};
   char *i18n_str;

   if (!wd || !wd->base) return;

   sprintf(str, "%d", wd->year);
   edje_object_part_text_set(wd->base, "elm.text.date.year", str);

   i18n_str = _get_i18n_string(obj, ABMON_1+wd->month-1);
   if (i18n_str)
     {
        edje_object_part_text_set(wd->base, "elm.text.date.month", i18n_str);
        free(i18n_str);
     }

   sprintf(str, "%02d", wd->day);
   edje_object_part_text_set(wd->base, "elm.text.date.day", str);

   if (!wd->time_mode) //24 mode
     sprintf(str, "%02d", wd->hour);
   else
     {
        if (wd->hour >= HOUR_12H_MAXIMUM)
          {
             wd->pm = EINA_TRUE;
             i18n_str = _get_i18n_string(obj, PM_STR);
             if ((i18n_str)&&(wd->time_ampm))
               {
                  elm_object_text_set(wd->time_ampm, i18n_str);
                  free(i18n_str);
               }
          }
        else
          {
             wd->pm = EINA_FALSE;
             i18n_str = _get_i18n_string(obj, AM_STR);
             if ((i18n_str)&&(wd->time_ampm))
               {
                  elm_object_text_set(wd->time_ampm, i18n_str);
                  free(i18n_str);
               }
          }

        if (wd->hour > HOUR_12H_MAXIMUM)
          sprintf(str, "%02d", wd->hour - HOUR_12H_MAXIMUM);
        else if (wd->hour == 0)
          sprintf(str, "%02d", HOUR_12H_MAXIMUM);
        else
          sprintf(str, "%02d", wd->hour);
     }
   edje_object_part_text_set(wd->base, "elm.text.time.hour", str);
   sprintf(str, "%02d", wd->min);
   edje_object_part_text_set(wd->base, "elm.text.time.min", str);

   diskselector = elm_ctxpopup_content_unset(wd->ctxpopup);
   if (diskselector) evas_object_del(diskselector);
   evas_object_hide(wd->ctxpopup);
   if (wd->ctxpopup_show)
     wd->ctxpopup_show = EINA_FALSE;
}


/**
 * Add a new datefield object
 * The date format and strings are based on current locale
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Datefield
 */
EAPI Evas_Object *
elm_datefield_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   char buf[4096];
   Widget_Data *wd;

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);

   e = evas_object_evas_get(parent);
   if (!e) return NULL;
   wd = ELM_NEW(Widget_Data);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "datefield");
   elm_widget_type_set(obj, "datefield");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_on_focus_hook_set( obj, _on_focus_hook, NULL );
   elm_widget_can_focus_set(obj, EINA_TRUE);

   wd->base = edje_object_add(e);
   elm_widget_resize_object_set(obj, wd->base);
   edje_object_signal_callback_add(wd->base, "mouse,down,1",
                    "elm.rect.date.year.over", _signal_rect_mouse_down, obj);
   edje_object_signal_callback_add(wd->base, "mouse,down,1",
                    "elm.rect.date.month.over", _signal_rect_mouse_down, obj);
   edje_object_signal_callback_add(wd->base, "mouse,down,1",
                    "elm.rect.date.day.over", _signal_rect_mouse_down, obj);

   edje_object_signal_callback_add(wd->base, "mouse,down,1",
                    "elm.rect.time.hour.over", _signal_rect_mouse_down, obj);
   edje_object_signal_callback_add(wd->base, "mouse,down,1",
                     "elm.rect.time.min.over", _signal_rect_mouse_down, obj);

   wd->ctxpopup = elm_ctxpopup_add(elm_widget_top_get(obj));
   snprintf(buf,sizeof(buf),"extended/timepicker/%s",elm_widget_style_get(obj));
   elm_object_style_set(wd->ctxpopup, buf);
   elm_ctxpopup_horizontal_set(wd->ctxpopup, EINA_TRUE);
   elm_ctxpopup_direction_priority_set(wd->ctxpopup,ELM_CTXPOPUP_DIRECTION_DOWN,
               ELM_CTXPOPUP_DIRECTION_UP,ELM_CTXPOPUP_DIRECTION_LEFT,
               ELM_CTXPOPUP_DIRECTION_RIGHT);
   evas_object_size_hint_weight_set(wd->ctxpopup, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(wd->ctxpopup, EVAS_HINT_FILL,EVAS_HINT_FILL);
   elm_object_focus_allow_set(wd->ctxpopup, EINA_FALSE);
   evas_object_smart_callback_add(wd->ctxpopup, "dismissed",
                                  _ctxpopup_dismissed_cb, obj);
   evas_object_event_callback_add(wd->base, EVAS_CALLBACK_RESIZE,
                                  _datefield_resize_cb, obj);

   wd->y_min = 1900;
   wd->m_min = 1;
   wd->d_min = 1;
   wd->y_max = -1;
   wd->m_max = 12;
   wd->d_max = 31;
   wd->year = wd->y_min;
   wd->month = 1;
   wd->day = 1;
   wd->ctxpopup_show = EINA_FALSE;

   wd->layout = ELM_DATEFIELD_LAYOUT_DATEANDTIME;
   wd->time_mode = EINA_TRUE;

   _theme_hook(obj);

   return obj;
}

/**
 * set layout for the datefield
 *
 * @param obj The datefield object
 * @param layout set layout for date/time/dateandtime
 * (default: ELM_DATEFIELD_LAYOUT_DATEANDTIME)
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_layout_set(Evas_Object *obj, Elm_Datefield_Layout layout)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;
   if (layout > ELM_DATEFIELD_LAYOUT_DATEANDTIME) return;

   if (wd->layout != layout)
     {
        if (wd->time_ampm)
          {
             edje_object_part_unswallow(wd->base,wd->time_ampm);
             evas_object_del(wd->time_ampm);
             wd->time_ampm = NULL;
          }
        wd->layout = layout;
        _theme_hook(obj);
     }
   return;
}

/**
 * get layout of the datefield
 *
 * @param obj The datefield object
 * @return layout of the datefield
 *
 * @ingroup Datefield
 */
EAPI Elm_Datefield_Layout
elm_datefield_layout_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) 0;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return 0;

   return wd->layout;
}

/**
 * Set selected date of the datefield
 *
 * @param obj The datefield object
 * @param year The year to set
 * @param month The month to set
 * @param day The day to set
 * @param hour The hours to set (24hour mode - 0~23)
 * @param min The minutes to set (0~59)
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_date_set(Evas_Object *obj, int year, int month, int day, int hour,
                       int min)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;

   wd->year = _check_date_boundary(obj, year, ENTRY_YEAR);
   wd->month = _check_date_boundary(obj, month, ENTRY_MON);
   wd->day = _check_date_boundary(obj, day, ENTRY_DAY);

   if (hour > HOUR_24H_MAXIMUM) wd->hour = HOUR_24H_MAXIMUM;
   else if (hour < 0) wd->hour = 0;
   else wd->hour = hour;

   if (min > MIN_MAXIMUM) wd->min = MIN_MAXIMUM;
   else if (min < 0) wd->min = 0;
   else wd->min = min;

   _date_update(obj);
}

/**
 * Get selected date of the datefield
 *
 * @param obj The datefield object
 * @param year The pointer to the variable get the selected year
 * @param month The pointer to the variable get the selected month
 * @param day The pointer to the variable get the selected day
 * @param hour The pointer to the variable get the selected hour (24hour mode)
 * @param hour The pointer to the variable get the selected min
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_date_get(const Evas_Object *obj, int *year, int *month, int *day,
                       int *hour, int *min)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;

   if (year)
     *year = wd->year;
   if (month)
     *month = wd->month;
   if (day)
     *day = wd->day;
   if (hour)
     *hour = wd->hour;
   if (min)
     *min = wd->min;
}

/**
 * Set upper boundary of the datefield
 *
 * @param obj The datefield object
 * @param year The year to set
 * @param month The month to set
 * @param day The day to set
 * @return TRUE/FALSE
 *
 * @ingroup Datefield
 */
EAPI Eina_Bool
elm_datefield_date_max_set(Evas_Object *obj, int year, int month, int day)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   int day_of_month;
   Eina_Bool update = EINA_FALSE;

   if (!wd) return EINA_FALSE;
   if (month < 1 || month > MONTH_MAXIMUM) return EINA_FALSE;
   day_of_month = _maximum_day_get(year, month);
   if (day < 1 || day > day_of_month) return EINA_FALSE;

   wd->y_max = year;
   wd->m_max = month;
   wd->d_max = day;

   if (wd->year > wd->y_max)
     {
        wd->year = wd->y_max;
        update = EINA_TRUE;
     }
   if (wd->year == wd->y_max && wd->month > wd->m_max)
     {
        wd->month = wd->m_max;
        update = EINA_TRUE;
     }
   if (wd->year == wd->y_max && wd->month == wd->m_max && wd->day > wd->d_max)
     {
        wd->day = wd->d_max;
        update = EINA_TRUE;
     }

   if (update) _date_update(obj);
   return EINA_TRUE;
}

/**
 * Get upper boundary of the datefield
 *
 * @param obj The datefield object
 * @param year The pointer to the variable get the maximum year
 * @param month The pointer to the variable get the maximum month
 * @param day The pointer to the variable get the maximum day
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_date_max_get(const Evas_Object *obj, int *year, int *month,
                           int *day)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;

   if (year)
     *year = wd->y_max;
   if (month)
     *month = wd->m_max;
   if (day)
     *day = wd->d_max;
}

/**
 * Set lower boundary of the datefield
 *
 * @param obj The datefield object
 * @param year The year to set
 * @param month The month to set
 * @param day The day to set
 * @return TRUE/FALSE
 *
 * @ingroup Datepicker
 */
EAPI Eina_Bool
elm_datefield_date_min_set(Evas_Object *obj, int year, int month, int day)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   int day_of_month;
   Eina_Bool update = EINA_FALSE;

   if (!wd) return EINA_FALSE;
   if (month < 1 || month > MONTH_MAXIMUM) return EINA_FALSE;
   day_of_month = _maximum_day_get(year, month);
   if (day < 1 || day > day_of_month) return EINA_FALSE;

   wd->y_min = year;
   wd->m_min = month;
   wd->d_min = day;

   if (wd->year < wd->y_min)
     {
        wd->year = wd->y_min;
        update = EINA_TRUE;
     }
   if (wd->year == wd->y_min && wd->month < wd->m_min)
     {
        wd->month = wd->m_min;
        update = EINA_TRUE;
     }
   if (wd->year == wd->y_min && wd->month == wd->m_min && wd->day < wd->d_min)
     {
        wd->day = wd->d_min;
        update = EINA_TRUE;
     }

   if (update) _date_update(obj);
   return EINA_TRUE;
}

/**
 * Get lower boundary of the datefield
 *
 * @param obj The datefield object
 * @param year The pointer to the variable get the maximum year
 * @param month The pointer to the variable get the maximum month
 * @param day The pointer to the variable get the maximum day
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_date_min_get(const Evas_Object *obj, int *year, int *month,
                           int *day)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;

   if (year)
     *year = wd->y_min;
   if (month)
     *month = wd->m_min;
   if (day)
     *day = wd->d_min;
}

/**
 * Set if the datefield show hours in military or am/pm mode
 *
 * @param obj The datefield object
 * @param mode option for the hours mode. If true, it is shown as 12h mode,
 * if false, it is shown as 24h mode. Default value is true
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_time_mode_set(Evas_Object *obj, Eina_Bool mode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;

   if (wd->time_mode != mode)
     {
   wd->time_mode = mode;
   if (!wd->time_mode) edje_object_signal_emit(wd->base, "elm,state,mode,24h",
                                               "elm");
   else edje_object_signal_emit(wd->base, "elm,state,mode,12h", "elm");
   _date_update(obj);
     }
}

/**
 * get time mode of the datefield
 *
 * @param obj The datefield object
 * @return time mode (EINA_TRUE: 12hour mode / EINA_FALSE: 24hour mode)
 *
 * @ingroup Datefield
 */
EAPI Eina_Bool
elm_datefield_time_mode_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return EINA_FALSE;

   return wd->time_mode;
}

/**
 * Set date format of datefield
 *
 * @param obj The datefield object
 * @param fmt The date format, ex) yymmdd. Default value is mmddyy.
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_date_format_set(Evas_Object *obj, const char *fmt)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   char sig[32] = "elm,state,format,";
   int i = 0, j;

   if (!wd || !fmt) return;

   j = strlen(sig);
   while (j < 31 )
     {
        sig[j++] = tolower(fmt[i++]);
     }
   if (j < 32) sig[j] = '\0';
   edje_object_signal_emit(wd->base, sig, "elm");

   if (strstr(sig, "yymmdd")) wd->date_format = DATE_FORMAT_YYMMDD;
   else if (strstr(sig, "yyddmm")) wd->date_format = DATE_FORMAT_YYDDMM;
   else if (strstr(sig, "mmyydd")) wd->date_format = DATE_FORMAT_MMYYDD;
   else if (strstr(sig, "mmddyy")) wd->date_format = DATE_FORMAT_MMDDYY;
   else if (strstr(sig, "ddyymm")) wd->date_format = DATE_FORMAT_DDYYMM;
   else if (strstr(sig, "ddmmyy")) wd->date_format = DATE_FORMAT_DDMMYY;
   wd->format_exists = EINA_TRUE;
}

/**
 * get date format of the datefield
 *
 * @param obj The datefield object
 * @return date format string. ex) yymmdd
 *
 * @ingroup Datefield
 */
EAPI const char *
elm_datefield_date_format_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   switch (wd->date_format)
     {
      case DATE_FORMAT_YYMMDD: return "yymmdd";
      case DATE_FORMAT_YYDDMM: return "yyddmm";
      case DATE_FORMAT_MMYYDD: return "mmyydd";
      case DATE_FORMAT_MMDDYY: return "mmddyy";
      case DATE_FORMAT_DDYYMM: return "ddyymm";
      case DATE_FORMAT_DDMMYY: return "ddmmyy";
      default: return NULL;
     }
}

/**
 * Add a callback function for input panel state
 *
 * @param obj The datefield object
 * @param func The function to be called when the event is triggered
 * (value will be the Ecore_IMF_Input_Panel_State)
 * @param data The data pointer to be passed to @p func
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_input_panel_state_callback_add(Evas_Object *obj,
          void (*pEventCallbackFunc) (void *data, Evas_Object *obj, int value),
          void *data)
{
   // This API will be no more in use after the redesigning of datefield widget
   //with ctxpopup & diskselector instead of using entry objects edited by Vkpd.
   // API will be deprecated soon.
   printf( "#####\nWARNING: API elm_datefield_input_panel_state_callback_add "
            "will be deprecated soon \n#####\n");
}

/**
 * Delete a callback function for input panel state
 *
 * @param obj The datefield object
 * @param func The function to be called when the event is triggered
 *
 * @ingroup Datefield
 */
EAPI void
elm_datefield_input_panel_state_callback_del(Evas_Object *obj,
          void (*pEventCallbackFunc) (void *data, Evas_Object *obj, int value))
{
   // This API will be no more in use after the redesigning of datefield widget
   //with ctxpopup & diskselector instead of using entry objects edited by Vkpd.
   // API will be deprecated soon.
   printf( "#####\nWARNING: API elm_datefield_input_panel_state_callback_del"
            "will be deprecated soon \n#####\n");
}
