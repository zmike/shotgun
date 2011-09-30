#include "ui.h"

/* start at 255, end at shotgun/color/message */
static Eina_Bool
_contact_chat_window_animator_in(Contact *c, double pos)
{
   Evas_Object *obj;
   int x, y, w, h;
   double frame;
   int r, g, b;

   if ((!c->chat_window) || contact_chat_window_current(c))
     {
        contact_chat_window_animator_del(c);
        return EINA_FALSE;
     }

   r = c->list->alert_colors[0];
   g = c->list->alert_colors[1];
   b = c->list->alert_colors[2];
   obj = elm_toolbar_item_object_get(c->chat_tb_item);
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   evas_object_move(c->animated, x - 1, y - 1);
   frame = ecore_animator_pos_map(pos, ECORE_POS_MAP_SINUSOIDAL, 0, 0);
   evas_object_color_set(c->animated, 255 - ((255 - r) * frame), 255 - ((255 - g) * frame), 255 - ((255 - b) * frame), 255);
   return EINA_TRUE;
}

/* start at shotgun/color/message, end at 255 */
static Eina_Bool
_contact_chat_window_animator_out(Contact *c, double pos)
{
   Evas_Object *obj;
   int x, y, w, h;
   int r, g, b;
   double frame;

   if ((!c->chat_window) || contact_chat_window_current(c))
     {
        contact_chat_window_animator_del(c);
        return EINA_FALSE;
     }

   r = c->list->alert_colors[0];
   g = c->list->alert_colors[1];
   b = c->list->alert_colors[2];
   obj = elm_toolbar_item_object_get(c->chat_tb_item);
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   evas_object_move(c->animated, x - 1, y - 1);
   frame = ecore_animator_pos_map(pos, ECORE_POS_MAP_SINUSOIDAL, 0, 0);
   evas_object_color_set(c->animated, r + ((255 - r) * frame), g + ((255 - g) * frame), b + ((255 - b) * frame), 255);
   return EINA_TRUE;
}

static Eina_Bool
_contact_chat_window_animator_switch(Contact *c)
{
   int r, g, b, a;
   Ecore_Timeline_Cb cb;

   if ((!c->chat_window) || contact_chat_window_current(c))
     {
        contact_chat_window_animator_del(c);
        return EINA_FALSE;
     }
   evas_object_color_get(c->animated, &r, &g, &b, &a);
   cb = (Ecore_Timeline_Cb)((r + g + b + a >= 1000) ? _contact_chat_window_animator_in : _contact_chat_window_animator_out);
   //INF("Next %s: %d, %d, %d", ((void*)cb == (void*)_contact_chat_window_animator_in) ? "_contact_chat_window_animator_in" : "_contact_chat_window_animator_out", r, g, b);
   c->animator = ecore_animator_timeline_add(2, cb, c);

   return EINA_TRUE;
}

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
   EINA_LIST_FREE(c->plist, pres)
     shotgun_event_presence_free(pres);
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

Eina_Bool
contact_chat_window_current(Contact *c)
{
   if (!c->chat_window) return EINA_FALSE;

   return c->chat_box == elm_pager_content_top_get(c->chat_window->pager);
}

void
contact_chat_window_animator_add(Contact *c)
{
   Evas_Object *obj, *clip;
   int x, y, w, h;
   if (c->animator) return;

   obj = elm_toolbar_item_object_get(c->chat_tb_item);
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   c->animated = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_resize(c->animated, w + 2, h + 2);
   evas_object_color_set(c->animated, 0, 0, 0, 0);
   /* here we inject the newly created rect as an intermediate clip for the tb item */
   clip = evas_object_clip_get(obj);
   evas_object_clip_set(obj, c->animated);
   evas_object_clip_set(c->animated, clip);
   evas_object_show(c->animated);
   c->animator = ecore_animator_timeline_add(2, (Ecore_Timeline_Cb)_contact_chat_window_animator_in, c);
   ecore_timer_add(2, (Ecore_Task_Cb)_contact_chat_window_animator_switch, c);
}

void
contact_chat_window_animator_del(Contact *c)
{
   Evas_Object *obj, *clip;

   if (c->animator) ecore_animator_del(c->animator);
   c->animator = NULL;
   if (c->animated)
     {
        if (c->chat_tb_item)
          {
             obj = elm_toolbar_item_object_get(c->chat_tb_item);
             clip = evas_object_clip_get(c->animated);
             evas_object_clip_set(obj, clip);
          }
        evas_object_del(c->animated);
     }
   c->animated = NULL;
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
