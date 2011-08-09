#include "ui.h"

Eina_Bool
event_iq_cb(Contact_List *cl, int type __UNUSED__, Shotgun_Event_Iq *ev)
{
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
           if (c->cur && c->cur->photo)
             {
                INF("Found contact photo sha1: %s", c->cur->photo);
                info->photo.sha1 = eina_stringshare_ref(c->cur->photo);
             }
           c->info = info;
           ev->ev = NULL;
           if (c->list_item && (info->photo.data || info->full_name)) cl->list_item_update[cl->mode](c->list_item);
           ui_eet_userinfo_add(cl->account, info);
           break;
        }
      default:
        ERR("WTF!");
     }
   return EINA_TRUE;
}

static int
_list_sort_cb(Shotgun_Event_Presence *a, Shotgun_Event_Presence *b)
{
   return a->priority - b->priority;
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
             pres->photo = ev->photo;
             ev->photo = NULL;
             pres->vcard = ev->vcard;
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
             pres->photo = ev->photo;
             ev->photo = NULL;
             pres->vcard = ev->vcard;
          }
        if (c->cur)
          {
             if (pres->photo && (!c->cur->photo))
               c->cur->photo = eina_stringshare_ref(pres->photo);
             c->cur->vcard |= pres->vcard;
             if (ev->priority < c->cur->priority)
               {
                  c->plist = eina_list_append(c->plist, pres);
                  if (ev->vcard && ((!c->info) || (c->cur && c->info &&
                      ((c->info->photo.sha1 != c->cur->photo) || (c->cur->photo && (!c->info->photo.data))))))
                    shotgun_iq_vcard_get(ev->account, c->base->jid);
                  return EINA_TRUE;
               }
             c->plist = eina_list_remove(c->plist, pres);
             c->plist = eina_list_append(c->plist, c->cur);
          }
        c->cur = pres;
     }

   c->status = c->cur->status;
   if (c->status_line && (c->description != c->cur->description) &&
       ((!c->description) || (!c->cur->description) || strcmp(c->description, c->cur->description)))
     {
        elm_entry_entry_set(c->status_line, "");
        if (c->cur->description) elm_entry_entry_append(c->status_line, c->cur->description);
     }
   c->description = c->cur->description;
   c->priority = c->cur->priority;
   if (cl->view || (c->base->subscription > SHOTGUN_USER_SUBSCRIPTION_NONE))
     {
        c->tooltip_changed = EINA_TRUE;
        if (!c->list_item)
          {
             contact_list_user_add(cl, c);
             if (ev->vcard)
               {
                  c->info = ui_eet_userinfo_get(cl->account, c->base->jid);
                  if (c->info) cl->list_item_update[cl->mode](c->list_item);
                  if ((!c->info) || (c->cur && c->info &&
                      ((c->info->photo.sha1 != c->cur->photo) || (c->cur->photo && (!c->info->photo.data)))))
                    shotgun_iq_vcard_get(ev->account, c->base->jid);
               }
          }
        else
          {
             if (ev->vcard && ((!c->info) || (c->cur && c->info &&
                 ((c->info->photo.sha1 != c->cur->photo) || (c->cur->photo && (!c->info->photo.data))))))
               shotgun_iq_vcard_get(ev->account, c->base->jid);
             cl->list_item_update[cl->mode](c->list_item);
          }
     }
   if (c->plist) c->plist = eina_list_sort(c->plist, 0, (Eina_Compare_Cb)_list_sort_cb);
   return EINA_TRUE;
}

Eina_Bool
event_message_cb(void *data, int type __UNUSED__, void *event)
{
   Shotgun_Event_Message *msg = event;
   Contact_List *cl = data;
   Contact *c;
   char *jid, *p;

   jid = strdupa(msg->jid);
   p = strchr(jid, '/');
   if (p) *p = 0;
   c = eina_hash_find(cl->users, jid);
   if (!c) return EINA_TRUE;

   if (!c->chat_window)
     chat_window_new(c);

   if (msg->msg)
     chat_message_insert(c, contact_name_get(c), msg->msg, EINA_FALSE);
   if (msg->status)
     chat_message_status(c, msg);

   return EINA_TRUE;
}
