#include "ui.h"

static void
_contact_list_free_cb(Contact_List *cl, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *ev __UNUSED__)
{
   Contact *c;
   ecore_event_handler_del(cl->event_handlers.iq);
   ecore_event_handler_del(cl->event_handlers.presence);
   ecore_event_handler_del(cl->event_handlers.message);

   eina_hash_free(cl->users);
   eina_hash_free(cl->images);
   eina_hash_free(cl->user_convs);
   EINA_LIST_FREE(cl->users_list, c)
     contact_free(c);

   shotgun_disconnect(cl->account);

   free(cl);
}

static void
_contact_list_close(Contact_List *cl, Evas_Object *obj __UNUSED__, void *ev  __UNUSED__)
{
   evas_object_del(cl->win);
}

static void
_contact_list_click_cb(Contact_List *cl, Evas_Object *obj __UNUSED__, void *ev)
{
   Contact *c;

   c = cl->list_item_contact_get[cl->mode](ev);
   if (c->chat_window)
     {
        elm_win_activate(c->chat_window);
        return;
     }
   chat_window_new(c);
}

static char *
_it_label_get_grid(Contact *c, Evas_Object *obj __UNUSED__, const char *part __UNUSED__)
{
   return strdup(contact_name_get(c));
}

static char *
_it_label_get_list(Contact *c, Evas_Object *obj __UNUSED__, const char *part)
{
   if (!strcmp(part, "elm.text"))
     return strdup(contact_name_get(c));
   if (!strcmp(part, "elm.text.sub"))
     {
        char *buf;
        const char *status;

        switch(c->status)
          {
           case SHOTGUN_USER_STATUS_NORMAL:
              status = "Normal";
              break;
           case SHOTGUN_USER_STATUS_AWAY:
              status = "Away";
              break;
           case SHOTGUN_USER_STATUS_CHAT:
              status = "Chat";
              break;
           case SHOTGUN_USER_STATUS_DND:
              status = "Busy";
              break;
           case SHOTGUN_USER_STATUS_XA:
              status = "Very Away";
              break;
           case SHOTGUN_USER_STATUS_NONE:
              status = "Offline?";
              break;
           default:
              status = "What the fuck aren't we handling?";
          }

        if (!c->description)
          return strdup(status);
        asprintf(&buf, "%s: %s", status, c->description);
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
   evas_object_show(ic);

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
_contact_list_list_add(Contact_List *cl)
{
   Evas_Object *list;
   const Eina_List *l;

   cl->list = list = elm_genlist_add(cl->win);
   cl->mode = EINA_FALSE;
   elm_genlist_always_select_mode_set(list, EINA_FALSE);
   elm_genlist_bounce_set(list, EINA_FALSE, EINA_FALSE);
   elm_genlist_scroller_policy_set(list, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
   WEIGHT(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   ALIGN(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_genlist_compress_mode_set(list, EINA_TRUE);
   l = elm_box_children_get(cl->box);
   elm_box_pack_after(cl->box, list, l->data);
   evas_object_show(list);
   evas_object_smart_callback_add(list, "activated",
                                  (Evas_Smart_Cb)_contact_list_click_cb, cl);
}

static void
_contact_list_grid_add(Contact_List *cl)
{
   Evas_Object *grid;
   const Eina_List *l;

   cl->list = grid = elm_gengrid_add(cl->win);
   cl->mode = EINA_TRUE;
   elm_gengrid_always_select_mode_set(grid, EINA_FALSE);
   elm_gengrid_item_size_set(grid, 75, 100);
   elm_gengrid_bounce_set(grid, EINA_FALSE, EINA_FALSE);
   WEIGHT(grid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   ALIGN(grid, EVAS_HINT_FILL, EVAS_HINT_FILL);
   l = elm_box_children_get(cl->box);
   elm_box_pack_after(cl->box, grid, l->data);
   evas_object_show(grid);
   evas_object_smart_callback_add(grid, "activated",
                                  (Evas_Smart_Cb)_contact_list_click_cb, cl);
}

static void
_contact_list_mode_toggle(Contact_List *cl, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Eina_List *l;
   Contact *c;
   evas_object_del(cl->list);
   if (cl->mode)
     _contact_list_list_add(cl);
   else
     _contact_list_grid_add(cl);
   EINA_LIST_FOREACH(cl->users_list, l, c)
     {
        if (cl->view || ((c->base->subscription > SHOTGUN_USER_SUBSCRIPTION_NONE) && c->status))
          contact_list_user_add(cl, c);
     }
}

static void
_contact_list_show_toggle(Contact_List *cl, Evas_Object *obj __UNUSED__, Elm_Menu_Item *ev)
{
   Eina_List *l;
   Contact *c;

   cl->view = !cl->view;
   if (cl->view)
     elm_menu_item_label_set(ev, "Hide Offline Contacts");
   else
     elm_menu_item_label_set(ev, "Show Offline Contacts");
   EINA_LIST_FOREACH(cl->users_list, l, c)
     {
        if (cl->view && (!c->list_item)) contact_list_user_add(cl, c);
        else if ((!cl->view) && (((!c->cur) || (!c->cur->status)) || (!c->base->subscription)))
          contact_list_user_del(c, c->cur ?: NULL);
     }
}

static void
_contact_list_status_changed(Contact_List *cl, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   ecore_timer_delay(cl->status_timer, 3);
}

static Eina_Bool
_contact_list_status_send(Contact_List *cl)
{
   shotgun_presence_send(cl->account);

   cl->status_timer = NULL;
   evas_object_smart_callback_del(cl->status_entry, "changed", (Evas_Smart_Cb)_contact_list_status_changed);
   return EINA_FALSE;
}

static void
_contact_list_status_menu(Contact_List *cl, Evas_Object *obj __UNUSED__, Elm_Menu_Item *it)
{
   Evas_Object *radio;
   Shotgun_User_Status val;

   radio = (Evas_Object*)elm_menu_item_object_content_get(it);
   val = elm_radio_state_value_get(radio);
   if ((Shotgun_User_Status)elm_radio_value_get(radio) == val) return;
   elm_radio_value_set(radio, val);
   shotgun_presence_status_set(cl->account, val);
   elm_object_focus(cl->status_entry);
   elm_entry_select_all(cl->status_entry);
   cl->status_timer = ecore_timer_add(3.0, (Ecore_Task_Cb)_contact_list_status_send, cl);
   evas_object_smart_callback_add(cl->status_entry, "changed", (Evas_Smart_Cb)_contact_list_status_changed, cl);
}

static void
_contact_list_status_priority(Contact_List *cl, Evas_Object *obj, void *ev __UNUSED__)
{
   shotgun_presence_priority_set(cl->account, elm_spinner_value_get(obj));
   if (cl->status_timer) ecore_timer_delay(cl->status_timer, 3);
   else
     {
        cl->status_timer = ecore_timer_add(3.0, (Ecore_Task_Cb)_contact_list_status_send, cl);
        evas_object_smart_callback_add(cl->status_entry, "changed", (Evas_Smart_Cb)_contact_list_status_changed, cl);
     }
}

static void
_contact_list_status_message(Contact_List *cl, Evas_Object *obj, void *ev __UNUSED__)
{
   char *s;

   s = elm_entry_markup_to_utf8(elm_entry_entry_get(obj));
   shotgun_presence_desc_manage(cl->account, s);
   shotgun_presence_send(cl->account);
   if (cl->status_timer)
     {
        ecore_timer_del(cl->status_timer);
        cl->status_timer = NULL;
     }
}

static Evas_Object *
_contact_list_item_tooltip_cb(Contact *c, Evas_Object *obj __UNUSED__, Evas_Object *tt, void *it __UNUSED__)
{
   Evas_Object *label;
   const char *text;
   Eina_Strbuf *buf;
   Eina_List *l;
   Shotgun_Event_Presence *p;

   if (!c->tooltip_changed) goto out;
   buf = eina_strbuf_new();
   eina_strbuf_append_printf(buf, "<b><title>%s</title></b><ps>"
                                  "<b><subtitle><u>%s (%i)%c</u></subtitle></b><ps>"
                                  "%s%s",
                                  c->base->jid,
                                  c->cur->jid + strlen(c->base->jid) + 1, c->cur->priority, c->description ? ':' : 0,
                                  c->description ?: "", c->description ? "<ps>" : "");
   EINA_LIST_FOREACH(c->plist, l, p)
     eina_strbuf_append_printf(buf, "<b>%s (%i)%c</b><ps>"
                                    "%s%s",
                                    p->jid + strlen(c->base->jid) + 1, c->cur->priority, c->description ? ':' : 0,
                                    p->description ?: "", p->description ? "<ps>" : "");
   text = eina_stringshare_add(eina_strbuf_string_get(buf));
   eina_strbuf_free(buf);
   eina_stringshare_del(c->tooltip_label);
   c->tooltip_label = text;
out:
   label = elm_label_add(tt);
   elm_label_line_wrap_set(label, ELM_WRAP_NONE);
   elm_object_text_set(label, text);
   WEIGHT(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   ALIGN(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
   return label;
}

static void
_contact_list_saveinfo_cb(Contact_List *cl, Evas_Object *obj __UNUSED__, void *ev __UNUSED__)
{
   ui_eet_auth_set(cl->account, EINA_TRUE, EINA_FALSE);
}

void
contact_list_user_add(Contact_List *cl, Contact *c)
{
   static Elm_Genlist_Item_Class glit = {
        .item_style = "double_label",
        .func = {
             .label_get = (GenlistItemLabelGetFunc)_it_label_get_list,
             .icon_get = (GenlistItemIconGetFunc)_it_icon_get,
             .state_get = (GenlistItemStateGetFunc)_it_state_get,
             .del = (GenlistItemDelFunc)_it_del
        }
   };
   static Elm_Gengrid_Item_Class ggit = {
        .item_style = "default",
        .func = {
             .label_get = (GridItemLabelGetFunc)_it_label_get_grid,
             .icon_get = (GridItemIconGetFunc)_it_icon_get,
             .state_get = (GridItemStateGetFunc)_it_state_get,
             .del = (GridItemDelFunc)_it_del
        }
   };
   if (cl->mode)
     c->list_item = elm_gengrid_item_append(cl->list, &ggit, c, NULL, NULL);
   else
     c->list_item = elm_genlist_item_append(cl->list, &glit, c, NULL,
                                                     ELM_GENLIST_ITEM_NONE, NULL, NULL);
   cl->list_item_tooltip_add[cl->mode](c->list_item,
     (Elm_Tooltip_Item_Content_Cb)_contact_list_item_tooltip_cb, c, NULL);
   cl->list_item_tooltip_resize[cl->mode](c->list_item, EINA_TRUE);
}

void
contact_list_user_del(Contact *c, Shotgun_Event_Presence *ev)
{
   Eina_List *l, *ll;
   Shotgun_Event_Presence *pres;
   if ((!c->plist) || (!ev))
     {
        shotgun_event_presence_free(c->cur);
        if (c->list_item)
          {
             INF("Removing user %s", c->base->jid);
             c->list->list_item_del[c->list->mode](c->list_item);
          }
        c->list_item = NULL;
        c->cur = NULL;
        c->force_resource = NULL;
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
        if (ev->jid == c->force_resource) c->force_resource = NULL;
        c->plist = eina_list_remove(c->plist, c->cur);
     }
   else
     {
        EINA_LIST_FOREACH_SAFE(c->plist, l, ll, pres)
          {
             if (ev->jid == pres->jid)
               {
                  if (ev->jid == c->force_resource) c->force_resource = NULL;
                  shotgun_event_presence_free(pres);
                  c->plist = eina_list_remove_list(c->plist, l);
                  break;
               }
          }
     }
   contact_jids_menu_del(c, ev->jid);
   if (!c->cur) return;
   EINA_LIST_FOREACH_SAFE(c->plist, l, ll, pres)
     {
        if (pres->jid) continue;
        c->plist = eina_list_remove_list(c->plist, l);
        shotgun_event_presence_free(pres);
     }
   c->status = c->cur->status;
   c->description = c->cur->description;
   c->list->list_item_update[c->list->mode](c->list_item);
}

Contact_List *
contact_list_new(Shotgun_Auth *auth)
{
   Evas_Object *win, *obj, *tb, *radio, *box, *menu, *entry;
   Elm_Toolbar_Item *it;
   Contact_List *cl;

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

   cl = calloc(1, sizeof(Contact_List));
   cl->account = auth;

   cl->win = win = elm_win_add(NULL, "Contacts", ELM_WIN_BASIC);
   elm_win_title_set(win, "Contacts");
   elm_win_autodel_set(win, 1);

   obj = elm_bg_add(win);
   WEIGHT(obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, obj);
   evas_object_show(obj);

   cl->box = box = elm_box_add(win);
   WEIGHT(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, box);
   evas_object_show(box);

   tb = elm_toolbar_add(win);
   elm_toolbar_mode_shrink_set(tb, ELM_TOOLBAR_SHRINK_MENU);
   ALIGN(tb, EVAS_HINT_FILL, 0);
   elm_toolbar_align_set(tb, 0);
   it = elm_toolbar_item_append(tb, NULL, "Shotgun", NULL, NULL);
   elm_toolbar_item_menu_set(it, 1);
   elm_toolbar_menu_parent_set(tb, win);
   elm_box_pack_end(box, tb);
   evas_object_show(tb);
   menu = elm_toolbar_item_menu_get(it);
   elm_menu_item_add(menu, NULL, "menu/folder", "Save Account Info", (Evas_Smart_Cb)_contact_list_saveinfo_cb, cl);
   elm_menu_item_add(menu, NULL, "refresh", "Toggle View Mode", (Evas_Smart_Cb)_contact_list_mode_toggle, cl);
   elm_menu_item_add(menu, NULL, "chat", "Show Offline Contacts", (Evas_Smart_Cb)_contact_list_show_toggle, cl);
   elm_menu_item_add(menu, NULL, "close", "Quit", (Evas_Smart_Cb)_contact_list_close, cl);

   it = elm_toolbar_item_append(tb, NULL, "Status", NULL, NULL);
   elm_toolbar_item_menu_set(it, 1);
   elm_toolbar_menu_parent_set(tb, win);
   menu = elm_toolbar_item_menu_get(it);

   radio = elm_radio_add(win);
   elm_radio_state_value_set(radio, SHOTGUN_USER_STATUS_NORMAL);
   elm_object_text_set(radio, "Available");
   evas_object_show(radio);
   elm_menu_item_add_object(menu, NULL, radio, (Evas_Smart_Cb)_contact_list_status_menu, cl);

   obj = elm_radio_add(win);
   elm_radio_state_value_set(obj, SHOTGUN_USER_STATUS_CHAT);
   elm_radio_group_add(obj, radio);
   elm_object_text_set(obj, "Chatty");
   evas_object_show(obj);
   elm_menu_item_add_object(menu, NULL, obj, (Evas_Smart_Cb)_contact_list_status_menu, cl);

   obj = elm_radio_add(win);
   elm_radio_state_value_set(obj, SHOTGUN_USER_STATUS_AWAY);
   elm_radio_group_add(obj, radio);
   elm_object_text_set(obj, "Away");
   evas_object_show(obj);
   elm_menu_item_add_object(menu, NULL, obj, (Evas_Smart_Cb)_contact_list_status_menu, cl);

   obj = elm_radio_add(win);
   elm_radio_state_value_set(obj, SHOTGUN_USER_STATUS_XA);
   elm_radio_group_add(obj, radio);
   elm_object_text_set(obj, "Really Away");
   evas_object_show(obj);
   elm_menu_item_add_object(menu, NULL, obj, (Evas_Smart_Cb)_contact_list_status_menu, cl);

   obj = elm_radio_add(win);
   elm_radio_state_value_set(obj, SHOTGUN_USER_STATUS_DND);
   elm_radio_group_add(obj, radio);
   elm_object_text_set(obj, "DND");
   evas_object_show(obj);
   elm_menu_item_add_object(menu, NULL, obj, (Evas_Smart_Cb)_contact_list_status_menu, cl);
   elm_radio_value_set(radio, SHOTGUN_USER_STATUS_NORMAL);

   cl->list_item_contact_get[0] = (Ecore_Data_Cb)elm_genlist_item_data_get;
   cl->list_item_contact_get[1] = (Ecore_Data_Cb)elm_gengrid_item_data_get;
   cl->list_item_parent_get[0] = (Ecore_Data_Cb)elm_genlist_item_genlist_get;
   cl->list_item_parent_get[1] = (Ecore_Data_Cb)elm_gengrid_item_gengrid_get;
   cl->list_item_del[0] = (Ecore_Cb)elm_genlist_item_del;
   cl->list_item_del[1] = (Ecore_Cb)elm_gengrid_item_del;
   cl->list_item_update[0] = (Ecore_Cb)elm_genlist_item_update;
   cl->list_item_update[1] = (Ecore_Cb)elm_gengrid_item_update;
   cl->list_item_tooltip_add[0] = (Contact_List_Item_Tooltip_Cb)elm_genlist_item_tooltip_content_cb_set;
   cl->list_item_tooltip_add[1] = (Contact_List_Item_Tooltip_Cb)elm_gengrid_item_tooltip_content_cb_set;
   cl->list_item_tooltip_resize[0] = (Contact_List_Item_Tooltip_Resize_Cb)elm_genlist_item_tooltip_size_restrict_disable;
   cl->list_item_tooltip_resize[1] = (Contact_List_Item_Tooltip_Resize_Cb)elm_gengrid_item_tooltip_size_restrict_disable;

   _contact_list_list_add(cl);

   obj = elm_separator_add(win);
   elm_separator_horizontal_set(obj, EINA_TRUE);
   elm_box_pack_end(box, obj);
   evas_object_show(obj);

   box = elm_box_add(win);
   WEIGHT(box, EVAS_HINT_EXPAND, 0.0);
   ALIGN(box, EVAS_HINT_FILL, 1.0);
   elm_box_horizontal_set(box, EINA_TRUE);
   elm_box_pack_end(cl->box, box);
   evas_object_show(box);

   cl->status_entry = entry = elm_entry_add(win);
   elm_entry_line_wrap_set(entry, ELM_WRAP_MIXED);
   elm_entry_single_line_set(entry, 1);
   elm_entry_scrollable_set(entry, 1);
   WEIGHT(entry, EVAS_HINT_EXPAND, 0);
   ALIGN(entry, EVAS_HINT_FILL, 0.5);
   elm_box_pack_end(box, entry);
   evas_object_smart_callback_add(entry, "activated", (Evas_Smart_Cb)_contact_list_status_message, cl);
   elm_entry_entry_set(entry, shotgun_presence_desc_get(cl->account));
   evas_object_show(entry);

   obj = elm_spinner_add(win);
   elm_spinner_wrap_set(obj, EINA_TRUE);
   elm_spinner_step_set(obj, 1);
   elm_spinner_min_max_set(obj, 0, 9999);
   elm_spinner_value_set(obj, shotgun_presence_priority_get(cl->account));
   ALIGN(obj, 1.0, 0.5);
   elm_box_pack_end(box, obj);
   evas_object_smart_callback_add(obj, "delay,changed", (Evas_Smart_Cb)_contact_list_status_priority, cl);
   evas_object_show(obj);

   evas_object_event_callback_add(win, EVAS_CALLBACK_FREE,
                                  (Evas_Object_Event_Cb)_contact_list_free_cb, cl);

   cl->users = eina_hash_string_superfast_new(NULL);
//   cl->users = eina_hash_string_superfast_new((Eina_Free_Cb)contact_free);
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
   return cl;
}
