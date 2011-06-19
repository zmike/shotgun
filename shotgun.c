#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Con.h>

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
shotgun_data_tokenize(Ecore_Con_Event_Server_Data *ev)
{
   if (((char*)(ev->data))[0] != '<') return SHOTGUN_DATA_TYPE_UNKNOWN;

   switch (((char*)(ev->data))[1])
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
data(Shotgun_Auth *auth, int type __UNUSED__, Ecore_Con_Event_Server_Data *ev)
{
   char *recv;

   if (auth != ecore_con_server_data_get(ev->server))
     return ECORE_CALLBACK_PASS_ON;

   recv = alloca(ev->size + 1);
   memcpy(recv, ev->data, ev->size);
   recv[ev->size] = 0;
   DBG("Receiving %i bytes:\n%s", ev->size, recv);

   if (auth->state < SHOTGUN_STATE_CONNECTED)
     {
        shotgun_login(auth, ev);
        return ECORE_CALLBACK_RENEW;
     }

   switch (shotgun_data_tokenize(ev))
     {
      case SHOTGUN_DATA_TYPE_MSG:
        shotgun_message_feed(auth, ev);
        break;
      case SHOTGUN_DATA_TYPE_IQ:
        shotgun_iq_feed(auth, ev);
        break;
      case SHOTGUN_DATA_TYPE_PRES:
        shotgun_presence_feed(auth, ev);
      default:
        break;
     }

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

int
main(int argc, char *argv[])
{
   char *pass;
   Shotgun_Auth auth;
   char *getpass_x(const char *prompt);

   if (argc != 3)
     {
        fprintf(stderr, "Usage: %s [username] [domain]\n", argv[0]);
        return 1;
     }

   shotgun_init();

   eina_log_domain_level_set("shotgun", EINA_LOG_LEVEL_DBG);
   //eina_log_domain_level_set("ecore_con", EINA_LOG_LEVEL_DBG);

   memset(&auth, 0, sizeof(Shotgun_Auth));
   pass = getpass_x("Password: ");
   if (!pass)
     {
        ERR("No password entered!");
        return 1;
     }
   auth.pass = pass;

   auth.user = eina_stringshare_add(argv[1]);
   auth.from = eina_stringshare_add(argv[2]);
   auth.resource = eina_stringshare_add("SHOTGUN!");

   shotgun_gchat_connect(&auth);
   ecore_main_loop_begin();

   return 0;
}
