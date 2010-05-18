#include <Elementary.h>
#ifndef ELM_LIB_QUICKLAUNCH
typedef struct _Testitem
{
   Elm_Genlist_Item *item;
   int mode;
   int onoff;
} Testitem;


static Elm_Genlist_Item_Class itc1;
char *gl_label_get(const void *data, Evas_Object *obj, const char *part)
{
   char buf[256];
   snprintf(buf, sizeof(buf), "Item # %i", (int)data);
   return strdup(buf);
}

Evas_Object *gl_icon_get(const void *data, Evas_Object *obj, const char *part)
{
   char buf[PATH_MAX];
   Evas_Object *ic = elm_icon_add(obj);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", PACKAGE_DATA_DIR);
   elm_icon_file_set(ic, buf, NULL);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   return ic;
}
Eina_Bool gl_state_get(const void *data, Evas_Object *obj, const char *part)
{
   return EINA_FALSE;
}
void gl_del(const void *data, Evas_Object *obj)
{
}

static void
gl_sel(void *data, Evas_Object *obj, void *event_info)
{
   printf("sel item data [%p] on genlist obj [%p], item pointer [%p]\n", data, obj, event_info);
}

static void
_move(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Evas_Object *gl = data;
   Evas_Event_Mouse_Move *ev = event_info;
   int where = 0;
   Elm_Genlist_Item *gli;
   gli = elm_genlist_at_xy_item_get(gl, ev->cur.canvas.x, ev->cur.canvas.y, &where);
   if (gli)
     printf("over %p, where %i\n", elm_genlist_item_data_get(gli), where);
   else
     printf("over none, where %i\n", where);
}

static void
_bt50_cb(void *data, Evas_Object *obj, void *event_info)
{
    elm_genlist_item_bring_in(data);
}

static void
_bt1500_cb(void *data, Evas_Object *obj, void *event_info)
{
    elm_genlist_item_middle_bring_in(data);
}

static void
_gl_selected(void *data, Evas_Object *obj, void *event_info)
{
   printf("selected: %p\n", event_info);
}

static void
_gl_clicked(void *data, Evas_Object *obj, void *event_info)
{
   printf("clicked: %p\n", event_info);
}

static void
_gl_longpress(void *data, Evas_Object *obj, void *event_info)
{
   printf("longpress %p\n", event_info);
}

void
test_genlist(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *win, *bg, *gl, *bt_50, *bt_1500, *bx;
   Evas_Object *over;
   Elm_Genlist_Item *gli;
   int i;

   win = elm_win_add(NULL, "genlist", ELM_WIN_BASIC);
   elm_win_title_set(win, "Genlist");
   elm_win_autodel_set(win, 1);

   bg = elm_bg_add(win);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   gl = elm_genlist_add(win);
   evas_object_smart_callback_add(gl, "selected", _gl_selected, NULL);
   evas_object_smart_callback_add(gl, "clicked", _gl_clicked, NULL);
   evas_object_smart_callback_add(gl, "longpressed", _gl_longpress, NULL);
   // FIXME: This causes genlist to resize the horiz axis very slowly :(
   // Reenable this and resize the window horizontally, then try to resize it back
   //elm_genlist_horizontal_mode_set(gl, ELM_LIST_LIMIT);
   evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, gl);
   evas_object_show(gl);

   over = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_color_set(over, 0, 0, 0, 0);
   evas_object_event_callback_add(over, EVAS_CALLBACK_MOUSE_MOVE, _move, gl);
   evas_object_repeat_events_set(over, 1);
   evas_object_show(over);
   evas_object_size_hint_weight_set(over, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, over);

   itc1.item_style     = "default";
   itc1.func.label_get = gl_label_get;
   itc1.func.icon_get  = gl_icon_get;
   itc1.func.state_get = gl_state_get;
   itc1.func.del       = gl_del;

   bt_50 = elm_button_add(win);
   elm_button_label_set(bt_50, "Go to 50");
   evas_object_show(bt_50);
   elm_box_pack_end(bx, bt_50);

   bt_1500 = elm_button_add(win);
   elm_button_label_set(bt_1500, "Go to 1500");
   evas_object_show(bt_1500);
   elm_box_pack_end(bx, bt_1500);

   for (i = 0; i < 2000; i++)
     {
        gli = elm_genlist_item_append(gl, &itc1,
                                      (void *)i/* item data */,
                                      NULL/* parent */,
                                      ELM_GENLIST_ITEM_NONE,
                                      gl_sel/* func */,
                                      (void *)(i * 10)/* func data */);
        if (i == 50)
          evas_object_smart_callback_add(bt_50, "clicked", _bt50_cb, gli);
        else if (i == 1500)
          evas_object_smart_callback_add(bt_1500, "clicked", _bt1500_cb, gli);
     }
   evas_object_resize(win, 480, 800);
   evas_object_show(win);
}

/*************/

static void
my_gl_clear(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *gl = data;
   elm_genlist_clear(gl);
}

static void
my_gl_add(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *gl = data;
   Elm_Genlist_Item *gli;
   static int i = 0;

   itc1.item_style     = "default";
   itc1.func.label_get = gl_label_get;
   itc1.func.icon_get  = gl_icon_get;
   itc1.func.state_get = gl_state_get;
   itc1.func.del       = gl_del;

   gli = elm_genlist_item_append(gl, &itc1,
				 (void *)i/* item data */,
				 NULL/* parent */,
				 ELM_GENLIST_ITEM_NONE,
				 gl_sel/* func */,
				 (void *)(i * 10)/* func data */);
   i++;
}

static void
my_gl_insert_before(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *gl = data;
   Elm_Genlist_Item *gli;
   static int i = 0;
   Elm_Genlist_Item *gli_selected;

   itc1.item_style     = "default";
   itc1.func.label_get = gl_label_get;
   itc1.func.icon_get  = gl_icon_get;
   itc1.func.state_get = gl_state_get;
   itc1.func.del       = gl_del;

   gli_selected = elm_genlist_selected_item_get(gl);
   if(!gli_selected)
   {
       printf("no item selected\n");
       return ;
   }

   gli = elm_genlist_item_insert_before(gl, &itc1,
				 (void *)i/* item data */,
				 gli_selected /* item before */,
				 ELM_GENLIST_ITEM_NONE,
				 gl_sel/* func */,
				 (void *)(i * 10)/* func data */);
   i++;
}

static void
my_gl_insert_after(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *gl = data;
   Elm_Genlist_Item *gli;
   static int i = 0;
   Elm_Genlist_Item *gli_selected;

   itc1.item_style     = "default";
   itc1.func.label_get = gl_label_get;
   itc1.func.icon_get  = gl_icon_get;
   itc1.func.state_get = gl_state_get;
   itc1.func.del       = gl_del;

   gli_selected = elm_genlist_selected_item_get(gl);
   if(!gli_selected)
   {
       printf("no item selected\n");
       return ;
   }

   gli = elm_genlist_item_insert_after(gl, &itc1,
				 (void *)i/* item data */,
				 gli_selected /* item after */,
				 ELM_GENLIST_ITEM_NONE,
				 gl_sel/* func */,
				 (void *)(i * 10)/* func data */);
   i++;
}

static void
my_gl_del(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *gl = data;
   Elm_Genlist_Item *gli = elm_genlist_selected_item_get(gl);
   if (!gli)
     {
	printf("no item selected\n");
	return;
     }
   elm_genlist_item_del(gli);
}

static void
my_gl_disable(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *gl = data;
   Elm_Genlist_Item *gli = elm_genlist_selected_item_get(gl);
   if (!gli)
     {
	printf("no item selected\n");
	return;
     }
   elm_genlist_item_disabled_set(gli, 1);
   elm_genlist_item_selected_set(gli, 0);
   elm_genlist_item_update(gli);
}

static void
my_gl_update_all(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *gl = data;
   int i = 0;
   Elm_Genlist_Item *it = elm_genlist_first_item_get(gl);
   while (it)
     {
	elm_genlist_item_update(it);
	printf("%i\n", i);
	i++;
	it = elm_genlist_item_next_get(it);
     }
}

static void
my_gl_first(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *gl = data;
   Elm_Genlist_Item *gli = elm_genlist_first_item_get(gl);
   if (!gli) return;
   elm_genlist_item_show(gli);
   elm_genlist_item_selected_set(gli, 1);
}

static void
my_gl_last(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *gl = data;
   Elm_Genlist_Item *gli = elm_genlist_last_item_get(gl);
   if (!gli) return;
   elm_genlist_item_show(gli);
   elm_genlist_item_selected_set(gli, 1);
}

static int
my_gl_flush_delay(void *data)
{
   elm_all_flush();
   return 0;
}

static void
my_gl_flush(void *data, Evas_Object *obj, void *event_info)
{
   ecore_timer_add(1.2, my_gl_flush_delay, NULL);
}

void
test_genlist2(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *win, *bg, *gl, *bx, *bx2, *bx3, *bt;
   Elm_Genlist_Item *gli[10];
   char buf[PATH_MAX];

   win = elm_win_add(NULL, "genlist-2", ELM_WIN_BASIC);
   elm_win_title_set(win, "Genlist 2");
   elm_win_autodel_set(win, 1);

   bg = elm_bg_add(win);
   snprintf(buf, sizeof(buf), "%s/images/plant_01.jpg", PACKAGE_DATA_DIR);
   elm_bg_file_set(bg, buf, NULL);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   gl = elm_genlist_add(win);
   evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(gl);

   itc1.item_style     = "default";
   itc1.func.label_get = gl_label_get;
   itc1.func.icon_get  = gl_icon_get;
   itc1.func.state_get = gl_state_get;
   itc1.func.del       = gl_del;

   gli[0] = elm_genlist_item_append(gl, &itc1,
				    (void *)1001/* item data */, NULL/* parent */, ELM_GENLIST_ITEM_NONE, gl_sel/* func */,
				    (void *)1001/* func data */);
   gli[1] = elm_genlist_item_append(gl, &itc1,
				    (void *)1002/* item data */, NULL/* parent */, ELM_GENLIST_ITEM_NONE, gl_sel/* func */,
				    (void *)1002/* func data */);
   gli[2] = elm_genlist_item_append(gl, &itc1,
				    (void *)1003/* item data */, NULL/* parent */, ELM_GENLIST_ITEM_NONE, gl_sel/* func */,
				    (void *)1003/* func data */);
   gli[3] = elm_genlist_item_prepend(gl, &itc1,
				     (void *)1004/* item data */, NULL/* parent */, ELM_GENLIST_ITEM_NONE, gl_sel/* func */,
				     (void *)1004/* func data */);
   gli[4] = elm_genlist_item_prepend(gl, &itc1,
				     (void *)1005/* item data */, NULL/* parent */, ELM_GENLIST_ITEM_NONE, gl_sel/* func */,
				     (void *)1005/* func data */);
   gli[5] = elm_genlist_item_insert_before(gl, &itc1,
					   (void *)1006/* item data */, gli[2]/* rel */, ELM_GENLIST_ITEM_NONE, gl_sel/* func */,
					   (void *)1006/* func data */);
   gli[6] = elm_genlist_item_insert_after(gl, &itc1,
					  (void *)1007/* item data */, gli[2]/* rel */, ELM_GENLIST_ITEM_NONE, gl_sel/* func */,
					  (void *)1007/* func data */);

   elm_box_pack_end(bx, gl);

   bx2 = elm_box_add(win);
   elm_box_horizontal_set(bx2, 1);
   elm_box_homogenous_set(bx2, 1);
   evas_object_size_hint_weight_set(bx2, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(bx2, EVAS_HINT_FILL, EVAS_HINT_FILL);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "/\\");
   evas_object_smart_callback_add(bt, "clicked", my_gl_first, gl);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "\\/");
   evas_object_smart_callback_add(bt, "clicked", my_gl_last, gl);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "#");
   evas_object_smart_callback_add(bt, "clicked", my_gl_disable, gl);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "U");
   evas_object_smart_callback_add(bt, "clicked", my_gl_update_all, gl);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   elm_box_pack_end(bx, bx2);
   evas_object_show(bx2);

   bx2 = elm_box_add(win);
   elm_box_horizontal_set(bx2, 1);
   elm_box_homogenous_set(bx2, 1);
   evas_object_size_hint_weight_set(bx2, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(bx2, EVAS_HINT_FILL, EVAS_HINT_FILL);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "X");
   evas_object_smart_callback_add(bt, "clicked", my_gl_clear, gl);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "+");
   evas_object_smart_callback_add(bt, "clicked", my_gl_add, gl);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "-");
   evas_object_smart_callback_add(bt, "clicked", my_gl_del, gl);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   elm_box_pack_end(bx, bx2);
   evas_object_show(bx2);

   bx3 = elm_box_add(win);
   elm_box_horizontal_set(bx3, 1);
   elm_box_homogenous_set(bx3, 1);
   evas_object_size_hint_weight_set(bx3, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(bx3, EVAS_HINT_FILL, EVAS_HINT_FILL);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "+ before");
   evas_object_smart_callback_add(bt, "clicked", my_gl_insert_before, gl);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx3, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "+ after");
   evas_object_smart_callback_add(bt, "clicked", my_gl_insert_after, gl);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx3, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "Flush");
   evas_object_smart_callback_add(bt, "clicked", my_gl_flush, gl);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx3, bt);
   evas_object_show(bt);

   elm_box_pack_end(bx, bx3);
   evas_object_show(bx3);


   evas_object_resize(win, 320, 320);
   evas_object_show(win);
}

/*************/

static Elm_Genlist_Item_Class itc2;
char *gl2_label_get(const void *data, Evas_Object *obj, const char *part)
{
   const Testitem *tit = data;
   char buf[256];
   snprintf(buf, sizeof(buf), "Item mode %i", tit->mode);
   return strdup(buf);
}
Evas_Object *gl2_icon_get(const void *data, Evas_Object *obj, const char *part)
{
   const Testitem *tit = data;
   char buf[PATH_MAX];
   Evas_Object *ic = elm_icon_add(obj);
   if (!strcmp(part, "elm.swallow.icon"))
     {
	if ((tit->mode & 0x3) == 0)
	  snprintf(buf, sizeof(buf), "%s/images/logo_small.png", PACKAGE_DATA_DIR);
	else if ((tit->mode & 0x3) == 1)
	  snprintf(buf, sizeof(buf), "%s/images/logo.png", PACKAGE_DATA_DIR);
	else if ((tit->mode & 0x3) == 2)
	  snprintf(buf, sizeof(buf), "%s/images/panel_01.jpg", PACKAGE_DATA_DIR);
	else if ((tit->mode & 0x3) == 3)
	  snprintf(buf, sizeof(buf), "%s/images/rock_01.jpg", PACKAGE_DATA_DIR);
	elm_icon_file_set(ic, buf, NULL);
     }
   else if (!strcmp(part, "elm.swallow.end"))
     {
	if ((tit->mode & 0x3) == 0)
	  snprintf(buf, sizeof(buf), "%s/images/sky_01.jpg", PACKAGE_DATA_DIR);
	else if ((tit->mode & 0x3) == 1)
	  snprintf(buf, sizeof(buf), "%s/images/sky_02.jpg", PACKAGE_DATA_DIR);
	else if ((tit->mode & 0x3) == 2)
	  snprintf(buf, sizeof(buf), "%s/images/sky_03.jpg", PACKAGE_DATA_DIR);
	else if ((tit->mode & 0x3) == 3)
	  snprintf(buf, sizeof(buf), "%s/images/sky_04.jpg", PACKAGE_DATA_DIR);
	elm_icon_file_set(ic, buf, NULL);
     }
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   return ic;
}
Eina_Bool gl2_state_get(const void *data, Evas_Object *obj, const char *part)
{
   return EINA_FALSE;
}
void gl2_del(const void *data, Evas_Object *obj)
{
}

static void
my_gl_update(void *data, Evas_Object *obj, void *event_info)
{
   Testitem *tit = data;
   tit->mode++;
   elm_genlist_item_update(tit->item);
}

void
test_genlist3(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *win, *bg, *gl, *bx, *bx2, *bt;
   static Testitem tit[3];

   win = elm_win_add(NULL, "genlist-3", ELM_WIN_BASIC);
   elm_win_title_set(win, "Genlist 3");
   elm_win_autodel_set(win, 1);

   bg = elm_bg_add(win);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   gl = elm_genlist_add(win);
   evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(gl);

   itc2.item_style     = "default";
   itc2.func.label_get = gl2_label_get;
   itc2.func.icon_get  = gl2_icon_get;
   itc2.func.state_get = gl2_state_get;
   itc2.func.del       = gl2_del;

   tit[0].mode = 0;
   tit[0].item = elm_genlist_item_append(gl, &itc2,
					 &(tit[0])/* item data */, NULL/* parent */, ELM_GENLIST_ITEM_NONE, gl_sel/* func */,
					 NULL/* func data */);
   tit[1].mode = 1;
   tit[1].item = elm_genlist_item_append(gl, &itc2,
					 &(tit[1])/* item data */, NULL/* parent */, ELM_GENLIST_ITEM_NONE, gl_sel/* func */,
					 NULL/* func data */);
   tit[2].mode = 2;
   tit[2].item = elm_genlist_item_append(gl, &itc2,
					 &(tit[2])/* item data */, NULL/* parent */, ELM_GENLIST_ITEM_NONE, gl_sel/* func */,
					 NULL/* func data */);

   elm_box_pack_end(bx, gl);
   evas_object_show(bx);

   bx2 = elm_box_add(win);
   elm_box_horizontal_set(bx2, 1);
   elm_box_homogenous_set(bx2, 1);
   evas_object_size_hint_weight_set(bx2, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(bx2, EVAS_HINT_FILL, EVAS_HINT_FILL);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "[1]");
   evas_object_smart_callback_add(bt, "clicked", my_gl_update, &(tit[0]));
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "[2]");
   evas_object_smart_callback_add(bt, "clicked", my_gl_update, &(tit[1]));
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "[3]");
   evas_object_smart_callback_add(bt, "clicked", my_gl_update, &(tit[2]));
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   elm_box_pack_end(bx, bx2);
   evas_object_show(bx2);

   evas_object_resize(win, 320, 320);
   evas_object_show(win);
}

/*************/

static void
my_gl_item_check_changed(void *data, Evas_Object *obj, void *event_info)
{
   Testitem *tit = data;
   tit->onoff = elm_check_state_get(obj);
   printf("item %p onoff = %i\n", tit, tit->onoff);
}

static Elm_Genlist_Item_Class itc3;
char *gl3_label_get(const void *data, Evas_Object *obj, const char *part)
{
   const Testitem *tit = data;
   char buf[256];
   snprintf(buf, sizeof(buf), "Item mode %i", tit->mode);
   return strdup(buf);
}
Evas_Object *gl3_icon_get(const void *data, Evas_Object *obj, const char *part)
{
   const Testitem *tit = data;
   char buf[PATH_MAX];
   if (!strcmp(part, "elm.swallow.icon"))
     {
        Evas_Object *lb;
        
        lb = elm_label_add(obj);
        elm_label_line_wrap_set(lb, 1);
        elm_label_wrap_width_set(lb, 201);
        elm_label_label_set(lb, "ashj ascjscjh n asjkl hcjlh ls hzshnn zjh sh zxjcjsnd h dfw sdv edev efe fwefvv vsd cvs ws wf  fvwf wd fwe f  we wef we wfe rfwewef wfv wswf wefg sdfws w wsdcfwcf wsc vdv  sdsd sdcd cv wsc sdcv wsc d sdcdcsd sdcdsc wdvd sdcsd wscxcv wssvd sd");
        evas_object_show(lb);
        return lb;
        
	Evas_Object *bx = elm_box_add(obj);
	Evas_Object *ic;
	ic = elm_icon_add(obj);
	snprintf(buf, sizeof(buf), "%s/images/logo_small.png", PACKAGE_DATA_DIR);
	elm_icon_file_set(ic, buf, NULL);
	elm_icon_scale_set(ic, 0, 0);
	evas_object_show(ic);
	elm_box_pack_end(bx, ic);
	ic = elm_icon_add(obj);
	elm_icon_file_set(ic, buf, NULL);
	elm_icon_scale_set(ic, 0, 0);
	evas_object_show(ic);
	elm_box_pack_end(bx, ic);
        elm_box_horizontal_set(bx, 1);
	evas_object_show(bx);
	return bx;
     }
   else if (!strcmp(part, "elm.swallow.end"))
     {
	Evas_Object *ck;
	ck = elm_check_add(obj);
	evas_object_propagate_events_set(ck, 0);
	elm_check_state_set(ck, tit->onoff);
	evas_object_smart_callback_add(ck, "changed", my_gl_item_check_changed, data);
	evas_object_show(ck);
	return ck;
     }
   return NULL;
}
Eina_Bool gl3_state_get(const void *data, Evas_Object *obj, const char *part)
{
   return EINA_FALSE;
}
void gl3_del(const void *data, Evas_Object *obj)
{
}

void
test_genlist4(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *win, *bg, *gl, *bx, *bx2, *bt;
   static Testitem tit[3];

   win = elm_win_add(NULL, "genlist-4", ELM_WIN_BASIC);
   elm_win_title_set(win, "Genlist 4");
   elm_win_autodel_set(win, 1);

   bg = elm_bg_add(win);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   gl = elm_genlist_add(win);
   elm_genlist_multi_select_set(gl, 1);
   evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(gl);

   itc3.item_style     = "default";
   itc3.func.label_get = gl3_label_get;
   itc3.func.icon_get  = gl3_icon_get;
   itc3.func.state_get = gl3_state_get;
   itc3.func.del       = gl3_del;

   tit[0].mode = 0;
   tit[0].item = elm_genlist_item_append(gl, &itc3,
					 &(tit[0])/* item data */, NULL/* parent */, ELM_GENLIST_ITEM_NONE, gl_sel/* func */,
					 NULL/* func data */);
   tit[1].mode = 1;
   tit[1].item = elm_genlist_item_append(gl, &itc3,
					 &(tit[1])/* item data */, NULL/* parent */, ELM_GENLIST_ITEM_NONE, gl_sel/* func */,
					 NULL/* func data */);
   tit[2].mode = 2;
   tit[2].item = elm_genlist_item_append(gl, &itc3,
					 &(tit[2])/* item data */, NULL/* parent */, ELM_GENLIST_ITEM_NONE, gl_sel/* func */,
					 NULL/* func data */);

   elm_box_pack_end(bx, gl);
   evas_object_show(bx);

   bx2 = elm_box_add(win);
   elm_box_horizontal_set(bx2, 1);
   elm_box_homogenous_set(bx2, 1);
   evas_object_size_hint_weight_set(bx2, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(bx2, EVAS_HINT_FILL, EVAS_HINT_FILL);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "[1]");
   evas_object_smart_callback_add(bt, "clicked", my_gl_update, &(tit[0]));
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "[2]");
   evas_object_smart_callback_add(bt, "clicked", my_gl_update, &(tit[1]));
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "[3]");
   evas_object_smart_callback_add(bt, "clicked", my_gl_update, &(tit[2]));
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   elm_box_pack_end(bx, bx2);
   evas_object_show(bx2);

   evas_object_resize(win, 320, 320);
   evas_object_show(win);
}


/*************/
static void
my_gl_item_check_changed2(void *data, Evas_Object *obj, void *event_info)
{
   Testitem *tit = data;
   tit->onoff = elm_check_state_get(obj);
   printf("item %p onoff = %i\n", tit, tit->onoff);
}

static Elm_Genlist_Item_Class itc5;
char *gl5_label_get(const void *data, Evas_Object *obj, const char *part)
{
   const Testitem *tit = data;
   char buf[256];
   if (!strcmp(part, "elm.text"))
     {
	snprintf(buf, sizeof(buf), "Item mode %i", tit->mode);
     }
   else if (!strcmp(part, "elm.text.sub"))
     {
	snprintf(buf, sizeof(buf), "%i bottles on the wall", tit->mode);
     }
   return strdup(buf);
}
Evas_Object *gl5_icon_get(const void *data, Evas_Object *obj, const char *part)
{
   const Testitem *tit = data;
   char buf[PATH_MAX];
   if (!strcmp(part, "elm.swallow.icon"))
     {
	Evas_Object *bx = elm_box_add(obj);
	Evas_Object *ic;
	elm_box_horizontal_set(bx, 1);
	ic = elm_icon_add(obj);
	snprintf(buf, sizeof(buf), "%s/images/logo_small.png", PACKAGE_DATA_DIR);
	elm_icon_file_set(ic, buf, NULL);
	elm_icon_scale_set(ic, 0, 0);
	evas_object_show(ic);
	elm_box_pack_end(bx, ic);
	ic = elm_icon_add(obj);
	elm_icon_file_set(ic, buf, NULL);
	elm_icon_scale_set(ic, 0, 0);
	evas_object_show(ic);
	elm_box_pack_end(bx, ic);
        elm_box_horizontal_set(bx, 1);
	evas_object_show(bx);
	return bx;
     }
   else if (!strcmp(part, "elm.swallow.end"))
     {
	Evas_Object *ck;
	ck = elm_check_add(obj);
	evas_object_propagate_events_set(ck, 0);
	elm_check_state_set(ck, tit->onoff);
	evas_object_smart_callback_add(ck, "changed", my_gl_item_check_changed2, data);
	evas_object_show(ck);
	return ck;
     }
   return NULL;
}
Eina_Bool gl5_state_get(const void *data, Evas_Object *obj, const char *part)
{
   return EINA_FALSE;
}
void gl5_del(const void *data, Evas_Object *obj)
{
}

static void
item_drag_up(void *data, Evas_Object *obj, void *event_info)
{
   printf("drag up\n");
}

static void
item_drag_down(void *data, Evas_Object *obj, void *event_info)
{
   printf("drag down\n");
}

static void
item_drag_left(void *data, Evas_Object *obj, void *event_info)
{
   printf("drag left\n");
}

static void
item_drag_right(void *data, Evas_Object *obj, void *event_info)
{
   printf("drag right\n");
}

static void
item_drag(void *data, Evas_Object *obj, void *event_info)
{
   printf("drag\n");
}

static void
item_drag_stop(void *data, Evas_Object *obj, void *event_info)
{
   printf("drag stop\n");
}

static void
item_longpress(void *data, Evas_Object *obj, void *event_info)
{
   printf("longpress\n");
}

void
test_genlist5(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *win, *bg, *gl, *bx, *bx2, *bt;
   static Testitem tit[3];

   win = elm_win_add(NULL, "genlist-5", ELM_WIN_BASIC);
   elm_win_title_set(win, "Genlist 5");
   elm_win_autodel_set(win, 1);

   bg = elm_bg_add(win);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   gl = elm_genlist_add(win);
   elm_genlist_always_select_mode_set(gl, 1);
   evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(gl);
   itc5.item_style     = "double_label";
   itc5.func.label_get = gl5_label_get;
   itc5.func.icon_get  = gl5_icon_get;
   itc5.func.state_get = gl5_state_get;
   itc5.func.del       = gl5_del;

   tit[0].mode = 0;
   tit[0].item = elm_genlist_item_append(gl, &itc5,
					 &(tit[0])/* item data */, NULL/* parent */, ELM_GENLIST_ITEM_NONE, gl_sel/* func */,
					 NULL/* func data */);
   tit[1].mode = 1;
   tit[1].item = elm_genlist_item_append(gl, &itc5,
					 &(tit[1])/* item data */, NULL/* parent */, ELM_GENLIST_ITEM_NONE, gl_sel/* func */,
					 NULL/* func data */);
   tit[2].mode = 2;
   tit[2].item = elm_genlist_item_append(gl, &itc5,
					 &(tit[2])/* item data */, NULL/* parent */, ELM_GENLIST_ITEM_NONE, gl_sel/* func */,
					 NULL/* func data */);

   elm_box_pack_end(bx, gl);
   evas_object_show(bx);

   evas_object_smart_callback_add(gl, "drag,start,up", item_drag_up, NULL);
   evas_object_smart_callback_add(gl, "drag,start,down", item_drag_down, NULL);
   evas_object_smart_callback_add(gl, "drag,start,left", item_drag_left, NULL);
   evas_object_smart_callback_add(gl, "drag,start,right", item_drag_right, NULL);
   evas_object_smart_callback_add(gl, "drag", item_drag, NULL);
   evas_object_smart_callback_add(gl, "drag,stop", item_drag_stop, NULL);
   evas_object_smart_callback_add(gl, "longpressed", item_longpress, NULL);

   bx2 = elm_box_add(win);
   elm_box_horizontal_set(bx2, 1);
   elm_box_homogenous_set(bx2, 1);
   evas_object_size_hint_weight_set(bx2, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(bx2, EVAS_HINT_FILL, EVAS_HINT_FILL);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "[1]");
   evas_object_smart_callback_add(bt, "clicked", my_gl_update, &(tit[0]));
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "[2]");
   evas_object_smart_callback_add(bt, "clicked", my_gl_update, &(tit[1]));
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "[3]");
   evas_object_smart_callback_add(bt, "clicked", my_gl_update, &(tit[2]));
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   elm_box_pack_end(bx, bx2);
   evas_object_show(bx2);

   evas_object_resize(win, 320, 320);
   evas_object_show(win);
}

/*************/

static Elm_Genlist_Item_Class itc4;

static void
gl4_sel(void *data, Evas_Object *obj, void *event_info)
{
}
static void
gl4_exp(void *data, Evas_Object *obj, void *event_info)
{
   Elm_Genlist_Item *it = event_info;
   Evas_Object *gl = elm_genlist_item_genlist_get(it);
   int val = (int)elm_genlist_item_data_get(it);
   val *= 10;
   elm_genlist_item_append(gl, &itc4,
			   (void *)(val + 1)/* item data */, it/* parent */, ELM_GENLIST_ITEM_NONE, gl4_sel/* func */,
			   NULL/* func data */);
   elm_genlist_item_append(gl, &itc4,
			   (void *)(val + 2)/* item data */, it/* parent */, ELM_GENLIST_ITEM_NONE, gl4_sel/* func */,
			   NULL/* func data */);
   elm_genlist_item_append(gl, &itc4,
			   (void *)(val + 3)/* item data */, it/* parent */, ELM_GENLIST_ITEM_SUBITEMS, gl4_sel/* func */,
			   NULL/* func data */);
}
static void
gl4_con(void *data, Evas_Object *obj, void *event_info)
{
   Elm_Genlist_Item *it = event_info;
   elm_genlist_item_subitems_clear(it);
}

static void
gl4_exp_req(void *data, Evas_Object *obj, void *event_info)
{
   Elm_Genlist_Item *it = event_info;
   elm_genlist_item_expanded_set(it, 1);
}
static void
gl4_con_req(void *data, Evas_Object *obj, void *event_info)
{
   Elm_Genlist_Item *it = event_info;
   elm_genlist_item_expanded_set(it, 0);
}

char *gl4_label_get(const void *data, Evas_Object *obj, const char *part)
{
   char buf[256];
   snprintf(buf, sizeof(buf), "Item mode %i", (int)data);
   return strdup(buf);
}
Evas_Object *gl4_icon_get(const void *data, Evas_Object *obj, const char *part)
{
   char buf[PATH_MAX];
   if (!strcmp(part, "elm.swallow.icon"))
     {
	Evas_Object *ic = elm_icon_add(obj);
	snprintf(buf, sizeof(buf), "%s/images/logo_small.png", PACKAGE_DATA_DIR);
	elm_icon_file_set(ic, buf, NULL);
	evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
	evas_object_show(ic);
	return ic;
     }
   else if (!strcmp(part, "elm.swallow.end"))
     {
	Evas_Object *ck;
	ck = elm_check_add(obj);
	evas_object_show(ck);
	return ck;
     }
   return NULL;
}
Eina_Bool gl4_state_get(const void *data, Evas_Object *obj, const char *part)
{
   return EINA_FALSE;
}
void gl4_del(const void *data, Evas_Object *obj)
{
}

void
test_genlist6(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *win, *bg, *gl, *bx, *bx2, *bt;

   win = elm_win_add(NULL, "genlist-tree", ELM_WIN_BASIC);
   elm_win_title_set(win, "Genlist Tree");
   elm_win_autodel_set(win, 1);

   bg = elm_bg_add(win);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   gl = elm_genlist_add(win);
   evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(gl);

   itc4.item_style     = "default";
   itc4.func.label_get = gl4_label_get;
   itc4.func.icon_get  = gl4_icon_get;
   itc4.func.state_get = gl4_state_get;
   itc4.func.del       = gl4_del;

   elm_genlist_item_append(gl, &itc4,
			   (void *)1/* item data */, NULL/* parent */, ELM_GENLIST_ITEM_SUBITEMS, gl4_sel/* func */,
			   NULL/* func data */);
   elm_genlist_item_append(gl, &itc4,
			   (void *)2/* item data */, NULL/* parent */, ELM_GENLIST_ITEM_SUBITEMS, gl4_sel/* func */,
			   NULL/* func data */);
   elm_genlist_item_append(gl, &itc4,
			   (void *)3/* item data */, NULL/* parent */, ELM_GENLIST_ITEM_NONE, gl4_sel/* func */,
			   NULL/* func data */);

   evas_object_smart_callback_add(gl, "expand,request", gl4_exp_req, gl);
   evas_object_smart_callback_add(gl, "contract,request", gl4_con_req, gl);
   evas_object_smart_callback_add(gl, "expanded", gl4_exp, gl);
   evas_object_smart_callback_add(gl, "contracted", gl4_con, gl);

   elm_box_pack_end(bx, gl);
   evas_object_show(bx);

   bx2 = elm_box_add(win);
   elm_box_horizontal_set(bx2, 1);
   elm_box_homogenous_set(bx2, 1);
   evas_object_size_hint_weight_set(bx2, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(bx2, EVAS_HINT_FILL, EVAS_HINT_FILL);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "[1]");
//   evas_object_smart_callback_add(bt, "clicked", my_gl_update, &(tit[0]));
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "[2]");
//   evas_object_smart_callback_add(bt, "clicked", my_gl_update, &(tit[1]));
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "[3]");
//   evas_object_smart_callback_add(bt, "clicked", my_gl_update, &(tit[2]));
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   elm_box_pack_end(bx, bx2);
   evas_object_show(bx2);

   evas_object_resize(win, 320, 320);
   evas_object_show(win);
}
#endif
