#include "ui.h"

const char *
contact_name_get(Contact *c)
{
   if (!c) return NULL;
   if (c->base->name)
     return c->base->name;
   if (c->info && c->info->full_name)
     return c->info->full_name;
   return c->base->jid;
}

void
contact_free(Contact *c)
{
   Shotgun_Event_Presence *pres;

   if (!c) return;
   if (c->cur && c->plist)
     c->plist = eina_list_remove(c->plist, c->cur);
   EINA_LIST_FREE(c->plist, pres)
     {
        if (pres && (pres == c->cur))
          {
             CRI("c->cur is in c->plist!!!!");
             c->cur = NULL;
          }
        shotgun_event_presence_free(pres);
     }
   shotgun_event_presence_free(c->cur);
   if (c->list_item)
     c->list->list_item_del[c->list->mode](c->list_item);
   if (c->tooltip_timer) ecore_timer_del(c->tooltip_timer);
   shotgun_user_free(c->base);
   shotgun_user_info_free(c->info);
   eina_stringshare_del(c->last_conv);
   eina_stringshare_del(c->tooltip_label);
   free(c);
}

void
contact_jids_menu_del(Contact *c, const char *jid)
{
   const Eina_List *l, *ll;
   Elm_Menu_Item *it;
   Evas_Object *radio;
   const char *s, *rs;

   if (!c->chat_jid_menu) return;
   it = elm_menu_last_item_get(c->chat_jid_menu);
   if (!it) return;
   l = elm_menu_item_subitems_get(it);
   if (!l) return;
   s = strchr(jid, '/');
   if (s) s++;
   else s = jid;
   EINA_LIST_REVERSE_FOREACH(l, ll, it)
     {
        radio = elm_menu_item_object_content_get(it);
        rs = elm_object_text_get(radio);
        if (strcmp(rs, s)) continue;
        if (elm_radio_state_value_get(radio) == elm_radio_value_get(radio))
          {
             elm_radio_value_set(radio, 0);
             c->force_resource = NULL;
          }
        elm_menu_item_del(it);
        break;
     }
}

Contact *
do_something_with_user(Contact_List *cl, Shotgun_User *user)
{
   Contact *c;
   char *jid, *p;

   p = strchr(user->jid, '/');
   if (p) jid = strndupa(user->jid, p - user->jid);
   else jid = (char*)user->jid;
   c = eina_hash_find(cl->users, jid);

   if (c)
     {
        shotgun_user_free(c->base);
        c->base = user;
        return c;
     }

   c = calloc(1, sizeof(Contact));
   c->base = user;
   c->list = cl;
   eina_hash_add(cl->users, jid, c);
   cl->users_list = eina_list_append(cl->users_list, c);
   return c;
}

void
contact_subscription_set(Contact *c, Shotgun_Presence_Type type, Shotgun_User_Subscription sub)
{
   Shotgun_Event_Presence *pres;
   Eina_List *l;

   if (!c) return;
   if (c->cur)
     {
        c->cur->type = type;
        EINA_LIST_FOREACH(c->plist, l, pres)
          pres->type = type;
     }
   c->base->subscription = sub;
}
