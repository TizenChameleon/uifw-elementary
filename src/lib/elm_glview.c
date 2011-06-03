#include <Elementary.h>
#include "elm_priv.h"

/**
 * @defgroup GLView
 *
 * A simple GLView widget that allows GL rendering.
 *
 * Signals that you can add callbacks for are:
 *
 * "clicked" - This is called when a user has clicked the image
 */
typedef struct _Widget_Data Widget_Data;

struct _Widget_Data
{
   Evas_Object	           *glview_image;

   Elm_GLView_Mode          mode;
   Elm_GLView_Resize_Policy  scale_policy;
   Elm_GLView_Render_Policy render_policy;

   Evas_GL                 *evasgl;
   Evas_GL_Config           config;
   Evas_GL_Surface         *surface;
   Evas_GL_Context         *context;

   Evas_Coord               w, h;

   Elm_GLView_Func          render_func;
   Ecore_Idle_Enterer      *render_idle_enterer;

   Eina_Bool                initialized;
};

static const char *widtype = NULL;
static void _del_hook(Evas_Object *obj);
static void _on_focus_hook(void *data, Evas_Object *obj);

static const char SIG_FOCUSED[] = "focused";
static const char SIG_UNFOCUSED[] = "unfocused";

static void
_del_hook(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   
   if (wd->render_idle_enterer) ecore_idle_enterer_del(wd->render_idle_enterer);

   if (wd->surface) evas_gl_surface_destroy(wd->evasgl, wd->surface);
   if (wd->context) evas_gl_context_destroy(wd->evasgl, wd->context);
   if (wd->evasgl) evas_gl_free(wd->evasgl);

   free(wd);
}

static void
_on_focus_hook(void *data __UNUSED__, Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (elm_widget_focus_get(obj))
     {
        evas_object_focus_set(wd->glview_image, EINA_TRUE);
        evas_object_smart_callback_call(obj, SIG_FOCUSED, NULL);
     }
   else
     {
        evas_object_focus_set(wd->glview_image, EINA_FALSE);
        evas_object_smart_callback_call(obj, SIG_UNFOCUSED, NULL);
     }
}

static void
_glview_update_surface(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   if (wd->surface)
     {
        evas_object_image_native_surface_set(wd->glview_image, NULL);
        evas_gl_surface_destroy(wd->evasgl, wd->surface);
        wd->surface = NULL;
     }
   
   evas_object_image_size_set(wd->glview_image, wd->w, wd->h);
   
   if (!wd->surface) 
     {
        Evas_Native_Surface ns;
        
        wd->surface = evas_gl_surface_create(wd->evasgl, &wd->config, 
                                             wd->w, wd->h);
        evas_gl_native_surface_get(wd->evasgl, wd->surface, &ns);
        evas_object_image_native_surface_set(wd->glview_image, &ns);
        elm_glview_changed_set(obj);
     }
}

static void
_glview_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Widget_Data *wd = elm_widget_data_get(data);
   Evas_Coord w, h;

   if (!wd) return;
   if (wd->scale_policy == ELM_GLVIEW_RESIZE_POLICY_RECREATE) 
     {
        evas_object_geometry_get(wd->glview_image, NULL, NULL, &w, &h);
        if ((w == 0) || (h == 0))
          {
             w = 64;
             h = 64;
          }
        if ((wd->w == w) && (wd->h == h)) return;
        wd->w = w;
        wd->h = h;
        _glview_update_surface(data);
        if (wd->render_func)
          {
             evas_gl_make_current(wd->evasgl, wd->surface, wd->context);
             wd->render_func(data);
          }
     }
}


static Eina_Bool 
_render_cb(void *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;

   // Do a make current
   if (!evas_gl_make_current(wd->evasgl, wd->surface, wd->context))
     {
        wd->render_idle_enterer = NULL;
        return EINA_FALSE;
     }

   // Call the render function
   if (wd->render_func) wd->render_func(obj);

   // Depending on the policy return true or false
   if (wd->render_policy == ELM_GLVIEW_RENDER_POLICY_ON_DEMAND)
      return EINA_TRUE;
   else if (wd->render_policy == ELM_GLVIEW_RENDER_POLICY_ALWAYS)
     {
        // Return false so it only runs once
        wd->render_idle_enterer = NULL;
        return EINA_FALSE;
     }
   else
     {
        ERR("Invalid Render Policy.\n");
        wd->render_idle_enterer = NULL;
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

static void
_set_render_policy_callback(Evas_Object *obj)
{
   Widget_Data *wd = elm_widget_data_get(obj);

   switch (wd->render_policy)
     {
      case ELM_GLVIEW_RENDER_POLICY_ON_DEMAND:
        // Delete idle_enterer if it for some reason is around
        if (wd->render_idle_enterer)
          {
             ecore_idle_enterer_del(wd->render_idle_enterer);
             wd->render_idle_enterer = NULL;
          }
        
        // Set pixel getter callback
        evas_object_image_pixels_get_callback_set
           (wd->glview_image, (Evas_Object_Image_Pixels_Get_Cb)_render_cb, obj);
        break;
      case ELM_GLVIEW_RENDER_POLICY_ALWAYS:
        
        // Unset the pixel getter callback if set already
        evas_object_image_pixels_get_callback_set(wd->glview_image, NULL, NULL);
        break;
      default:
        ERR("Invalid Render Policy.\n");
        return;
     }
}



/**
 * Add a new glview to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup GLView
 */
EAPI Evas_Object *
elm_glview_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas *e;
   Widget_Data *wd;
   Evas_GL_Config cfg = { EVAS_GL_RGB_8, 
                          EVAS_GL_DEPTH_NONE, 
                          EVAS_GL_STENCIL_NONE };

   ELM_WIDGET_STANDARD_SETUP(wd, Widget_Data, parent, e, obj, NULL);

   ELM_SET_WIDTYPE(widtype, "glview");
   elm_widget_type_set(obj, "glview");
   elm_widget_sub_object_add(parent, obj);
   elm_widget_on_focus_hook_set(obj, _on_focus_hook, NULL);
   elm_widget_data_set(obj, wd);
   elm_widget_del_hook_set(obj, _del_hook);

   // Evas_GL
   wd->evasgl = evas_gl_new(e);
   if (!wd->evasgl)
     {
        ERR("Failed Creating an Evas GL Object.\n");
        return NULL;
     }

   // Create image to render Evas_GL Surface
   wd->glview_image = evas_object_image_filled_add(e);
   evas_object_image_size_set(wd->glview_image, 1, 1);
   evas_object_event_callback_add(wd->glview_image, EVAS_CALLBACK_RESIZE,
                                  _glview_resize, obj);
   elm_widget_resize_object_set(obj, wd->glview_image);
   evas_object_show(wd->glview_image);

   // Initialize variables
   wd->mode          = 0;
   wd->scale_policy  = ELM_GLVIEW_RESIZE_POLICY_RECREATE;
   wd->render_policy = ELM_GLVIEW_RENDER_POLICY_ON_DEMAND;
   wd->config        = cfg;
   wd->surface       = NULL;

   wd->w             = 64;
   wd->h             = 64;

   wd->render_idle_enterer = NULL;

   // Create Context 
   if (!wd->context)
     {
        wd->context = evas_gl_context_create(wd->evasgl, NULL);
        if (!wd->context)
          {
             ERR("Error Creating an Evas_GL Context.\n");
             return NULL;
          }
     }

   return obj;

}

/**
 * Gets the gl api struct for gl rendering
 *
 * @param obj The glview object
 * @return The api object or NULL if it cannot be created
 *
 * @ingroup GLView
 */
EAPI Evas_GL_API *
elm_glview_gl_api_get(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype) NULL;
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return NULL;

   return evas_gl_api_get(wd->evasgl);
}


/**
 * Set the mode of the GLView. Supports Three simple modes. 
 *
 * @param obj The glview object
 * @param mode The mode Options OR'ed enabling Alpha, Depth, Stencil. 
 * @return True if set properly.
 *
 * @ingroup GLView
 */
EAPI Eina_Bool
elm_glview_mode_set(Evas_Object *obj, Elm_GLView_Mode mode)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   Evas_GL_Config cfg = { EVAS_GL_RGBA_8, 
                          EVAS_GL_DEPTH_NONE, 
                          EVAS_GL_STENCIL_NONE };
   if (!wd) return EINA_FALSE;

   // Set the configs
   if (mode & ELM_GLVIEW_ALPHA)
      cfg.color_format = EVAS_GL_RGBA_8;

   if (mode & ELM_GLVIEW_DEPTH)
      cfg.depth_bits = EVAS_GL_DEPTH_BIT_24;

   if (mode & ELM_GLVIEW_STENCIL)
      cfg.stencil_bits = EVAS_GL_STENCIL_BIT_8;

   // Check for Alpha Channel and enable it
   if (mode & ELM_GLVIEW_ALPHA)
      evas_object_image_alpha_set(wd->glview_image, EINA_TRUE);
   else 
      evas_object_image_alpha_set(wd->glview_image, EINA_FALSE);
   
   wd->mode   = mode;
   wd->config = cfg;
   
   elm_glview_changed_set(obj);

   return EINA_TRUE;
}

/**
 * Set the scaling policy for the glview object.
 *
 * @param obj The glview object.
 * @param policy The scaling policy.
 *
 * By default, the scaling policy is set to ELM_GLVIEW_RESIZE_POLICY_RECREATE.
 * When resize is called it destroys the previous surface and recreates the newly
 * specified size. If the policy is set to ELM_GLVIEW_RESIZE_POLICY_SCALE, however, 
 * glview only scales the image object and not the underlying GL Surface.
 * 
 * @ingroup GLView
 */
EAPI Eina_Bool
elm_glview_scale_policy_set(Evas_Object *obj, Elm_GLView_Resize_Policy policy)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;

   if (policy == wd->scale_policy) return EINA_TRUE;
   switch (policy)
     {
      case ELM_GLVIEW_RESIZE_POLICY_RECREATE:
      case ELM_GLVIEW_RESIZE_POLICY_SCALE:
        wd->scale_policy = policy;
        return EINA_TRUE;
      default:
        ERR("Invalid Scale Policy.\n");
        return EINA_FALSE;
     }
   _glview_update_surface(obj);
   elm_glview_changed_set(obj);
}

/**
 * Set the render policy for the glview object.
 *
 * @param obj The glview object.
 * @param policy The render policy.
 *
 * By default, the render policy is set to ELM_GLVIEW_RENDER_POLICY_ON_DEMAND.
 * This policy is set such that during the render loop, glview is only redrawn
 * if it needs to be redrawn. (i.e. When it is visible) If the policy is set
 * to ELM_GLVIEWW_RENDER_POLICY_ALWAYS, it redraws regardless of whether it is 
 * visible/need redrawing or not.
 * 
 * @ingroup GLView
 */
EAPI Eina_Bool
elm_glview_render_policy_set(Evas_Object *obj, Elm_GLView_Render_Policy policy)
{
   ELM_CHECK_WIDTYPE(obj, widtype) EINA_FALSE;
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return EINA_FALSE;

   if ((policy != ELM_GLVIEW_RENDER_POLICY_ON_DEMAND) && 
       (policy != ELM_GLVIEW_RENDER_POLICY_ALWAYS))
     {
        ERR("Invalid Render Policy.\n");
        return EINA_FALSE;
     }
   if (wd->render_policy == policy) return EINA_TRUE;
   wd->render_policy = policy;
   _set_render_policy_callback(obj);
   _glview_update_surface(obj);
   return EINA_TRUE;
}

/**
 * Sets the size of the glview
 *
 * @param obj The glview object
 * @param width width of the glview object
 * @param height height of the glview object
 *
 * @ingroup GLView
 */
EAPI void
elm_glview_size_set(Evas_Object *obj, int width, int height)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   
   if ((width == wd->w) && (height == wd->h)) return;
   wd->w = width;
   wd->h = height;
   _glview_update_surface(obj);
   elm_glview_changed_set(obj);
}

/**
 * Gets the size of the glview.
 *
 * @param obj The glview object
 * @param width width of the glview object
 * @param height height of the glview object
 *
 * Note that this function returns the actual image size of the glview.
 * This means that when the scale policy is set to ELM_GLVIEW_RESIZE_POLICY_SCALE,
 * it'll return the non-scaled size.
 *
 * @ingroup GLView
 */
EAPI void
elm_glview_size_get(Evas_Object *obj, int *width, int *height)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;
   
   if (width) *width = wd->w;
   if (height) *height = wd->h;
}

/**
 * Set the render function that runs in the main loop.
 *
 * @param obj The glview object.
 * @param func The render function to be registered.
 *
 * @ingroup GLView
 */
EAPI void
elm_glview_render_func_set(Evas_Object *obj, Elm_GLView_Func func)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);
   if (!wd) return;

   wd->render_func = func;

   _set_render_policy_callback(obj);
}


/**
 * Notifies that there has been changes in the GLView.
 *
 * @param obj The glview object.
 *
 * @ingroup GLView
 */
EAPI void
elm_glview_changed_set(Evas_Object *obj)
{
   ELM_CHECK_WIDTYPE(obj, widtype);
   Widget_Data *wd = elm_widget_data_get(obj);

   if (!wd) return;

   evas_object_image_pixels_dirty_set(wd->glview_image, EINA_TRUE);

   if (wd->render_policy == ELM_GLVIEW_RENDER_POLICY_ALWAYS)
     {
        if (!wd->render_idle_enterer)

           wd->render_idle_enterer = ecore_idle_enterer_before_add((Ecore_Task_Cb)_render_cb, obj);
     }

}

/* vim:set ts=8 sw=3 sts=3 expandtab cino=>5n-2f0^-2{2(0W1st0 :*/
