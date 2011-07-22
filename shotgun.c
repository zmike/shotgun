#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Con.h>

#include <ctype.h>

#include "xml.h"

int shotgun_log_dom = -1;

int SHOTGUN_EVENT_MESSAGE = 0;
int SHOTGUN_EVENT_PRESENCE = 0;
int SHOTGUN_EVENT_IQ = 0;

/*
char *xml_stream_init_create(const char *from, const char *to, const char *lang, int64_t *len);
Shotgun_Auth *xml_stream_init_read(char *xml, size_t size);
char *xml_starttls_write(int64_t *size);
Eina_Bool xml_starttls_read(char *xml, size_t size);
*/

static Eina_Bool
disc(void *data __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Server_Add *ev __UNUSED__)
{
   ecore_main_loop_quit();
   INF("Disconnected");
   return ECORE_CALLBACK_RENEW;
}

static Shotgun_Data_Type
shotgun_data_tokenize(Shotgun_Auth *auth, Ecore_Con_Event_Server_Data *ev)
{
   const char *data;
   data = auth->buf ? eina_strbuf_string_get(auth->buf) : ev->data;
   if (data[0] != '<') return SHOTGUN_DATA_TYPE_UNKNOWN;

   switch (data[1])
     {
      case 'm':
        return SHOTGUN_DATA_TYPE_MSG;
      case 'i':
        return SHOTGUN_DATA_TYPE_IQ;
      case 'p':
        return SHOTGUN_DATA_TYPE_PRES;
      default:
        break;
     }
   return SHOTGUN_DATA_TYPE_UNKNOWN;
}

static Eina_Bool
shotgun_data_detect(Shotgun_Auth *auth, Ecore_Con_Event_Server_Data *ev)
{
   if (((char*)ev->data)[ev->size - 1] != '>')
     {
        if (!auth->buf) auth->buf = eina_strbuf_new();
        eina_strbuf_append_length(auth->buf, ev->data, ev->size);
        return EINA_FALSE;
     }
   if (auth->buf)
     {
        size_t len;
        const char *data, *tag;
        char buf[24] = {0};

        DBG("Appending %i to buffer", ev->size);
        eina_strbuf_append_length(auth->buf, ev->data, ev->size);
        len = eina_strbuf_length_get(auth->buf);
        data = eina_strbuf_string_get(auth->buf);
        tag = data + (len - 2);
        while (tag[0] != '<') tag--;
        tag += 2;
        if (!memcmp(data + 1, tag, len - (tag - data) - 1)) /* open/end tags maybe match? */
          return EINA_TRUE;
        memcpy(buf, data, sizeof(buf) - 1);
        DBG("%s and %s do not match!", buf, tag);
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

static Eina_Bool
data(Shotgun_Auth *auth, int type __UNUSED__, Ecore_Con_Event_Server_Data *ev)
{
   char *recv, *data, *p;
   size_t size;

   if (auth != ecore_con_server_data_get(ev->server))
     return ECORE_CALLBACK_PASS_ON;

   if (ev->size == 1)
     {
        DBG("Received carriage return");
        return ECORE_CALLBACK_RENEW;
     }
   recv = alloca(ev->size + 1);
   memcpy(recv, ev->data, ev->size);
   for (p = recv + ev->size - 1; isspace(*p); p--)
     *p = 0;
   recv[ev->size] = 0;
   DBG("Receiving %i bytes:\n%s", ev->size, recv);

   if (auth->state < SHOTGUN_STATE_CONNECTED)
     {
        shotgun_login(auth, ev);
        return ECORE_CALLBACK_RENEW;
     }

   if (!shotgun_data_detect(auth, ev))
     return ECORE_CALLBACK_RENEW;

   data = auth->buf ? (char*)eina_strbuf_string_get(auth->buf) : (char*)ev->data;
   size = auth->buf ? eina_strbuf_length_get(auth->buf) : (size_t)ev->size;

   switch (shotgun_data_tokenize(auth, ev))
     {
      case SHOTGUN_DATA_TYPE_MSG:
        shotgun_message_feed(auth, data, size);
        break;
      case SHOTGUN_DATA_TYPE_IQ:
        shotgun_iq_feed(auth, data, size);
        break;
      case SHOTGUN_DATA_TYPE_PRES:
        shotgun_presence_feed(auth, data, size);
      default:
        break;
     }
   if (auth->buf) eina_strbuf_free(auth->buf);
   auth->buf = NULL;

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
error(void *d __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Server_Error *ev)
{
   ERR("%s", ev->error);
   ecore_main_loop_quit();
   return ECORE_CALLBACK_RENEW;
}

int
shotgun_init(void)
{
   eina_init();
   ecore_init();
   ecore_con_init();

   /* real men don't accept failure as a possibility */
   shotgun_log_dom = eina_log_domain_register("shotgun", EINA_COLOR_RED);

   SHOTGUN_EVENT_MESSAGE = ecore_event_type_new();
   SHOTGUN_EVENT_PRESENCE = ecore_event_type_new();
   SHOTGUN_EVENT_IQ = ecore_event_type_new();

   return 1;
}

Eina_Bool
shotgun_gchat_connect(Shotgun_Auth *auth)
{
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ADD, (Ecore_Event_Handler_Cb)shotgun_login_con, auth);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DEL, (Ecore_Event_Handler_Cb)disc, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DATA, (Ecore_Event_Handler_Cb)data, auth);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ERROR, (Ecore_Event_Handler_Cb)error, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_UPGRADE, (Ecore_Event_Handler_Cb)shotgun_login_con, auth);
   auth->svr = ecore_con_server_connect(ECORE_CON_REMOTE_NODELAY, "talk.google.com", 5222, auth);

   return EINA_TRUE;
}

Shotgun_Auth *
shotgun_new(const char *username, const char *domain)
{
   Shotgun_Auth *auth;
   EINA_SAFETY_ON_NULL_RETURN_VAL(username, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(domain, NULL);

   auth = calloc(1, sizeof(Shotgun_Auth));
   auth->user = eina_stringshare_add(username);
   auth->from = eina_stringshare_add(domain);
   auth->resource = eina_stringshare_add("SHOTGUN!");
   return auth;
}

void
shotgun_password_set(Shotgun_Auth *auth, const char *password)
{
   EINA_SAFETY_ON_NULL_RETURN(auth);
   EINA_SAFETY_ON_NULL_RETURN(password);

   auth->pass = password;
}

void
shotgun_password_del(Shotgun_Auth *auth)
{
   EINA_SAFETY_ON_NULL_RETURN(auth);
   auth->pass = NULL;
}
