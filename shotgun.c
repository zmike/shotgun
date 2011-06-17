#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Con.h>

#include "xml.h"

int shotgun_log_dom = -1;

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

static void
init_stream(Shotgun_Auth *auth)
{
   size_t len;
   char *xml;

   xml = xml_stream_init_create(auth->user, auth->from, "en", &len);
   shotgun_write(auth->svr, xml, len - 1);
   free(xml);
}

static Eina_Bool
con(Shotgun_Auth *auth, int type, Ecore_Con_Event_Server_Add *ev __UNUSED__)
{
   if (type == ECORE_CON_EVENT_SERVER_ADD)
     INF("Connected!");
   else
     {
        INF("STARTTLS succeeded!");
        auth->state++;
     }

   init_stream(auth);
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
data(Shotgun_Auth *auth, int type __UNUSED__, Ecore_Con_Event_Server_Data *ev)
{
   char *recv, *out;
   size_t len;

   recv = alloca(ev->size + 1);
   snprintf(recv, ev->size + 1, "%s", (char*)ev->data);
   DBG("Receiving:\n%*s", ev->size, recv);

   switch (auth->state)
     {
      case SHOTGUN_STATE_NONE:
        if (!xml_stream_init_read(auth, ev->data, ev->size)) break;

        if (auth->features.starttls)
          {
             auth->state = SHOTGUN_STATE_TLS;
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
        if (!xml_stream_init_read(auth, ev->data, ev->size)) break;
        out = sasl_init(auth);
        if (!out) ecore_main_loop_quit();
        else
          {
             char *send;

             send = xml_sasl_write(out, &len);
             shotgun_write(ev->server, send, len);
             free(out);
             free(send);
             auth->state++;
          }
        break;
      case SHOTGUN_STATE_SASL:
        if (!xml_sasl_read(ev->data, ev->size))
          {
             ERR("Login failed!");
             ecore_main_loop_quit();
             break;
          }
        /* yes, another stream. */
        init_stream(auth);
        auth->state++;
        break;
      case SHOTGUN_STATE_BIND:
        if (!xml_stream_init_read(auth, ev->data, ev->size))
          break;

        out = xml_bind_write(&len);
        EINA_SAFETY_ON_NULL_GOTO(out, error);

        shotgun_write(ev->server, out, strlen(out));
        free(out);
        auth->state++;
        break;
      case SHOTGUN_STATE_CONNECTED:
        INF("Login complete!");
      default:
        ecore_main_loop_quit();
     }
   return ECORE_CALLBACK_RENEW;
error:
   ERR("wtf");
   ecore_main_loop_quit();
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
   char *pass;
   Shotgun_Auth auth;
   char *getpass_x(const char *prompt);

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
   pass = getpass_x("Password:");
   if (!pass)
     {
        ERR("No password entered!");
        return 1;
     }
   auth.pass = pass;

   auth.user = eina_stringshare_add(argv[1]);
   auth.from = eina_stringshare_add(argv[2]);

   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ADD, (Ecore_Event_Handler_Cb)con, &auth);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DEL, (Ecore_Event_Handler_Cb)disc, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DATA, (Ecore_Event_Handler_Cb)data, &auth);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ERROR, (Ecore_Event_Handler_Cb)error, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_UPGRADE, (Ecore_Event_Handler_Cb)con, &auth);
   auth.svr = ecore_con_server_connect(ECORE_CON_REMOTE_NODELAY, "talk.google.com", 5222, &auth);
   ecore_main_loop_begin();

   return 0;
}
