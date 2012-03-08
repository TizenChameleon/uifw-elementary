/**
 * @defgroup Route Route
 *
 * For displaying a route on the map widget.
 *
 * @{
 */

/**
 * Add a new route object to the parent's canvas
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 */
EAPI Evas_Object *elm_route_add(Evas_Object *parent);

#ifdef ELM_EMAP
EAPI void         elm_route_emap_set(Evas_Object *obj, EMap_Route *emap);
#endif


EINA_DEPRECATED EAPI double       elm_route_lon_min_get(Evas_Object *obj);
EINA_DEPRECATED EAPI double       elm_route_lat_min_get(Evas_Object *obj);
EINA_DEPRECATED EAPI double       elm_route_lon_max_get(Evas_Object *obj);
EINA_DEPRECATED EAPI double       elm_route_lat_max_get(Evas_Object *obj);

/**
 * Get the minimum and maximum values along the longitude.
 *
 * @param obj The route object.
 * @param min Pointer to store the minimum value.
 * @param max Pointer to store the maximum value.
 *
 * @note If only one value is needed, the other pointer can be passed
 * as @c NULL.
 *
 * @ingroup Route
 */
EAPI void        elm_route_longitude_min_max_get(const Evas_Object *obj, double *min, double *max);

/**
 * Get the minimum and maximum values along the latitude.
 *
 * @param obj The route object.
 * @param min Pointer to store the minimum value.
 * @param max Pointer to store the maximum value.
 *
 * @note If only one value is needed, the other pointer can be passed
 * as @c NULL.
 *
 * @ingroup Route
 */
EAPI void        elm_route_latitude_min_max_get(const Evas_Object *obj, double *min, double *max);
