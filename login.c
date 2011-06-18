#include <Ecore.h>
#include "shotgun_private.h"
#include "xml.h"

// http://www.ietf.org/rfc/rfc4616.txt

static char *
sasl_init(Shotgun_Auth *auth)
{
   Eina_Binbuf *buf;
   char *ret;

   buf = eina_binbuf_new();

   eina_binbuf_append_char(buf, 0);
   eina_binbuf_append_length(buf, auth->user, eina_stringshare_strlen(auth->user));
   eina_binbuf_append_char(buf, 0);
   eina_binbuf_append_length(buf, auth->pass, strlen(auth->pass));
   ret = shotgun_base64_encode(eina_binbuf_string_get(buf), eina_binbuf_length_get(buf));
   eina_binbuf_free(buf);
   return ret;
}

static void
shotgun_stream_init(Shotgun_Auth *auth)
{
   size_t len;
   char *xml;

   xml = xml_stream_init_create(auth->user, auth->from, "en", &len);
   shotgun_write(auth->svr, xml, len - 1);
   free(xml);
}

Eina_Bool
shotgun_login_con(Shotgun_Auth *auth, int type, Ecore_Con_Event_Server_Add *ev)
{
   if (auth != ecore_con_server_data_get(ev->server))
     return ECORE_CALLBACK_PASS_ON;

   if (type == ECORE_CON_EVENT_SERVER_ADD)
     INF("Connected!");
   else
     {
        INF("STARTTLS succeeded!");
        auth->state++;
     }

   shotgun_stream_init(auth);
   return ECORE_CALLBACK_RENEW;
}

void
shotgun_login(Shotgun_Auth *auth, Ecore_Con_Event_Server_Data *ev)
{
   char *out;
   size_t len;

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
        return;

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
        shotgun_stream_init(auth);
        auth->state++;
        break;
      case SHOTGUN_STATE_BIND:
        if (!xml_stream_init_read(auth, ev->data, ev->size))
          break;

        out = xml_iq_write(auth, SHOTGUN_IQ_PRESET_BIND, &len);
        EINA_SAFETY_ON_NULL_GOTO(out, error);

        shotgun_write(ev->server, out, strlen(out));
        free(out);
        auth->state++;
        break;
      case SHOTGUN_STATE_CONNECTING:
        INF("Login complete!");
        auth->state++;
        shotgun_iq_roster_get(auth);
      default:
        break;
     }
   return;
error:
   ERR("wtf");
   ecore_main_loop_quit();
}
