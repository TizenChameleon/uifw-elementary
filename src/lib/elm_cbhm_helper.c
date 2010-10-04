#include <Elementary.h>
#include "elm_priv.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>


/**
 * @defgroup CBHM_helper CBHM_helper
 * @ingroup Elementary
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
unsigned int _get_cbhm_serial_number();
void _search_clipboard_window(Ecore_X_Window w);
int _send_clipboard_events(char *cmd);
int _get_clipboard_data(Atom datom, char **datomptr);

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

unsigned int _get_cbhm_serial_number()
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems, bytes_after;
	unsigned char *prop_return = NULL;
	unsigned int senum = 0;
	Atom atomCbhmSN = XInternAtom(cbhm_disp, "CBHM_SERIAL_NUMBER", False);

	// FIXME : is it really needed?
	XSync(cbhm_disp, EINA_FALSE);

	if(Success == 
	   XGetWindowProperty(cbhm_disp, cbhm_win, atomCbhmSN, 
						  0, sizeof(Ecore_X_Window), False, XA_INTEGER, 
						  &actual_type, &actual_format, &nitems, &bytes_after, &prop_return) && 
	   prop_return)
	{
		senum = *(unsigned int*)prop_return;
		XFree(prop_return);
	}
	fprintf(stderr, "## chbm_serial = %d\n", senum);
	return senum;
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
	Window wroot;
	Window wparent;
	Window *wchild;
	unsigned nchildren;
	int i;
	if(0 != XQueryTree(cbhm_disp, w, &wroot, &wparent, &wchild, &nchildren))
	{
		for(i = 0; i < nchildren; i++)
			_search_clipboard_window(wchild[i]);
	}
}

int _send_clipboard_events(char *cmd)
{
	if (cmd == NULL)
		return -1;

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

int _get_clipboard_data(Atom datom, char **datomptr)
{
	Atom atomUTF8String = XInternAtom(cbhm_disp, "UTF8_STRING", False);
	Atom type;
	int format;
	unsigned long nitems;
	unsigned long nsize;
	unsigned char *propname = NULL;

	// FIXME : is it really needed?
	XSync(cbhm_disp, EINA_FALSE);

	if (Success == 
		XGetWindowProperty(cbhm_disp, self_win, datom, 0, 0, False,
						   AnyPropertyType, &type, &format, &nitems, &nsize, &propname))
		XFree(propname);
	else
		return -1;

	fprintf(stderr, "## format = %d\n", format);
	fprintf(stderr, "## nsize = %d\n", nsize);

	if (format != 8)
		return -1;

	if (Success == 
		XGetWindowProperty(cbhm_disp, self_win, datom, 0, (long)nsize, False,
						   AnyPropertyType, &type, &format, &nitems, &nsize, &propname))
	{
		if (nsize != 0)
			XGetWindowProperty(cbhm_disp, self_win, datom, 0, (long)nsize, False,
							   AnyPropertyType, &type, &format, &nitems, &nsize, &propname);

		if(propname != NULL)
		{
			fprintf(stderr, "## get data(0x%x) : %s\n", propname, propname);
			fprintf(stderr, "## after nsize = %d\n", nsize);
			*datomptr = propname;
//			XFree(propName);
		}

		XDeleteProperty(cbhm_disp, self_win, datom);
		XFlush(cbhm_disp);
	}

	if (propname != NULL)
		return 0;

	*datomptr = NULL;
	return -1;
}


void free_clipboard_data(char *dptr)
{
	return XFree(dptr);
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
	if (cbhm_win == None)
		_get_clipboard_window();
	if (cbhm_win == None)
		_search_clipboard_window(DefaultRootWindow(cbhm_disp));
	if (self_win == None)
		self_win = ecore_evas_software_x11_window_get(ecore_evas_ecore_evas_get(evas_object_evas_get(self)));
   
	if (cbhm_disp && cbhm_win && self_win)
		init_flag = EINA_TRUE;

	return init_flag;
}

/**
 * getting serial number of CBHM
 *
 * @return return serial number of clipboard history manager
 *
 * @ingroup CBHM_helper
 */
EAPI unsigned int 
elm_cbhm_get_serial_number()
{
	if (init_flag == EINA_FALSE)
		return 0;

	unsigned int num = 0;
	num = _get_cbhm_serial_number();
	return num;
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

	char *retptr = NULL;
	int count = 0;

	_send_clipboard_events("get count");

	Atom atomCBHM_cCOUNT = XInternAtom(cbhm_disp, "CBHM_cCOUNT", False);

	_get_clipboard_data(atomCBHM_cCOUNT, &retptr);

	if (retptr != NULL)
	{
		fprintf(stderr, "## c get retptr : %s\n", retptr);
		count = atoi(retptr);

		free_clipboard_data(retptr);
		retptr = NULL;
	}

	return count;
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

	char *retptr = NULL;

	_send_clipboard_events("get raw");

	Atom atomCBHM_cRAW = XInternAtom(cbhm_disp, "CBHM_cRAW", False);

	_get_clipboard_data(atomCBHM_cRAW, &retptr);

	if (retptr != NULL)
	{
		free_clipboard_data(retptr);
		retptr = NULL;
	}

	return 0;
}

/**
 * sending raw command to CBHM
 *
 * @return void
 *
 * @ingroup CBHM_helper
 */
EAPI void
elm_cbhm_send_raw_data(char *cmd)
{
	if (init_flag == EINA_FALSE)
		return;

	if (cmd == NULL)
		return;

	_send_clipboard_events(cmd);
	fprintf(stderr, "## cbhm - send raw cmd = %s\n", cmd);

	return;
}

/**
 * getting data by history position of CBHM's contents
 * 0 is current content.
 *
 * @return return data pointer of position of history contents
 *
 * @ingroup CBHM_helper
 */
EAPI int
elm_cbhm_get_data_by_position(int pos, char **dataptr)
{
	if (init_flag == EINA_FALSE)
		return -1;

	char reqbuf[16];
	int ret = 0;
	sprintf(reqbuf, "get #%d", pos);

	_send_clipboard_events(reqbuf);

	sprintf(reqbuf, "CBHM_c%d", pos);

	Atom atomCBHM_cPOS = XInternAtom(cbhm_disp, reqbuf, False);

	ret = _get_clipboard_data(atomCBHM_cPOS, dataptr);

	if (ret >= 0 && *dataptr != NULL)
	{
		fprintf(stderr, "## d get retptr : %s\n", *dataptr);
		fprintf(stderr, "## dptr = 0x%x\n", *dataptr);

		return 0;
	}

	return -1;
}

/**
 * free data by history position of CBHM's contents
 *
 * @return None
 *
 * @ingroup CBHM_helper
 */
EAPI void
elm_cbhm_free_data(char *dptr)
{
	if (dptr != NULL)
		free_clipboard_data(dptr);
}
