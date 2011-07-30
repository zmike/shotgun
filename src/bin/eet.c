#include "ui.h"
#include <Eet.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

const char *sha1_buffer(const unsigned char *data, size_t len);

static Eet_File *images = NULL;

Eina_Bool
ui_eet_init(Shotgun_Auth *auth)
{
   const char *home;
   char buf[4096];
   Eet_File *ef;

   eet_init();

   home = util_configdir_get();
   if (!ecore_file_exists(home))
     {
        if (mkdir(home, S_IRWXU))
          {
             int x = errno;
             snprintf(buf, sizeof(buf), "%s/.config", getenv("HOME"));
             if (ecore_file_exists(buf) || mkdir(buf, S_IRWXU))
               {
                  ERR("Could not create %s: '%s'", home, strerror(x));
                  eet_shutdown();
                  return EINA_FALSE;
               }
             eet_shutdown();
             return ui_eet_init(auth);
          }
     }
   if (!images)
     {
        snprintf(buf, sizeof(buf), "%s/images.eet", home);
        images = eet_open(buf, EET_FILE_MODE_READ_WRITE);
        if (!images) ERR("Could not open image cache file!");
     }
   
   snprintf(buf, sizeof(buf), "%s/%s.eet", home, shotgun_username_get(auth));
   ef = eet_open(buf, EET_FILE_MODE_READ_WRITE);
   if (!ef)
     {
        ERR("Could not open account info file!");
        eet_close(images);
        eet_shutdown();
        return EINA_FALSE;
     }
   INF("All files loaded successfully!");
   shotgun_data_set(auth, ef);
   return EINA_TRUE;
}

void
ui_eet_shutdown(Shotgun_Auth *auth)
{
   if (!auth) return;
   if (images) eet_close(images);
   eet_close(shotgun_data_get(auth));
   eet_shutdown();
}

void
ui_eet_image_add(const char *url, Eina_Binbuf *buf)
{
   const char *sha1;
   int lsize;
   char **list;

   if (!images) return;

   sha1 = sha1_buffer(eina_binbuf_string_get(buf), eina_binbuf_length_get(buf));
   INF("Image: %s - %s", url, sha1);

   list = eet_list(images, url, &lsize);
   if (lsize)
     {
        eina_stringshare_del(sha1);
        free(list);
        return; /* should never happen */
     }
   list = eet_list(images, sha1, &lsize);
   if (lsize)
     {
        eet_alias(images, url, sha1, 1);
        eet_sync(images);
        INF("Added new alias for image %s", sha1);
        eina_stringshare_del(sha1);
        free(list);
        return;
     }

   eet_write(images, sha1, eina_binbuf_string_get(buf), eina_binbuf_length_get(buf), 1);
   eet_alias(images, url, sha1, 1);
   eet_sync(images);
   INF("Added new image with length %zu: %s", eina_binbuf_length_get(buf), sha1);
}

Eina_Binbuf *
ui_eet_image_get(const char *url)
{
   size_t size;
   unsigned char *img;
   Eina_Binbuf *buf = NULL;
   char **list;
   int lsize;

   if (!images) return NULL;

   list = eet_list(images, url, &lsize);
   if (!lsize) return NULL;
   free(list);

   img = eet_read(images, url, (int*)&size);
   buf = eina_binbuf_new(); /* FIXME: eina_binbuf_managed_new */
   eina_binbuf_append_length(buf, img, size);

   free(img);
   return buf;
}
