#include <Elementary.h>



#include <stdio.h>
#include <stdlib.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#define ELM_MAX(v1, v2)    (((v1) > (v2)) ? (v1) : (v2))


struct _Elm_Xml_Animator
{
   Evas_Object *parent;
   Eina_List *object_list;
   Eina_List *animation_list;
};

typedef struct _Animation_List Animation_List;

struct _Animation_List
{
	char *animation_title;
	Eina_List *scene_list;
};

typedef struct _Object_List Object_List;

struct _Object_List
{
	Evas_Object *object;
	Animation_List *animation_list;
	Eina_Bool remove_me;
    void (*end_func) (void *data, Evas_Object *obj);
	void *data;
};

typedef enum _Standard_Type
{
	STANDARD_TYPE_OBJECT,
	STANDARD_TYPE_SCREEN,
	STANDARD_TYPE_CURRENT
}Standard_Type;

typedef enum _Change_Type
{
	CHANGE_TYPE_LINEAR,
	CHANGE_TYPE_ACCELERATE,
	CHANGE_TYPE_DECELERATE,
	CHANGE_TYPE_RETURN
}Change_Type;

typedef enum _Attribute_Type
{
	ATTRIBUTE_TYPE_RELATIVE,
	ATTRIBUTE_TYPE_ABSOLUTE,
}Attribute_Type;

typedef struct _Location_Data Location_Data;

struct _Location_Data 
{
   Standard_Type standard;
   Standard_Type standard_from;
   Standard_Type standard_to;
   Evas_Coord from;
   Evas_Coord to;
   Evas_Coord current;
   Change_Type type;
};

typedef struct _Rotation_Data Rotation_Data;

struct _Rotation_Data 
{
   double from;
   double to;
   Change_Type type;
};

typedef struct _Color_Data Color_Data;

struct _Color_Data 
{
   int from;
   int to;
   Change_Type type;
};

typedef struct _Animation_Data Animation_Data;

struct _Animation_Data 
{
   Evas_Object * obj;
   char * scene_title;
   
   Location_Data *CoordX;
   Location_Data *CoordY;
   Location_Data *CoordW;
   Location_Data *CoordH;

   Rotation_Data *AngleX;
   Rotation_Data *AngleY;
   Rotation_Data *AngleZ;
   double Center_x;
   double Center_y;
   double Center_z;
   Attribute_Type Center_type;

   Color_Data *Red;
   Color_Data *Green;
   Color_Data *Blue;
   Color_Data *Alpha;

   double start;
   double end;
   double duration;

   unsigned int start_time;
   Eina_Bool (*mv_func) (void *data);
   void (*end_func) (void *data, Evas_Object * obj);
   void *data;
   Ecore_Animator * timer;
};


static void move_continue(void *data, Evas_Object *obj);

static unsigned int
current_time_get() 
{
   struct timeval timev;

   gettimeofday(&timev, NULL);
   return ((timev.tv_sec * 1000) + ((timev.tv_usec) / 1000));
}

static void
set_evas_map_3d(Evas_Object * obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h, double zoomw, double zoomh,
	      double dx, double dy, double dz, Evas_Coord cx, Evas_Coord cy, Evas_Coord cz) 
{
   if (obj == NULL)
     {
	return;
     }
   Evas_Map * map = evas_map_new(4);
   if (map == NULL)
      return;
//	printf("map >> x : %d y : %d w : %d h : %d\n", x, y, w, h); 
   evas_map_smooth_set(map, EINA_TRUE);
//   evas_map_util_points_populate_from_object_full(map, obj, 0);
   evas_object_map_enable_set(obj, EINA_TRUE);
   evas_map_util_3d_perspective(map, x + w / 2, y + h / 2, 0, w * 10);
   evas_map_util_points_populate_from_geometry(map, x, y, w, h, 0);
   evas_map_util_zoom(map, zoomw, zoomh, x, y);
   evas_map_util_3d_rotate(map, dx, dy, dz, cx, cy, cz);
   evas_object_map_set(obj, map);
   evas_map_free(map);
}

static int 
get_value_by_type(Change_Type type, double amount, int dx)
{
	switch(type)
	{
		case CHANGE_TYPE_LINEAR:
			return amount * dx;
		case CHANGE_TYPE_ACCELERATE:
			return pow(amount, 2) * dx;
		case CHANGE_TYPE_DECELERATE:
			return (1 * sin(pow(amount, 1) * (M_PI / 2)) * dx);
		case CHANGE_TYPE_RETURN:
			return (1 * sin(2 * pow(amount, 1) * (M_PI / 2)) * dx);
		default:
			return amount * dx;
	}
}

static int 
get_value_by_standard(Standard_Type type, int object_value, int current, int degree)
{
//	printf("%d %d %d %d\n", type, object_value, current, degree);
	switch(type)
	{
		case STANDARD_TYPE_SCREEN:
			return degree;
		case STANDARD_TYPE_OBJECT:
			return object_value + degree;
		case STANDARD_TYPE_CURRENT:
			return current + degree;
		default:
			return degree;
	}
}


static void
get_value_by_attribute(Attribute_Type type, Evas_Coord w, Evas_Coord h, double x, double y, double z, int *cx, int *cy, int *cz)
{
	switch(type)
	{
		case ATTRIBUTE_TYPE_RELATIVE:
			*cx = (int)((double)w * x);
			*cy = (int)((double)h * y);
			*cz = (int)z;
			return;
		case ATTRIBUTE_TYPE_ABSOLUTE:
			*cx = (int)x;
			*cy = (int)y;
			*cz = (int)z;
			return;
		default:
			*cx = (int)((double)w * x);
			*cy = (int)((double)h * y);
			*cz = (int)z;
			return;
	}
}

static Eina_Bool
set_evas_map_disable(void *data)
{
	Evas_Object *obj = (Evas_Object *)data;
	evas_object_map_enable_set(obj, EINA_FALSE);
	evas_render_updates(evas_object_evas_get(obj));

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
move_evas_map(void *data) 
{
   Animation_Data *next_ad;
   Evas_Coord tx, ty, tw, th;
   double t;
   double amount;

   int x, y, w, h;
   int dx, dy, dw, dh;
   int sfx, sfy, sfw, sfh;
   int stx, sty, stw, sth;
   int px, py, pw, ph;
   double zoomw, zoomh;

   double ax, ay, az;
   double dax, day, daz;
   double pax, pay, paz;

   int cx, cy, cz;

   int r, g, b, a;
   int dr, dg, db, da;
   int pr, pg, pb, pa;


   Animation_Data * ad = (Animation_Data *) data;
   t = ELM_MAX(0.0, current_time_get() - ad->start_time) / 1000;

   evas_object_geometry_get(ad->obj, &tx, &ty, &tw, &th);

 //  sfx = ad->CoordX->from;
 //  sfy = ad->CoordY->from;
 //  sfw = ad->CoordW->from;
 //  sfh = ad->CoordH->from;

   sfx = get_value_by_standard(ad->CoordX->standard_from != NULL ? ad->CoordX->standard_from : ad->CoordX->standard, tx, ad->CoordX->current, ad->CoordX->from);
   sfy = get_value_by_standard(ad->CoordY->standard_from != NULL ? ad->CoordY->standard_from : ad->CoordY->standard, ty, ad->CoordY->current, ad->CoordY->from);
   sfw = get_value_by_standard(ad->CoordW->standard_from != NULL ? ad->CoordW->standard_from : ad->CoordW->standard, tw, ad->CoordW->current, ad->CoordW->from);
   sfh = get_value_by_standard(ad->CoordH->standard_from != NULL ? ad->CoordH->standard_from : ad->CoordH->standard, th, ad->CoordH->current, ad->CoordH->from);

   stx = get_value_by_standard(ad->CoordX->standard_to != NULL ? ad->CoordX->standard_to : ad->CoordX->standard, tx, ad->CoordX->current, ad->CoordX->to);
   sty = get_value_by_standard(ad->CoordY->standard_to != NULL ? ad->CoordY->standard_to : ad->CoordY->standard, ty, ad->CoordY->current, ad->CoordY->to);
   stw = get_value_by_standard(ad->CoordW->standard_to != NULL ? ad->CoordW->standard_to : ad->CoordW->standard, tw, ad->CoordW->current, ad->CoordW->to);
   sth = get_value_by_standard(ad->CoordH->standard_to != NULL ? ad->CoordH->standard_to : ad->CoordH->standard, th, ad->CoordH->current, ad->CoordH->to);

//   printf("%d %d %d %d :: %d %d %d %d\n", sfx, sfy, sfw, sfh, stx, sty, stw, sth);
   if(t < ad->start) {
	   set_evas_map_3d(ad->obj, sfx, sfy, sfw, sfh, 1.0, 1.0, ad->AngleX->from, ad->AngleY->from, ad->AngleZ->from, ad->CoordX->from + ad->Center_x, ad->CoordY->from + ad->Center_y, ad->Center_z);
	   return ECORE_CALLBACK_RENEW;
   }
   else t -= ad->start;

   dx = stx - sfx;//ad->CoordX->to - sfx;
   dy = sty - sfy;//ad->CoordY->to - sfy;
   dw = stw - sfw;//ad->CoordW->to - sfw;
   dh = sth - sfh;//ad->CoordH->to - sfh;
   dax = ad->AngleX->to - ad->AngleX->from;
   day = ad->AngleY->to - ad->AngleY->from;
   daz = ad->AngleZ->to - ad->AngleZ->from;
   dr = ad->Red->to - ad->Red->from;
   dg = ad->Green->to - ad->Green->from;
   db = ad->Blue->to - ad->Blue->from;
   da = ad->Alpha->to - ad->Alpha->from;

   if (t <= ad->duration)
     {
		 amount = t / ad->duration;
		 x = get_value_by_type(ad->CoordX->type, amount, dx);
		 y = get_value_by_type(ad->CoordY->type, amount, dy);
		 w = get_value_by_type(ad->CoordW->type, amount, dw);
		 h = get_value_by_type(ad->CoordH->type, amount, dh);
		 ax = get_value_by_type(ad->AngleX->type, amount, dax);
		 ay = get_value_by_type(ad->AngleY->type, amount, day);
		 az = get_value_by_type(ad->AngleZ->type, amount, daz);
		 r = get_value_by_type(ad->Red->type, amount, dr);
		 g = get_value_by_type(ad->Green->type, amount, dg);
		 b = get_value_by_type(ad->Blue->type, amount, db);
		 a = get_value_by_type(ad->Alpha->type, amount, da);
     }
   else
     {
	x = dx;
	y = dy;
	w = dw;
	h = dh;
	ax = dax;
	ay = day;
	az = daz;
	r = dr;
	g = dg;
	b = db;
	a = da;
     }

   // standard chek
   px = sfx + x;
   py = sfy + y;
   pw = sfw + w;
   ph = sfh + h;
   zoomw = (double)pw/(double)tw;
   zoomh = (double)ph/(double)th;
   pax = ad->AngleX->from + ax;
   pay = ad->AngleY->from + ay;
   paz = ad->AngleZ->from + az;
   pr = ad->Red->from + r;
   pg = ad->Green->from + g;
   pb = ad->Blue->from + b;
   pa = ad->Alpha->from + a;

   get_value_by_attribute(ad->Center_type, pw, ph, ad->Center_x, ad->Center_y, ad->Center_z, &cx, &cy, &cz);
  
   if (x == dx && y == dy && w == dw && h == dh && ax == dax && ay == day && az == daz && r == dr && g == dg && b == db && a == da)
     {
	if(ad->timer)
	{
		ecore_animator_del(ad->timer);
		ad->timer = NULL;
	}
	//printf("px : %d py : %d pw : %d ph : %d\n", px, py, pw, ph); 
	set_evas_map_3d(ad->obj, px, py, tw, th, zoomw, zoomh, pax, pay, paz, px + cx, py + cy, cz);
	evas_object_color_set(ad->obj, pr, pg, pb, pa);

	if(ad->end_func == move_continue){
		next_ad = (Animation_Data *)ad->data;
	
		if(next_ad != NULL){
			next_ad->CoordX->current = px;
			next_ad->CoordY->current = py;
			next_ad->CoordW->current = pw;
			next_ad->CoordH->current = ph;
		}
	}
	if (ad->end_func != NULL)
		ad->end_func(ad->data, ad->obj);
	if(ad->end_func != move_continue)
		ecore_idler_add(set_evas_map_disable, ad->obj);
	
	return ECORE_CALLBACK_CANCEL;
     }
   else
     {
//	Evas_Object *rect = evas_object_rectangle_add(evas_object_evas_get(ad->obj));
//	evas_object_color_set(rect, 255, 255, 255, 255);
//	evas_object_resize(rect, 1, 1);
//	evas_object_move(rect, px, py);
//	evas_object_show(rect);
	//printf(" >>>> px : %d py : %d pw : %d ph : %d\n", px, py, pw, ph); 
	set_evas_map_3d(ad->obj, px, py, tw, th, zoomw, zoomh, pax, pay, paz, px + cx, py + cy, cz);
	evas_object_color_set(ad->obj, pr, pg, pb, pa);
     }
   return ECORE_CALLBACK_RENEW;
}

static void
move_continue(void *data, Evas_Object *obj)
{
	Animation_Data *ad = (Animation_Data *)data;
	ad->start_time = current_time_get();
	ad->timer = ecore_animator_add(ad->mv_func, ad);
}

static void
move_object_with_animation(Eina_List *list, Evas_Object * obj, Eina_Bool (*mv_func) (void *data), 
		void (*func) (void *data, Evas_Object * obj), void *data) 
{
   Animation_Data *ad, *prev_ad = NULL;
   Eina_List *l;
   ad = (Animation_Data *)eina_list_data_get(list);

   EINA_LIST_FOREACH(list, l, ad){
	   ad->obj = obj;
	   ad->mv_func = mv_func;
	   if(ad->end < ad->start) return;
	   ad->duration = ad->end - ad->start;
	   if(ad == (Animation_Data *)eina_list_data_get(eina_list_last(list))){
		   ad->end_func = func;
           ad->data = data;
		   if(prev_ad != NULL) prev_ad->data = ad;
	   }else{
		   ad->end_func = move_continue;
           if(prev_ad != NULL) prev_ad->data = ad;
	   }
	   prev_ad = ad;
	   if(ad == (Animation_Data *)eina_list_data_get(list)){
	       ad->start_time = current_time_get();
		   ad->timer = ecore_animator_add(ad->mv_func, ad);
	   }
   }
}

static Standard_Type
get_standard_type(char *str)
{
	if(!strcmp(str, "OBJECT"))
		return STANDARD_TYPE_OBJECT;
	else if(!strcmp(str, "SCREEN"))
		return STANDARD_TYPE_SCREEN;
	else if(!strcmp(str, "CURRENT"))
		return STANDARD_TYPE_CURRENT;

	return STANDARD_TYPE_SCREEN;
}

static Change_Type
get_change_type(char *str)
{
	if(!strcmp(str, "LINEAR"))
		return CHANGE_TYPE_LINEAR;
	else if(!strcmp(str, "ACCELERATE"))
		return CHANGE_TYPE_ACCELERATE;
	else if(!strcmp(str, "DECELERATE"))
		return CHANGE_TYPE_DECELERATE;
	else if(!strcmp(str, "RETURN"))
		return CHANGE_TYPE_RETURN;

	return CHANGE_TYPE_LINEAR;
}

static Attribute_Type
get_attribute_type(char *str)
{
	if(!strcmp(str, "RELATIVE"))
		return ATTRIBUTE_TYPE_RELATIVE;
	else if(!strcmp(str, "ABSOLUTE"))
		return ATTRIBUTE_TYPE_ABSOLUTE;

	return ATTRIBUTE_TYPE_RELATIVE;
}

static void
init_animation_data(Animation_Data *ad)
{
	ad->Red->from = 255;
	ad->Green->from = 255;
	ad->Blue->from = 255;
	ad->Alpha->from = 255;
	ad->Red->to = 255;
	ad->Green->to = 255;
	ad->Blue->to = 255;
	ad->Alpha->to = 255;
}

static Eina_List *
save_animation_data(Eina_List *list, xmlDocPtr doc, xmlNodePtr cur)
{
	Animation_List *al = NULL;
	Animation_Data *ad = NULL;
	xmlAttrPtr pAttr;
	char *tmp;

	while(cur != NULL){
		if (cur->type == XML_ELEMENT_NODE) {

			if(!strcmp((char *)cur->name, "AML")){
				list = save_animation_data(list, doc, cur->xmlChildrenNode);
			}else{

				if(!strcmp((char *)cur->name, "Animation")){

					al = (Animation_List *) calloc(1, sizeof(Animation_List));

					if ( cur->properties != NULL)  {
						pAttr = (xmlAttrPtr)cur->properties;
						while (pAttr != NULL) {
							if(!strcmp((char *)pAttr->name, "title")){
								al->animation_title = strdup((char *)pAttr->children->content);
								//printf("--------- %s=\"%s\"\n", pAttr->name, ad->scene_title);
							}
							pAttr=pAttr->next;
						}
					}

					list = eina_list_append(list, al);
				}
				else{
					if(list != NULL)
						al = (Animation_List *)eina_list_data_get(eina_list_last(list));
				}

				if(!strcmp((char *)cur->name, "Scene")){

					ad = (Animation_Data *) calloc(1, sizeof(Animation_Data));
					ad->CoordX = (Location_Data *) calloc(1, sizeof(Location_Data));
					ad->CoordY = (Location_Data *) calloc(1, sizeof(Location_Data));
					ad->CoordW = (Location_Data *) calloc(1, sizeof(Location_Data));
					ad->CoordH = (Location_Data *) calloc(1, sizeof(Location_Data));
					ad->AngleX = (Rotation_Data *) calloc(1, sizeof(Rotation_Data));
					ad->AngleY = (Rotation_Data *) calloc(1, sizeof(Rotation_Data));
					ad->AngleZ = (Rotation_Data *) calloc(1, sizeof(Rotation_Data));
					ad->Red = (Color_Data *) calloc(1, sizeof(Color_Data));
					ad->Green = (Color_Data *) calloc(1, sizeof(Color_Data));
					ad->Blue = (Color_Data *) calloc(1, sizeof(Color_Data));
					ad->Alpha = (Color_Data *) calloc(1, sizeof(Color_Data));

					init_animation_data(ad);

					al->scene_list = eina_list_append(al->scene_list, ad);

					if ( cur->properties != NULL)  {
						pAttr = (xmlAttrPtr)cur->properties;
						while (pAttr != NULL) {
							if(!strcmp((char *)pAttr->name, "title")){
								ad->scene_title = strdup((char *)pAttr->children->content);
								//printf("--------- %s=\"%s\"\n", pAttr->name, ad->scene_title);
							}
							pAttr=pAttr->next;
						}
					}
				}
				else{
					ad = (Animation_Data *)eina_list_data_get(eina_list_last(al->scene_list));
				}

				if(!strcmp((char *)cur->name, "Time")){
					if ( cur->properties != NULL)  {
						pAttr = (xmlAttrPtr)cur->properties;
						while (pAttr != NULL) {
							if(!strcmp((char *)pAttr->name, "start")){
								tmp = strdup((char *)pAttr->children->content);
								ad->start = atof(tmp);
								free(tmp);
								//printf("--------- %s=\"%2.1f\"\n", pAttr->name, ad->start);
							}
							if(!strcmp((char *)pAttr->name, "end")){
								tmp = strdup((char *)pAttr->children->content);
								ad->end = atof(tmp);
								free(tmp);
								//printf("--------- %s=\"%2.1f\"\n", pAttr->name, ad->end);
							}
							pAttr=pAttr->next;
						}
					}
				}
				else if(!strcmp((char *)cur->name, "CoordX")){
					if ( cur->properties != NULL)  {
						pAttr = (xmlAttrPtr)cur->properties;
						while (pAttr != NULL) {
							if(!strcmp((char *)pAttr->name, "standard")){
								ad->CoordX->standard = get_standard_type((char *)pAttr->children->content);
								//printf("--------- %s=\"%s\"\n", pAttr->name, ad->CoordX->standard);
							}
							if(!strcmp((char *)pAttr->name, "standard_from")){
								ad->CoordX->standard_from = get_standard_type((char *)pAttr->children->content);
							}
							if(!strcmp((char *)pAttr->name, "standard_to")){
								ad->CoordX->standard_to = get_standard_type((char *)pAttr->children->content);
							}
							if(!strcmp((char *)pAttr->name, "from")){
								tmp = strdup((char *)pAttr->children->content);
								ad->CoordX->from = atoi(tmp);
								free(tmp);
								//printf("--------- %s=\"%d\"\n", pAttr->name, ad->CoordX->from);
							}
							if(!strcmp((char *)pAttr->name, "to")){
								tmp = strdup((char *)pAttr->children->content);
								ad->CoordX->to = atoi(tmp);
								free(tmp);
								//printf("--------- %s=\"%d\"\n", pAttr->name, ad->CoordX->to);
							}
							if(!strcmp((char *)pAttr->name, "type")){
								ad->CoordX->type = get_change_type((char *)pAttr->children->content);
								//printf("--------- %s=\"%s\"\n", pAttr->name, (char *)pAttr->children->content);
							}
							pAttr=pAttr->next;
						}
					}
				}
				else if(!strcmp((char *)cur->name, "CoordY")){
					if ( cur->properties != NULL)  {
						pAttr = (xmlAttrPtr)cur->properties;
						while (pAttr != NULL) {
							if(!strcmp((char *)pAttr->name, "standard")){
								ad->CoordY->standard = get_standard_type((char *)pAttr->children->content);
								//printf("--------- %s=\"%s\"\n", pAttr->name, ad->CoordY->standard);
							}
							if(!strcmp((char *)pAttr->name, "standard_from")){
								ad->CoordY->standard_from = get_standard_type((char *)pAttr->children->content);
							}
							if(!strcmp((char *)pAttr->name, "standard_to")){
								ad->CoordY->standard_to = get_standard_type((char *)pAttr->children->content);
							}
							if(!strcmp((char *)pAttr->name, "from")){
								tmp = strdup((char *)pAttr->children->content);
								ad->CoordY->from = atoi(tmp);
								free(tmp);
								//printf("--------- %s=\"%d\"\n", pAttr->name, ad->CoordY->from);
							}
							if(!strcmp((char *)pAttr->name, "to")){
								tmp = strdup((char *)pAttr->children->content);
								ad->CoordY->to = atoi(tmp);
								free(tmp);
								//printf("--------- %s=\"%d\"\n", pAttr->name, ad->CoordY->to);
							}
							if(!strcmp((char *)pAttr->name, "type")){
								ad->CoordY->type = get_change_type((char *)pAttr->children->content);
								//printf("--------- %s=\"%s\"\n", pAttr->name, ad->CoordY->type);
							}
							pAttr=pAttr->next;
						}
					}
				}
				else if(!strcmp((char *)cur->name, "CoordW")){
					if ( cur->properties != NULL)  {
						pAttr = (xmlAttrPtr)cur->properties;
						while (pAttr != NULL) {
							if(!strcmp((char *)pAttr->name, "standard")){
								ad->CoordW->standard = get_standard_type((char *)pAttr->children->content);
								//printf("--------- %s=\"%s\"\n", (char *)pAttr->name, ad->CoordW->standard);
							}
							if(!strcmp((char *)pAttr->name, "standard_from")){
								ad->CoordW->standard_from = get_standard_type((char *)pAttr->children->content);
								//printf("--------- %s=\"%s\"\n", (char *)pAttr->name, ad->CoordW->standard);
							}
							if(!strcmp((char *)pAttr->name, "standard_to")){
								ad->CoordW->standard_to = get_standard_type((char *)pAttr->children->content);
								//printf("--------- %s=\"%s\"\n", (char *)pAttr->name, ad->CoordW->standard);
							}
							else if(!strcmp((char *)pAttr->name, "from")){
								tmp = strdup((char *)pAttr->children->content);
								ad->CoordW->from = atoi(tmp);
								free(tmp);
								//printf("--------- %s=\"%d\"\n", (char *)pAttr->name, ad->CoordW->from);
							}
							else if(!strcmp((char *)pAttr->name, "to")){
								tmp = strdup((char *)pAttr->children->content);
								ad->CoordW->to = atoi(tmp);
								free(tmp);
								//printf("--------- %s=\"%d\"\n", (char *)pAttr->name, ad->CoordW->to);
							}
							else if(!strcmp((char *)pAttr->name, "type")){
								ad->CoordW->type = get_change_type((char *)pAttr->children->content);
								//printf("--------- %s=\"%s\"\n", (char *)pAttr->name, ad->CoordW->type);
							}
							pAttr=pAttr->next;
						}
					}
				}
				else if(!strcmp((char *)cur->name, "CoordH")){
					if ( cur->properties != NULL)  {
						pAttr = (xmlAttrPtr)cur->properties;
						while (pAttr != NULL) {
							if(!strcmp((char *)pAttr->name, "standard")){
								ad->CoordH->standard = get_standard_type((char *)pAttr->children->content);
								//printf("--------- %s=\"%s\"\n", (char *)pAttr->name, ad->CoordH->standard);
							}
							if(!strcmp((char *)pAttr->name, "standard_from")){
								ad->CoordH->standard_from = get_standard_type((char *)pAttr->children->content);
							}
							if(!strcmp((char *)pAttr->name, "standard_to")){
								ad->CoordH->standard_to = get_standard_type((char *)pAttr->children->content);
							}
							else if(!strcmp((char *)pAttr->name, "from")){
								tmp = strdup((char *)pAttr->children->content);
								ad->CoordH->from = atoi(tmp);
								free(tmp);
								//printf("--------- %s=\"%d\"\n", (char *)pAttr->name, ad->CoordH->from);
							}
							else if(!strcmp((char *)pAttr->name, "to")){
								tmp = strdup((char *)pAttr->children->content);
								ad->CoordH->to = atoi(tmp);
								free(tmp);
								//printf("--------- %s=\"%d\"\n", (char *)pAttr->name, ad->CoordH->to);
							}
							else if(!strcmp((char *)pAttr->name, "type")){
								ad->CoordH->type = get_change_type((char *)pAttr->children->content);
								//printf("--------- %s=\"%s\"\n", (char *)pAttr->name, ad->CoordH->type);
							}
							pAttr=pAttr->next;
						}
					}
				}
				else if(!strcmp((char *)cur->name, "AngleX")){
					if ( cur->properties != NULL)  {
						pAttr = (xmlAttrPtr)cur->properties;
						while (pAttr != NULL) {
							if(!strcmp((char *)pAttr->name, "from")){
								tmp = strdup((char *)pAttr->children->content);
								ad->AngleX->from = atof(tmp);
								free(tmp);
								//printf("--------- %s=\"%2.1f\"\n", (char *)pAttr->name, ad->AngleX->from);
							}
							else if(!strcmp((char *)pAttr->name, "to")){
								tmp = strdup((char *)pAttr->children->content);
								ad->AngleX->to = atof(tmp);
								free(tmp);
								//printf("--------- %s=\"%2.1f\"\n", (char *)pAttr->name, ad->AngleX->to);
							}
							else if(!strcmp((char *)pAttr->name, "type")){
								ad->AngleX->type = get_change_type((char *)pAttr->children->content);
								//printf("--------- %s=\"%s\"\n", (char *)pAttr->name, ad->AngleX->type);
							}
							pAttr=pAttr->next;
						}
					}
				}
				else if(!strcmp((char *)cur->name, "AngleY")){
					if ( cur->properties != NULL)  {
						pAttr = (xmlAttrPtr)cur->properties;
						while (pAttr != NULL) {
							if(!strcmp((char *)pAttr->name, "from")){
								tmp = strdup((char *)pAttr->children->content);
								ad->AngleY->from = atof(tmp);
								free(tmp);
								//printf("--------- %s=\"%2.1f\"\n", (char *)pAttr->name, ad->AngleY->from);
							}
							else if(!strcmp((char *)pAttr->name, "to")){
								tmp = strdup((char *)pAttr->children->content);
								ad->AngleY->to = atof(tmp);
								free(tmp);
								//printf("--------- %s=\"%2.1f\"\n", (char *)pAttr->name, ad->AngleY->to);
							}
							else if(!strcmp((char *)pAttr->name, "type")){
								ad->AngleY->type = get_change_type((char *)pAttr->children->content);
								//printf("--------- %s=\"%s\"\n", (char *)pAttr->name, ad->AngleY->type);
							}
							pAttr=pAttr->next;
						}
					}
				}
				else if(!strcmp((char *)cur->name, "AngleZ")){
					if ( cur->properties != NULL)  {
						pAttr = (xmlAttrPtr)cur->properties;
						while (pAttr != NULL) {
							if(!strcmp((char *)pAttr->name, "from")){
								tmp = strdup((char *)pAttr->children->content);
								ad->AngleZ->from = atof(tmp);
								free(tmp);
								//printf("--------- %s=\"%2.1f\"\n", (char *)pAttr->name, ad->AngleZ->from);
							}
							else if(!strcmp((char *)pAttr->name, "to")){
								tmp = strdup((char *)pAttr->children->content);
								ad->AngleZ->to = atof(tmp);
								free(tmp);
								//printf("--------- %s=\"%2.1f\"\n", (char *)pAttr->name, ad->AngleZ->to);
							}
							else if(!strcmp((char *)pAttr->name, "type")){
								ad->AngleZ->type = get_change_type((char *)pAttr->children->content);
								//printf("--------- %s=\"%s\"\n", (char *)pAttr->name, ad->AngleZ->type);
							}
							pAttr=pAttr->next;
						}
					}
				}
				else if(!strcmp((char *)cur->name, "Center")){
					if ( cur->properties != NULL)  {
						pAttr = (xmlAttrPtr)cur->properties;
						while (pAttr != NULL) {
							if(!strcmp((char *)pAttr->name, "x")){
								tmp = strdup((char *)pAttr->children->content);
								ad->Center_x = atof(tmp);
								free(tmp);
								//printf("--------- %s=\"%2.2f\"\n", (char *)pAttr->name, ad->Center_x);
							}
							else if(!strcmp((char *)pAttr->name, "y")){
								tmp = strdup((char *)pAttr->children->content);
								ad->Center_y = atof(tmp);
								free(tmp);
								//printf("--------- %s=\"%2.2f\"\n", (char *)pAttr->name, ad->Center_y);
							}
							else if(!strcmp((char *)pAttr->name, "z")){
								tmp = strdup((char *)pAttr->children->content);
								ad->Center_z = atof(tmp);
								free(tmp);
								//printf("--------- %s=\"%2.2f\"\n", (char *)pAttr->name, ad->Center_z);
							}
							else if(!strcmp((char *)pAttr->name, "attribute")){
								ad->Center_type = get_attribute_type((char *)pAttr->children->content);
								//printf("--------- %s=\"%s\"\n", (char *)pAttr->name, ad->Red->type);
							}
							pAttr=pAttr->next;
						}
					}
				}
				else if(!strcmp((char *)cur->name, "Red")){
					if ( cur->properties != NULL)  {
						pAttr = (xmlAttrPtr)cur->properties;
						while (pAttr != NULL) {
							if(!strcmp((char *)pAttr->name, "from")){
								tmp = strdup((char *)pAttr->children->content);
								ad->Red->from = atoi(tmp);
								free(tmp);
								//printf("--------- %s=\"%d\"\n", (char *)pAttr->name, ad->Red->from);
							}
							else if(!strcmp((char *)pAttr->name, "to")){
								tmp = strdup((char *)pAttr->children->content);
								ad->Red->to = atoi(tmp);
								free(tmp);
								//printf("--------- %s=\"%d\"\n", (char *)pAttr->name, ad->Red->to);
							}
							else if(!strcmp((char *)pAttr->name, "type")){
								ad->Red->type = get_change_type((char *)pAttr->children->content);
								//printf("--------- %s=\"%s\"\n", (char *)pAttr->name, ad->Red->type);
							}
							pAttr=pAttr->next;
						}
					}
				}
				else if(!strcmp((char *)cur->name, "Green")){
					if ( cur->properties != NULL)  {
						pAttr = (xmlAttrPtr)cur->properties;
						while (pAttr != NULL) {
							if(!strcmp((char *)pAttr->name, "from")){
								tmp = strdup((char *)pAttr->children->content);
								ad->Green->from = atoi(tmp);
								free(tmp);
								//printf("--------- %s=\"%d\"\n", (char *)pAttr->name, ad->Green->from);
							}
							else if(!strcmp((char *)pAttr->name, "to")){
								tmp = strdup((char *)pAttr->children->content);
								ad->Green->to = atoi(tmp);
								free(tmp);
								//printf("--------- %s=\"%d\"\n", (char *)pAttr->name, ad->Green->to);
							}
							else if(!strcmp((char *)pAttr->name, "type")){
								ad->Green->type = get_change_type((char *)pAttr->children->content);
								//printf("--------- %s=\"%s\"\n", (char *)pAttr->name, ad->Green->type);
							}
							pAttr=pAttr->next;
						}
					}
				}
				else if(!strcmp((char *)cur->name, "Blue")){
					if ( cur->properties != NULL)  {
						pAttr = (xmlAttrPtr)cur->properties;
						while (pAttr != NULL) {
							if(!strcmp((char *)pAttr->name, "from")){
								tmp = strdup((char *)pAttr->children->content);
								ad->Blue->from = atoi(tmp);
								free(tmp);
								//printf("--------- %s=\"%d\"\n", (char *)pAttr->name, ad->Blue->from);
							}
							else if(!strcmp((char *)pAttr->name, "to")){
								tmp = strdup((char *)pAttr->children->content);
								ad->Blue->to = atoi(tmp);
								free(tmp);
								//printf("--------- %s=\"%d\"\n", (char *)pAttr->name, ad->Blue->to);
							}
							else if(!strcmp((char *)pAttr->name, "type")){
								ad->Blue->type = get_change_type((char *)pAttr->children->content);
								//printf("--------- %s=\"%s\"\n", (char *)pAttr->name, ad->Blue->type);
							}
							pAttr=pAttr->next;
						}
					}
				}
				else if(!strcmp((char *)cur->name, "Alpha")){
					if ( cur->properties != NULL)  {
						pAttr = (xmlAttrPtr)cur->properties;
						while (pAttr != NULL) {
							if(!strcmp((char *)pAttr->name, "from")){
								tmp = strdup((char *)pAttr->children->content);
								ad->Alpha->from = atoi(tmp);
								free(tmp);
								//printf("--------- %s=\"%d\"\n", (char *)pAttr->name, ad->Alpha->from);
							}
							else if(!strcmp((char *)pAttr->name, "to")){
								tmp = strdup((char *)pAttr->children->content);
								ad->Alpha->to = atoi(tmp);
								free(tmp);
								//printf("--------- %s=\"%d\"\n", (char *)pAttr->name, ad->Alpha->to);
							}
							else if(!strcmp((char *)pAttr->name, "type")){
								ad->Alpha->type = get_change_type((char *)pAttr->children->content);
								//printf("--------- %s=\"%s\"\n", (char *)pAttr->name, ad->Alpha->type);
							}
							pAttr=pAttr->next;
						}
					}
				}
				list = save_animation_data(list, doc, cur->xmlChildrenNode);
			}
		}
		cur = cur->next;
	}

	return list;
}

static Eina_List *
read_data_from_xml(Eina_List *list, const char *xmlFile)
{
    xmlDocPtr doc = NULL;
    xmlNodePtr root = NULL;

	doc = xmlReadFile(xmlFile, NULL, 0);

	if (doc == NULL) {
        printf("error: could not parse file %s\n", xmlFile);
		return NULL;
    }

    /*Get the root element node */
    root = xmlDocGetRootElement(doc);

    list = save_animation_data(list, doc, root);

    /*free the document */
    xmlFreeDoc(doc);
  
    xmlCleanupParser();

	return list;
}


/**
 * Add new xml_animator. 
 *
 * @param parent Parent object
 * @return xml_animator
 *
 * @ingroup Xml_Animator
 */
EAPI Elm_Xml_Animator *
elm_xml_animator_add(Evas_Object *parent)
{
	Elm_Xml_Animator *xa;
	xa = (Elm_Xml_Animator *)calloc(1, sizeof(Elm_Xml_Animator));

	xa->parent = parent;

//	Animation_Data *ad_;
  //  ad_ = (Animation_Data *)eina_list_data_get(list);

	//move_object_with_animation(list, icon, move_evas_map, NULL, NULL);

   return xa;
}

/**
 * Read xml file. 
 *
 * @param xa xml_animator data
 * @param file xml file name
 * @return true or false
 *
 * @ingroup Xml_Animator 
 */
EAPI Eina_Bool
elm_xml_animator_file_set(Elm_Xml_Animator *xa, const char *file)
{
	xa->animation_list = read_data_from_xml(xa->animation_list, file);
	if(xa->animation_list == NULL)
		return EINA_FALSE;

	return EINA_TRUE;
}

/**
 * Link object with animation. 
 *
 * @param xa xml_animator data
 * @param obj The object for animation
 * @param title the name of animation
 * @return true or false
 *
 * @ingroup Xml_Animator 
 */
EAPI Eina_Bool
elm_xml_animator_object_add(Elm_Xml_Animator *xa, Evas_Object *obj, const char *title, void (*end_func)(void *data, Evas_Object *obj), void *data)
{
	Animation_List *al;
	const Eina_List *l;
	Eina_Bool check = EINA_FALSE;

	if (!xa) return EINA_FALSE;

	Object_List *ol = (Object_List *)calloc(1, sizeof(Object_List));

	ol->object = obj;
	ol->end_func = end_func;
	ol->data = data;
	ol->remove_me = EINA_FALSE;
	
	EINA_LIST_FOREACH(xa->animation_list, l, al){
		if(!strcmp(al->animation_title, title)){
			ol->animation_list = al;
			check = EINA_TRUE;
		}
	}

	if(check)
	{
		xa->object_list = eina_list_append(xa->object_list, ol);
	}
	else
	{
		free(ol);
		ol = NULL;
	}
	return check;
}

static Eina_Bool
object_delete(void *data)
{
	const Eina_List *l;
	Object_List *ol;
	Elm_Xml_Animator *xa = (Elm_Xml_Animator *)data;
	
	EINA_LIST_FOREACH(xa->object_list, l, ol){
		if(ol->remove_me){
			xa->object_list = eina_list_remove(xa->object_list, ol);
			free(ol);
		}
	}

	return ECORE_CALLBACK_CANCEL;
}

/**
 * Delete the link of the object. 
 *
 * @param xa xml_animator data
 * @param obj The object for animation
 *
 * @ingroup Xml_Animator 
 */
EAPI void
elm_xml_animator_object_del(Elm_Xml_Animator *xa, Evas_Object *obj)
{
	const Eina_List *l;
	Object_List *ol;

	EINA_LIST_FOREACH(xa->object_list, l, ol){
		if(ol->object == obj){
			ol->remove_me = EINA_TRUE;
		}
	}
	ecore_idler_add(object_delete, xa); 
}

/**
 * Delete animation. 
 *
 * @param xa xml_animator data
 *
 * @ingroup Xml_Animator 
 */
EAPI void 
elm_xml_animator_del(Elm_Xml_Animator *xa)
{
	Object_List *ol;
	Animation_List *al;
	Animation_Data *ad;
	const Eina_List *l, *ll;

	EINA_LIST_FOREACH(xa->object_list, l, ol){
		xa->object_list = eina_list_remove(xa->object_list, ol);
		free(ol);
		ol = NULL;
	}

	EINA_LIST_FOREACH(xa->animation_list, l, al){
		EINA_LIST_FOREACH(al->scene_list, ll, ad){
			if(ad->CoordX){
				free(ad->CoordX);
				ad->CoordX = NULL;
			}
			if(ad->CoordY){
				free(ad->CoordY);
				ad->CoordY = NULL;
			}
			if(ad->CoordW){
				free(ad->CoordW);
				ad->CoordW = NULL;
			}
			if(ad->CoordH){
				free(ad->CoordH);
				ad->CoordH = NULL;
			}
			if(ad->AngleX){
				free(ad->AngleX);
				ad->AngleX = NULL;
			}
			if(ad->AngleY){
				free(ad->AngleY);
				ad->AngleY = NULL;
			}
			if(ad->AngleZ){
				free(ad->AngleZ);
				ad->AngleZ = NULL;
			}
			if(ad->Red){
				free(ad->Red);
				ad->Red = NULL;
			}
			if(ad->Green){
				free(ad->Green);
				ad->Green = NULL;
			}
			if(ad->Blue){
				free(ad->Blue);
				ad->Blue = NULL;
			}
			if(ad->Alpha){
				free(ad->Alpha);
				ad->Alpha = NULL;
			}
			if(ad->timer){
				ecore_animator_del(ad->timer);
				ad->timer = NULL;
			}
			al->scene_list = eina_list_remove(al->scene_list, ad);
			free(ad);
			ad = NULL;
		}
		xa->animation_list = eina_list_remove(xa->animation_list, al);
		free(al);
		al = NULL;
	}
	
	free(xa);
	xa = NULL;
}

/**
 * Run animation. 
 *
 * @param xa xml_animator data
 *
 * @ingroup Xml_Animator 
 */
EAPI void 
elm_xml_animator_run(Elm_Xml_Animator *xa)
{
	Object_List *ol;
	const Eina_List *l;

	EINA_LIST_FOREACH(xa->object_list, l, ol){
		move_object_with_animation(ol->animation_list->scene_list, ol->object, move_evas_map, ol->end_func, ol->data);
	}
}
