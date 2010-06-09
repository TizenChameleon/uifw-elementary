#include <Elementary.h>
#include "elm_priv.h"

#define TRUE   1
#define FALSE 0

//#define LAYOUT_x 240
//#define LAYOUT_y 400
//#define LAYOUT_x 480
//#define LAYOUT_y 800

typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object * viewflipper;
   Evas_Object * left_lay;
   Evas_Object * middle_lay;
   Evas_Object * right_lay;

   int is_left_show;
   int is_middle_show;
   int is_right_show;   

   int is_can_move;

   int is_auto_start;
   int is_show_button;

   int request_direction;
   int request_pos;
   int time_sec;
   Ecore_Timer * auto_timer;

   int temp_x_down;
   int temp_x_up;
   
   Eina_List *view_list;
   int selected_view;
   int all_count;
};

Widget_Data * this_pointer=NULL;

static const char *widtype = NULL;

static void _set_this(Widget_Data * in_this_pointer);
static void _del_hook(Evas_Object *obj);
static void _theme_hook(Evas_Object *obj);
static void _sizing_eval(Evas_Object *obj);
static int    _is_can_move();
static int    _is_first_view();
static int    _is_last_view();
static void _mouse_flip_cb(int mouse_direction);
static void _mouse_up(void *data, Evas *e, Evas_Object *o, void *event_info);
static void _mouse_down(void *data, Evas *e, Evas_Object *o, void *event_info);
static void _complete_move(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _left_button_click(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _right_button_click(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__);
static int   _flipping_timer_cb(void *data);
static void _set_layout_each_view(Evas_Object * left,Evas_Object * middle,Evas_Object *right);
static int   _setView();
static void _setup_button_show_left_right();



	
static void
_set_this(Widget_Data * in_this_pointer)
{
	this_pointer=in_this_pointer;
}

Widget_Data * _get_this()
{
	return this_pointer;
}
static void
_del_pre_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   elm_viewflipper_stopAutoFlipping();
  _set_layout_each_view(NULL,NULL,NULL);
  if(wd->viewflipper!=NULL)
  {
	evas_object_del(wd->viewflipper);
  }
  eina_list_free(wd->view_list);
  wd->view_list=NULL;
}
static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);   

   if (!wd) return;
   free(wd);
}

static void
_theme_hook(Evas_Object *obj)
{
   printf("_theme_hook  \n");
   
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   _sizing_eval(obj);
   evas_object_show(wd->viewflipper);
}
static void
_sizing_eval(Evas_Object *obj)
{
    Widget_Data *wd = elm_widget_data_get(obj);
   Evas_Coord minw = -1, minh = -1;

   edje_object_size_min_calc(wd->viewflipper, &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, -1, -1);
   evas_object_resize(wd->viewflipper, minw, minh);	

    printf("_sizing_eval %d %d  \n",minw,minh);

    Eina_List *l;
    Evas_Object * it;

    EINA_LIST_FOREACH(_get_this()->view_list, l, it)
    {
	   evas_object_resize(it, minw, minh);	
    }  
}  

static int
_is_can_move()
{
	Widget_Data *wd = _get_this();
	return wd->is_can_move;
}

static int
_is_first_view()
{
	if(elm_viewflipper_getCountChild()==0)
		return FALSE;

	if(_get_this()->selected_view==0)
		return TRUE;
	return FALSE;
}

static int
_is_last_view()
{
	if(elm_viewflipper_getCountChild()==0)
		return FALSE;

	Widget_Data *wd = _get_this();
	if(wd->selected_view==(elm_viewflipper_getCountChild()-1))
		return TRUE;
	return FALSE;
}

static void
_mouse_flip_cb(int mouse_direction)
{
	if(mouse_direction == DIR_RIGHT)
		elm_viewflipper_setNextView();
	else elm_viewflipper_setPreviousView();
}

static void
_mouse_up(void *data, Evas *e, Evas_Object *o, void *event_info)
{
   Evas_Event_Mouse_Up *ev = event_info;
   _get_this()->temp_x_up = ev->canvas.x;
   printf("mouse up start\n");

  int temp_value= _get_this()->temp_x_up - _get_this()->temp_x_down;
  if(temp_value <0)
  	temp_value = temp_value * -1;

   if(temp_value < FLIP_RANGE_X)
   	 return;
   
   if(_get_this()->temp_x_up>_get_this()->temp_x_down)
   {
		printf("left flip\n");
		_mouse_flip_cb(DIR_LEFT);
   }
   else
   {
		printf("right flip\n");
		_mouse_flip_cb(DIR_RIGHT);
   }
     printf("mouse up end\n");
}

static void
_mouse_down(void *data, Evas *e, Evas_Object *o, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
  _get_this()->temp_x_down = ev->canvas.x; 
  printf("mouse down\n");
}

static void
_complete_move(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
	_get_this()->is_can_move=TRUE;
	_setView();
}
static void
_left_button_click(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
	elm_viewflipper_setPreviousView();
	printf("_left_button_click\n");
}
static void
_right_button_click(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
	elm_viewflipper_setNextView();
	printf("_right_button_click\n");
}

static int
_flipping_timer_cb(void *data)
{
	if(elm_viewflipper_isAutoStart()==FALSE)
		return 0;

	if(_get_this()->request_pos!=EACH_LAST && _get_this()->selected_view ==_get_this()->request_pos)
	{
		printf("_flipping_timer_cb stop using each_last");
		elm_viewflipper_stopAutoFlipping();
		return 0;
	}

	if(_get_this()->request_direction==DIR_RIGHT)
		elm_viewflipper_setNextView();
	else elm_viewflipper_setPreviousView();
	
	if(_is_last_view()==TRUE)
	{	
		printf("_flipping_timer_cb stop");
		elm_viewflipper_stopAutoFlipping();
		return 0;
	}	
        printf("_flipping_timer_cb ing");
}

static void
_set_layout_each_view(Evas_Object * left,Evas_Object * middle,Evas_Object *right)
{
	Widget_Data *wd = _get_this();
	edje_object_signal_emit(_get_this()->viewflipper, "change_default_pos", "elm");
	
	if(wd->left_lay!=NULL)
	{
		edje_object_part_unswallow(wd->viewflipper, wd->left_lay);
		evas_object_hide(wd->left_lay);
		wd->left_lay=NULL;	
	}	
	if(wd->middle_lay!=NULL)
	{
		edje_object_part_unswallow(wd->viewflipper, wd->middle_lay);
		evas_object_hide(wd->middle_lay);
		wd->middle_lay=NULL;	
	}
	if(wd->right_lay!=NULL)
	{
		edje_object_part_unswallow(wd->viewflipper, wd->right_lay);	
		evas_object_hide(wd->right_lay);
		wd->right_lay=NULL;
	}

         if(left==NULL)
	 {
	 	wd->is_left_show = FALSE;		
	 }
	 else 
	 {
		wd->left_lay=left;				
		edje_object_part_swallow(wd->viewflipper, "first_view",left);
		wd->is_left_show = TRUE;			

	        printf("_set_layout_each_view 1\n");		
	 }

	 if(middle==NULL)
	 {
		_get_this()->is_middle_show=FALSE;
	 }
	 else
	 {
		wd->middle_lay=middle;	
		edje_object_part_swallow(wd->viewflipper, "second_view",middle );
		_get_this()->is_left_show = TRUE;
		
		printf("_set_layout_each_view 2\n");	
	 }

	if(right==NULL)
	{	
		_get_this()->is_right_show=FALSE;
	}
	else
	{
		wd->right_lay=right;	
		edje_object_part_swallow(wd->viewflipper, "third_view", right);		
 		_get_this()->is_right_show = TRUE;

     	         printf("_set_layout_each_view 3\n");	
	}
}
static int 
_setView()
{
	Widget_Data *wd = _get_this();
	if(elm_viewflipper_getCountChild()==0)
		return ERROR_3;
	if(_is_first_view()==TRUE)
	{
		if(elm_viewflipper_getCountChild()==1)
		{
			_set_layout_each_view(NULL, elm_viewflipper_getChildAt(0),NULL);
		         printf("_setView 1\n");
		}
		else//than 1
		{
			_set_layout_each_view(NULL, elm_viewflipper_getChildAt(0), elm_viewflipper_getChildAt(1));	
			printf("_setView 2\n");
		}		
	}
	else if(_is_last_view()==TRUE)
	{
		if(elm_viewflipper_getCountChild()==1)
		{
			_set_layout_each_view(NULL, elm_viewflipper_getChildAt(wd->selected_view),NULL);
			 printf("_setView 3\n");
		}
		else//than 1
		{
			_set_layout_each_view( elm_viewflipper_getChildAt((wd->selected_view)-1), elm_viewflipper_getChildAt(wd->selected_view),NULL);
			 printf("_setView 4\n");
		}	
	}
	else
	{
		_set_layout_each_view( elm_viewflipper_getChildAt((wd->selected_view)-1), elm_viewflipper_getChildAt(wd->selected_view), elm_viewflipper_getChildAt((wd->selected_view)+1));
		 printf("_setView 5\n");
	}
	_setup_button_show_left_right();	
}	

static void 
_setup_button_show_left_right()
{
	if(_get_this()->is_show_button==TRUE)
	{
		if(elm_viewflipper_getCountChild()==0)
			return;
		else if(_is_first_view()==TRUE)
		{
			edje_object_signal_emit(_get_this()->viewflipper, "right_button_show", "elm");
			edje_object_signal_emit(_get_this()->viewflipper, "left_button_hide", "elm");
		}
		else if(_is_last_view()==TRUE)
		{
			edje_object_signal_emit(_get_this()->viewflipper, "left_button_show", "elm");
			edje_object_signal_emit(_get_this()->viewflipper, "right_button_hide", "elm");
		}
		else 
		{		
			edje_object_signal_emit(_get_this()->viewflipper, "button_show", "elm");
		}
	}
}


EAPI Evas_Object * 
elm_viewflipper_add(Evas_Object * parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;
   Evas_Coord w, h;

   wd = ELM_NEW(Widget_Data);
   e = evas_object_evas_get(parent);
   obj = elm_widget_add(e);
   ELM_SET_WIDTYPE(widtype, "viewflipper");
   elm_widget_type_set(obj, "viewflipper");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_data_set(obj, wd);
   elm_widget_del_pre_hook_set(obj, _del_pre_hook);
   elm_widget_del_hook_set(obj, _del_hook);
   elm_widget_theme_hook_set(obj, _theme_hook);
   
    wd->viewflipper = edje_object_add(evas_object_evas_get(obj));
    _elm_theme_set(wd->viewflipper,  "viewflipper", "base", "default");


//   evas_object_resize(wd->viewflipper, LAYOUT_x, LAYOUT_y);
//   evas_object_move(wd->viewflipper,0,0);
//   evas_object_show(wd->viewflipper);
  
    evas_object_event_callback_add(wd->viewflipper, EVAS_CALLBACK_MOUSE_DOWN,
                                  _mouse_down, obj);
    evas_object_event_callback_add(wd->viewflipper, EVAS_CALLBACK_MOUSE_UP,
                                  _mouse_up, obj);
 

    edje_object_signal_callback_add(wd->viewflipper, "elm,action,moving", "elm",
                                   _complete_move, obj);
    edje_object_signal_callback_add(wd->viewflipper, "clicked", "btn_left",
                                   _left_button_click, obj);
    edje_object_signal_callback_add(wd->viewflipper, "clicked", "btn_right",
                                   _right_button_click, obj);

   
    _set_this(wd);
    _get_this()->is_can_move=TRUE;

   elm_widget_resize_object_set(obj, wd->viewflipper);
   _sizing_eval(obj);

   printf("create_viewflipper\n");
   return obj;
}


EAPI int 
elm_viewflipper_isFlipbutton()
{
	return _get_this()->is_show_button;
}
EAPI 
int elm_viewflipper_isAutoStart()
{
	return _get_this()->is_auto_start;
}
EAPI int 
elm_viewflipper_isValidPos(int pos)
{
	if(elm_viewflipper_getCountChild()==0)
		return FALSE;

	if(pos<0 || pos >=elm_viewflipper_getCountChild())
		return FALSE;

	return TRUE;	
}
EAPI int 
elm_viewflipper_addChild(Evas_Object * child_layout)
{
	if(_get_this()==NULL)
		return ERROR_2;
	
	if(strncmp(elm_widget_style_get(child_layout),"layout",6)==0)
		return ERROR_1;
	
	_get_this()->view_list = eina_list_append(_get_this()->view_list, child_layout);
  printf("addChild\n");
	return SUCCESS;
}

EAPI int 
elm_viewflipper_removeChild(Evas_Object * child_layout)
{
	if(_get_this()==NULL)
		return ERROR_2;
	
	if(strncmp(elm_widget_style_get(child_layout),"layout",6)==0)
		return ERROR_1;
	
 
       Eina_List *l;
       Evas_Object * it;

       EINA_LIST_FOREACH(_get_this()->view_list, l, it)
    	{
		  if (it == child_layout)
		  {		
		     evas_object_hide(child_layout);
		     _get_this()->view_list = eina_list_remove_list(_get_this()->view_list, l);
		     _setView();
		     return SUCCESS;
		  }
    	 }   
	 return ERROR_0;  
}
EAPI int 
elm_viewflipper_removeChildAt(int pos)
{
	if(_get_this()==NULL)
		return ERROR_2;

	if(elm_viewflipper_isValidPos(pos)==FALSE)
		return ERROR_4;
	
 	int i=0;
       Eina_List *l;_set_layout_each_view(NULL, elm_viewflipper_getChildAt(0), elm_viewflipper_getChildAt(1));		
      Evas_Object * it;

       EINA_LIST_FOREACH(_get_this()->view_list, l, it)
    	{
		  if (i == pos)
		  {
                     Evas_Object * child_layout = elm_viewflipper_getChildAt(pos);
		     if(child_layout!=NULL)//defense code
		           evas_object_hide(child_layout);
		     _get_this()->view_list = eina_list_remove_list(_get_this()->view_list, l);
		     _setView();	 
		     return SUCCESS;
		  }
		  i++;
    	 }   
	 return ERROR_0;  
}

EAPI int 
elm_viewflipper_getPositionChild(Evas_Object * child_layout)
{
	if(_get_this()==NULL)
		return ERROR_2;
	
	if(strncmp(elm_widget_style_get(child_layout),"layout",6)==0)
		return ERROR_1;

	int i=0;
	Eina_List *l;
        Evas_Object * it;

       EINA_LIST_FOREACH(_get_this()->view_list, l, it)
    	{
		  if (it == child_layout)
		  {
		     return i;
		  }
		  i++;
    	 }   
	 return ERROR_0;  
}
EAPI Evas_Object * 
elm_viewflipper_getChildAt(int pos)//start 0
{
   if(_get_this()==NULL)
		return NULL;

   if(elm_viewflipper_isValidPos(pos)==FALSE)
		return NULL;

   int i=0;	
   Eina_List *l;
   Evas_Object *it;
   EINA_LIST_FOREACH(_get_this()->view_list, l, it)
   {
	  if (i== pos)
	  {
	       return it;
	  }
	  i++;
   } 
   return NULL;
}
EAPI int 
elm_viewflipper_getCountChild()
{
	if(_get_this()==NULL)
		return ERROR_2;
	   
	return eina_list_count(_get_this()->view_list);
}

EAPI int 
elm_viewflipper_setDisplayedChild(int pos)
{
	if(elm_viewflipper_isValidPos(pos)==FALSE)
		return ERROR_4;

	if(_get_this()->selected_view==pos)
		return ERROR_5;
	
	elm_viewflipper_stopAutoFlipping();

	if(_get_this()->selected_view>pos)
	{
		elm_viewflipper_startAutoFlippingWithOption(DIR_LEFT,pos);
	}
	else
	{
		elm_viewflipper_startAutoFlippingWithOption(DIR_RIGHT,pos);
	}
	
}

EAPI int 
elm_viewflipper_setNowView()
{
	return _setView();	
}
EAPI int 
elm_viewflipper_setNextView()
{
	if(_is_last_view()==TRUE)
		return ERROR_5;

	if(_get_this()->is_can_move==FALSE)
		return ERROR_6;

	_get_this()->selected_view = _get_this()->selected_view+1;
	_get_this()->is_can_move=FALSE;
	
	edje_object_signal_emit(_get_this()->viewflipper, "moving_left_pos", "elm");
	printf("elm_viewflipper_setNextView\n");
}
EAPI int 
elm_viewflipper_setPreviousView()
{
	if(_is_first_view()==TRUE)
		return ERROR_5;
	
	if(_get_this()->is_can_move==FALSE)
		return ERROR_6;

	_get_this()->selected_view = _get_this()->selected_view-1;
	_get_this()->is_can_move=FALSE;
	
	edje_object_signal_emit(_get_this()->viewflipper, "moving_right_pos", "elm");	
	printf("elm_viewflipper_setPreviousView\n");
}

EAPI int 
elm_viewflipper_setFlipInterval(int milliseconds) 
{
	if(milliseconds<=0)
		return ERROR_7;

	_get_this()->time_sec=milliseconds;
}



EAPI int 
elm_viewflipper_setFlipbutton(int showbutton)
{
	if(showbutton==TRUE)
	{
		_get_this()->is_show_button=TRUE;
		_setup_button_show_left_right();
	}
	else
	{
		_get_this()->is_show_button=FALSE;
		edje_object_signal_emit(_get_this()->viewflipper, "button_hide", "elm");	
	}
}


EAPI int 
elm_viewflipper_startAutoFlippingWithOption(int drection,int pos)
{	
	if(_get_this()->time_sec==0)
		return ERROR_7;

	if(elm_viewflipper_isValidPos(pos)==FALSE)
		return ERROR_4;
	
	if(elm_viewflipper_isAutoStart()==TRUE)
		elm_viewflipper_stopAutoFlipping();

	_get_this()->request_direction= drection;
	_get_this()->request_pos = pos;

	_get_this()->auto_timer=  ecore_timer_add(_get_this()->time_sec , _flipping_timer_cb, NULL);
	_get_this()->is_auto_start=TRUE;
       	printf("elm_viewflipper_startAutoFlippingWithOption\n");
}
EAPI int 
elm_viewflipper_startAutoFlipping() 
{
	if(_get_this()->time_sec==0)
		return ERROR_7;
	
	if(elm_viewflipper_isAutoStart()==TRUE)
		elm_viewflipper_stopAutoFlipping();

	_get_this()->request_direction= DIR_RIGHT;
	_get_this()->request_pos = EACH_LAST;

	_get_this()->auto_timer=  ecore_timer_add(_get_this()->time_sec , _flipping_timer_cb, NULL);
	_get_this()->is_auto_start=TRUE;
	printf("elm_viewflipper_startAutoFlipping\n");
}
EAPI int 
elm_viewflipper_stopAutoFlipping()
{	
	if(elm_viewflipper_isAutoStart()==FALSE)
		return ERROR_5;

	if(_get_this()->auto_timer)//defense code
		ecore_timer_del(_get_this()->auto_timer);
	_get_this()->auto_timer=NULL;
	_get_this()->is_auto_start=FALSE;	
	_get_this()->request_direction= 0;
	_get_this()->request_pos = 0;	
        printf("elm_viewflipper_stopAutoFlipping\n");
}


