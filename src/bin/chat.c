#include "ui.h"

static void
_chat_conv_anchor_click(Evas_Object *entry, Evas_Object *obj __UNUSED__, Elm_Entry_Anchor_Info *ev)
{
   DBG("anchor click: '%s' (%i, %i)", ev->name, ev->x, ev->y);
   switch (ev->button)
     {
      case 1:
        {
           const char *browser;
           char *cmd;
           size_t size;

           browser = getenv("BROWSER");
           if (!browser) return;
           size = sizeof(char) * (strlen(ev->name) + strlen(browser) + 1) + 1;
           cmd = alloca(size);
           snprintf(cmd, size, "%s %s", browser, ev->name);
           ecore_exe_run(cmd, NULL);
           return;
        }
      case 3:
#ifdef HAVE_ECORE_X
        {
           size_t len;
           len = strlen(ev->name);
           if (len == sizeof(int)) len++; /* workaround for stupid elm cnp bug which breaks the universe */
           ecore_x_selection_clipboard_set(elm_win_xwindow_get(elm_object_top_widget_get(entry)), ev->name, len);
        }
#endif
      default:
        break;
     }
}

void
chat_message_insert(Contact *c, const char *from, const char *msg, Eina_Bool me)
{
   size_t len;
   char timebuf[11];
   char *buf, *s;
   Evas_Object *e = c->chat_buffer;
   int r, g, b;

   len = strftime(timebuf, sizeof(timebuf), "[%H:%M:%S]",
            localtime((time_t[]){ time(NULL) }));

   s = elm_entry_utf8_to_markup(msg);
   if (me)
     {
        if (!edje_color_class_get("shotgun_color_me", &r, &g, &b, 0, 0, 0, 0, 0, 0, 0, 0, 0))
          {
             DBG("color_class 'shotgun_color_me' not found in theme, using defaults");
             r = 0, g = 255, b = 1;
          }
     }
   else
     {
        if (!edje_color_class_get("shotgun_color_you", &r, &g, &b, 0, 0, 0, 0, 0, 0, 0, 0, 0))
          {
             DBG("color_class 'shotgun_color_you' not found in theme, using defaults");
             r = 0, g = 1, b = 255;
          }
     }
   len += strlen(from) + strlen(s) + sizeof("<color=#%2x%2x%2x>%s <b>%s:</b></color> %s<ps>") + 5;
   buf = alloca(len);
   snprintf(buf, len, "<color=#%02X%02X%02X>%s <b>%s:</b></color> %s<ps>", r, g, b, timebuf, from, s);
   free(s);

#ifdef HAVE_NOTIFY
   if (!me)
     {
        Ecore_X_Window xwin = elm_win_xwindow_get(c->chat_window);

        if (xwin != ecore_x_window_focus_get())
          ui_dbus_notify(from, msg);
     }
#endif
   elm_entry_entry_append(e, buf);
   elm_entry_cursor_end_set(e);
/* FIXME:
   if (cl->raise windows on event)
     elm_win_activate(c->chat_window);
   if (cl->list_promote on event)
     cl->item_promote[cl->mode](c->list_item);
*/
}

void
chat_message_status(Contact *c __UNUSED__, Shotgun_Event_Message *msg)
{
   switch (msg->status)
     {
      case SHOTGUN_MESSAGE_STATUS_ACTIVE:
      case SHOTGUN_MESSAGE_STATUS_COMPOSING:
      case SHOTGUN_MESSAGE_STATUS_PAUSED:
      case SHOTGUN_MESSAGE_STATUS_INACTIVE:
      case SHOTGUN_MESSAGE_STATUS_GONE:
      default:
        break;
     }
}

static void
_chat_window_send_cb(Contact *c, Evas_Object *obj, void *ev __UNUSED__)
{
   char *s;
   const char *jid;

   s = elm_entry_markup_to_utf8(elm_entry_entry_get(obj));

   if (c->ignore_resource)
     jid = c->base->jid;
   else if (c->force_resource)
     jid = c->force_resource;
   else if (c->cur)
     jid = c->cur->jid;
   else
     jid = c->base->jid;
   shotgun_message_send(c->base->account, jid, s, 0);
   chat_message_insert(c, "me", s, EINA_TRUE);
#ifdef HAVE_DBUS
   ui_dbus_signal_message_self(c->list, jid, s);
#endif
   elm_entry_entry_set(obj, "");
   elm_entry_cursor_end_set(obj);

   free(s);
}

static void
_chat_window_close_cb(void *data, Evas_Object *obj __UNUSED__, const char *ev __UNUSED__)
{
   Contact_List *cl;
   Contact *c;

   INF("Closing window for %s", elm_win_title_get(data));
   cl = evas_object_data_get(data, "list");
   c = evas_object_data_get(data, "contact");
   eina_stringshare_replace(&c->last_conv, elm_entry_entry_get(c->chat_buffer));
   c->chat_window = NULL;
   c->chat_buffer = NULL;
   c->chat_input = NULL;
   c->status_line = NULL;
   eina_hash_del_by_data(cl->user_convs, data);
}

static void
_chat_conv_filter(Contact_List *cl, Evas_Object *obj __UNUSED__, char **str)
{
   char *http;
   const char *start, *end;
   Eina_Strbuf *buf;

   start = *str;
   http = strstr(start, "http");
   if (!http) return;

   buf = eina_strbuf_new();
   while (http)
     {
        int d;
        size_t len;
        char fmt[64];

        d = http - start;
        if (d > 0)
          {
             if (d > 3 && (!memcmp(http - 3, "&lt", 3)))
               eina_strbuf_append_length(buf, start, d - 3);
             else
               eina_strbuf_append_length(buf, start, d);
          }
        start = end = strchr(http, ' ');
        if (!end) /* address goes to end of message */
          {
             len = strlen(http);
             if (((unsigned int)d != eina_strbuf_length_get(buf)) &&
                 (!memcmp(http + len - 3, "&gt", 3)))
               len -= 3;
             else if (!strcmp(http + len - 4, "<ps>"))
               len -= 4;
             if (http[len - 1] == '>')
               {
                  while (--len > 1)
                    if (http[len - 1] == '<') break;
               }
             if ((len <= 1) || (http[len - 1] != '<') ||
                 strcmp(http + len, "</a><ps>") || (d < 5) ||
                 memcmp(http - 5, "href=", 5))
               {
                  snprintf(fmt, sizeof(fmt), "<a href=%%.%is>%%.%is</a><ps>", len, len);
                  eina_strbuf_append_printf(buf, fmt, http, http);
               }
             else
               {
                  eina_strbuf_free(buf);
                  buf = NULL;
               }
             char_image_add(cl, eina_stringshare_add_length(http, len));
             //DBG("ANCHOR: ");
             //DBG(fmt, http);
             break;
          }
        len = end - http;
        if (((unsigned int)d != eina_strbuf_length_get(buf)) &&
            (!memcmp(http + len - 3, "&gt", 3)))
               len -= 3;
        else if (!strcmp(http + len - 4, "<ps>"))
          len -= 4;
        if (http[len - 1] == '>')
          {
             while (--len > 1)
               if (http[len - 1] == '<') break;
          }
        if ((len <= 1) || (http[len - 1] != '<') ||
            strcmp(http + len, "</a><ps>") || (d < 5) ||
            memcmp(http - 5, "href=", 5))
          {
             snprintf(fmt, sizeof(fmt), "<a href=%%.%is>%%.%is</a><ps>", len, len);
             eina_strbuf_append_printf(buf, fmt, http, http);
          }
        else
          {
             eina_strbuf_free(buf);
             return;
          }
        char_image_add(cl, eina_stringshare_add_length(http, len));
             //DBG("ANCHOR: ");
             //DBG(fmt, http);
        http = strstr(start, "http");
     }
   if (!buf) return;
   free(*str);
   *str = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
}

static void
_chat_resource_ignore_toggle(Contact *c, Evas_Object *obj __UNUSED__, Elm_Menu_Item *ev)
{
   Elm_Menu_Item *next = elm_menu_item_next_get(ev);
   c->ignore_resource = !c->ignore_resource;
   if (c->ignore_resource)
     {
        const Eina_List *l;
        Evas_Object *radio;
        elm_menu_item_label_set(ev, "Unignore Resource");
        l = elm_menu_item_subitems_get(next);
        radio = elm_menu_item_object_content_get(l->data);
        c->force_resource = NULL;
        elm_radio_state_value_set(radio, 0);
     }
   else
     elm_menu_item_label_set(ev, "Ignore Resource");
   elm_menu_item_disabled_set(next, c->ignore_resource);
}

static void
_chat_resource_force(Contact *c, Evas_Object *obj __UNUSED__, Elm_Menu_Item *ev)
{
   Eina_List *l;
   Shotgun_Event_Presence *pres;
   const char *res, *s;
   Evas_Object *radio;
   int val;


   radio = (Evas_Object*)elm_menu_item_object_content_get(ev);
   val = elm_radio_state_value_get(radio);
   if (!val)
     {
        /* selected use priority */
        c->force_resource = NULL;
        elm_radio_state_value_set(radio, 0);
        return;
     }
   res = elm_object_text_get(radio);
   if (c->force_resource)
     {
        s = strchr(c->force_resource, '/');
        if (s) s++;
        else s = c->base->jid;
        /* selected previously set resource */
        if (!memcmp(res, s, strlen(s))) return;
     }
   EINA_LIST_FOREACH(c->plist, l, pres)
     {
        s = strchr(pres->jid, '/');
        if (*s) s++;
        else s = c->base->jid;
        if (!memcmp(res, s, strlen(s))) continue;
        c->force_resource = pres->jid;
        break;
     }
   elm_radio_state_value_set(radio, elm_menu_item_index_get(ev));
}

static void
_chat_window_key(Evas_Object *win, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, Evas_Event_Key_Down *ev)
{
   DBG("%s", ev->keyname);
   if (!strcmp(ev->keyname, "Escape"))
     evas_object_smart_callback_call(win, "delete,request", NULL);
}

void
chat_window_new(Contact *c)
{
   Evas_Object *win, *bg, *box, *convo, *entry, *radio, *obj;
   Evas_Object *frame, *status, *menu;
   void *it;
   Eina_List *l;
   Shotgun_Event_Presence *pres;

   if (c->chat_window) return;

   win = elm_win_add(NULL, "chat-window", ELM_WIN_BASIC);
   elm_object_focus_allow_set(win, 0);
   elm_win_title_set(win, contact_name_get(c));
   evas_object_smart_callback_add(win, "delete,request", (Evas_Smart_Cb)_chat_window_close_cb, win);
   evas_object_event_callback_add(win, EVAS_CALLBACK_KEY_DOWN, (Evas_Object_Event_Cb)_chat_window_key, win);
   1 | evas_object_key_grab(win, "Escape", 0, 0, 1); /* worst warn_unused ever. */
   evas_object_resize(win, 450, 320);
   evas_object_show(win);

   bg = elm_bg_add(win);
   elm_object_focus_allow_set(bg, 0);
   WEIGHT(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   box = elm_box_add(win);
   elm_object_focus_allow_set(box, 0);
   WEIGHT(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, box);
   evas_object_show(box);

   obj = elm_toolbar_add(win);
   elm_toolbar_mode_shrink_set(obj, ELM_TOOLBAR_SHRINK_MENU);
   ALIGN(obj, EVAS_HINT_FILL, 0.5);
   elm_toolbar_align_set(obj, 0);
   it = elm_toolbar_item_append(obj, NULL, "Options", NULL, NULL);
   elm_toolbar_item_menu_set(it, 1);
   elm_toolbar_menu_parent_set(obj, win);
   elm_box_pack_end(box, obj);
   evas_object_show(obj);
   c->chat_jid_menu = menu = elm_toolbar_item_menu_get(it);
   elm_menu_item_add(menu, NULL, NULL, "Ignore Resource", (Evas_Smart_Cb)_chat_resource_ignore_toggle, c);
   it = elm_menu_item_add(menu, NULL, "menu/arrow_right", "Send to", NULL, NULL);

   radio = elm_radio_add(win);
   elm_radio_state_value_set(radio, 0);
   elm_object_text_set(radio, "Use Priority");
   evas_object_show(radio);
   elm_menu_item_add_object(menu, it, radio, (Evas_Smart_Cb)_chat_resource_force, c);

   EINA_LIST_FOREACH(c->plist, l, pres)
     {
        const char *s;
        char *buf;
        size_t len;
        int i = 1;

        s = strchr(pres->jid, '/');
        s = s ? s + 1 : pres->jid;
        len = strlen(s);
        buf = alloca(len + 20);
        snprintf(buf, len, "%s (%i)", s ?: c->base->jid, pres->priority);
        obj = elm_radio_add(win);
        elm_radio_group_add(obj, radio);
        elm_radio_state_value_set(obj, i++);
        elm_object_text_set(obj, buf);
        evas_object_show(obj);
        elm_menu_item_add_object(menu, it, obj, (Evas_Smart_Cb)_chat_resource_force, c);
     }
   elm_radio_value_set(radio, 0);

   frame = elm_frame_add(win);
   elm_object_focus_allow_set(frame, 0);
   elm_object_text_set(frame, contact_name_get(c));
   WEIGHT(frame, EVAS_HINT_EXPAND, 0);
   ALIGN(frame, EVAS_HINT_FILL, 0);
   elm_box_pack_end(box, frame);
   evas_object_show(frame);

   status = elm_entry_add(win);
   elm_entry_cnp_textonly_set(status, 1);
   elm_entry_single_line_set(status, 1);
   elm_entry_editable_set(status, 0);
   elm_object_focus_allow_set(status, 0);
   elm_entry_line_wrap_set(status, ELM_WRAP_MIXED);
   WEIGHT(status, EVAS_HINT_EXPAND, 0);
   elm_entry_text_filter_append(status, (Elm_Entry_Filter_Cb)_chat_conv_filter, c->list);
   evas_object_smart_callback_add(status, "anchor,in", (Evas_Smart_Cb)chat_conv_image_show, status);
   evas_object_smart_callback_add(status, "anchor,out", (Evas_Smart_Cb)chat_conv_image_hide, status);
   evas_object_smart_callback_add(status, "anchor,clicked", (Evas_Smart_Cb)_chat_conv_anchor_click, status);

   elm_frame_content_set(frame, status);
   evas_object_show(status);

   convo = elm_entry_add(win);
   elm_entry_cnp_textonly_set(convo, 1);
   elm_entry_scrollbar_policy_set(convo, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
   elm_entry_editable_set(convo, 0);
   elm_entry_single_line_set(convo, 0);
   elm_entry_scrollable_set(convo, 1);
   elm_object_focus_allow_set(convo, 0);
   elm_entry_line_wrap_set(convo, ELM_WRAP_MIXED);
   elm_entry_text_filter_append(convo, (Elm_Entry_Filter_Cb)_chat_conv_filter, c->list);
   evas_object_smart_callback_add(convo, "anchor,in", (Evas_Smart_Cb)chat_conv_image_show, convo);
   evas_object_smart_callback_add(convo, "anchor,out", (Evas_Smart_Cb)chat_conv_image_hide, convo);
   evas_object_smart_callback_add(convo, "anchor,clicked", (Evas_Smart_Cb)_chat_conv_anchor_click, convo);
   WEIGHT(convo, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   ALIGN(convo, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, convo);
   evas_object_show(convo);

   entry = elm_entry_add(win);
   elm_entry_single_line_set(entry, 1);
   elm_entry_scrollable_set(entry, 1);
   elm_entry_line_wrap_set(entry, ELM_WRAP_MIXED);
   WEIGHT(entry, EVAS_HINT_EXPAND, 0);
   ALIGN(entry, EVAS_HINT_FILL, 0);
   elm_box_pack_end(box, entry);
   evas_object_show(entry);
   elm_object_focus_set(entry, EINA_TRUE);

   evas_object_smart_callback_add(entry, "activated", (Evas_Smart_Cb)_chat_window_send_cb, c);

   eina_hash_add(c->list->user_convs, c->base->jid, win);
   evas_object_data_set(win, "contact", c);
   evas_object_data_set(win, "list", c->list);

   c->chat_window = win;
   c->chat_buffer = convo;
   c->chat_input = entry;
   c->status_line = status;
   if (c->description)
     elm_entry_entry_append(status, c->description);
   if (c->last_conv)
     elm_entry_entry_set(convo, c->last_conv);
   elm_entry_cursor_end_set(convo);
   elm_win_activate(win);
}
