#include "ui.h"

Eina_Bool
event_iq_cb(Contact_List *cl, int type __UNUSED__, Shotgun_Event_Iq *ev)
{
   DBG("EVENT_IQ %d: %p", ev->type, ev->ev);
   switch(ev->type)
     {
      case SHOTGUN_IQ_EVENT_TYPE_ROSTER:
        {
           Shotgun_User *user;
           EINA_LIST_FREE(ev->ev, user)
             do_something_with_user(cl, user);
           break;
        }
      case SHOTGUN_IQ_EVENT_TYPE_INFO:
        {
           Contact *c;
           Shotgun_User_Info *info = ev->ev;

           c = eina_hash_find(cl->users, info->jid);
           if (!c)
             {
                ERR("WTF!");
                break;
             }
           shotgun_user_info_free(c->info);
           c->info = info;
           ev->ev = NULL;
           if (c->list_item && info->photo.data) elm_genlist_item_update(c->list_item);
           break;
        }
      default:
        ERR("WTF!");
     }
   return EINA_TRUE;
}

Eina_Bool
event_presence_cb(Contact_List *cl, int type __UNUSED__, Shotgun_Event_Presence *ev)
{
   Contact *c;
   Shotgun_Event_Presence *pres;
   char *jid, *p;
   Eina_List *l;

   p = strchr(ev->jid, '/');
   if (p) jid = strndupa(ev->jid, p - ev->jid);
   else jid = (char*)ev->jid;
   c = eina_hash_find(cl->users, jid);
   if (!c) return EINA_TRUE;

   if (!ev->status)
     {
        contact_list_user_del(c, ev);
        return EINA_TRUE;
     }
   if ((!c->cur) || (ev->jid != c->cur->jid))
     {
        EINA_LIST_FOREACH(c->plist, l, pres)
          {
             if (ev->jid != pres->jid) continue;

             free(pres->description);
             pres->description = ev->description;
             ev->description = NULL;
             pres->priority = ev->priority;
             pres->status = ev->status;
             break;
          }
        if ((!pres) || (pres->jid != ev->jid))
          {
             pres = calloc(1, sizeof(Shotgun_Event_Presence));
             pres->jid = ev->jid;
             ev->jid = NULL;
             pres->description = ev->description;
             ev->description = NULL;
             pres->priority = ev->priority;
             pres->status = ev->status;
          }
        if (c->cur)
          {
             if (ev->priority < c->cur->priority)
               {
                  c->plist = eina_list_append(c->plist, pres);
                  return EINA_TRUE;
               }
             c->plist = eina_list_append(c->plist, c->cur);
          }
        c->cur = pres;
     }

   c->status = c->cur->status;
   c->description = c->cur->description;
   if (c->status_line)
     elm_entry_entry_set(c->status_line, c->description);
   if (c->base->subscription > SHOTGUN_USER_SUBSCRIPTION_NONE)
     {
        if (!c->list_item)
          {
             contact_list_user_add(cl, c);
             if (ev->vcard) shotgun_iq_vcard_get(ev->account, c->base->jid);
          }
        else
          elm_genlist_item_update(c->list_item);
     }
   return EINA_TRUE;
}

Eina_Bool
event_message_cb(void *data, int type __UNUSED__, void *event)
{
   Shotgun_Event_Message *msg = event;
   Contact_List *cl = data;
   Contact *c;
   char *jid, *p;
   const char *from;

   jid = strdupa(msg->jid);
   p = strchr(jid, '/');
   *p = 0;
   c = eina_hash_find(cl->users, jid);
   if (!c) return EINA_TRUE;

   if (!c->chat_window)
     chat_window_new(c);

   from = c->base->name ? : c->base->jid;
   if (msg->msg)
     chat_message_insert(c, from, msg->msg);
   if (msg->status)
     chat_message_status(c, msg);

   return EINA_TRUE;
}
