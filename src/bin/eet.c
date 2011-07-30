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
          { /* FIXME: ~/.config not existing */
             ERR("Could not create %s: '%s'", home, strerror(errno));
             eet_shutdown();
             return EINA_FALSE;
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
   Eet_Dictionary *ed;

   if (!images) return;

   sha1 = sha1_buffer(eina_binbuf_string_get(buf), eina_binbuf_length_get(buf));
   INF("Image: %s - %s", url, sha1);
   ed = eet_dictionary_get(images);
   if (eet_dictionary_string_check(ed, url))
     {
        eina_stringshare_del(sha1);
        return; /* should never happen */
     }
   if (eet_dictionary_string_check(ed, sha1))
     {
        eet_alias(images, sha1, url, 1);
        INF("Added new alias for image %s", sha1);
        eina_stringshare_del(sha1);
        return;
     }
   eet_write(images, sha1, eina_binbuf_string_get(buf), eina_binbuf_length_get(buf), 1);
   eet_alias(images, sha1, url, 1);
   INF("Added new image with length %zu: %s", eina_binbuf_length_get(buf), sha1);
}

Eina_Binbuf *
ui_eet_image_get(const char *url)
{
   const char *sha1;
   size_t size;
   unsigned char *img;
   Eina_Binbuf *buf = NULL;
   if (!images) return NULL;
   Eet_Dictionary *ed;

   ed = eet_dictionary_get(images);
   if (!eet_dictionary_string_check(ed, url)) return NULL;
   sha1 = eet_alias_get(images, url);
   EINA_SAFETY_ON_NULL_RETURN_VAL(sha1, NULL);

   img = eet_read(images, sha1, (int*)&size);
   buf = eina_binbuf_new(); /* FIXME: eina_binbuf_managed_new */
   eina_binbuf_append_length(buf, img, size);

   free(img);
   eina_stringshare_del(sha1);
   return buf;
}
