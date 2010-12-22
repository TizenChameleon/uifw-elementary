#include <Elementary.h>
#include "elm_priv.h"
#include <Ecore.h>

/**
 * @defgroup Colorpicker Colorpicker
 * @ingroup Elementary
 *
 * By using colorpicker, you can select a color.
 * Colorpicker made a color using HSV/HSB mode.
 */

/**
 * Add a new colorpicker to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Colorpicker
 */
EAPI Evas_Object *
elm_colorpicker_add(Evas_Object *parent)
{
   return elm_colorselector_add(parent);
}

/**
 * Set a color for the colorpicker
 *
 * @param obj	Colorpicker object
 * @param r	r-value of color
 * @param g	g-value of color
 * @param b	b-value of color
 *
 * @ingroup Colorpicker
 */
EAPI void
elm_colorpicker_color_set(Evas_Object *obj, int r, int g, int b)
{
   elm_colorselector_color_set(obj, r, g, b, 255);
}

/**
 * Get a color from the colorpicker
 *
 * @param obj	Colorpicker object
 * @param r	integer pointer for r-value of color
 * @param g	integer pointer for g-value of color
 * @param b	integer pointer for b-value of color
 *
 * @ingroup Colorpicker
 */
EAPI void
elm_colorpicker_color_get(Evas_Object *obj, int *r, int *g, int *b)
{
   elm_colorselector_color_get(obj, r, g, b, NULL);
}

/* vim:set ts=8 sw=3 sts=3 expandtab cino=>5n-2f0^-2{2(0W1st0 :*/
