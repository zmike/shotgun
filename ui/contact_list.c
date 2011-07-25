#include "ui.h"

static char *
_it_label_get(Contact *c, Evas_Object *obj __UNUSED__, const char *part)
{
   if (!strcmp(part, "elm.text"))
     {
        const char *ret = NULL;
        if (c->base->name)
          ret = c->base->name;
        else
          ret = c->base->jid;
        return strdup(ret);
     }
   else if (!strcmp(part, "elm.text.sub"))
     {
        char *buf;
        const char *status;
        size_t st, len = 0;

        switch(c->status)
          {
           case SHOTGUN_USER_STATUS_NORMAL:
              status = "Normal";
              st = sizeof("Normal");
              break;
           case SHOTGUN_USER_STATUS_AWAY:
              status = "Away";
              st = sizeof("Away");
              break;
           case SHOTGUN_USER_STATUS_CHAT:
              status = "Chat";
              st = sizeof("Chat");
              break;
           case SHOTGUN_USER_STATUS_DND:
              status = "Busy";
              st = sizeof("Busy");
              break;
           case SHOTGUN_USER_STATUS_XA:
              status = "Very Away";
              st = sizeof("Very Away");
              break;
           case SHOTGUN_USER_STATUS_NONE:
              status = "Offline?";
              st = sizeof("Offline?");
              break;
           default:
              status = "What the fuck aren't we handling?";
              st = sizeof("What the fuck aren't we handling?");
          }

        if (!c->description)
          return strdup(status);
        len = st + strlen(c->description) + 2; /* st includes trailing null */
        buf = malloc(len);
        snprintf(buf, len, "%s: %s", status, c->description);
        return buf;
     }

   return NULL;
}

static Evas_Object *
_it_icon_get(Contact *c, Evas_Object *obj, const char *part)
{
   Evas_Object *ic;

   if ((!c->info) || (!c->info->photo.data) || strcmp(part, "elm.swallow.end")) return NULL;
   ic = elm_icon_add(obj);
   elm_icon_memfile_set(ic, c->info->photo.data, c->info->photo.size, NULL, NULL);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);

   return ic;
}

static Eina_Bool
_it_state_get(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *part __UNUSED__)
{
   return EINA_FALSE;
}

static void
_it_del(Contact *c, Evas_Object *obj __UNUSED__)
{
   c->list_item = NULL;
}

static void
_contact_list_free_cb(Contact_List *cl, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *ev __UNUSED__)
{
   ecore_event_handler_del(cl->event_handlers.iq);
   ecore_event_handler_del(cl->event_handlers.presence);
   ecore_event_handler_del(cl->event_handlers.message);

   eina_hash_free(cl->users);
   eina_hash_free(cl->images);
   eina_hash_free(cl->user_convs);

   free(cl);
}

static void
_contact_list_close(Contact_List *cl, Evas_Object *obj __UNUSED__, void *ev  __UNUSED__)
{
   evas_object_del(cl->win);
}

static void
_contact_list_click_cb(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *ev)
{
   Elm_Genlist_Item *it = ev;
   Contact *c;

   c = elm_genlist_item_data_get(it);
   if (c->chat_window)
     {
        elm_win_raise(c->chat_window);
        return;
     }

   chat_window_new(c);
}

void
contact_list_user_add(Contact_List *cl, Contact *c)
{
   static Elm_Genlist_Item_Class it = {
        .item_style = "double_label",
        .func = {
             .label_get = (GenlistItemLabelGetFunc)_it_label_get,
             .icon_get = (GenlistItemIconGetFunc)_it_icon_get,
             .state_get = (GenlistItemStateGetFunc)_it_state_get,
             .del = (GenlistItemDelFunc)_it_del
        }
   };
   c->list_item = elm_genlist_item_append(cl->list, &it, c, NULL,
                                                   ELM_GENLIST_ITEM_NONE, NULL, NULL);
}

void
contact_list_user_del(Contact *c, Shotgun_Event_Presence *ev)
{
   Eina_List *l;
   Shotgun_Event_Presence *pres;
   if (!c->plist)
     {
        shotgun_event_presence_free(c->cur);
        if (c->list_item)
          {
             INF("Removing user %s", c->base->jid);
             elm_genlist_item_del(c->list_item);
          }
        c->list_item = NULL;
        c->cur = NULL;
        return;
     }
   if (ev->jid == c->cur->jid)
     {
        shotgun_event_presence_free(c->cur);
        c->cur = NULL;
        EINA_LIST_FOREACH(c->plist, l, pres)
          {
             if (!c->cur)
               {
                  c->cur = pres;
                  continue;
               }
             if (pres->priority < c->cur->priority) continue;
             c->cur = pres;
          }
        c->plist = eina_list_remove(c->plist, c->cur);
     }
   else
     {
        EINA_LIST_FOREACH(c->plist, l, pres)
          {
             if (ev->jid == pres->jid)
               {
                  shotgun_event_presence_free(pres);
                  c->plist = eina_list_remove_list(c->plist, l);
                  break;
               }
          }
     }
   c->status = c->cur->status;
   c->description = c->cur->description;
   elm_genlist_item_update(c->list_item);
}

void
contact_list_new(void)
{
   Evas_Object *win, *bg, *box, *list, *btn;
   Contact_List *cl;

   //_setup_extension();

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

   cl = calloc(1, sizeof(Contact_List));

   win = elm_win_add(NULL, "Shotgun - Contacts", ELM_WIN_BASIC);
   elm_win_title_set(win, "Shotgun - Contacts");
   elm_win_autodel_set(win, 1);

   bg = elm_bg_add(win);
   WEIGHT(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   box = elm_box_add(win);
   WEIGHT(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, box);
   evas_object_show(box);

   list = elm_genlist_add(win);
   elm_genlist_always_select_mode_set(list, EINA_FALSE);
   WEIGHT(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   ALIGN(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, list);
   evas_object_show(list);

   btn = elm_button_add(win);
   elm_object_text_set(btn, "My wm has no close button");
   elm_box_pack_end(box, btn);
   evas_object_show(btn);

   evas_object_smart_callback_add(list, "clicked,double",
                                  _contact_list_click_cb, cl);
   evas_object_smart_callback_add(btn, "clicked", (Evas_Smart_Cb)_contact_list_close, cl);
   evas_object_event_callback_add(win, EVAS_CALLBACK_FREE,
                                  (Evas_Object_Event_Cb)_contact_list_free_cb, cl);

   cl->win = win;
   cl->list = list;

   cl->users = eina_hash_string_superfast_new((Eina_Free_Cb)contact_free);
   cl->user_convs = eina_hash_string_superfast_new((Eina_Free_Cb)evas_object_del);
   cl->images = eina_hash_string_superfast_new((Eina_Free_Cb)chat_image_free);

   cl->event_handlers.iq = ecore_event_handler_add(SHOTGUN_EVENT_IQ,
                                                       (Ecore_Event_Handler_Cb)event_iq_cb, cl);
   cl->event_handlers.presence =
      ecore_event_handler_add(SHOTGUN_EVENT_PRESENCE, (Ecore_Event_Handler_Cb)event_presence_cb,
                              cl);
   cl->event_handlers.message =
      ecore_event_handler_add(SHOTGUN_EVENT_MESSAGE, (Ecore_Event_Handler_Cb)event_message_cb,
                              cl);

   evas_object_data_set(win, "contact-list", cl);

   evas_object_resize(win, 300, 700);
   evas_object_show(win);
}
