#include "ui.h"

void
chat_message_insert(Contact *c, const char *from, const char *msg)
{
   size_t len;
   char timebuf[11];
   char *buf, *s;
   Evas_Object *e = c->chat_buffer;

   len = strftime(timebuf, sizeof(timebuf), "[%H:%M:%S]",
            localtime((time_t[]){ time(NULL) }));

   s = elm_entry_utf8_to_markup(msg);
   len += strlen(from) + strlen(s) + 20;
   buf = alloca(len);
   snprintf(buf, len, "%s <b>%s:</b> %s<br>", timebuf, from, s);
   free(s);

   elm_entry_entry_append(e, buf);
   elm_entry_cursor_end_set(e);
}

void
chat_message_status(Contact *c, Shotgun_Event_Message *msg)
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
_chat_window_send_cb(void *data, Evas_Object *obj, void *ev __UNUSED__)
{
   Contact *c = data;
   char *s;

   s = elm_entry_markup_to_utf8(elm_entry_entry_get(obj));

   shotgun_message_send(c->base->account, c->cur->jid, s, 0);
   chat_message_insert(c, "me", s);
   elm_entry_entry_set(obj, "");

   free(s);
}

static void
chat_window_close_cb(void *data, Evas_Object *obj __UNUSED__, const char *ev __UNUSED__)
{
   Contact_List *cl;
   Contact *c;
   Image *i;

   INF("Closing window for %s", elm_win_title_get(data));
   cl = evas_object_data_get(data, "list");
   c = evas_object_data_get(data, "contact");
   c->chat_window = NULL;
   c->chat_buffer = NULL;
   c->chat_input = NULL;
   c->status_line = NULL;
   EINA_LIST_FREE(c->imgs, i)
     eina_hash_del_by_key(i->wins, data);
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
             if ((unsigned int)d != eina_strbuf_length_get(buf))
               if (!memcmp(http + len - 4, "&gt", 3)) len -= 3;
             snprintf(fmt, sizeof(fmt), "<a href=%%.%is>%%.%is</a><br>", len - 4, len - 4);
             eina_strbuf_append_printf(buf, fmt, http, http);
             char_image_add(cl, strndupa(http, len - 4));
             //DBG("ANCHOR: ");
             //DBG(fmt, http);
             break;
          }
        len = end - http;
        if ((unsigned int)d != eina_strbuf_length_get(buf))
          if (!memcmp(http + len - 4, "&gt", 3)) len -= 3;
        snprintf(fmt, sizeof(fmt), "<a href=%%.%is>%%.%is</a>", len, len);
        eina_strbuf_append_printf(buf, fmt, http, http);
        char_image_add(cl, strndupa(http, len - 4));
             //DBG("ANCHOR: ");
             //DBG(fmt, http);
        http = strstr(start, "http");
     }
   free(*str);
   *str = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
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
   Evas_Object *parent_win, *win, *bg, *box, *convo, *entry;
   Evas_Object *topbox, *frame, *status, *close, *icon;

   win = eina_hash_find(c->list->user_convs, c->base->jid);
   if (win)
     {
        c->chat_window = win;
        c->chat_buffer = evas_object_data_get(win, "conv");
        c->chat_input = evas_object_data_get(win, "input");
        c->status_line = evas_object_data_get(win, "status");
        return;
     }

   parent_win = elm_object_top_widget_get(
      c->list->list_item_parent_get[c->list->mode](c->list_item));

   win = elm_win_add(NULL, "chat-window", ELM_WIN_BASIC);
   elm_object_focus_allow_set(win, 0);
   elm_win_title_set(win, c->base->jid);
   evas_object_smart_callback_add(win, "delete,request", (Evas_Smart_Cb)chat_window_close_cb, win);
   evas_object_event_callback_add(win, EVAS_CALLBACK_KEY_DOWN, (Evas_Object_Event_Cb)_chat_window_key, win);
   1 | evas_object_key_grab(win, "Escape", 0, 0, 1); /* worst warn_unused ever. */
   evas_object_resize(win, 300, 320);
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

   frame = elm_frame_add(win);
   elm_object_focus_allow_set(frame, 0);
   elm_object_text_set(frame, c->base->name ? : c->base->jid);
   WEIGHT(frame, EVAS_HINT_EXPAND, 0);
   ALIGN(frame, EVAS_HINT_FILL, 0);
   elm_box_pack_end(box, frame);
   evas_object_show(frame);

   topbox = elm_box_add(win);
   elm_box_horizontal_set(topbox, 1);
   elm_object_focus_allow_set(topbox, 0);
   WEIGHT(topbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_frame_content_set(frame, topbox);
   evas_object_show(topbox);

   status = elm_entry_add(win);
   elm_entry_single_line_set(status, 1);
   elm_entry_scrollable_set(status, 1);
   elm_entry_editable_set(status, 0);
   elm_object_focus_allow_set(status, 0);
   WEIGHT(status, EVAS_HINT_EXPAND, 0);
   ALIGN(status, EVAS_HINT_FILL, 0);
   elm_box_pack_end(topbox, status);
   evas_object_show(status);

   close = elm_button_add(win);
   elm_object_focus_allow_set(close, 0);
   elm_box_pack_end(topbox, close);
   evas_object_show(close);
   icon = elm_icon_add(win);
   elm_icon_standard_set(icon, "close");
   elm_button_icon_set(close, icon);

   convo = elm_entry_add(win);
   elm_entry_editable_set(convo, 0);
   elm_entry_single_line_set(convo, 0);
   elm_entry_scrollable_set(convo, 1);
   elm_object_focus_allow_set(convo, 0);
   elm_entry_line_wrap_set(convo, ELM_WRAP_MIXED);
   elm_entry_text_filter_append(convo, (Elm_Entry_Filter_Cb)_chat_conv_filter, c->list);
   evas_object_smart_callback_add(convo, "anchor,in", (Evas_Smart_Cb)chat_conv_image_show, convo);
   evas_object_smart_callback_add(convo, "anchor,out", (Evas_Smart_Cb)chat_conv_image_hide, convo);
   WEIGHT(convo, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   ALIGN(convo, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, convo);
   evas_object_show(convo);

   entry = elm_entry_add(win);
   elm_entry_single_line_set(entry, 1);
   elm_entry_scrollable_set(entry, 1);
   WEIGHT(entry, EVAS_HINT_EXPAND, 0);
   ALIGN(entry, EVAS_HINT_FILL, 0);
   elm_box_pack_end(box, entry);
   evas_object_show(entry);
   elm_object_focus(entry);

   evas_object_smart_callback_add(entry, "activated", _chat_window_send_cb, c);
   evas_object_smart_callback_add(close, "clicked", (Evas_Smart_Cb)chat_window_close_cb, win);

   eina_hash_add(c->list->user_convs, c->base->jid, win);
   evas_object_data_set(win, "contact", c);
   evas_object_data_set(win, "conv", convo);
   evas_object_data_set(win, "input", entry);
   evas_object_data_set(win, "status", status);
   evas_object_data_set(win, "list", c->list);

   c->chat_window = win;
   c->chat_buffer = convo;
   c->chat_input = entry;
   c->status_line = status;
}
