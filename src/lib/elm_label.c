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
   Evas_Coord lastw;
   Ecore_Job *deferred_recalc_job;
   Evas_Coord wrap_w;
   Evas_Coord wrap_h;
   Eina_Bool linewrap : 1;
   Eina_Bool wrapmode : 1;
   Eina_Bool slidingmode : 1;
   Eina_Bool slidingellipsis : 1;
   Eina_Bool changed : 1;
   Eina_Bool bgcolor : 1;
   Eina_Bool ellipsis : 1;
   int slide_duration;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static int _get_value_in_key_string(const char *oldstring, char *key, char **value);
static int _strbuf_key_value_replace(Eina_Strbuf *srcbuf, char *key, const char *value, int deleteflag);
static int _stringshare_key_value_replace(const char **srcstring, char *key, const char *value, int deleteflag);
static int _is_width_over(Evas_Object *obj, int linemode);
static void _ellipsis_label_to_width(Evas_Object *obj, int linemode);

static void
_elm_win_recalc_job(void *data)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Coord minw = -1, minh = -1, maxh = -1;
   Evas_Coord resw, resh, minminw, minminh;
   if (!wd) return;
   wd->deferred_recalc_job = NULL;
   evas_object_geometry_get(wd->lbl, NULL, NULL, &resw, &resh);
   resh = 0;
   minminw = 0;
   edje_object_size_min_restricted_calc(wd->lbl, &minw, &minh, 0, 0);
   minminw = minw;
   minminh = minh;
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

   if (wd->ellipsis && wd->linewrap && wd->wrap_h > 0 && _is_width_over(data, 1) == 1) 
     _ellipsis_label_to_width(data, 1);

   maxh = minh;
   evas_object_size_hint_max_set(data, -1, maxh);
}

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->deferred_recalc_job) ecore_job_del(wd->deferred_recalc_job);
   if (wd->label) eina_stringshare_del(wd->label);
   if (wd->bg) evas_object_del(wd->bg);
   free(wd);
}

static void
_theme_change(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->linewrap)
     {
       if (wd->ellipsis)
         _elm_theme_object_set(obj, wd->lbl, "label", "base_wrap_ellipsis", elm_widget_style_get(obj));
       else
         _elm_theme_object_set(obj, wd->lbl, "label", "base_wrap", elm_widget_style_get(obj));
     }
   else
     _elm_theme_object_set(obj, wd->lbl, "label", "base", elm_widget_style_get(obj));

}

static void
_theme_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   _theme_change(obj);
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
   Evas_Coord resw, resh;
   if (!wd) return;
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
        evas_object_geometry_get(wd->lbl, NULL, NULL, &resw, &resh);
        edje_object_size_min_calc(wd->lbl, &minw, &minh);
		if (wd->wrap_w > 0 && minw > wd->wrap_w)
			minw = wd->wrap_w;
        evas_object_size_hint_min_set(obj, minw, minh);
        maxh = minh;
        evas_object_size_hint_max_set(obj, maxw, maxh);

        if (wd->ellipsis && _is_width_over(obj, 0) == 1) 
          _ellipsis_label_to_width(obj, 0);
     }
}

static void 
_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   if (!wd) return;
   if (wd->linewrap) _sizing_eval(data);
}

static int
_get_value_in_key_string(const char *oldstring, char *key, char **value)
{
   char *curlocater, *starttag, *endtag;
   int firstindex = 0, foundflag = -1;

   curlocater = strstr(oldstring, key);
   if (curlocater)
     {
        starttag = curlocater;
        endtag = curlocater + strlen(key);
        if (endtag == NULL || *endtag != '=') 
          {
            foundflag = 0;
            return -1;
          }

        firstindex = abs(oldstring - curlocater);
        firstindex += strlen(key)+1; // strlen("key") + strlen("=")
        *value = (char*)(oldstring + firstindex);

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
            if (starttag == NULL) break;
          }

        while (NULL != endtag)
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
            if (endtag == NULL) break;
          }

        if (foundflag != 0 && *starttag == '<' && *endtag == '>') 
          foundflag = 1;
        else 
          foundflag = 0;
     }
   else
     {
       foundflag = 0;
     }

   if (foundflag == 1) return 0;

   return -1;
}

static int
_strbuf_key_value_replace(Eina_Strbuf *srcbuf, char *key, const char *value, int deleteflag)
{
   const char *srcstring = NULL;
   Eina_Strbuf *repbuf = NULL, *diffbuf = NULL;
   char *curlocater, *replocater;
   char *starttag, *endtag;
   int tagtxtlen = 0, insertflag = 0;

   srcstring = eina_strbuf_string_get(srcbuf);
   curlocater = strstr(srcstring, key);

   if (curlocater == NULL)
     {
       insertflag = 1;
     }
   else
     {
       do 
         {
           starttag = strchr(srcstring, '<');
           endtag = strchr(srcstring, '>');
           tagtxtlen = endtag - starttag;
           if (tagtxtlen <= 0) tagtxtlen = 0;
           if (starttag < curlocater && curlocater < endtag) break;
           if (endtag != NULL && endtag+1 != NULL)
             srcstring = endtag+1;
           else
             break;
         } while (strlen(srcstring) > 1);

       if (starttag && endtag && tagtxtlen > strlen(key))
         {
           repbuf = eina_strbuf_new();
           diffbuf = eina_strbuf_new();
           eina_strbuf_append_n(repbuf, starttag, tagtxtlen);
           srcstring = eina_strbuf_string_get(repbuf);
           curlocater = strstr(srcstring, key);

           if (curlocater != NULL)
             {
               replocater = curlocater + strlen(key) + 1;

               while (*replocater == ' ' || *replocater == '=')
			   {
                 replocater++;
			   }

                while (replocater != NULL && *replocater != ' ' && *replocater != '>')
                  replocater++;

               if (replocater-curlocater > strlen(key)+1)
                 {
                   replocater--;
                   eina_strbuf_append_n(diffbuf, curlocater, replocater-curlocater+1);
                 }
               else
                 insertflag = 1;
             }
           else
             {
               insertflag = 1;
             }
           eina_strbuf_reset(repbuf);
         }
       else
         {
           insertflag = 1; 
         }
     }

   if (repbuf == NULL) repbuf = eina_strbuf_new();
   if (diffbuf == NULL) diffbuf = eina_strbuf_new();

   if (insertflag)
     {
       eina_strbuf_append_printf(repbuf, "<%s=%s>", key, value);
       eina_strbuf_prepend(srcbuf, eina_strbuf_string_get(repbuf));
     }
   else
     {
        if (deleteflag)
          {
            eina_strbuf_prepend(diffbuf, "<");
            eina_strbuf_append(diffbuf, ">");
            eina_strbuf_replace_first(srcbuf, eina_strbuf_string_get(diffbuf), "");
          }
        else
          {
            eina_strbuf_append_printf(repbuf, "%s=%s", key, value);
            eina_strbuf_replace_first(srcbuf, eina_strbuf_string_get(diffbuf), eina_strbuf_string_get(repbuf));
          }
     }

   if (repbuf) eina_strbuf_free(repbuf);
   if (diffbuf) eina_strbuf_free(diffbuf);
  
   return 0;           
}

static int
_stringshare_key_value_replace(const char **srcstring, char *key, const char *value, int deleteflag)
{
   Eina_Strbuf *sharebuf = NULL;   
   
   sharebuf = eina_strbuf_new();
   eina_strbuf_append(sharebuf, *srcstring);
   _strbuf_key_value_replace(sharebuf, key, value, deleteflag);
   eina_stringshare_del(*srcstring);
   *srcstring = eina_stringshare_add(eina_strbuf_string_get(sharebuf));
   eina_strbuf_free(sharebuf);

   return 0;
}


// FIXME: move to some where(such as elm_util??).
//        copied from elm_entry for check pure text length w/o tags.
static char *
_str_append(char *str, const char *txt, int *len, int *alloc)
{
   int txt_len = strlen(txt);

   if (txt_len <= 0) return str;
   if ((*len + txt_len) >= *alloc)
     {
	char *str2;
	int alloc2;

	alloc2 = *alloc + txt_len + 128;
	str2 = realloc(str, alloc2);
	if (!str2) return str;
	*alloc = alloc2;
	str = str2;
     }
   strcpy(str + *len, txt);
   *len += txt_len;
   return str;
}

// FIXME: move to some where(such as elm_util??).
//        copied from elm_entry for check pure text length w/o tags.
static char *
_strncpy(char* dest, const char* src, size_t count)
{
   if (!dest) 
     {
	ERR( "dest is NULL" );
	return NULL;
     }
   if (!src) 
     {
	ERR( "src is NULL" );
	return NULL;
     }
   if (count < 0)
     {
	ERR( "count is smaller than 0" );
	return NULL;
     }

   return strncpy( dest, src, count );
}

// FIXME: move to some where(such as elm_util??).
//        copied from elm_entry for check pure text length w/o tags.
static char *
_mkup_to_text(const char *mkup)
{
   char *str = NULL;
   int str_len = 0, str_alloc = 0;
   char *s, *p;
   char *tag_start, *tag_end, *esc_start, *esc_end, *ts;

   if (!mkup) return NULL;
   s=p=NULL;
   tag_start = tag_end = esc_start = esc_end = NULL;
   p = (char *)mkup;
   s = p;
   for (;;)
     {
	if (((p!=NULL)&&(*p == 0)) ||
	    (tag_end) || (esc_end) ||
	    (tag_start) || (esc_start))
	  {
	     if (tag_end)
	       {
		  char *ttag;

		  ttag = malloc(tag_end - tag_start);
		  if (ttag)
		    {
		       _strncpy(ttag, tag_start + 1, tag_end - tag_start - 1);
		       ttag[tag_end - tag_start - 1] = 0;
		       if (!strcmp(ttag, "br"))
			 str = _str_append(str, "\n", &str_len, &str_alloc);
		       else if (!strcmp(ttag, "\n"))
			 str = _str_append(str, "\n", &str_len, &str_alloc);
		       else if (!strcmp(ttag, "\\n"))
			 str = _str_append(str, "\n", &str_len, &str_alloc);
		       else if (!strcmp(ttag, "\t"))
			 str = _str_append(str, "\t", &str_len, &str_alloc);
		       else if (!strcmp(ttag, "\\t"))
			 str = _str_append(str, "\t", &str_len, &str_alloc);
		       else if (!strcmp(ttag, "ps")) /* Unicode paragraph separator */
			 str = _str_append(str, "\xE2\x80\xA9", &str_len, &str_alloc);
		       free(ttag);
		    }
		  tag_start = tag_end = NULL;
	       }
	     else if (esc_end)
	       {
		  ts = malloc(esc_end - esc_start + 1);
		  if (ts)
		    {
		       const char *esc;
		       _strncpy(ts, esc_start, esc_end - esc_start);
		       ts[esc_end - esc_start] = 0;
		       esc = evas_textblock_escape_string_get(ts);
		       if (esc)
			 str = _str_append(str, esc, &str_len, &str_alloc);
		       free(ts);
		    }
		  esc_start = esc_end = NULL;
	       }
	     else if ((p!=NULL)&&(*p == 0))
	       {
		  ts = malloc(p - s + 1);
		  if (ts)
		    {
		       _strncpy(ts, s, p - s);
		       ts[p - s] = 0;
		       str = _str_append(str, ts, &str_len, &str_alloc);
		       free(ts);
		    }
                  break;
	       }
	  }
	if ((p!=NULL)&&(*p == '<'))
	  {
	     if (!esc_start)
	       {
		  tag_start = p;
		  tag_end = NULL;
		  ts = malloc(p - s + 1);
		  if (ts)
		    {
		       _strncpy(ts, s, p - s);
		       ts[p - s] = 0;
		       str = _str_append(str, ts, &str_len, &str_alloc);
		       free(ts);
		    }
		  s = NULL;
	       }
	  }
	else if ((p!=NULL)&&(*p == '>'))
	  {
	     if (tag_start)
	       {
		  tag_end = p;
		  s = p + 1;
	       }
	  }
	else if ((p!=NULL)&&(*p == '&'))
	  {
	     if (!tag_start)
	       {
		  esc_start = p;
		  esc_end = NULL;
		  ts = malloc(p - s + 1);
		  if (ts)
		    {
		       _strncpy(ts, s, p - s);
		       ts[p - s] = 0;
		       str = _str_append(str, ts, &str_len, &str_alloc);
		       free(ts);
		    }
		  s = NULL;
	       }
	  }
	else if ((p!=NULL)&&(*p == ';'))
	  {
	     if (esc_start)
	       {
		  esc_end = p;
		  s = p + 1;
	       }
	  }
	p++;
     }
   return str;
}

static int
_is_width_over(Evas_Object *obj, int linemode)
{
   Evas_Coord x, y, w, h;
   Evas_Coord vx, vy, vw, vh;
   Widget_Data *wd = elm_widget_data_get(obj);
   const char *ellipsis_string = "...";
   size_t ellen = strlen(ellipsis_string)+1;

   if (!wd) return 0;

   // too short to ellipsis
   char *plaintxt = _mkup_to_text(edje_object_part_text_get(wd->lbl, "elm.text"));
   int plainlen = 0;
   if (plaintxt != NULL)
   {
	   plainlen = strlen(plaintxt);
       free(plaintxt);
   }
   if (plainlen <= ellen) return 0;   

   edje_object_part_geometry_get(wd->lbl, "elm.text", &x, &y, &w, &h);

   evas_object_geometry_get(obj, &vx, &vy, &vw, &vh);

/*
   fprintf(stderr, "## _is_width_over\n");
   fprintf(stderr, "## x = %d, y = %d, w = %d, h = %d\n", x, y, w, h);
   fprintf(stderr, "## vx = %d, vy = %d, vw = %d, vh = %d\n", vx, vy, vw, vh);
   if (linemode)
	   fprintf(stderr, "## wd->wrap_w = %d, wd->wrap_h = %d\n", wd->wrap_w, wd->wrap_h);
   else
	   fprintf(stderr, "## wd->wrap_w = %d\n", wd->wrap_w);
   fprintf(stderr, "## check str = %s\n", edje_object_part_text_get(wd->lbl, "elm.text"));
*/

   if (linemode == 0) // single line
     {
       // skip if too early to check widget size
       if (w < 0 && h < 0)
         return 0;
       // if string fits at widget
       if ((x >= 0) && (y >= 0))
	   {
		   if ((wd->wrap_w > 0) && (wd->wrap_w < w))
		   {
			   Evas_Coord minw, minh;
			   edje_object_size_min_calc(wd->lbl, &minw, &minh);

			   if (minw < wd->wrap_w)
			   { // min insufficient
				   return 0;
			   }
			   else
				   return 1;
		   }
		   else
			   return 0;
	   }

       if (0 < wd->wrap_w && w > wd->wrap_w) return 1;
     }
   else // multiline
     {
//       if (vy > h && h > wd->wrap_h) return 1;
		 if ((x >= 0 || y >= 0) && h > wd->wrap_h) return 1;
     }

   return 0;
}

static void
_ellipsis_fontsize_set(Evas_Object *obj, int fontsize)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   Eina_Strbuf *fontbuf = NULL;
   Eina_Strbuf *txtbuf = NULL;
   txtbuf = eina_strbuf_new();
   fontbuf = eina_strbuf_new();
   eina_strbuf_append(txtbuf, edje_object_part_text_get(wd->lbl, "elm.text"));
   eina_strbuf_append_printf(fontbuf, "%d", fontsize);
   _strbuf_key_value_replace(txtbuf, "font_size", eina_strbuf_string_get(fontbuf), 0);
   edje_object_part_text_set(wd->lbl, "elm.text", eina_strbuf_string_get(txtbuf));
   eina_strbuf_free(fontbuf);
   eina_strbuf_free(txtbuf);
}

static Eina_Bool
_ellipsis_cut_chars_to_widget(Evas_Object *obj, int fontsize, int linemode)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;

   const char *ellipsis_string = "...";
   int minshowcount = strlen(ellipsis_string);
   Evas_Coord w, h;
   Evas_Textblock_Cursor *tc1, *tc2;
   char *cutstr, *elstr;
   int limitw = 0;
   int lencutstr = 0;
   int i = 0;

   edje_object_part_geometry_get(wd->lbl,"elm.text", NULL, NULL, &w, &h);
   if (w <= 0)
	   return EINA_FALSE;
   tc1 = evas_object_textblock_cursor_new((Evas_Object*)edje_object_part_object_get(wd->lbl, "elm.text"));
   tc2 = evas_object_textblock_cursor_new((Evas_Object*)edje_object_part_object_get(wd->lbl, "elm.text"));
   
   if (wd->wrap_w > 0 && wd->wrap_w < w)
	   limitw = wd->wrap_w;
   else
	   limitw = w;
   evas_textblock_cursor_pos_set(tc1, 0);
   evas_textblock_cursor_char_coord_set(tc2, limitw, 0);
   for (i = 0; i <= minshowcount; i++)
	   evas_textblock_cursor_char_prev(tc2);
   cutstr = evas_textblock_cursor_range_text_get(tc1, tc2, EVAS_TEXTBLOCK_TEXT_PLAIN);

/*
   int eolpos = evas_textblock_cursor_paragraph_text_length_get(tc1);
   Evas_Coord cx, cy, cw, ch;
   for (i = eolpos; i > minshowcount; i--)
   {
	   evas_textblock_cursor_pos_set(tc2, i);
	   fprintf(stderr, "## tc2 = %d\n", evas_textblock_cursor_pos_get(tc2));
	   evas_textblock_cursor_char_geometry_get(tc2, &cx, &cy, &cw, &ch);
	   fprintf(stderr, "## limitw = %d\n", limitw);
	   fprintf(stderr, "## cx = %d, cy = %d, cw = %d, ch = %d\n", cx, cy, cw, ch);
	   if (cx <= limitw)
		   break;
   }
*/

   // FIXME: consider other unicode encoding, currently only care about utf-8
   lencutstr = strlen(cutstr);
   elstr = malloc(sizeof(char)*(lencutstr+minshowcount+1));
   strcpy(elstr, cutstr);
   strcat(elstr, ellipsis_string);

   edje_object_part_text_set(wd->lbl, "elm.text", elstr);

   free(elstr);
   evas_textblock_cursor_free(tc1);
   evas_textblock_cursor_free(tc2);
	
   return EINA_TRUE;
}

static Eina_Bool
_ellipsis_cut_lines_to_widget(Evas_Object *obj, int fontsize, int linemode)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;

   const char *ellipsis_string = "...";
   int minshowcount = strlen(ellipsis_string);
   Evas_Coord w, h;
   Evas_Textblock_Cursor *tc1, *tc2;
   int linenum = 0, cutline = 0;
   double lineheight = 0.0;
   char *cutstr, *elstr;
   int lencutstr = 0;
   int limith = 0;
   int i;

   edje_object_part_geometry_get(wd->lbl,"elm.text", NULL, NULL, &w, &h);

   tc1 = evas_object_textblock_cursor_new((Evas_Object*)edje_object_part_object_get(wd->lbl, "elm.text"));
   tc2 = evas_object_textblock_cursor_new((Evas_Object*)edje_object_part_object_get(wd->lbl, "elm.text"));
   // goto last paragraph
   while (evas_textblock_cursor_paragraph_next(tc2) == EINA_TRUE)
	   ;
   evas_textblock_cursor_paragraph_last(tc2);
   // get total linenumber
   linenum = evas_textblock_cursor_line_geometry_get(tc2, NULL, NULL, NULL, NULL);
   lineheight = h/linenum * 1.0;
   if (wd->wrap_h > 0 && wd->wrap_h < h)
	   limith = wd->wrap_h;
   else
	   limith = h;
   cutline = limith / lineheight;
   if (cutline < 1)
	   cutline = 1;

   evas_textblock_cursor_pos_set(tc1, 0);
   evas_textblock_cursor_line_set(tc2, cutline-1);
   evas_textblock_cursor_line_char_last(tc2);
   for (i = 0; i <= minshowcount; i++)
	   evas_textblock_cursor_char_prev(tc2);
   cutstr = evas_textblock_cursor_range_text_get(tc1, tc2, EVAS_TEXTBLOCK_TEXT_PLAIN);

   // FIXME: consider other unicode encoding, currently only care about utf-8
   lencutstr = strlen(cutstr);
   elstr = malloc(sizeof(char)*(lencutstr+minshowcount+1));
   strcpy(elstr, cutstr);
   strcat(elstr, ellipsis_string);

   edje_object_part_text_set(wd->lbl, "elm.text", elstr);

   free(elstr);
   evas_textblock_cursor_free(tc1);
   evas_textblock_cursor_free(tc2);

   return EINA_TRUE;
}

static void
_ellipsis_label_to_width(Evas_Object *obj, int linemode)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   int cur_fontsize = 0;
   char *kvalue;
   const char *minfont, *deffont, *maxfont;
   int minfontsize, maxfontsize;

   minfont = edje_object_data_get(wd->lbl, "min_font_size");
   if (minfont) minfontsize = atoi(minfont);
   else minfontsize = 1;
   maxfont = edje_object_data_get(wd->lbl, "max_font_size");
   if (maxfont) maxfontsize = atoi(maxfont);
   else maxfontsize = 1;
   deffont = edje_object_data_get(wd->lbl, "default_font_size");
   if (deffont) cur_fontsize = atoi(deffont);
   else cur_fontsize = 1;
   if (minfontsize > maxfontsize || cur_fontsize == 1) return; // theme is not ready for ellipsis
   if (eina_stringshare_strlen(wd->label) <= 0) return;

   if (_get_value_in_key_string(wd->label, "font_size", &kvalue) == 0)
     {
       if (kvalue != NULL) cur_fontsize = atoi(kvalue);
     }

   while (_is_width_over(obj, linemode))
     {
       if (cur_fontsize > minfontsize)
         {
           cur_fontsize -= 3;
		   if (cur_fontsize < minfontsize)
             cur_fontsize = minfontsize;
		   _ellipsis_fontsize_set(obj, cur_fontsize);
         }
       else
         {
           if (linemode == 0) // single line
             {
               _ellipsis_cut_chars_to_widget(obj, cur_fontsize, linemode);
			   break;
			 }
           else // multiline
             {
               _ellipsis_cut_lines_to_widget(obj, cur_fontsize, linemode);
			   break;
			 }
         }
     }
}

/*
 * setting internal state of mulitline label.
 * singleline doesn't need it
 */

void _label_state_change(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->linewrap)
   {
	   if (wd->wrapmode)
		   edje_object_signal_emit(wd->lbl, "elm,state,wordwrap", "elm");
	   else
		   edje_object_signal_emit(wd->lbl, "elm,state,default", "elm");
   }
}

void _label_sliding_change(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->linewrap)
   {
	   wd->slidingmode = EINA_FALSE;
	   fprintf(stderr, "ERR: elm_label dosen't support multiline sliding effect!!!\n");
	   fprintf(stderr, "ERR: elm_label dosen't support multiline sliding effect!!!\n");
	   fprintf(stderr, "ERR: elm_label dosen't support multiline sliding effect!!!\n");
	   return;
   }

   char *plaintxt = _mkup_to_text(edje_object_part_text_get(wd->lbl, "elm.text"));
   int plainlen = 0;
   if (plaintxt != NULL)
   {
	   plainlen = strlen(plaintxt);
       free(plaintxt);
   }
   if (plainlen < 1) 
   {
	   wd->slidingmode = EINA_TRUE;
	   fprintf(stderr, "ERR: too short to slide label!!!\n");
	   return;
   }

   if (wd->slidingmode)
   {
	   if (wd->ellipsis)
	   {
		   wd->slidingellipsis = EINA_TRUE;
		   elm_label_ellipsis_set(obj, EINA_FALSE);
	   }
	   Edje_Message_Int_Set *msg = alloca(sizeof(Edje_Message_Int_Set) + (sizeof(int)));
	   
	   msg->count=1;
	   msg->val[0] = (int)wd->slide_duration;

	   edje_object_message_send(wd->lbl, EDJE_MESSAGE_INT_SET, 0, msg);
	   edje_object_signal_emit(wd->lbl, "elm,state,slide,start", "elm");
   }
   else
   {
	   edje_object_signal_emit(wd->lbl, "elm,state,slide,stop", "elm");
	   if (wd->slidingellipsis)
	   {
		   wd->slidingellipsis = EINA_FALSE;
		   elm_label_ellipsis_set(obj, EINA_TRUE);
	   }
   }
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
   wd->bgcolor = EINA_FALSE;
   wd->ellipsis = EINA_FALSE;
   wd->wrapmode = EINA_FALSE;
   wd->slidingmode = EINA_FALSE;
   wd->slidingellipsis = EINA_FALSE;
   wd->wrap_w = 0;
   wd->wrap_h = 0;
   wd->slide_duration = 10;

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
   _theme_change(obj);
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
   if (w < 0) w = 0;
   if (wd->wrap_w == w) return;
   if (wd->ellipsis) edje_object_part_text_set(wd->lbl, "elm.text", wd->label);
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
 * Set wrap height of the label
 *
 * @param obj The label object
 * @param w The wrap width in pixels at a minimum where words need to wrap
 * @ingroup Label
 */
EAPI void
elm_label_wrap_height_set(Evas_Object *obj, Evas_Coord h)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (h < 0) h = 0;
   if (wd->wrap_h == h) return;
   if (wd->ellipsis) edje_object_part_text_set(wd->lbl, "elm.text", wd->label);
   wd->wrap_h = h;
   _sizing_eval(obj);
}

/**
 * get wrap width of the label
 *
 * @param obj The label object
 * @return The wrap height in pixels at a minimum where words need to wrap
 * @ingroup Label
 */
EAPI Evas_Coord
elm_label_wrap_height_get(const Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) 0;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return 0;
   return wd->wrap_h;
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
elm_label_fontsize_set(Evas_Object *obj, int fontsize)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_Strbuf *fontbuf = NULL;
   int len, removeflag = 0;

   if (!wd) return;
   len = strlen(wd->label);
   if (len <= 0) return;
   fontbuf = eina_strbuf_new();
   eina_strbuf_append_printf(fontbuf, "%d", fontsize);

   if (fontsize == 0) removeflag = 1; // remove fontsize tag

   if (_stringshare_key_value_replace(&wd->label, "font_size", eina_strbuf_string_get(fontbuf), removeflag) == 0)
     {
       edje_object_part_text_set(wd->lbl, "elm.text", wd->label);
       wd->changed = 1;
       _sizing_eval(obj);
     }
   eina_strbuf_free(fontbuf);
}

/**
 * Set the text align on the label object
 *
 * @param obj The label object
 * @param align align mode ("left", "center", "right")
 *
 * @ingroup Label
 */
EAPI void
elm_label_text_align_set(Evas_Object *obj, const char *alignmode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   int len;

   if (!wd) return;
   len = strlen(wd->label);
   if (len <= 0) return;

   if (_stringshare_key_value_replace(&wd->label, "align", alignmode, 0) == 0)
     edje_object_part_text_set(wd->lbl, "elm.text", wd->label);

   wd->changed = 1;
   _sizing_eval(obj);
}

/**
 * Set the text color on the label object
 *
 * @param obj The label object
 * @param r Red property background color of The label object 
 * @param g Green property background color of The label object 
 * @param b Blue property background color of The label object 
 * @param a Alpha property background color of The label object 
 *
 * @ingroup Label
 */
EAPI void
elm_label_text_color_set(Evas_Object *obj, unsigned int r, unsigned int g, unsigned int b, unsigned int a)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Eina_Strbuf *colorbuf = NULL;
   int len;

   if (!wd) return;
   len = strlen(wd->label);
   if (len <= 0) return;
   colorbuf = eina_strbuf_new();
   eina_strbuf_append_printf(colorbuf, "#%02X%02X%02X%02X", r, g, b, a);

   if (_stringshare_key_value_replace(&wd->label, "color", eina_strbuf_string_get(colorbuf), 0) == 0)
     {
       edje_object_part_text_set(wd->lbl, "elm.text", wd->label);
       wd->changed = 1;
       _sizing_eval(obj);
     }
   eina_strbuf_free(colorbuf);
}


/**
 * Set background color of the label
 *
 * @param obj The label object
 * @param r Red property background color of The label object 
 * @param g Green property background color of The label object 
 * @param b Blue property background color of The label object 
 * @param a Alpha property background color of The label object 
 * @ingroup Label
 */
EAPI void
elm_label_background_color_set(Evas_Object *obj, unsigned int r, unsigned int g, unsigned int b, unsigned int a)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
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
   if (!wd) return;
   if (wd->ellipsis == ellipsis) return;
   wd->ellipsis = ellipsis;
   if (wd->linewrap) _theme_change(obj);
   edje_object_part_text_set(wd->lbl, "elm.text", wd->label);
   wd->changed = 1;
   _sizing_eval(obj);
}

/**
 * Set the wrapmode of the label
 *
 * @param obj The label object
 * @param wrapmode 0 is charwrap, 1 is wordwrap
 * @ingroup Label
 */
EAPI void
elm_label_wrap_mode_set(Evas_Object *obj, Eina_Bool wrapmode)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   if (wd->wrapmode == wrapmode) return;
   wd->wrapmode = wrapmode;
   _label_state_change(obj);
   wd->changed = 1;
   _sizing_eval(obj);
}

/**
 * Set the text slide of the label
 *
 * @param obj The label object
 * @param slide To start slide or stop
 * @ingroup Label
 */
EAPI void
elm_label_slide_set(Evas_Object *obj, Eina_Bool slide)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->slidingmode == slide) return;
   wd->slidingmode = slide;
   _label_sliding_change(obj);
   wd->changed = 1;
   _sizing_eval(obj);
}

/**
 * get the text slide mode of the label
 *
 * @param obj The label object
 * @return slide slide mode value
 * @ingroup Label
 */
EAPI Eina_Bool
elm_label_slide_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;

   return wd->slidingmode;
}

/**
 * set the slide duration(speed) of the label
 *
 * @param obj The label object
 * @return The duration time in moving text from slide begin position to slide end position
 * @ingroup Label
 */
EAPI void
elm_label_slide_duration_set(Evas_Object *obj, int duration)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   Edje_Message_Int_Set *msg = alloca(sizeof(Edje_Message_Int_Set) + (sizeof(int)));
   
   if (!wd) return;
   wd->slide_duration = duration;

   msg->count=1;
   msg->val[0] = (int)wd->slide_duration;

   edje_object_message_send(wd->lbl, EDJE_MESSAGE_INT_SET, 0, msg);
}

/**
 * get the slide duration(speed) of the label
 *
 * @param obj The label object
 * @return The duration time in moving text from slide begin position to slide end position
 * @ingroup Label
 */
EAPI int
elm_label_slide_duration_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) 0;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return 0;
   return wd->slide_duration;
}
