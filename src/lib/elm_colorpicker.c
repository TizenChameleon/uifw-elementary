#include <Elementary.h>
#include "elm_priv.h"
#include <Ecore.h>

EAPI Evas_Object *
elm_colorpicker_add(Evas_Object *parent)
{
   return elm_colorselector_add(parent);
}

EAPI void
elm_colorpicker_color_set(Evas_Object *obj, int r, int g, int b)
{
   elm_colorselector_color_set(obj, r, g, b, 255);
}

EAPI void
elm_colorpicker_color_get(Evas_Object *obj, int *r, int *g, int *b)
{
   elm_colorselector_color_get(obj, r, g, b, NULL);
}

