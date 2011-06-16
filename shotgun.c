#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Con.h>

#include "xml.h"

static Shotgun_Auth *auth;

int shotgun_log_dom = -1;

/*
char *xml_stream_init_create(const char *from, const char *to, const char *lang, int64_t *len);
Shotgun_Auth *xml_stream_init_read(char *xml, size_t size);
char *xml_starttls_write(int64_t *size);
Eina_Bool xml_starttls_read(char *xml, size_t size);
*/

static Eina_Bool
con(char *argv[], int type, Ecore_Con_Event_Server_Add *ev)
{
   int64_t len;
   char *xml;

   if (type == ECORE_CON_EVENT_SERVER_ADD)
     INF("Connected!");
   else
     {
        INF("STARTTLS succeeded!");
        auth->state++;
     }
   
   xml = xml_stream_init_create(argv[1], strchr(argv[1], '@') + 1, "en", &len);
   ecore_con_server_send(ev->server, xml, len - 1);
   DBG("Sending:\n%s", xml);
   free(xml);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
disc(void *data __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Server_Add *ev __UNUSED__)
{
   ecore_main_loop_quit();
   INF("Disconnected");
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
data(char *argv[] __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Server_Data *ev)
{
   const char *xml;
   char *recv;
   int64_t len;

   recv = alloca(ev->size + 1);
   snprintf(recv, ev->size + 1, "%s", (char*)ev->data);
   DBG("Receiving:\n%*s", ev->size, recv);
   if (!auth) auth = calloc(1, sizeof(Shotgun_Auth));

   switch (auth->state)
     {
      case SHOTGUN_STATE_NONE:
        xml_stream_init_read(auth, ev->data, ev->size);
        if (!auth->state) break;

        if (auth->features.starttls)
          {
             xml = xml_starttls_write(&len);
             auth->state = SHOTGUN_STATE_TLS;
             DBG("Sending:\n%s", xml);
             ecore_con_server_send(ev->server, xml, len);
          }
        else /* who cares */
          ecore_main_loop_quit();
        break;
      case SHOTGUN_STATE_TLS:
        if (xml_starttls_read(ev->data, ev->size))
          ecore_con_ssl_server_upgrade(ev->server, ECORE_CON_USE_TLS);
        else
          ecore_main_loop_quit();
        return ECORE_CALLBACK_RENEW;

      case SHOTGUN_STATE_INIT:
      default:
        ecore_main_loop_quit();
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
main(int argc, char *argv[])
{

   if (argc != 3)
     {
        fprintf(stderr, "Usage: %s [username@domain] [password]\n", argv[0]);
        return 1;
     }
   if (!strchr(argv[1], '@'))
     {
        fprintf(stderr, "Usage: %s [username@domain] [password]\n", argv[0]);
        return 1;
     }
   eina_init();
   ecore_init();
   ecore_con_init();

   memset(&auth, 0, sizeof(Shotgun_Auth));

   /* real men don't accept failure as a possibility */
   shotgun_log_dom = eina_log_domain_register("shotgun", EINA_COLOR_RED);
   eina_log_domain_level_set("shotgun", EINA_LOG_LEVEL_DBG);
   eina_log_domain_level_set("ecore_con", EINA_LOG_LEVEL_DBG);

   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ADD, (Ecore_Event_Handler_Cb)con, argv);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DEL, (Ecore_Event_Handler_Cb)disc, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DATA, (Ecore_Event_Handler_Cb)data, argv);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ERROR, (Ecore_Event_Handler_Cb)error, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_UPGRADE, (Ecore_Event_Handler_Cb)connect, argv);
   ecore_con_server_connect(ECORE_CON_REMOTE_NODELAY, "talk.google.com", 5222, NULL);
   ecore_main_loop_begin();

   return 0;
}
