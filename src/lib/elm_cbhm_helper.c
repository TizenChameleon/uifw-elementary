#include <Elementary.h>
#include "elm_priv.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>


/**
 * @defgroup CBHM_helper CBHM_helper
 *
 * retrieving date from Clipboard History Manager
 * CBHM_helper supports to get CBHM's contents
 */

#define ATOM_CLIPBOARD_NAME "CLIPBOARD"
#define ATOM_CLIPBOARD_MANGER_NAME "CLIPBOARD_MANAGER"
#define CLIPBOARD_MANAGER_WINDOW_TITLE_STRING "X11_CLIPBOARD_HISTORY_MANAGER"
#define ATOM_CBHM_WINDOW_NAME "CBHM_XWIN"

static Ecore_X_Display *cbhm_disp = NULL;
static Ecore_X_Window cbhm_win = None;
static Ecore_X_Window self_win = None;
static Eina_Bool init_flag = EINA_FALSE;

void _get_clipboard_window();
void _search_clipboard_window(Ecore_X_Window w);
int _send_clipboard_events(char *cmd);
int get_clipboard_data(Atom datom);

void _get_clipboard_window()
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems, bytes_after;
	unsigned char *prop_return = NULL;
	Atom atomCbhmWin = XInternAtom(cbhm_disp, ATOM_CBHM_WINDOW_NAME, False);
	if(Success == 
	   XGetWindowProperty(cbhm_disp, DefaultRootWindow(cbhm_disp), atomCbhmWin, 
						  0, sizeof(Ecore_X_Window), False, XA_WINDOW, 
						  &actual_type, &actual_format, &nitems, &bytes_after, &prop_return) && 
	   prop_return)
	{
		cbhm_win = *(Ecore_X_Window*)prop_return;
		XFree(prop_return);
		fprintf(stderr, "## find clipboard history manager at root\n");
	}
}

void _search_clipboard_window(Ecore_X_Window w)
{
    // Get the PID for the current Window.
	Atom atomWMName = XInternAtom(cbhm_disp, "_NET_WM_NAME", False);
	Atom atomUTF8String = XInternAtom(cbhm_disp, "UTF8_STRING", False);
	Atom type;
	int format;
	unsigned long nitems;
	unsigned long bytes_after;
	unsigned long nsize;
	unsigned char *propName = 0;
	if(Success == 
	   XGetWindowProperty(cbhm_disp, w, atomWMName, 0, (long)nsize, False,
						  atomUTF8String, &type, &format, &nitems, &bytes_after, &propName))

	{
		if(propName != 0)
		{
			if (strcmp(CLIPBOARD_MANAGER_WINDOW_TITLE_STRING, propName) == 0)
				cbhm_win = w;
			XFree(propName);
		}
	}

    // Recurse into child windows.
	Window    wRoot;
	Window    wParent;
	Window   *wChild;
	unsigned  nChildren;
	int i;
	if(0 != XQueryTree(cbhm_disp, w, &wRoot, &wParent, &wChild, &nChildren))
	{
		for(i = 0; i < nChildren; i++)
			_search_clipboard_window(wChild[i]);
	}
}

int _send_clipboard_events(char *cmd)
{
	Atom atomCBHM_MSG = XInternAtom(cbhm_disp, "CBHM_MSG", False);

	XClientMessageEvent m;
	memset(&m, sizeof(m), 0);
	m.type = ClientMessage;
	m.display = cbhm_disp;
	m.window = self_win;
	m.message_type = atomCBHM_MSG;
	m.format = 8;
	sprintf(m.data.b, "%s", cmd);

	XSendEvent(cbhm_disp, cbhm_win, False, NoEventMask, (XEvent*)&m);

	return 0;
}

int get_clipboard_data(Atom datom)
{
	Atom atomUTF8String = XInternAtom(cbhm_disp, "UTF8_STRING", False);

	Atom type;
	int format;
	unsigned long nitems;
	unsigned long nsize;
	unsigned char *propName = NULL;

	// FIXME : is it really needed?
	XSync(cbhm_disp, 0);

	if (Success == 
		XGetWindowProperty(cbhm_disp, self_win, datom, 0, 0, False,
						   AnyPropertyType, &type, &format, &nitems, &nsize, &propName))
		XFree(propName);
	else
		return -1;

	fprintf(stderr, "## format = %d\n", format);
	fprintf(stderr, "## nsize = %d\n", nsize);

	if (format != 8)
		return -1;

	if (Success == 
		XGetWindowProperty(cbhm_disp, self_win, datom, 0, (long)nsize, False,
						   AnyPropertyType, &type, &format, &nitems, &nsize, &propName))
	{
		if (nsize != 0)
			XGetWindowProperty(cbhm_disp, self_win, datom, 0, (long)nsize, False,
							   AnyPropertyType, &type, &format, &nitems, &nsize, &propName);

		if(propName != NULL)
		{
			fprintf(stderr, "## get data : %s\n", propName);
			fprintf(stderr, "## nsize = %d\n", nsize);
			XFree(propName);
		}

		XDeleteProperty(cbhm_disp, self_win, datom);
		XFlush(cbhm_disp);
	}
	else
		return -1;
}


/**
 * initalizing CBHM_helper
 *
 * @param self The self window object which receive events
 * @return return TRUE or FALSE if it cannot be created
 *
 * @ingroup CBHM_helper
 */
EAPI Eina_Bool 
elm_cbhm_helper_init(Evas_Object *self)
{
   init_flag = EINA_FALSE;

   cbhm_disp = ecore_x_display_get();
   if (cbhm_disp == NULL)
	   return init_flag;

   _get_clipboard_window();
   if (cbhm_win == None)
	   _search_clipboard_window(DefaultRootWindow(cbhm_disp));
   self_win = ecore_evas_software_x11_window_get(ecore_evas_ecore_evas_get(evas_object_evas_get(self)));
   
   if (cbhm_disp && cbhm_win && self_win)
	   init_flag = EINA_TRUE;

   return init_flag;
}

/**
 * getting count of CBHM's contents
 *
 * @return return count of history contents
 *
 * @ingroup CBHM_helper
 */
EAPI int 
elm_cbhm_get_count()
{
	if (init_flag == EINA_FALSE)
		return -1;

	_send_clipboard_events("get count");

	Atom atomCBHM_cCOUNT = XInternAtom(cbhm_disp, "CBHM_cCOUNT", False);

	get_clipboard_data(atomCBHM_cCOUNT);

	return 0;
}

/**
 * getting raw data of CBHM's contents
 *
 * @return return raw data of history contents
 *
 * @ingroup CBHM_helper
 */
EAPI int 
elm_cbhm_get_raw_data()
{
	if (init_flag == EINA_FALSE)
		return -1;

	_send_clipboard_events("get raw");

   return 0;
}

/**
 * getting data by history position of CBHM's contents
 * 0 is current content.
 *
 * @return return data by position of history contents
 *
 * @ingroup CBHM_helper
 */
EAPI int 
elm_cbhm_get_data_by_position(int pos)
{
	if (init_flag == EINA_FALSE)
		return -1;

	char reqbuf[16];
	sprintf(reqbuf, "get #%d", pos);

	_send_clipboard_events(reqbuf);

	sprintf(reqbuf, "CBHM_c%d", pos);

	Atom atomCBHM_cPOS = XInternAtom(cbhm_disp, reqbuf, False);

	get_clipboard_data(atomCBHM_cPOS);

	return 0;
}
