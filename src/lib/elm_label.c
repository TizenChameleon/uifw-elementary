#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup Label Label
 * @ingroup Elementary
 *
 * Display text, with simple html-like markup. The theme of course
 * can invent new markup tags and style them any way it likes
 */

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object *lbl;
   Evas_Object *bg;
   const char *label;
   const char *ellipsis_label;
   Evas_Coord lastw;
   Ecore_Job *deferred_recalc_job;
   Evas_Coord wrap_w;
   Eina_Bool linewrap : 1;
   Eina_Bool changed : 1;
   Eina_Bool bgcolor : 1;
   Eina_Bool ellipsis : 1;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static int _get_value_in_key_string(char *oldstring, char *key, char *value);
static int _string_key_value_replace(char *oldstring, char *key, char *value, char *tagstring);
static int _is_width_over(Evas_Object *obj);
static void _ellipsis_label_to_width(Evas_Object *obj);

#define MIN_LABEL_FONT_SIZE 8
#define MAX_LABEL_FONT_SIZE 60

static void
_elm_win_recalc_job(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Coord minw = -1, minh = -1, /*maxw = -1,*/ maxh = -1;
   Evas_Coord posx, posy, resw, resh, minminw;

   wd->deferred_recalc_job = NULL;
   evas_object_geometry_get(wd->lbl, &posx, &posy, &resw, &resh);
   resh = 0;
   minminw = 0;
   edje_object_size_min_restricted_calc(wd->lbl, &minw, &minh, 0, 0);
   minminw = minw;
   if (wd->wrap_w >= resw)
     {
        resw = wd->wrap_w;
        edje_object_size_min_restricted_calc(wd->lbl, &minw, &minh, resw, 0);
        evas_object_size_hint_min_set(data, minw, minh);
     }
   else
     {
        if (wd->wrap_w > minminw) minminw = wd->wrap_w;
        edje_object_size_min_restricted_calc(wd->lbl, &minw, &minh, resw, 0);
        evas_object_size_hint_min_set(data, minminw, minh);
     }
   maxh = minh;
   evas_object_size_hint_max_set(data, -1, maxh);
}

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (wd->deferred_recalc_job) ecore_job_del(wd->deferred_recalc_job);
   if (wd->label) eina_stringshare_del(wd->label);
   if (wd->bg) evas_object_del(wd->bg);
   free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   if (wd->linewrap)
     _elm_theme_object_set(obj, wd->lbl, "label", "base_wrap", elm_widget_style_get(obj));
   else
     _elm_theme_object_set(obj, wd->lbl, "label", "base", elm_widget_style_get(obj));
   edje_object_part_text_set(wd->lbl, "elm.text", wd->label);
   edje_object_scale_set(wd->lbl, elm_widget_scale_get(obj) *
                         _elm_config->scale);
   _sizing_eval(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
   Evas_Coord posx, posy, resw, resh/*, minminw*/;

   if (wd->linewrap)
     {
        evas_object_geometry_get(wd->lbl, NULL, NULL, &resw, &resh);
        if ((resw == wd->lastw) && (!wd->changed)) return;
        wd->changed = EINA_FALSE;
        wd->lastw = resw;
        _elm_win_recalc_job(obj);
// FIXME: works ok. but NOT for genlist. what should genlist do?
//        if (wd->deferred_recalc_job) ecore_job_del(wd->deferred_recalc_job);
//        wd->deferred_recalc_job = ecore_job_add(_elm_win_recalc_job, obj);
     }
   else
     {
        evas_object_geometry_get(wd->lbl, &posx, &posy, &resw, &resh);
        edje_object_size_min_calc(wd->lbl, &minw, &minh);
        evas_object_size_hint_min_set(obj, minw, minh);
        maxh = minh;
        evas_object_size_hint_max_set(obj, maxw, maxh);

        if (wd->ellipsis)
        {
           if (_is_width_over(obj) == 1)
              {
                 _ellipsis_label_to_width(obj);
              }
        }
    }
}

static void
_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);

   if (wd->linewrap)
     {
        _sizing_eval(data);
     }
}

static int
_get_value_in_key_string(char *oldstring, char *key, char *value)
{
	char *curlocater, *starttag, *endtag;
	int firstindex = 0, foundflag = -1;

   curlocater = strstr(oldstring, key);
   if (curlocater == NULL)
   {
	   foundflag = 0;
   }
   else
   {
	   starttag = curlocater;
	   endtag = curlocater + strlen(key);
	   if (*endtag != '=')
		   foundflag = 0;

	   firstindex = abs(oldstring - curlocater);
	   firstindex += strlen(key)+1; // strlen("font_size") + strlen("=")
	   value = oldstring + firstindex;

	   while (oldstring != starttag)
	   {
		   if (*starttag == '>')
		   {
			   foundflag = 0;
			   break;
		   }
		   if (*starttag == '<')
			   break;
		   else
			   starttag--;
		   if (starttag == NULL)
			   break;
	   }

	   while (NULL != *endtag)
	   {
		   if (*endtag == '<')
		   {
			   foundflag = 0;
			   break;
		   }
		   if (*endtag == '>')
			   break;
		   else
			   endtag++;
		   if (endtag == NULL)
			   break;
	   }

	   if (foundflag != 0 && *starttag == '<' && *endtag == '>')
		   foundflag = 1;
	   else
   		   foundflag = 0;
   }

   if (foundflag == 1)
	   return 0;

   return -1;
}


static int
_string_key_value_replace(char *oldstring, char *key, char *value, char *tagstring)
{
	char *curlocater, *starttag, *endtag;
	int firstindex = 0, insertflag = 0;

   curlocater = strstr(oldstring, key);
   if (curlocater == NULL)
   {
	   insertflag = 1;
   }
   else
   {
	   starttag = curlocater - 1;
	   endtag = curlocater + strlen(key);
	   if (*endtag != '=')
		   insertflag = 1;

	   firstindex = abs(oldstring - curlocater);
	   firstindex += strlen(key)+1; // strlen("font_size") + strlen("=")
	   strncpy(tagstring, oldstring, firstindex);
	   tagstring[firstindex] = '\0';
	   sprintf(&tagstring[firstindex], "%s", value);

	   while (curlocater != NULL)
	   {
		   if (*curlocater == ' ' || *curlocater == '>')
			   break;
		   curlocater++;
	   }
	   strcat(tagstring, curlocater);

	   while (oldstring != starttag)
	   {
		   if (*starttag == '>')
		   {
			   insertflag = 1;
			   break;
		   }
		   if (*starttag == '<')
			   break;
		   else
			   starttag--;
		   if (starttag == NULL)
			   break;
	   }

	   while (NULL != *endtag)
	   {
		   if (*endtag == '<')
		   {
			   insertflag = 1;
			   break;
		   }
		   if (*endtag == '>')
			   break;
		   else
			   endtag++;
		   if (endtag == NULL)
			   break;
	   }

	   if (insertflag == 0 && *starttag == '<' && *endtag == '>')
		   return 0; 
	   else
   		   insertflag = 1;

   }

   if (insertflag == 1)
   {
	   sprintf(tagstring, "<%s=%s>", key, value);
	   strcat(tagstring, oldstring);
	   return 0;
   }

   return -1;
}

static int
_is_width_over(Evas_Object *obj)
{
   Evas_Coord x, y, w, h;
   Evas_Coord vx, vy, vw, vh;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return 0;

   edje_object_part_geometry_get(wd->lbl,"elm.text",&x,&y,&w,&h);

   evas_object_geometry_get (obj, &vx,&vy,&vw,&vh);

   if (x >= 0 && y >= 0)
	   return 0;

   if (4 < wd->wrap_w && w > wd->wrap_w)
	   return 1;

   return 0;
}

static void
_ellipsis_label_to_width(Evas_Object *obj)
{
	Widget_Data *wd = elm_widget_data_get(obj);
	int cur_fontsize = 0, len, showcount, textcount;
	char *oldlabel, *value, *textlocater;
	char *label, fontbuf[16];

	len = strlen(wd->label);
	oldlabel = malloc(sizeof(char)*(len+128));
	strcpy(oldlabel, wd->label);

	if (_get_value_in_key_string(oldlabel, "font_size", value) == 0 
		&& value != NULL)
	{
		cur_fontsize = atoi(value);
	}
	else
		cur_fontsize = 24; /* default size in aqua.edc */

	free(oldlabel);
	oldlabel = NULL;

	while (_is_width_over(obj) == 1)
	{
		if (cur_fontsize > MIN_LABEL_FONT_SIZE)
		{
			cur_fontsize--;

			len = strlen(wd->label);
			if (len <= 0) return;
			label = malloc(sizeof(char)*(len+32));
			oldlabel = malloc(sizeof(char)*(len+32));
			sprintf(fontbuf, "%d", cur_fontsize);

			strcpy(oldlabel, wd->label);
			_string_key_value_replace(oldlabel, "font_size", fontbuf, label);
			edje_object_part_text_set(wd->lbl, "elm.text", label);
			free(label);
			free(oldlabel);
		}
		else
		{
			len = strlen(wd->label) - 1;
			oldlabel = malloc(sizeof(char)*(len+32));
			strcpy(oldlabel, edje_object_part_text_get(wd->lbl, "elm.text"));
			showcount = len - 1;
			label = malloc(sizeof(char)*(len+128));
			while (showcount > 4)
			{
				oldlabel[showcount] = '\0';
				strcpy(label, oldlabel);
				strcat(label, "...");
				edje_object_part_text_set(wd->lbl, "elm.text", label);

				if (_is_width_over(obj) == 1)
					showcount--;
				else
					break;
			}
			free(label);
			free(oldlabel);
		}
	}
   wd->changed = 1;
   _sizing_eval(obj);
}


/**
 * Add a new label to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Label
 */
EAPI Evas_Object *
elm_label_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   wd->bgcolor = EINA_FALSE;
   wd->bg = evas_object_rectangle_add(e);
   evas_object_color_set(wd->bg, 0, 0, 0, 0);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "label");
   elm_widget_type_set(obj, "label");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   elm_widget_can_focus_set(obj, 0);

   wd->linewrap = EINA_FALSE;
   wd->ellipsis = EINA_FALSE;

   wd->lbl = edje_object_add(e);
   _elm_theme_object_set(obj, wd->lbl, "label", "base", "default");
   wd->label = eina_stringshare_add("<br>");
   edje_object_part_text_set(wd->lbl, "elm.text", "<br>");
   elm_widget_resize_object_set(obj, wd->lbl);

   evas_object_event_callback_add(wd->lbl, EVAS_CALLBACK_RESIZE, _resize, obj);

   wd->changed = 1;
   _sizing_eval(obj);
   return obj;
}

/**
 * Set the label on the label object
 *
 * @param obj The label object
 * @param label The label will be used on the label object
 *
 * @ingroup Label
 */
EAPI void
elm_label_label_set(Evas_Object *obj, const char *label)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (!label) label = "";
   eina_stringshare_replace(&wd->label, label);
   edje_object_part_text_set(wd->lbl, "elm.text", label);
   wd->changed = 1;
   _sizing_eval(obj);
}

/**
 * Get the label used on the label object
 *
 * @param obj The label object
 * @return The string inside the label
 * @ingroup Label
 */
EAPI const char *
elm_label_label_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return NULL;
   return wd->label;
}

/**
 * Set the wrapping behavior of the label
 *
 * @param obj The label object
 * @param wrap To wrap text or not
 * @ingroup Label
 */
EAPI void
elm_label_line_wrap_set(Evas_Object *obj, Eina_Bool wrap)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   const char *t;
   if (wd->linewrap == wrap) return;
   wd->linewrap = wrap;
   t = eina_stringshare_add(elm_label_label_get(obj));
   if (wd->linewrap)
     _elm_theme_object_set(obj, wd->lbl, "label", "base_wrap", elm_widget_style_get(obj));
   else
     _elm_theme_object_set(obj, wd->lbl, "label", "base", elm_widget_style_get(obj));
   elm_label_label_set(obj, t);
   eina_stringshare_del(t);
   wd->changed = 1;
   _sizing_eval(obj);
}

/**
 * Get the wrapping behavior of the label
 *
 * @param obj The label object
 * @return To wrap text or not
 * @ingroup Label
 */
EAPI Eina_Bool
elm_label_line_wrap_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return wd->linewrap;
}

/**
 * Set wrap width of the label
 *
 * @param obj The label object
 * @param w The wrap width in pixels at a minimum where words need to wrap
 * @ingroup Label
 */
EAPI void
elm_label_wrap_width_set(Evas_Object *obj, Evas_Coord w)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->wrap_w == w) return;
   wd->wrap_w = w;
   _sizing_eval(obj);
}

/**
 * get wrap width of the label
 *
 * @param obj The label object
 * @return The wrap width in pixels at a minimum where words need to wrap
 * @ingroup Label
 */
EAPI Evas_Coord
elm_label_wrap_width_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) 0;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return 0;
   return wd->wrap_w;
}

/**
 * Set the font size on the label object
 *
 * @param obj The label object
 * @param size font size
 *
 * @ingroup Label
 */
EAPI void
elm_label_fontsize_set(Evas_Object *obj, const int fontsize)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   char *label, fontvalue[16];
   int len;

   if (!wd) return;
   len = strlen(wd->label);
   if (len <= 0) return;
   label = alloca(sizeof(char)*(len+32));
   sprintf(fontvalue, "%d", fontsize);

   if (_string_key_value_replace(wd->label, "font_size", fontvalue, label) == 0)
   {
      if (wd->label) eina_stringshare_del(wd->label);
      wd->label = eina_stringshare_add(label);
      edje_object_part_text_set(wd->lbl, "elm.text", label);
      wd->changed = 1;
      _sizing_eval(obj);
   }
}

/**
 * Set the text align on the label object
 *
 * @param obj The label object
 * @param align align mode
 *
 * @ingroup Label
 */
EAPI void
elm_label_text_align_set(Evas_Object *obj, char *alignmode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   char *label;
   int len;

   if (!wd) return;
   len = strlen(wd->label);
   if (len <= 0) return;
   label = alloca(sizeof(char)*(len+32));

   if (_string_key_value_replace(wd->label, "align", alignmode, label) == 0)
   {
      if (wd->label) eina_stringshare_del(wd->label);
      wd->label = eina_stringshare_add(label);
      edje_object_part_text_set(wd->lbl, "elm.text", label);
      wd->changed = 1;
      _sizing_eval(obj);
   }
}

/**
 * Set the text color on the label object
 *
 * @param obj The label object
 * @param r Red property background color of The label object 
 * @param g Green property background color of The label object 
 * @param b Blue property background color of The label object 
 * @param a Alpha property background alpha of The label object 
 *
 * @ingroup Label
 */
EAPI void
elm_label_text_color_set(Evas_Object *obj, unsigned int r, unsigned int g, unsigned int b, unsigned int a)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   char *label, colorstring[16];
   int len;

   if (!wd) return;
   len = strlen(wd->label);
   if (len <= 0) return;
   label = alloca(sizeof(char)*(len+32));
   sprintf(colorstring, "#%02X%02X%02X%02X", r, g, b, a);

   if (_string_key_value_replace(wd->label, "color", colorstring, label) == 0)
   {
      if (wd->label) eina_stringshare_del(wd->label);
      wd->label = eina_stringshare_add(label);
      edje_object_part_text_set(wd->lbl, "elm.text", label);
      wd->changed = 1;
      _sizing_eval(obj);
   }
}


/**
 * Set background color of the label
 *
 * @param obj The label object
 * @param r Red property background color of The label object 
 * @param g Green property background color of The label object 
 * @param b Blue property background color of The label object 
 * @param a Alpha property background alpha of The label object 
 * @ingroup Label
 */
EAPI void
elm_label_background_color_set(Evas_Object *obj, unsigned int r, unsigned int g, unsigned int b, unsigned int a)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   evas_object_color_set(wd->bg, r, g, b, a);

   if (wd->bgcolor == EINA_FALSE)
     {
        wd->bgcolor = 1;
		edje_object_part_swallow(wd->lbl, "label.swallow.background", wd->bg);
     }
}

/**
 * Set the ellipsis behavior of the label
 *
 * @param obj The label object
 * @param ellipsis To ellipsis text or not
 * @ingroup Label
 */
EAPI void
elm_label_ellipsis_set(Evas_Object *obj, Eina_Bool ellipsis)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   const char *t;
   if (wd->ellipsis == ellipsis) return;
   wd->ellipsis = ellipsis;
   wd->changed = 1;
   _sizing_eval(obj);
}
