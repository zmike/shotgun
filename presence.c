#include <Ecore.h>
#include "shotgun_private.h"
#include "xml.h"

static void
shotgun_presence_free(void *d __UNUSED__, Shotgun_Event_Presence *pres)
{
   eina_stringshare_del(pres->jid);
   free(pres->description);
   free(pres->photo);
   free(pres);
}

Shotgun_Event_Presence *
shotgun_presence_new(Shotgun_Auth *auth)
{
   Shotgun_Event_Presence *pres;

   pres = calloc(1, sizeof(Shotgun_Event_Presence));
   pres->account = auth;
   return pres;
}

void
shotgun_presence_feed(Shotgun_Auth *auth, char *data, size_t size)
{
   Shotgun_Event_Presence *pres;

   pres = xml_presence_read(auth, data, size);
   EINA_SAFETY_ON_NULL_GOTO(pres, error);

   switch (pres->status)
     {
      case SHOTGUN_USER_STATUS_NORMAL:
        INF("Presence 'normal' from %s: %s", pres->jid, pres->description ? pres->description : "");
        break;
      case SHOTGUN_USER_STATUS_AWAY:
        INF("Presence 'away' from %s: %s", pres->jid, pres->description ? pres->description : "");
        break;
      case SHOTGUN_USER_STATUS_CHAT:
        INF("Presence 'chat' from %s: %s", pres->jid, pres->description ? pres->description : "");
        break;
      case SHOTGUN_USER_STATUS_DND:
        INF("Presence 'dnd' from %s: %s", pres->jid, pres->description ? pres->description : "");
        break;
      case SHOTGUN_USER_STATUS_XA:
        INF("Presence 'xa' from %s: %s", pres->jid, pres->description ? pres->description : "");
        break;
      default:
        INF("Presence 'unavailable' from %s: %s", pres->jid, pres->description ? pres->description : "");
     }
   ecore_event_add(SHOTGUN_EVENT_PRESENCE, pres, (Ecore_End_Cb)shotgun_presence_free, NULL);
   return;
error:
   ERR("wtf");
}

void
shotgun_event_presence_free(Shotgun_Event_Presence *pres)
{
   if (!pres) return;
   shotgun_presence_free(NULL, pres);
}

Eina_Bool
shotgun_presence_set(Shotgun_Auth *auth, Shotgun_User_Status st, const char *desc)
{
   size_t len;
   char *xml;

   xml = xml_presence_write(auth, st, desc, &len);
   shotgun_write(auth->svr, xml, len);
   free(xml);
   return EINA_TRUE;
}
