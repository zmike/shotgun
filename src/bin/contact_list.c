#include "ui.h"

static void
_contact_list_free_cb(Contact_List *cl, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *ev __UNUSED__)
{
   Contact *c;

   ecore_event_handler_del(cl->event_handlers.iq);
   ecore_event_handler_del(cl->event_handlers.presence);
   ecore_event_handler_del(cl->event_handlers.message);

   if (cl->users) eina_hash_free(cl->users);
   if (cl->images) eina_hash_free(cl->images);
   if (cl->user_convs) eina_hash_free(cl->user_convs);
   EINA_LIST_FREE(cl->users_list, c)
     contact_free(c);

   shotgun_disconnect(cl->account);

   free(cl);
}

static void
_contact_list_user_del_cb(Contact *c, Evas_Object *obj __UNUSED__, void *ev  __UNUSED__)
{
   if (!c) return;
   shotgun_iq_contact_del(c->list->account, c->base->jid);
   eina_hash_del_by_data(c->list->users, c);
   c->list->users_list = eina_list_remove(c->list->users_list, c);
   contact_free(c);
}

static void
_contact_list_user_del(Contact *c)
{
   Evas_Object *menu, *ic;
   Elm_Menu_Item *mi;
   char buf[128];
   int x, y;

   menu = elm_menu_add(c->list->win);
   snprintf(buf, sizeof(buf), "Really remove %s?", c->base->jid);
   mi = elm_menu_item_add(menu, NULL, NULL, buf, NULL, NULL);
   elm_menu_item_disabled_set(mi, 1);
   elm_menu_item_separator_add(menu, NULL);
   mi = elm_menu_item_add(menu, NULL, "shotgun/dialog_ok", "Yes", (Evas_Smart_Cb)_contact_list_user_del_cb, c);
   ic = elm_menu_item_object_content_get(mi);
   evas_object_color_set(ic, 0, 255, 0, 255);
   mi = elm_menu_item_add(menu, NULL, "close", "No", (Evas_Smart_Cb)_contact_list_user_del_cb, NULL);
   ic = elm_menu_item_object_content_get(mi);
   evas_object_color_set(ic, 255, 0, 0, 255);
   evas_pointer_canvas_xy_get(evas_object_evas_get(c->list->win), &x, &y);
   elm_menu_move(menu, x, y);
   evas_object_show(menu);
}

static void
_contact_list_close(Contact_List *cl, Evas_Object *obj __UNUSED__, void *ev  __UNUSED__)
{
   evas_object_del(cl->win);
}

static void
_contact_list_click_cb(Contact_List *cl __UNUSED__, Evas_Object *obj __UNUSED__, void *ev)
{
   Contact *c;

   c = elm_object_item_data_get(ev);
   if (c->chat_window)
     {
        elm_win_activate(c->chat_window);
        return;
     }
   chat_window_new(c);
}

static void
_contact_list_remove_cb(Elm_Object_Item *it, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   _contact_list_user_del(elm_object_item_data_get(it));
}

static void
_contact_list_subscribe_cb(Elm_Object_Item *it, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Contact *c;
   Eina_Bool s;

   c = elm_object_item_data_get(it);
   s = (c->base->subscription == SHOTGUN_USER_SUBSCRIPTION_FROM) ? EINA_TRUE : EINA_FALSE;
   shotgun_presence_subscription_set(c->list->account, c->base->jid, s);
}

static void
_contact_list_add_pager_cb_next(Contact_List *cl, Evas_Object *obj __UNUSED__, void *ev __UNUSED__)
{
   const char *text;
   Evas_Object *o;

   cl->pager_state++;
   if (cl->pager_state)
     {
        elm_pager_content_promote(cl->pager, elm_pager_content_bottom_get(cl->pager));
        elm_object_focus_set(cl->pager_entries->next->data, 1);
        elm_entry_cursor_end_set(cl->pager_entries->next->data);
        return;
     }
   o = cl->pager_entries->data;
   text = elm_entry_entry_get(o);
   if ((!text) || (!strchr(text, '@'))) /* FIXME */
     {
        ERR("Invalid contact jid");
        cl->pager_state--;
        return;
     }
   o = cl->pager_entries->next->data;
   shotgun_iq_contact_add(cl->account, text, elm_entry_entry_get(o), NULL); /* add contact */
   shotgun_presence_subscription_set(cl->account, text, EINA_TRUE); /* subscribe */
   evas_object_del(cl->pager);
   cl->pager_entries = eina_list_free(cl->pager_entries);
   cl->pager = NULL;
}

static void
_contact_list_add_pager_cb_prev(Contact_List *cl, Evas_Object *obj __UNUSED__, void *ev __UNUSED__)
{
   cl->pager_state--;
   if (cl->pager_state)
     {
        evas_object_del(cl->pager);
        cl->pager_entries = eina_list_free(cl->pager_entries);
        cl->pager = NULL;
        cl->pager_state = 0;
        return;
     }

   elm_pager_content_promote(cl->pager, elm_pager_content_bottom_get(cl->pager));
   elm_object_focus_set(cl->pager_entries->data, 1);
   elm_entry_cursor_end_set(cl->pager_entries->data);
}

static void
_contact_list_add_cb(Contact_List *cl, Evas_Object *obj __UNUSED__, Elm_Toolbar_Item *ev)
{
   Evas_Object *p, *b, *o, *i;

   elm_toolbar_item_selected_set(ev, EINA_FALSE);
   if (cl->pager) return;
   cl->pager = p = elm_pager_add(cl->win);
   ALIGN(p, EVAS_HINT_FILL, 0);
   elm_box_pack_after(cl->box, p, cl->list);
   elm_object_style_set(p, "slide");

   {
      b = elm_box_add(cl->win);
      ALIGN(b, EVAS_HINT_FILL, EVAS_HINT_FILL);
      elm_box_horizontal_set(b, 1);

      i = elm_icon_add(cl->win);
      elm_icon_order_lookup_set(i, ELM_ICON_LOOKUP_THEME);
      elm_icon_standard_set(i, "arrow_left");
      evas_object_show(i);
      o = elm_button_add(cl->win);
      elm_button_icon_set(o, i);
      elm_box_pack_end(b, o);
      evas_object_smart_callback_add(o, "clicked", (Evas_Smart_Cb)_contact_list_add_pager_cb_prev, cl);
      evas_object_show(o);

      o = elm_label_add(cl->win);
      ALIGN(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
      elm_object_text_set(o, "Contact's Name (Alias):");
      elm_box_pack_end(b, o);
      evas_object_show(o);

      i = elm_icon_add(cl->win);
      elm_icon_order_lookup_set(i, ELM_ICON_LOOKUP_THEME);
      elm_icon_standard_set(i, "shotgun/dialog_ok");
      evas_object_color_set(i, 0, 255, 0, 255);
      evas_object_show(i);
      o = elm_button_add(cl->win);
      elm_button_icon_set(o, i);
      elm_box_pack_end(b, o);
      evas_object_smart_callback_add(o, "clicked", (Evas_Smart_Cb)_contact_list_add_pager_cb_next, cl);
      evas_object_show(o);

      evas_object_show(b);

      o = elm_box_add(cl->win);
      ALIGN(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
      elm_box_pack_end(o, b);
      b = o;

      o = elm_entry_add(cl->win);
      WEIGHT(o, EVAS_HINT_EXPAND, 0);
      ALIGN(o, EVAS_HINT_FILL, 0.5);
      cl->pager_entries = eina_list_append(cl->pager_entries, o);
      elm_entry_entry_append(o, "Example Name");
      elm_entry_line_wrap_set(o, ELM_WRAP_MIXED);
      elm_entry_single_line_set(o, 1);
      elm_entry_editable_set(o, 1);
      elm_entry_scrollable_set(o, 1);
      elm_entry_scrollbar_policy_set(o, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
      elm_box_pack_end(b, o);
      evas_object_smart_callback_add(o, "activated", (Evas_Smart_Cb)_contact_list_add_pager_cb_next, cl);
      evas_object_show(o);
      elm_entry_cursor_begin_set(o);
      elm_entry_select_all(o);
      elm_object_focus_set(o, 1);

      evas_object_show(b);
      elm_pager_content_push(p, b);
   }

   {
      b = elm_box_add(cl->win);
      ALIGN(b, EVAS_HINT_FILL, EVAS_HINT_FILL);
      elm_box_horizontal_set(b, 1);

      i = elm_icon_add(cl->win);
      elm_icon_order_lookup_set(i, ELM_ICON_LOOKUP_THEME);
      elm_icon_standard_set(i, "close");
      evas_object_color_set(i, 255, 0, 0, 255);
      evas_object_show(i);
      o = elm_button_add(cl->win);
      elm_button_icon_set(o, i);
      elm_box_pack_end(b, o);
      evas_object_smart_callback_add(o, "clicked", (Evas_Smart_Cb)_contact_list_add_pager_cb_prev, cl);
      evas_object_show(o);

      o = elm_label_add(cl->win);
      ALIGN(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
      elm_object_text_set(o, "Contact's Address:");
      elm_box_pack_end(b, o);
      evas_object_show(o);

      i = elm_icon_add(cl->win);
      elm_icon_order_lookup_set(i, ELM_ICON_LOOKUP_THEME);
      elm_icon_standard_set(i, "arrow_right");
      evas_object_show(i);
      o = elm_button_add(cl->win);
      elm_button_icon_set(o, i);
      elm_box_pack_end(b, o);
      evas_object_smart_callback_add(o, "clicked", (Evas_Smart_Cb)_contact_list_add_pager_cb_next, cl);
      evas_object_show(o);

      evas_object_show(b);

      o = elm_box_add(cl->win);
      ALIGN(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
      elm_box_pack_end(o, b);
      b = o;

      o = elm_entry_add(cl->win);
      WEIGHT(o, EVAS_HINT_EXPAND, 0);
      ALIGN(o, EVAS_HINT_FILL, 0.5);
      cl->pager_entries = eina_list_prepend(cl->pager_entries, o);
      elm_entry_entry_append(o, "contact@example.com");
      elm_entry_line_wrap_set(o, ELM_WRAP_MIXED);
      elm_entry_single_line_set(o, 1);
      elm_entry_editable_set(o, 1);
      elm_entry_scrollable_set(o, 1);
      elm_entry_scrollbar_policy_set(o, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
      elm_box_pack_end(b, o);
      evas_object_smart_callback_add(o, "activated", (Evas_Smart_Cb)_contact_list_add_pager_cb_next, cl);
      evas_object_show(o);
      elm_entry_cursor_begin_set(o);
      elm_entry_select_all(o);
      elm_object_focus_set(o, 1);

      evas_object_show(b);
      elm_pager_content_push(p, b);
   }

   evas_object_show(p);
}

static void
_contact_list_del_cb(Contact_List *cl, Evas_Object *obj __UNUSED__, Elm_Toolbar_Item *ev)
{
   Contact *c;

   c = elm_object_item_data_get(cl->list_selected_item_get[cl->mode](cl->list));
   _contact_list_user_del(c);
   elm_toolbar_item_selected_set(ev, EINA_FALSE);
}

static void
_contact_list_rightclick_cb(Contact_List *cl, Evas *e __UNUSED__, Evas_Object *obj, Evas_Event_Mouse_Down *ev)
{
   Evas_Object *menu;
   Elm_Menu_Item *mi;
   void *it;
   Contact *c;
   const char *name;
   char buf[128];
   if (ev->button != 3) return;

/* FIXME: HACKS!! */
   if (cl->mode)
     it = elm_gengrid_selected_item_get(obj);
   else
     it = cl->list_at_xy_item_get[cl->mode](obj, ev->output.x, ev->output.y, NULL);
   if (!it) return;
   menu = elm_menu_add(elm_object_top_widget_get(obj));
   if (cl->mode)
     name = elm_object_item_text_part_get(it, NULL);
   else
     name = elm_object_item_text_part_get(it, "elm.text");
   c = elm_object_item_data_get(it);
   switch (c->base->subscription)
     {
      case SHOTGUN_USER_SUBSCRIPTION_TO:
      case SHOTGUN_USER_SUBSCRIPTION_BOTH:
        snprintf(buf, sizeof(buf), "Unsubscribe from %s", name);
        elm_menu_item_add(menu, NULL, "shotgun/arrow_pending_right", buf, (Evas_Smart_Cb)_contact_list_subscribe_cb, it);
        break;
      case SHOTGUN_USER_SUBSCRIPTION_FROM:
        if (c->base->subscription_pending)
          {
             mi = elm_menu_item_add(menu, NULL, "shotgun/arrow_pending_left", "Subscription request sent", NULL, NULL);
             elm_menu_item_disabled_set(mi, EINA_TRUE);
          }
        else
          {
             snprintf(buf, sizeof(buf), "Subscribe to %s", name);
             elm_menu_item_add(menu, NULL, "shotgun/arrow_pending_left", buf, (Evas_Smart_Cb)_contact_list_subscribe_cb, it);
          }
        break;
      default:
        break;
     }
   snprintf(buf, sizeof(buf), "Remove %s", name);
   elm_menu_item_separator_add(menu, NULL);
   elm_menu_item_add(menu, NULL, "menu/delete", buf, (Evas_Smart_Cb)_contact_list_remove_cb, it);
   elm_menu_move(menu, ev->output.x, ev->output.y);
   evas_object_show(menu);
}

static char *
_it_label_get_grid(Contact *c, Evas_Object *obj __UNUSED__, const char *part __UNUSED__)
{
   return strdup(contact_name_get(c));
}

static char *
_it_label_get_list(Contact *c, Evas_Object *obj __UNUSED__, const char *part)
{
   int ret;
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
        ret = asprintf(&buf, "%s: %s", status, c->description);
        if (ret > 0) return buf;
     }

   return NULL;
}

static Evas_Object *
_it_icon_get(Contact *c, Evas_Object *obj, const char *part)
{
   Evas_Object *ic;
   const char *str = NULL;
   int alpha = 255;

   if (!strcmp(part, "elm.swallow.end"))
     {
        if ((!c->info) || (!c->info->photo.data)) return NULL;
        ic = elm_icon_add(obj);
        elm_icon_memfile_set(ic, c->info->photo.data, c->info->photo.size, NULL, NULL);
        evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
        evas_object_show(ic);
        return ic;
     }
   ic = elm_icon_add(obj);
   elm_icon_order_lookup_set(ic, ELM_ICON_LOOKUP_THEME);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   if (c->cur)
     {
        switch (c->cur->type)
          {
           case SHOTGUN_PRESENCE_TYPE_SUBSCRIBE:
             str = "shotgun/arrows_pending_right";
             break;
           case SHOTGUN_PRESENCE_TYPE_UNSUBSCRIBE:
             str = "shotgun/arrows_rejected";
           default:
             break;
          }
        if (c->cur->idle)
          {
             unsigned int x;
             for (x = 10; (x <= c->cur->idle) && (alpha > 64); x += 10)
               alpha -= 12;
          }
     }
   else if (c->base->subscription_pending)
     str = "shotgun/arrows_pending_left";
   else if (!c->base->subscription)
     str = "shotgun/x";
   if (!str)
     {
        switch (c->base->subscription)
          {
           case SHOTGUN_USER_SUBSCRIPTION_NONE:
           case SHOTGUN_USER_SUBSCRIPTION_REMOVE:
             str = "close";
             break;
           case SHOTGUN_USER_SUBSCRIPTION_TO:
             str = "shotgun/arrows_pending_left";
             break;
           case SHOTGUN_USER_SUBSCRIPTION_FROM:
             str = "shotgun/arrows_pending_right";
             break;
           case SHOTGUN_USER_SUBSCRIPTION_BOTH:
             str = "shotgun/arrows_both";
           default:
             break;
          }
     }
   elm_icon_standard_set(ic, str);
   switch (c->status)
     {
        case SHOTGUN_USER_STATUS_NORMAL:
          evas_object_color_set(ic, 0, 200, 0, alpha);
          break;
        case SHOTGUN_USER_STATUS_AWAY:
          evas_object_color_set(ic, 255, 204, 51, alpha);
          break;
        case SHOTGUN_USER_STATUS_CHAT:
          evas_object_color_set(ic, 0, 255, 0, alpha);
          break;
        case SHOTGUN_USER_STATUS_DND:
          evas_object_color_set(ic, 0, 0, 255, alpha);
          break;
        case SHOTGUN_USER_STATUS_XA:
          evas_object_color_set(ic, 255, 0, 0, alpha);
        default:
          break;
     }

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
   evas_object_event_callback_add(list, EVAS_CALLBACK_MOUSE_DOWN,
                                  (Evas_Object_Event_Cb)_contact_list_rightclick_cb, cl);
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
   evas_object_event_callback_add(grid, EVAS_CALLBACK_MOUSE_DOWN,
                                  (Evas_Object_Event_Cb)_contact_list_rightclick_cb, cl);
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
_contact_list_show_toggle(Contact_List *cl, Evas_Object *obj __UNUSED__, Elm_Toolbar_Item *ev __UNUSED__)
{
   Eina_List *l;
   Contact *c;

   cl->view++;
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
#ifdef HAVE_DBUS
   ui_dbus_signal_status_self(cl);
#endif

   cl->status_timer = NULL;
   evas_object_smart_callback_del(cl->status_entry, "changed", (Evas_Smart_Cb)_contact_list_status_changed);
   elm_entry_select_none(cl->status_entry);
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
   elm_object_focus_set(cl->status_entry, EINA_TRUE);
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
   shotgun_presence_desc_set(cl->account, s);
   free(s);
   shotgun_presence_send(cl->account);
   if (cl->status_timer)
     {
        ecore_timer_del(cl->status_timer);
        cl->status_timer = NULL;
     }
}

static Eina_Bool
_contact_list_item_tooltip_update_cb(Contact *c)
{
   c->tooltip_changed = EINA_TRUE;
   return ECORE_CALLBACK_CANCEL;
}

static Evas_Object *
_contact_list_item_tooltip_cb(Contact *c, Evas_Object *obj __UNUSED__, Evas_Object *tt, void *it __UNUSED__)
{
   Evas_Object *label;
   const char *text;
   Eina_Strbuf *buf;
   Eina_List *l;
   Shotgun_Event_Presence *p;
   unsigned int timer = 0, t2;

   if (!c->tooltip_changed) goto out;
   if (c->status && c->cur)
     {
        buf = eina_strbuf_new();
        if (c->cur->idle)
          {
             eina_strbuf_append_printf(buf,
               "<b><title>%s</title></b><ps>"
               "<b><subtitle><u>%s (%i)%s</u></subtitle></b><ps>"
               "%s<ps>"
               "<i>Idle: %u minutes</i>",
               c->base->jid,
               c->cur->jid + strlen(c->base->jid) + 1, c->priority, c->description ? ":" : "",
               c->description ?: "",
               (c->cur->idle + (unsigned int)(ecore_time_unix_get() - c->cur->timestamp)) / 60 + (c->cur->idle % 60 > 30));
             timer = c->cur->idle % 60;
          }
        else
          eina_strbuf_append_printf(buf,
            "<b><title>%s</title></b><ps>"
            "<b><subtitle><u>%s (%i)%s</u></subtitle></b><ps>"
            "%s%s",
            c->base->jid,
            c->cur->jid + strlen(c->base->jid) + 1, c->priority, c->description ? ":" : "",
            c->description ?: "", c->description ? "<ps>" : "");
        EINA_LIST_FOREACH(c->plist, l, p)
          {
             if (p->idle)
               {
                  eina_strbuf_append_printf(buf,
                    "<ps><b>%s (%i)%s</b><ps>"
                    "%s<ps>"
                    "<i>Idle: %u minutes</i>",
                    p->jid + strlen(c->base->jid) + 1, p->priority, p->description ? ":" : "",
                    p->description ?: "",
                    (p->idle + (unsigned int)(ecore_time_unix_get() - p->timestamp)) / 60 + (p->idle % 60 > 30));
                  t2 = p->idle % 60;
                  if (t2 > timer) timer = t2;
               }
             else
               eina_strbuf_append_printf(buf,
                 "<ps><b>%s (%i)%s</b><ps>"
                 "%s%s",
                 p->jid + strlen(c->base->jid) + 1, p->priority, p->description ? ":" : "",
                 p->description ?: "", p->description ? "<ps>" : "");
          }
        text = eina_stringshare_add(eina_strbuf_string_get(buf));
        eina_strbuf_free(buf);
     }
   else
     text = eina_stringshare_printf("<b><title>%s</title></b><ps>", c->base->jid);

   if (timer)
     {
        if (c->tooltip_timer) ecore_timer_interval_set(c->tooltip_timer, timer);
        else c->tooltip_timer = ecore_timer_add((double)timer, (Ecore_Task_Cb)_contact_list_item_tooltip_update_cb, c);
     }
   else if (c->tooltip_timer)
     {
        ecore_timer_del(c->tooltip_timer);
        c->tooltip_timer = NULL;
     }
   eina_stringshare_del(c->tooltip_label);
   c->tooltip_label = text;
out:
   label = elm_label_add(tt);
   elm_label_line_wrap_set(label, ELM_WRAP_NONE);
   elm_object_text_set(label, c->tooltip_label);
   WEIGHT(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   ALIGN(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
   return label;
}

static void
_contact_list_saveinfo_cb(Contact_List *cl, Evas_Object *obj __UNUSED__, void *ev __UNUSED__)
{
   ui_eet_auth_set(cl->account, EINA_TRUE, EINA_FALSE);
}

static void
_contact_list_window_key(Contact_List *cl, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, Evas_Event_Key_Down *ev)
{
   //DBG("%s", ev->keyname);
   if (!strcmp(ev->keyname, "Escape"))
     {
        if (!cl->pager) return;
        _contact_list_add_pager_cb_prev(cl, NULL, NULL);
     }
}

void
contact_list_user_add(Contact_List *cl, Contact *c)
{
   static Elm_Genlist_Item_Class glit = {
        .item_style = "double_label",
        .func = {
             .label_get = (Elm_Genlist_Item_Label_Get_Cb)_it_label_get_list,
             .icon_get = (Elm_Genlist_Item_Icon_Get_Cb)_it_icon_get,
             .state_get = (Elm_Genlist_Item_State_Get_Cb)_it_state_get,
             .del = (Elm_Genlist_Item_Del_Cb)_it_del
        }
   };
   static Elm_Gengrid_Item_Class ggit = {
        .item_style = "default",
        .func = {
             .label_get = (Elm_Gengrid_Item_Label_Get_Cb)_it_label_get_grid,
             .icon_get = (Elm_Gengrid_Item_Icon_Get_Cb)_it_icon_get,
             .state_get = (Elm_Gengrid_Item_State_Get_Cb)_it_state_get,
             .del = (Elm_Gengrid_Item_Del_Cb)_it_del
        }
   };
   if ((!c) || c->list_item) return;
   if ((!cl->view) && (!c->status) && (c->base->subscription == SHOTGUN_USER_SUBSCRIPTION_BOTH)) return;
   c->tooltip_changed = EINA_TRUE;
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
        c->plist = eina_list_remove(c->plist, c->cur);
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
             if (ev->jid != pres->jid) continue;

             if (ev->jid == c->force_resource) c->force_resource = NULL;
             shotgun_event_presence_free(pres);
             c->plist = eina_list_remove_list(c->plist, l);
             break;
          }
     }
   contact_jids_menu_del(c, ev->jid);
   if (!c->cur)
     {
        c->status = 0;
        c->description = NULL;
        c->priority = 0;
        return;
     }
#if 0
   EINA_LIST_FOREACH_SAFE(c->plist, l, ll, pres)
     {
        if (pres->jid) continue;
        c->plist = eina_list_remove_list(c->plist, l);
        shotgun_event_presence_free(pres);
     }
#endif
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
   evas_object_smart_callback_add(win, "delete,request", (Evas_Smart_Cb)_contact_list_free_cb, cl);
   evas_object_event_callback_add(win, EVAS_CALLBACK_KEY_DOWN, (Evas_Object_Event_Cb)_contact_list_window_key, cl);
   1 | evas_object_key_grab(win, "Escape", 0, 0, 1); /* worst warn_unused ever. */

   obj = elm_bg_add(win);
   WEIGHT(obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, obj);
   evas_object_show(obj);

   cl->box = box = elm_box_add(win);
   WEIGHT(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, box);
   evas_object_show(box);

   tb = elm_toolbar_add(win);
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

   cl->list_at_xy_item_get[0] = (Contact_List_At_XY_Item_Get)elm_genlist_at_xy_item_get;
   cl->list_at_xy_item_get[1] = NULL;
   //cl->list_at_xy_item_get[1] = (Ecore_Data_Cb)elm_gengrid_at_xy_item_get;
   cl->list_selected_item_get[0] = (Ecore_Data_Cb)elm_genlist_selected_item_get;
   cl->list_selected_item_get[1] = (Ecore_Data_Cb)elm_gengrid_selected_item_get;
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

   tb = elm_toolbar_add(win);
   elm_toolbar_mode_shrink_set(tb, ELM_TOOLBAR_SHRINK_SCROLL);
   WEIGHT(tb, EVAS_HINT_EXPAND, 0.0);
   ALIGN(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_toolbar_align_set(tb, 0);
   it = elm_toolbar_item_append(tb, "shotgun/useradd", "Add Contact", (Evas_Smart_Cb)_contact_list_add_cb, cl);
   it = elm_toolbar_item_append(tb, "shotgun/userdel", "Remove Contact", (Evas_Smart_Cb)_contact_list_del_cb, cl);
   it = elm_toolbar_item_append(tb, "shotgun/useroffline", "Show Offline", (Evas_Smart_Cb)_contact_list_show_toggle, cl);
   elm_box_pack_end(box, tb);
   elm_object_scale_set(tb, 0.75);
   evas_object_show(tb);

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
   elm_entry_scrollbar_policy_set(entry, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
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
