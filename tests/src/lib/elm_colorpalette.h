   /**
    * @defgroup Colorpalette Colorpalette
    * @ingroup Elementary
    * @addtogroup Colorpalette
    * @{
    *
    * Using colorpalette, you can select a color by clicking
    * a color rectangle on the colorpalette.
    *
    * Smart callbacks that you can add are:
    *
    * clicked - This signal is sent when a color rectangle is clicked.
    */
   typedef struct _Colorpalette_Color Elm_Colorpalette_Color;
   struct _Colorpalette_Color
     {
        unsigned int r, g, b;
     };

   /**
    * Add a new colorpalette to the parent.
    *
    * @param parent The parent object
    * @return The new object or NULL if it cannot be created
    *
    * @ingroup Colorpalette
    */
   EAPI Evas_Object *elm_colorpalette_add(Evas_Object *parent);
   /**
    * Set colors to the colorpalette.
    *
    * @param obj   Colorpalette object
    * @param color_num     number of the colors on the colorpalette
    * @param color     Color lists
    */
   EAPI void         elm_colorpalette_color_set(Evas_Object *obj, int color_num, Elm_Colorpalette_Color *color);
   /**
    * Set row/column value for the colorpalette.
    *
    * @param obj   Colorpalette object
    * @param row   row value for the colorpalette
    * @param col   column value for the colorpalette
    */
   EAPI void         elm_colorpalette_row_column_set(Evas_Object *obj, int row, int col);

   /**
    * @}
    */

