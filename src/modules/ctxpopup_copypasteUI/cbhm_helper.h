#include <Elementary.h>
#include <Eina.h>
#include "elm_priv.h"

#ifdef HAVE_ELEMENTARY_X
#include <Ecore_X.h>
#endif

//#define DEBUG

#ifdef DEBUG
#define DMSG(fmt, args...) printf("[%s], "fmt, __func__, ##args)
#else
#define DMSG(args...)
#endif

Eina_Bool _cbhm_msg_send(Evas_Object* obj, char *msg);
int _cbhm_item_count_get(Evas_Object *obj);
#ifdef HAVE_ELEMENTARY_X
Eina_Bool _cbhm_item_get(Evas_Object *obj, int index, Ecore_X_Atom *data_type, char **buf);
#else
Eina_Bool _cbhm_item_get(Evas_Object *obj, int index, void *data_type, char **buf);
#endif

#ifdef HAVE_ELEMENTARY_X
Eina_Bool _cbhm_item_set(Evas_Object *obj, Ecore_X_Atom data_type, char *item_data);
#endif
