#include <Elementary.h>
#include "elm_priv.h"

/* what are moodules in elementary for? for modularising behavior and features
 * so they can be plugged in and out where you dont want the core source to
 * alwyas behave like that or do it that way. plug it at runtime!
 * 
 * they have module names (in config) and "slots" to plug that module into
 * to server a purpose. eg you plug plugin "xx" into the "entry-copy-paste"
 * slot so it would provide replacement copy & paste ui functionality and
 * specific symbols
 * 
 * config is something like:
 * 
 * export ELM_MODULES="xx>slot1:yy>slot2"
 * 
 * where a module named xx is plugged into slot1 & yy is plugged into slot2
 * 
 * real examples:
 * 
 * export ELM_MODULES="my_module>entry/api"
 * 
 * this loads the module called "my_module" into the slot "entry/api" which
 * is an api slot for entry modules to modify behavior and hook to
 * creation/deletion of the entry as well as replace the longpress behavior.
 */

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <dlfcn.h>      /* dlopen,dlclose,etc */

static Eina_Hash *modules = NULL;
static Eina_Hash *modules_as = NULL;

void
_elm_module_init(void)
{
   modules = eina_hash_string_small_new(NULL);
   modules_as = eina_hash_string_small_new(NULL);
}

void
_elm_module_shutdown(void)
{
   // FIXME: unload all modules
   eina_hash_free(modules);
   modules = NULL;
   eina_hash_free(modules_as);
   modules_as = NULL;
}

void
_elm_module_parse(const char *s)
{
   const char *p, *pe;
   
   p = s;
   pe = p;
   for (;;)
     {
        if ((*pe == ':') || (*pe == 0))
          { // p -> pe == 'name:'
             if (pe > p)
               {
                  char *n = malloc(pe - p + 1);
                  if (n)
                    {
                       char *nn;
                       
                       strncpy(n, p, pe - p);
                       n[pe - p] = 0;
                       nn = strchr(n, '>');
                       if (nn)
                         {
                            *nn = 0;
                            nn++;
                            _elm_module_add(n, nn);
                         }
                       free(n);
                    }
               }
             if (*pe == 0) break;
             p = pe + 1;
             pe = p;
          }
        else
          pe++;
     }
}

Elm_Module *
_elm_module_find_as(const char *as)
{
   Elm_Module *m;
   
   m = eina_hash_find(modules_as, as);
   return m;
}

Elm_Module *
_elm_module_add(const char *name, const char *as)
{
   Elm_Module *m;
   char buf[PATH_MAX];

   m = eina_hash_find(modules, name);
   if (m)
     {
        m->references++;
        return m;
     }
   m = calloc(1, sizeof(Elm_Module));
   if (!m) return NULL;
   m->version = 1;
   if (name[0] != '/')
     {
        const char *home = getenv("HOME");
        
        if (home)
          {
             snprintf(buf, sizeof(buf), "%s/.elementary/modules/%s/%s/module" EFL_SHARED_EXTENSION, home, name, MODULE_ARCH);
             m->handle = dlopen(buf, RTLD_NOW | RTLD_GLOBAL);
             if (m->handle)
               {
                  m->init_func = dlsym(m->handle, "elm_modapi_init");
                  if (m->init_func)
                    {
                       m->shutdown_func = dlsym(m->handle, "elm_modapi_shutdown");
                       m->so_path = eina_stringshare_add(buf);
                       m->name = eina_stringshare_add(name);
                       snprintf(buf, sizeof(buf), "%s/.elementary/modules/%s/%s", home, name, MODULE_ARCH);
                       m->bin_dir = eina_stringshare_add(buf);
                       snprintf(buf, sizeof(buf), "%s/.elementary/modules/%s", home, name);
                       m->data_dir = eina_stringshare_add(buf);
                    }
                  else
                    {
                       dlclose(m->handle);
                       free(m);
                       return NULL;
                    }
               }
          }
        if (!m->handle)
          {
             snprintf(buf, sizeof(buf), "%s/elementary/modules/%s/%s/module" EFL_SHARED_EXTENSION, _elm_lib_dir, name, MODULE_ARCH);
             m->handle = dlopen(buf, RTLD_NOW | RTLD_GLOBAL);
             if (m->handle)
               {
                  m->init_func = dlsym(m->handle, "elm_modapi_init");
                  if (m->init_func)
                    {
                       m->shutdown_func = dlsym(m->handle, "elm_modapi_shutdown");
                       m->so_path = eina_stringshare_add(buf);
                       m->name = eina_stringshare_add(name);
                       snprintf(buf, sizeof(buf), "%s/elementary/modules/%s/%s", _elm_lib_dir, name, MODULE_ARCH);
                       m->bin_dir = eina_stringshare_add(buf);
                       snprintf(buf, sizeof(buf), "%s/elementary/modules/%s", _elm_lib_dir, name);
                       m->data_dir = eina_stringshare_add(buf);
                    }
                  else
                    {
                       dlclose(m->handle);
                       free(m);
                       return NULL;
                    }
               }
          }
     }
   if (!m->handle)
     {
        free(m);
        return NULL;
     }
   if (!m->init_func(m))
     {
        dlclose(m->handle);
        eina_stringshare_del(m->name);
        eina_stringshare_del(m->so_path);
        eina_stringshare_del(m->data_dir);
        eina_stringshare_del(m->bin_dir);
        free(m);
        return NULL;
     }
   m->references = 1;
   eina_hash_direct_add(modules, m->name, m);
   m->as = eina_stringshare_add(as);
   eina_hash_direct_add(modules_as, m->as, m);
   return m;
}

void
_elm_module_del(Elm_Module *m)
{
   m->references--;
   if (m->references > 0) return;
   if (m->shutdown_func) m->shutdown_func(m);
   eina_hash_del(modules, m->name, m);
   eina_hash_del(modules_as, m->as, m);
   if (m->api) free(m->api);
   eina_stringshare_del(m->name);
   eina_stringshare_del(m->as);
   eina_stringshare_del(m->so_path);
   eina_stringshare_del(m->data_dir);
   eina_stringshare_del(m->bin_dir);
   dlclose(m->handle);
   free(m);
}

const void *
_elm_module_symbol_get(Elm_Module *m, const char *name)
{
   return dlsym(m->handle, name);
}
