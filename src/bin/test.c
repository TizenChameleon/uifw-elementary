#include <Elementary.h>
#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#ifndef ELM_LIB_QUICKLAUNCH

/* all tests prototypes */
void test_bg_plain(void *data, Evas_Object *obj, void *event_info);
void test_bg_image(void *data, Evas_Object *obj, void *event_info);
void test_bg_options(void *data, Evas_Object *obj, void *event_info);
void test_icon(void *data, Evas_Object *obj, void *event_info);
void test_box_vert(void *data, Evas_Object *obj, void *event_info);
void test_box_vert2(void *data, Evas_Object *obj, void *event_info);
void test_box_horiz(void *data, Evas_Object *obj, void *event_info);
void test_box_transition(void *data, Evas_Object *obj, void *event_info);
void test_button(void *data, Evas_Object *obj, void *event_info);
void test_transit(void *data, Evas_Object *obj, void *event_info);
void test_transit2(void *data, Evas_Object *obj, void *event_info);
void test_transit3(void *data, Evas_Object *obj, void *event_info);
void test_transit4(void *data, Evas_Object *obj, void *event_info);
void test_transit5(void *data, Evas_Object *obj, void *event_info);
void test_transit6(void *data, Evas_Object *obj, void *event_info);
void test_transit7(void *data, Evas_Object *obj, void *event_info);
void test_transit8(void *data, Evas_Object *obj, void *event_info);
void test_fileselector_button(void *data, Evas_Object *obj, void *event_info);
void test_fileselector_entry(void *data, Evas_Object *obj, void *event_info);
void test_toggle(void *data, Evas_Object *obj, void *event_info);
void test_clock(void *data, Evas_Object *obj, void *event_info);
void test_clock2(void *data, Evas_Object *obj, void *event_info);
void test_check(void *data, Evas_Object *obj, void *event_info);
void test_radio(void *data, Evas_Object *obj, void *event_info);
void test_layout(void *data, Evas_Object *obj, void *event_info);
void test_hover(void *data, Evas_Object *obj, void *event_info);
void test_hover2(void *data, Evas_Object *obj, void *event_info);
void test_entry(void *data, Evas_Object *obj, void *event_info);
void test_entry_scrolled(void *data, Evas_Object *obj, void *event_info);
void test_entry3(void *data, Evas_Object *obj, void *event_info);
void test_entry4(void *data, Evas_Object *obj, void *event_info);
void test_entry5(void *data, Evas_Object *obj, void *event_info);
void test_entry_notepad(void *data, Evas_Object *obj, void *event_info);
void test_anchorview(void *data, Evas_Object *obj, void *event_info);
void test_anchorblock(void *data, Evas_Object *obj, void *event_info);
void test_toolbar(void *data, Evas_Object *obj, void *event_info);
void test_toolbar2(void *data, Evas_Object *obj, void *event_info);
void test_toolbar3(void *data, Evas_Object *obj, void *event_info);
void test_toolbar4(void *data, Evas_Object *obj, void *event_info);
void test_toolbar5(void *data, Evas_Object *obj, void *event_info);
void test_hoversel(void *data, Evas_Object *obj, void *event_info);
void test_list(void *data, Evas_Object *obj, void *event_info);
void test_list_horizontal(void *data, Evas_Object *obj, void *event_info);
void test_list2(void *data, Evas_Object *obj, void *event_info);
void test_list3(void *data, Evas_Object *obj, void *event_info);
void test_list4(void *data, Evas_Object *obj, void *event_info);
void test_list5(void *data, Evas_Object *obj, void *event_info);
void test_inwin(void *data, Evas_Object *obj, void *event_info);
void test_inwin2(void *data, Evas_Object *obj, void *event_info);
void test_scaling(void *data, Evas_Object *obj, void *event_info);
void test_scaling2(void *data, Evas_Object *obj, void *event_info);
void test_slider(void *data, Evas_Object *obj, void *event_info);
void test_actionslider(void *data, Evas_Object *obj, void *event_info);
void test_genlist(void *data, Evas_Object *obj, void *event_info);
void test_genlist2(void *data, Evas_Object *obj, void *event_info);
void test_genlist3(void *data, Evas_Object *obj, void *event_info);
void test_genlist4(void *data, Evas_Object *obj, void *event_info);
void test_genlist5(void *data, Evas_Object *obj, void *event_info);
void test_genlist6(void *data, Evas_Object *obj, void *event_info);
void test_genlist7(void *data, Evas_Object *obj, void *event_info);
void test_genlist8(void *data, Evas_Object *obj, void *event_info);
void test_genlist9(void *data, Evas_Object *obj, void *event_info);
void test_genlist10(void *data, Evas_Object *obj, void *event_info);
void test_table(void *data, Evas_Object *obj, void *event_info);
void test_gengrid(void *data, Evas_Object *obj, void *event_info);
void test_gengrid2(void *data, Evas_Object *obj, void *event_info);
void test_pager(void *data, Evas_Object *obj, void *event_info);
void test_pager_slide(void *data, Evas_Object *obj, void *event_info);
void test_win_state(void *data, Evas_Object *obj, void *event_info);
void test_win_state2(void *data, Evas_Object *obj, void *event_info);
void test_progressbar(void *data, Evas_Object *obj, void *event_info);
void test_fileselector(void *data, Evas_Object *obj, void *event_info);
void test_separator(void *data, Evas_Object *obj, void *event_info);
void test_scroller(void *data, Evas_Object *obj, void *event_info);
void test_scroller2(void *data, Evas_Object *obj, void *event_info);
void test_spinner(void *data, Evas_Object *obj, void *event_info);
void test_index(void *data, Evas_Object *obj, void *event_info);
void test_index2(void *data, Evas_Object *obj, void *event_info);
void test_photocam(void *data, Evas_Object *obj, void *event_info);
void test_photo(void *data, Evas_Object *obj, void *event_info);
void test_thumb(void *data, Evas_Object *obj, void *event_info);
void test_icon_desktops(void *data, Evas_Object *obj, void *event_info);
void test_notify(void *data, Evas_Object *obj, void *event_info);
void test_slideshow(void *data, Evas_Object *obj, void *event_info);
void test_menu(void *data, Evas_Object *obj, void *event_info);
void test_panel(void *data, Evas_Object *obj, void *event_info);
void test_panes(void *data, Evas_Object *obj, void *event_info);
void test_calendar(void *data, Evas_Object *obj, void *event_info);
void test_calendar2(void *data, Evas_Object *obj, void *event_info);
void test_map(void *data, Evas_Object *obj, void *event_info);
void test_weather(void *data, Evas_Object *obj, void *event_info);
void test_flip(void *data, Evas_Object *obj, void *event_info);
void test_flip2(void *data, Evas_Object *obj, void *event_info);
void test_flip3(void *data, Evas_Object *obj, void *event_info);
void test_label(void *data, Evas_Object *obj, void *event_info);
void test_conformant(void *data, Evas_Object *obj, void *event_info);
void test_conformant2(void *data, Evas_Object *obj, void *event_info);
void test_multi(void *data, Evas_Object *obj, void *event_info);
void test_floating(void *data, Evas_Object *obj, void *event_info);
void test_launcher(void *data, Evas_Object *obj, void *event_info);
void test_launcher2(void *data, Evas_Object *obj, void *event_info);
void test_launcher3(void *data, Evas_Object *obj, void *event_info);
void test_anim(void *data, Evas_Object *obj, void *event_info);
void test_tooltip(void *data, Evas_Object *obj, void *event_info);
void test_tooltip2(void *data, Evas_Object *obj, void *event_info);
void test_cursor(void *data, Evas_Object *obj, void *event_info);
void test_cursor2(void *data, Evas_Object *obj, void *event_info);
void test_cursor3(void *data, Evas_Object *obj, void *event_info);
void test_focus(void *data, Evas_Object *obj, void *event_info);
void test_focus2(void *data, Evas_Object *obj, void *event_info);
void test_focus3(void *data, Evas_Object *obj, void *event_info);
void test_flipselector(void *data, Evas_Object *obj, void *event_info);
void test_diskselector(void *data, Evas_Object *obj, void *event_info);
void test_colorselector(void *data, Evas_Object *obj, void *event_info);
void test_ctxpopup(void *data, Evas_Object *obj, void *event_info);
void test_bubble(void *data, Evas_Object *obj, void *event_info);
void test_segment_control(void *data, Evas_Object *obj, void *event_info);
void test_store(void *data, Evas_Object *obj, void *event_info);
void test_win_inline(void *data, Evas_Object *obj, void *event_info);

struct elm_test
{
   const char *name;
   void (*cb)(void *, Evas_Object *, void *);
};

static int
elm_test_sort(const void *pa, const void *pb)
{
   const struct elm_test *a = pa, *b = pb;
   return strcasecmp(a->name, b->name);
}

static void
elm_test_add(Eina_List **p_list, const char *name, void (*cb)(void *, Evas_Object *, void *))
{
   struct elm_test *t = malloc(sizeof(struct elm_test));
   t->name = name;
   t->cb = cb;
   *p_list = eina_list_sorted_insert(*p_list, elm_test_sort, t);
}

static void
my_win_del(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   /* called when my_win_main is requested to be deleted */
   elm_exit(); /* exit the program's main loop that runs in elm_run() */
}

static void
index_changed(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   elm_list_item_show(event_info);
}

static void
_ui_tg_changed(void *data, Evas_Object *obj, void *event_info)
{
   (void) data; (void) event_info;
   elm_mirrored_set(elm_toggle_state_get(obj));
}


static void
my_win_main(char *autorun)
{
   Evas_Object *win, *bg, *bx0, *lb, *li, *idx, *fr, *tg;
   Eina_List *tests, *l;
   struct elm_test *t;

   /* create an elm window - it returns an evas object. this is a little
    * special as the object lives in the canvas that is inside the window
    * so what is returned is really inside the window, but as you manipulate
    * the evas object returned - the window will respond. elm_win makes sure
    * of that so you can blindly treat it like any other evas object
    * pretty much, just as long as you know it has special significance.
    * the first parameter is a "parent" window - eg for a dialog you want to
    * have a main window it is related to, here it is NULL meaning there
    * is no parent. "main" is the name of the window - used by the window
    * manager for identifying the window uniquely amongst all the windows
    * within this application (and all instances of the application). the
    * type is a basic window (the final parameter) */
   win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
   /* set the title of the window - this is in the titlebar */
   elm_win_title_set(win, "Elementary Tests");

   /* set a callback on the window when "delete,request" is emitted as
    * a callback. when this happens my_win_del() is called and the
    * data pointer (first param) is passed the final param here (in this
    * case it is NULL). This is how you can pass specific things to a
    * callback like objects or data layered on top */
   evas_object_smart_callback_add(win, "delete,request", my_win_del, NULL);

   /* add a background to our window. this just uses the standard theme set
    * background. without a background, you could make a window seem
    * transparent with elm_win_alpha_set(win, 1); for example. if you have
    * a compositor running this will make the window able to be
    * semi-transparent and any space not filled by object/widget pixels will
    * be transparent or translucent based on alpha. if you do not have a
    * comnpositor running this should fall back to using shaped windows
    * (which have a mask). both these features will be slow and rely on
    * a lot more resources, so only use it if you need it. */
   bg = elm_bg_add(win);
   /* set weight to 1.0 x 1.0 == expand in both x and y direction */
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   /* tell the window that this object is to be resized along with the window.
    * also as a result this object will be one of several objects that
    * controls the minimum/maximum size of the window */
   elm_win_resize_object_add(win, bg);
   /* and show the background */
   evas_object_show(bg);

   /* add a box layout widget to the window */
   bx0 = elm_box_add(win);
   /* allow base box (bx0) to expand in x and y */
   evas_object_size_hint_weight_set(bx0, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   /* tell the window that the box affects window size and also will be
    * resized when the window is */
   elm_win_resize_object_add(win, bx0);
   evas_object_show(bx0);

   fr = elm_frame_add(win);
   elm_frame_label_set(fr, "Information");
   elm_box_pack_end(bx0, fr);
   evas_object_show(fr);

   lb = elm_label_add(win);
   elm_label_label_set(lb,
                       "Please select a test from the list below<br>"
                       "by clicking the test button to show the<br>"
                       "test window.");
   elm_frame_content_set(fr, lb);
   evas_object_show(lb);

   tg = elm_toggle_add(win);
   elm_toggle_label_set(tg, "UI-Mirroring:");
   elm_toggle_state_set(tg, elm_mirrored_get());
   evas_object_smart_callback_add(tg, "changed", _ui_tg_changed, NULL);
   elm_box_pack_end(bx0, tg);
   evas_object_show(tg);

   li = elm_list_add(win);
   elm_list_always_select_mode_set(li, 1);
   evas_object_size_hint_weight_set(li, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(li, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx0, li);
   evas_object_show(li);

   idx = elm_index_add(win);
   evas_object_smart_callback_add(idx, "delay,changed", index_changed, NULL);
   evas_object_size_hint_weight_set(idx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, idx);
   evas_object_show(idx);

   tests = NULL;
#define ADD_TEST(name_, cb_) elm_test_add(&tests, name_, cb_)
   ADD_TEST("Bg Plain", test_bg_plain);
   ADD_TEST("Bg Image", test_bg_image);
   ADD_TEST("Bg Options", test_bg_options);
   ADD_TEST("Icon Transparent", test_icon);
   ADD_TEST("Box Vert", test_box_vert);
   ADD_TEST("Box Vert 2", test_box_vert2);
   ADD_TEST("Box Horiz", test_box_horiz);
   ADD_TEST("Box Transition", test_box_transition);
   ADD_TEST("Buttons", test_button);
   ADD_TEST("Transit", test_transit);
   ADD_TEST("Transit 2", test_transit2);
   ADD_TEST("Transit 3", test_transit3);
   ADD_TEST("Transit 4", test_transit4);
   ADD_TEST("Transit 5", test_transit5);
   ADD_TEST("Transit 6", test_transit6);
   ADD_TEST("Transit 7", test_transit7);
   ADD_TEST("Transit 8", test_transit8);
   ADD_TEST("File Selector Button", test_fileselector_button);
   ADD_TEST("File Selector Entry", test_fileselector_entry);
   ADD_TEST("Toggles", test_toggle);
   ADD_TEST("Table", test_table);
   ADD_TEST("Clock", test_clock);
   ADD_TEST("Clock 2", test_clock2);
   ADD_TEST("Layout", test_layout);
   ADD_TEST("Hover", test_hover);
   ADD_TEST("Hover 2", test_hover2);
   ADD_TEST("Entry", test_entry);
   ADD_TEST("Entry Scrolled", test_entry_scrolled);
   ADD_TEST("Entry 3", test_entry3);
   ADD_TEST("Entry 4", test_entry4);
   ADD_TEST("Entry 5", test_entry5);
   ADD_TEST("Entry Notepad", test_entry_notepad);
   ADD_TEST("Anchorview", test_anchorview);
   ADD_TEST("Anchorblock", test_anchorblock);
   ADD_TEST("Toolbar", test_toolbar);
   ADD_TEST("Toolbar 2", test_toolbar2);
   ADD_TEST("Toolbar 3", test_toolbar3);
   ADD_TEST("Toolbar 4", test_toolbar4);
   ADD_TEST("Toolbar 5", test_toolbar5);
   ADD_TEST("Hoversel", test_hoversel);
   ADD_TEST("List", test_list);
   ADD_TEST("List - Horizontal", test_list_horizontal);
   ADD_TEST("List 2", test_list2);
   ADD_TEST("List 3", test_list3);
   ADD_TEST("List 4", test_list4);
   ADD_TEST("List 5", test_list5);
   ADD_TEST("Inwin", test_inwin);
   ADD_TEST("Inwin 2", test_inwin2);
   ADD_TEST("Scaling", test_scaling);
   ADD_TEST("Scaling 2", test_scaling2);
   ADD_TEST("Slider", test_slider);
   ADD_TEST("Actionslider", test_actionslider);
   ADD_TEST("Genlist", test_genlist);
   ADD_TEST("Genlist 2", test_genlist2);
   ADD_TEST("Genlist 3", test_genlist3);
   ADD_TEST("Genlist 4", test_genlist4);
   ADD_TEST("Genlist 5", test_genlist5);
   ADD_TEST("Genlist 7", test_genlist7);
   ADD_TEST("Genlist Tree", test_genlist6);
   ADD_TEST("Genlist Group", test_genlist8);
   ADD_TEST("Genlist Group Tree", test_genlist9);
   ADD_TEST("Genlist Mode", test_genlist10);
   ADD_TEST("GenGrid", test_gengrid);
   ADD_TEST("GenGrid 2", test_gengrid2);
   ADD_TEST("Checks", test_check);
   ADD_TEST("Radios", test_radio);
   ADD_TEST("Pager", test_pager);
   ADD_TEST("Pager Slide", test_pager_slide);
   ADD_TEST("Window States", test_win_state);
   ADD_TEST("Window States 2", test_win_state2);
   ADD_TEST("Progressbar", test_progressbar);
   ADD_TEST("File Selector", test_fileselector);
   ADD_TEST("Separator", test_separator);
   ADD_TEST("Scroller", test_scroller);
   ADD_TEST("Scroller 2", test_scroller2);
   ADD_TEST("Spinner", test_spinner);
   ADD_TEST("Index", test_index);
   ADD_TEST("Index 2", test_index2);
   ADD_TEST("Photocam", test_photocam);
   ADD_TEST("Photo", test_photo);
   ADD_TEST("Thumb", test_thumb);
   ADD_TEST("Icon Desktops", test_icon_desktops);
   ADD_TEST("Notify", test_notify);
   ADD_TEST("Slideshow", test_slideshow);
   ADD_TEST("Menu", test_menu);
   ADD_TEST("Panel", test_panel);
   ADD_TEST("Panes", test_panes);
   ADD_TEST("Map", test_map);
   ADD_TEST("Weather", test_weather);
   ADD_TEST("Flip", test_flip);
   ADD_TEST("Flip 2", test_flip2);
   ADD_TEST("Flip 3", test_flip3);
   ADD_TEST("Label", test_label);
   ADD_TEST("Conformant", test_conformant);
   ADD_TEST("Conformant 2", test_conformant2);
   ADD_TEST("Multi Touch", test_multi);
   ADD_TEST("Floating Objects", test_floating);
   ADD_TEST("Launcher", test_launcher);
   ADD_TEST("Launcher 2", test_launcher2);
   ADD_TEST("Launcher 3", test_launcher3);
   ADD_TEST("Animation", test_anim);
   ADD_TEST("Calendar", test_calendar);
   ADD_TEST("Calendar 2", test_calendar2);
   ADD_TEST("Tooltip", test_tooltip);
   ADD_TEST("Tooltip2", test_tooltip2);
   ADD_TEST("Cursor", test_cursor);
   ADD_TEST("Cursor 2", test_cursor2);
   ADD_TEST("Cursor 3", test_cursor3);
   ADD_TEST("Focus", test_focus);
   ADD_TEST("Focus 2", test_focus2);
   ADD_TEST("Focus 3", test_focus3);
   ADD_TEST("Flip Selector", test_flipselector);
   ADD_TEST("Disk Selector", test_diskselector);
   ADD_TEST("Color Selector", test_colorselector);
   ADD_TEST("Ctxpopup", test_ctxpopup);
   ADD_TEST("Bubble", test_bubble);
   ADD_TEST("Segment Control", test_segment_control);
   ADD_TEST("Store", test_store);
   ADD_TEST("Window Inline", test_win_inline);
#undef ADD_TEST

   if (autorun)
     {
        EINA_LIST_FOREACH(tests, l, t)
           if ((t->name) && (t->cb) && (!strcmp(t->name, autorun)))
             t->cb(NULL, NULL, NULL);
     }

   if (tests)
     {
        char last_letter = 0;
        EINA_LIST_FREE(tests, t)
          {
             Elm_List_Item *it;
             it = elm_list_item_append(li, t->name, NULL, NULL, t->cb, NULL);
             if (last_letter != t->name[0])
               {
                  char letter[2] = {t->name[0], '\0'};
                  elm_index_item_append(idx, letter, it);
                  last_letter = t->name[0];
               }
             free(t);
          }
        elm_index_item_go(idx, 0);
     }

   elm_list_go(li);

   /* set an initial window size */
   evas_object_resize(win, 320, 480);
   /* show the window */
   evas_object_show(win);
}

/* this is your elementary main function - it MUST be called IMMEDIATELY
 * after elm_init() and MUST be passed argc and argv, and MUST be called
 * elm_main and not be static - must be a visible symbol with EAPI infront */
EAPI int
elm_main(int argc, char **argv)
{
   char *autorun = NULL;

   /* tell elm about our app so it can figure out where to get files */
   elm_app_info_set(elm_main, "elementary", "images/logo.png");
   elm_app_compile_bin_dir_set(PACKAGE_BIN_DIR);
   elm_app_compile_data_dir_set(PACKAGE_DATA_DIR);

   printf("my prefix: %s\n", elm_app_prefix_dir_get());

   printf("elm test data in %s\n", elm_app_data_dir_get());
   /* if called with a single argument try to autorun a test with
    * the same name as the given param
    * ex:  elementary_test "Box Vert 2" */
   if (argc == 2)
     autorun = argv[1];

   /* put here any init specific to this app like parsing args etc. */
   my_win_main(autorun); /* create main window */
   elm_run(); /* and run the program now  and handle all events etc. */
   /* if the mainloop that elm_run() runs exist - we exit the app */
   elm_shutdown(); /* clean up and shut down */
   /* exit code */
   return 0;
}
#endif
/* all elementary apps should use this. but it should be placed right after
 * elm_main() */
ELM_MAIN()
