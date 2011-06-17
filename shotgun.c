#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Con.h>

#include "xml.h"
#include "sasl.h"

static Shotgun_Auth auth;

int shotgun_log_dom = -1;

char *getpass_x(const char *prompt);

static void
shotgun_write(Ecore_Con_Server *svr, const void *data, size_t size)
{
   DBG("Sending:\n%s", (char*)data);
   ecore_con_server_send(svr, data, size);
}

/*
char *xml_stream_init_create(const char *from, const char *to, const char *lang, int64_t *len);
Shotgun_Auth *xml_stream_init_read(char *xml, size_t size);
char *xml_starttls_write(int64_t *size);
Eina_Bool xml_starttls_read(char *xml, size_t size);
*/

static Eina_Bool
con(char *argv[], int type, Ecore_Con_Event_Server_Add *ev)
{
   size_t len;
   char *xml;

   if (type == ECORE_CON_EVENT_SERVER_ADD)
     INF("Connected!");
   else
     {
        INF("STARTTLS succeeded!");
        auth.state++;
     }

   xml = xml_stream_init_create(argv[1], argv[2], "en", &len);
   shotgun_write(ev->server, xml, len - 1);
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
data(void *data __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Server_Data *ev)
{
   char *recv, *sasl;

   recv = alloca(ev->size + 1);
   snprintf(recv, ev->size + 1, "%s", (char*)ev->data);
   DBG("Receiving:\n%*s", ev->size, recv);

   switch (auth.state)
     {
      case SHOTGUN_STATE_NONE:
        if (!xml_stream_init_read(&auth, ev->data, ev->size)) break;

        if (auth.features.starttls)
          {
             auth.state = SHOTGUN_STATE_TLS;
             shotgun_write(ev->server, XML_STARTTLS, sizeof(XML_STARTTLS) - 1);
          }
        else /* who cares */
          ecore_main_loop_quit();
        break;
      case SHOTGUN_STATE_TLS:
        if (xml_starttls_read(ev->data, ev->size))
          ecore_con_ssl_server_upgrade(ev->server, ECORE_CON_USE_MIXED);
        else
          ecore_main_loop_quit();
        return ECORE_CALLBACK_RENEW;

      case SHOTGUN_STATE_FEATURES:
        if (!xml_stream_init_read(&auth, ev->data, ev->size)) break;
        sasl = sasl_init(&auth);
        if (!sasl) ecore_main_loop_quit();
        else
          {
             char *send;
             size_t len;

             send = xml_sasl_write(sasl, &len);
             free(sasl);
             shotgun_write(ev->server, send, len);
             free(send);
             auth.state++;
          }
        break;
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
        fprintf(stderr, "Usage: %s [username] [domain]\n", argv[0]);
        return 1;
     }
   eina_init();
   ecore_init();
   ecore_con_init();

   /* real men don't accept failure as a possibility */
   shotgun_log_dom = eina_log_domain_register("shotgun", EINA_COLOR_RED);
   eina_log_domain_level_set("shotgun", EINA_LOG_LEVEL_DBG);
   eina_log_domain_level_set("ecore_con", EINA_LOG_LEVEL_DBG);

   memset(&auth, 0, sizeof(Shotgun_Auth));
   auth.user = eina_stringshare_add(argv[1]);
   auth.from = eina_stringshare_add(argv[2]);
   auth.pass = getpass_x("Password: ");
   if (!auth.pass)
     {
        ERR("No password entered!");
        return 1;
     }

   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ADD, (Ecore_Event_Handler_Cb)con, argv);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DEL, (Ecore_Event_Handler_Cb)disc, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DATA, (Ecore_Event_Handler_Cb)data, argv);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ERROR, (Ecore_Event_Handler_Cb)error, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_UPGRADE, (Ecore_Event_Handler_Cb)con, argv);
   ecore_con_server_connect(ECORE_CON_REMOTE_NODELAY, "talk.google.com", 5222, NULL);
   ecore_main_loop_begin();

   return 0;
}
