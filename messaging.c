#include <Ecore.h>
#include "shotgun_private.h"
#include "xml.h"

static void
shotgun_message_free(Shotgun_Message *msg, void *data __UNUSED__)
{
   free(msg->msg);
   eina_stringshare_del(msg->user);
   free(msg);
}

Shotgun_Message *
shotgun_message_new(Shotgun_Auth *auth)
{
   Shotgun_Message *msg;
   EINA_SAFETY_ON_NULL_RETURN_VAL(auth, NULL);

   msg = calloc(1, sizeof(Shotgun_Message));
   msg->account = auth;
   return msg;
}

void
shotgun_message_feed(Shotgun_Auth *auth, Ecore_Con_Event_Server_Data *ev)
{
   Shotgun_Message *msg;
   
   msg = xml_message_read(auth, ev->data, ev->size);
   EINA_SAFETY_ON_NULL_GOTO(msg, error);

   INF("Message from %s: %s", msg->user, msg->msg);
   ecore_event_add(SHOTGUN_EVENT_MESSAGE, msg, (Ecore_End_Cb)shotgun_message_free, NULL);
   return;
error:
   ERR("wtf");
}
