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
static Eet_File *dummies = NULL;

typedef struct UI_Eet_Auth
{
   const char *username;
   const char *domain;
   const char *resource;
   const char *server;
} UI_Eet_Auth;

static Eet_Data_Descriptor *
eet_ss_edd_new(void)
{
   Eet_Data_Descriptor *edd;
   Eet_Data_Descriptor_Class eddc;
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Shotgun_Settings);
   edd = eet_data_descriptor_stream_new(&eddc);
#define ADD(name, type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Shotgun_Settings, #name, name, EET_T_##type)

   ADD(disable_notify, UCHAR);
   ADD(enable_chat_focus, UCHAR);
   ADD(enable_chat_promote, UCHAR);
   ADD(enable_chat_newselect, UCHAR);
   ADD(enable_account_info, UCHAR);
   ADD(enable_last_account, UCHAR);
   ADD(enable_logging, UCHAR);
#undef ADD
   return edd;
}

static Eet_Data_Descriptor *
eet_auth_edd_new(void)
{
   Eet_Data_Descriptor *edd;
   Eet_Data_Descriptor_Class eddc;
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, UI_Eet_Auth);
   edd = eet_data_descriptor_stream_new(&eddc);
#define ADD(name, type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC(edd, UI_Eet_Auth, #name, name, EET_T_##type)

   ADD(username, INLINED_STRING);
   ADD(domain, INLINED_STRING);
   ADD(server, INLINED_STRING);
#undef ADD
   return edd;
}

static Eet_Data_Descriptor *
eet_userinfo_edd_new(void)
{
   Eet_Data_Descriptor *edd;
   Eet_Data_Descriptor_Class eddc;
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Shotgun_User_Info);
   edd = eet_data_descriptor_stream_new(&eddc);
#define ADD(name, type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Shotgun_User_Info, #name, name, EET_T_##type)

   ADD(full_name, INLINED_STRING);
   ADD(photo.type, INLINED_STRING);
   ADD(photo.sha1, INLINED_STRING);
#undef ADD
   return edd;
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
   if (!dummies)
     {
        snprintf(buf, sizeof(buf), "%s/dummies.eet", home);
        dummies = eet_open(buf, EET_FILE_MODE_READ_WRITE);
        if (!dummies) ERR("Could not open dummy cache file!");
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
   if (images && dummies) INF("All files loaded successfully!");
   else WRN("Some files failed to open!");
   return EINA_TRUE;
error:
   ERR("Could not open account info file!");
   if (images) eet_close(images);
   if (dummies) eet_close(dummies);
   eet_shutdown();
   return EINA_FALSE;
}

void
ui_eet_shutdown(Shotgun_Auth *auth)
{
   if (!auth) return;
   if (images) eet_close(images);
   if (dummies) eet_close(dummies);
   eet_close(shotgun_data_get(auth));
   eet_shutdown();
}

Shotgun_Auth *
_ui_eet_auth_get(Eet_File *ef, const char *jid)
{
   Eet_Data_Descriptor *edd;
   UI_Eet_Auth *a;
   Shotgun_Auth *auth;
   char buf[4096], *p;
   int size;

   edd = eet_auth_edd_new();
   a = eet_data_read(ef, edd, jid);
   eet_data_descriptor_free(edd);
   if (!a)
     {
        eet_close(ef);
        return NULL;
     }
   auth = shotgun_new(a->server, a->username, a->domain);
   /* FIXME: use resource */
   if (auth)
     {
        snprintf(buf, sizeof(buf), "%s/pw", shotgun_jid_get(auth));
        p = eet_read_cipher(ef, buf, &size, shotgun_jid_get(auth));
        if (p) shotgun_password_set(auth, p);
        shotgun_data_set(auth, ef);
     }
   else
     {
        ERR("Could not create auth info!");
        eet_close(ef);
     }
   eina_stringshare_del(a->server);
   eina_stringshare_del(a->username);
   eina_stringshare_del(a->domain);
   eina_stringshare_del(a->resource);
   free(a);
   return auth;
}

Shotgun_Auth *
ui_eet_auth_get(const char *name, const char *domain)
{
   Eet_File *ef;
   char *jid, buf[4096];
   const char *home;
   int size;
   
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!util_configdir_create(), NULL);
   eet_init();
   home = util_configdir_get();

   snprintf(buf, sizeof(buf), "%s/shotgun.eet", home);
   ef = eet_open(buf, EET_FILE_MODE_READ_WRITE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(ef, NULL);

   jid = eet_read(ef, "last_account", &size);
   if ((!jid) || (jid[size - 1]))
     {
        if ((!name) && (!domain))
          {
             eet_close(ef);
             ERR("Could not read name of last account!");
             return NULL;
          }
        snprintf(buf, sizeof(buf), "%s@%s", name, domain);
     }
   else
     return _ui_eet_auth_get(ef, jid);
   return _ui_eet_auth_get(ef, buf);
}

void
ui_eet_auth_set(Shotgun_Auth *auth, Shotgun_Settings *settings, Eina_Bool use_auth)
{
   Eet_File *ef;
   const char *s, *jid;
   char buf[1024];
   Eet_Data_Descriptor *edd;
   UI_Eet_Auth a;

   ef = shotgun_data_get(auth);
   jid = shotgun_jid_get(auth);
   a.username = shotgun_username_get(auth);
   a.domain = shotgun_domain_get(auth);
   a.resource = shotgun_resource_get(auth);
   a.server = shotgun_servername_get(auth);

   if (settings->enable_last_account)
     eet_write(ef, "last_account", jid, strlen(jid) + 1, 0);
   else
     eet_delete(ef, "last_account");

   snprintf(buf, sizeof(buf), "%s/pw", jid);
   if (!settings->enable_account_info)
     {
        eet_delete(ef, buf);
        return;
     }
   /* FIXME: list for multiple accounts */
   edd = eet_auth_edd_new();

   eet_data_write(ef, edd, jid, &a, 0);
   eet_data_descriptor_free(edd);
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
ui_eet_userinfo_add(Shotgun_Auth *auth, Shotgun_User_Info *info)
{
   char buf[1024];
   const char *jid;
   Eet_Data_Descriptor *edd;
   Eet_File *ef = shotgun_data_get(auth);

   jid = shotgun_jid_get(auth);
   edd = eet_userinfo_edd_new();
   snprintf(buf, sizeof(buf), "%s/%s", jid, info->jid);
   if (!eet_data_write_cipher(ef, edd, buf, shotgun_password_get(auth), info, 0))
     {
        ERR("Failed to write userinfo for %s!", info->jid);
        eet_data_descriptor_free(edd);
        return;
     }
   INF("Wrote encrypted userinfo for %s to disk", info->jid);
   if (!info->photo.data) return;
   snprintf(buf, sizeof(buf), "%s/%s/img", jid, info->jid);
   eet_write(ef, buf, info->photo.data, info->photo.size, 1);
   eet_sync(ef);
   eet_data_descriptor_free(edd);
}

Shotgun_User_Info *
ui_eet_userinfo_get(Shotgun_Auth *auth, const char *jid)
{
   char buf[1024];
   const char *me;
   Eet_Data_Descriptor *edd;
   Shotgun_User_Info *info;
   Eet_File *ef = shotgun_data_get(auth);

   edd = eet_userinfo_edd_new();
   me = shotgun_jid_get(auth);
   snprintf(buf, sizeof(buf), "%s/%s", me, jid);
   info = eet_data_read_cipher(ef, edd, buf, shotgun_password_get(auth));
   if (!info)
     {
        INF("Userinfo for %s does not exist", jid);
        eet_data_descriptor_free(edd);
        return NULL;
     }
   snprintf(buf, sizeof(buf), "%s/%s/img", me, jid);
   info->photo.data = eet_read(ef, buf, (int*)&info->photo.size);
   eet_data_descriptor_free(edd);
   INF("Read encrypted userinfo for %s from disk", jid);
   return info;
}

void
ui_eet_dummy_add(const char *url)
{
   if (!dummies) return;
   eet_write(dummies, url, "0", 1, 0);
   INF("Added new dummy for url %s", url);
}

Eina_Bool
ui_eet_dummy_check(const char *url)
{
   char **list;
   int lsize;

   if (!dummies) return EINA_FALSE;
   list = eet_list(dummies, url, &lsize);
   if (lsize)
     {
        free(list);
        return EINA_TRUE;
     }
   return EINA_FALSE;
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

Shotgun_Settings *
ui_eet_settings_get(Shotgun_Auth *auth)
{
   Eet_Data_Descriptor *edd;
   Eet_File *ef = shotgun_data_get(auth);
   Shotgun_Settings *ss;

   edd = eet_ss_edd_new();
   ss = eet_data_read(ef, edd, "settings");
   eet_data_descriptor_free(edd);
   return ss;
}

void
ui_eet_settings_set(Shotgun_Auth *auth, Shotgun_Settings *ss)
{
   Eet_Data_Descriptor *edd;
   Eet_File *ef = shotgun_data_get(auth);

   edd = eet_ss_edd_new();
   eet_data_write(ef, edd, "settings", ss, 0);
   eet_data_descriptor_free(edd);
}
