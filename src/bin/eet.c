#include "ui.h"
#include <Eet.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#ifdef HAVE_PAM
# include <security/pam_appl.h>
# include <security/pam_misc.h>
#endif

const char *sha1_buffer(const unsigned char *data, size_t len);

static Eet_File *images = NULL;

typedef struct UI_Eet_Auth
{
   const char *username;
   const char *domain;
   const char *server;
} UI_Eet_Auth;

static Eet_Data_Descriptor *
eet_auth_edd_new(void)
{
   Eet_Data_Descriptor *ed;
   Eet_Data_Descriptor_Class eddc;
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, UI_Eet_Auth);
   ed = eet_data_descriptor_file_new(&eddc);
#define ADD(name, type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC(ed, UI_Eet_Auth, #name, name, EET_T_##type)

   ADD(username, INLINED_STRING);
   ADD(domain, INLINED_STRING);
   ADD(server, INLINED_STRING);
   return ed;
}

Eina_Bool
ui_eet_init(Shotgun_Auth *auth)
{
   const char *home;
   char buf[4096];
   Eet_File *ef;

   if (!util_configdir_create()) return EINA_FALSE;
   home = util_configdir_get();
   eet_init();
   if (!images)
     {
        snprintf(buf, sizeof(buf), "%s/images.eet", home);
        images = eet_open(buf, EET_FILE_MODE_READ_WRITE);
        if (!images) ERR("Could not open image cache file!");
     }

   if (shotgun_data_get(auth))
     {
        eet_shutdown();
        goto out;
     }
   snprintf(buf, sizeof(buf), "%s/shotgun.eet", home);
   ef = eet_open(buf, EET_FILE_MODE_READ_WRITE);
   if (!ef) goto error;
   shotgun_data_set(auth, ef);
out:
   if (images) INF("All files loaded successfully!");
   else WRN("Some files failed to open!");
   return EINA_TRUE;
error:
   ERR("Could not open account info file!");
   if (images) eet_close(images);
   eet_shutdown();
   return EINA_FALSE;
}

void
ui_eet_shutdown(Shotgun_Auth *auth)
{
   if (!auth) return;
   if (images) eet_close(images);
   eet_close(shotgun_data_get(auth));
   eet_shutdown();
}

Shotgun_Auth *
ui_eet_auth_get(void)
{
   Eet_File *ef;
   Eet_Data_Descriptor *ed;
   UI_Eet_Auth *a;
   char *jid, buf[4096], *p;
   const char *home;
   int size;
   Shotgun_Auth *auth;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(!util_configdir_create(), NULL);
   eet_init();
   home = util_configdir_get();

   snprintf(buf, sizeof(buf), "%s/shotgun.eet", home);
   ef = eet_open(buf, EET_FILE_MODE_READ_WRITE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(ef, NULL);

   jid = eet_read(ef, "last_account", &size);
   if ((!jid) || (jid[size - 1]))
     {
        eet_close(ef);
        ERR("Could not read name of last account!");
        return NULL;
     }
   ed = eet_auth_edd_new();
   a = eet_data_read(ef, ed, jid);
   eet_data_descriptor_free(ed);
   if (!a)
     {
        eet_close(ef);
        ERR("Could not read account info!");
        return NULL;
     }
   auth = shotgun_new(a->server, a->username, a->domain);
   if (!auth)
     {
        ERR("Could not create auth info!");
        goto error;
     }
   snprintf(buf, sizeof(buf), "%s/pw", shotgun_jid_get(auth));
   p = eet_read_cipher(ef, buf, &size, shotgun_jid_get(auth));
   if (p) shotgun_password_set(auth, p);
#ifdef HAVE_PAM
#endif
   shotgun_data_set(auth, ef);
   eina_stringshare_del(a->server);
   eina_stringshare_del(a->username);
   eina_stringshare_del(a->domain);
   free(a);
   return auth;
error:
   eet_close(ef);
   eina_stringshare_del(a->server);
   eina_stringshare_del(a->username);
   eina_stringshare_del(a->domain);
   free(a);
   return NULL;
}

void
ui_eet_auth_set(Shotgun_Auth *auth, Eina_Bool store_pw, Eina_Bool use_auth)
{
   Eet_File *ef;
   const char *s, *jid;
   char buf[1024];
   Eet_Data_Descriptor *ed;
   UI_Eet_Auth a;

   ef = shotgun_data_get(auth);
   s = shotgun_jid_get(auth);
   jid = shotgun_jid_get(auth);
   a.username = shotgun_username_get(auth);
   a.domain = shotgun_domain_get(auth);
   a.server = shotgun_servername_get(auth);
   /* FIXME: list for multiple accounts */
   ed = eet_auth_edd_new();

   eet_write(ef, "last_account", s, strlen(s) + 1, 0);

   eet_data_write(ef, ed, jid, &a, 0);
   eet_data_descriptor_free(ed);
   if (!store_pw) return;
   snprintf(buf, sizeof(buf), "%s/pw", jid);
   if (!use_auth)
     {
        s = shotgun_password_get(auth);
        /* feeble attempt at ciphering, but at least it isn't plaintext */
        eet_write_cipher(ef, buf, s, strlen(s) + 1, 0, jid);
        return;
     }
#ifdef HAVE_PAM
   pam_handle_t *p;
   struct pam_conv pc;

   if (pam_start("shotgun", user, conv, &p) != PAM_SUCCESS)
     {
        ERR("Could not start PAM session! Password not saved!");
        return;
     }

#else
   CRI("PAM support not detected! Unable to store password!");
   return;
#endif
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
        eet_alias(images, url, sha1, 0);
        eet_sync(images);
        INF("Added new alias for image %s", sha1);
        eina_stringshare_del(sha1);
        free(list);
        return;
     }

   eet_write(images, sha1, eina_binbuf_string_get(buf), eina_binbuf_length_get(buf), 1);
   eet_alias(images, url, sha1, 0);
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
