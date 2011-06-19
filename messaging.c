#include <Ecore.h>
#include "shotgun_private.h"
#include "xml.h"

static void
shotgun_message_free(void *data __UNUSED__, Shotgun_Event_Message *msg)
{
   free(msg->msg);
   eina_stringshare_del(msg->jid);
   free(msg);
}

Shotgun_Event_Message *
shotgun_message_new(Shotgun_Auth *auth)
{
   Shotgun_Event_Message *msg;
   EINA_SAFETY_ON_NULL_RETURN_VAL(auth, NULL);

   msg = calloc(1, sizeof(Shotgun_Event_Message));
   msg->account = auth;
   return msg;
}

Eina_Bool
shotgun_message_send(Shotgun_Auth *auth, const char *to, const char *msg)
{
   size_t len;
   char *xml;

   xml = xml_message_write(auth, to, msg, &len);
   shotgun_write(auth->svr, xml, len);
   free(xml);
   return EINA_TRUE;
}

void
shotgun_message_feed(Shotgun_Auth *auth, Ecore_Con_Event_Server_Data *ev)
{
   Shotgun_Event_Message *msg;

   msg = xml_message_read(auth, ev->data, ev->size);
   EINA_SAFETY_ON_NULL_GOTO(msg, error);

   INF("Message from %s: %s", msg->jid, msg->msg);
   ecore_event_add(SHOTGUN_EVENT_MESSAGE, msg, (Ecore_End_Cb)shotgun_message_free, NULL);
   return;
error:
   ERR("wtf");
}
