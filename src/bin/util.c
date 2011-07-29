#include "ui.h"

const char *
util_configdir_get(void)
{
   static char buf[4096];
   snprintf(buf, sizeof(buf), "%s/.config/shotgun", getenv("HOME"));
   return &buf[0];
}
