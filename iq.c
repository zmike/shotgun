#include <Ecore.h>
#include "shotgun_private.h"
#include "xml.h"
/*
errors: http://www.rfc-editor.org/rfc/rfc6120.txt
The "stanza-kind" MUST be one of message, presence, or iq.

The "error-type" MUST be one of the following:

o  auth -- retry after providing credentials

o  cancel -- do not retry (the error cannot be remedied)

o  continue -- proceed (the condition was only a warning)

o  modify -- retry after changing the data sent

o  wait -- retry after waiting (the error is temporary)
*/

static void
shotgun_user_free(Shotgun_User *user)
{
   eina_stringshare_del(user->jid);
   eina_stringshare_del(user->name);
   free(user);
}

static void
shotgun_iq_event_free(void *data __UNUSED__, Shotgun_Event_Iq *iq)
{
   Shotgun_User *user;
   switch (iq->type)
     {
      case SHOTGUN_IQ_EVENT_TYPE_ROSTER:
        EINA_LIST_FREE(iq->ev, user)
          shotgun_user_free(user);
      default:
        break;
     }
   free(iq);
}

Eina_Bool
shotgun_iq_roster_get(Shotgun_Auth *auth)
{
   size_t len;
   char *xml;

   xml = xml_iq_write(auth, SHOTGUN_IQ_PRESET_ROSTER, &len);
   shotgun_write(auth->svr, xml, len);
   free(xml);
   return EINA_TRUE;
}

void
shotgun_iq_feed(Shotgun_Auth *auth, char *data, size_t size)
{
   Shotgun_Event_Iq *iq;
   Eina_List *l;
   Shotgun_User *user;

   iq = xml_iq_read(auth, data, size);
   if (!iq) return; /* no event needed */

   switch (iq->type)
     {
      case SHOTGUN_IQ_EVENT_TYPE_ROSTER:
        EINA_LIST_FOREACH(iq->ev, l, user)
          {
             if (user->name)
               INF("User found: %s (%s)", user->name, user->jid);
             else
               INF("User found: %s", user->jid);
          }
        ecore_event_add(SHOTGUN_EVENT_IQ, iq, (Ecore_End_Cb)shotgun_iq_event_free, NULL);
      default:
        break;
     }
}
