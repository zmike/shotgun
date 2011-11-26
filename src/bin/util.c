#include "ui.h"

const char *
util_configdir_get(void)
{
   static char buf[4096];
   if (!buf[0])
     snprintf(buf, sizeof(buf), "%s/.config/shotgun", getenv("HOME"));
   return &buf[0];
}

Eina_Bool
util_configdir_create(void)
{
   const char *home;
   char buf[4096];
   static int x;
   int e;

   home = util_configdir_get();
   if (ecore_file_is_dir(home)) return EINA_TRUE;
   if (!mkdir(home, S_IRWXU)) return EINA_TRUE;
   e = errno;
   snprintf(buf, sizeof(buf), "%s/.config", getenv("HOME"));
   if (x++ || ecore_file_is_dir(buf) || mkdir(buf, S_IRWXU))
     {
        ERR("Could not create %s: '%s'", home, strerror(e));
        return EINA_FALSE;
     }
   return util_configdir_create();
}
