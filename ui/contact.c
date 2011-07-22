#include <Elementary.h>
#include <sys/stat.h>

#include "ui.h"

typedef struct
{
   Evas_Object *win;
   Evas_Object *list;

   Eina_Hash *users;
   Eina_Hash *user_convs;

   struct {
        Ecore_Event_Handler *iq;
        Ecore_Event_Handler *presence;
        Ecore_Event_Handler *message;
   } event_handlers;
} Contact_List;

typedef struct
{
   Shotgun_User *base;
   Shotgun_User_Info *info;
   Shotgun_Event_Presence *cur;
   Eina_List *plist;
   Shotgun_User_Status status;
   char *description;
   Elm_Genlist_Item *list_item;
   Evas_Object *chat_window;
   Evas_Object *chat_buffer;
   Evas_Object *status_line;
   Contact_List *list;
} Contact;

static void
_contact_free(Contact *c)
{
   Shotgun_Event_Presence *pres;
   shotgun_event_presence_free(c->cur);
   EINA_LIST_FREE(c->plist, pres)
     shotgun_event_presence_free(pres);
   if (c->list_item)
     elm_genlist_item_del(c->list_item);
   if (c->chat_window)
     evas_object_del(c->chat_window);
   shotgun_user_free(c->base);
   shotgun_user_info_free(c->info);
   free(c);
}

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
_do_something_with_user(Contact_List *cl, Shotgun_User *user)
{
   Contact *c;
   char *jid, *p;

   p = strchr(user->jid, '/');
   if (p) jid = strndupa(user->jid, p - user->jid);
   else jid = (char*)user->jid;
   c = eina_hash_find(cl->users, jid);

   if (c)
     {
        shotgun_user_free(user);
        return;
     }

   c = calloc(1, sizeof(Contact));
   c->base = user;
   c->list = cl;
   eina_hash_add(cl->users, jid, c);
}

static void
_chat_message_insert(Contact *c, const char *from, const char *msg)
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

static void
_chat_message_status(Contact *c, Shotgun_Event_Message *msg)
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
   _chat_message_insert(c, "me", s);
   elm_entry_entry_set(obj, "");

   free(s);
}

static void
_chat_window_free_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *ev __UNUSED__)
{
   Contact *c = data;
   c->chat_window = NULL;
   c->chat_buffer = NULL;
   c->status_line = NULL;
}

static void
_chat_window_close_cb(void *data, Evas_Object *obj __UNUSED__, const char *ev __UNUSED__)
{
   Contact_List *cl;

   INF("Closing window for %s", elm_win_title_get(data));
   cl = evas_object_data_get(data, "list");
   eina_hash_del_by_data(cl->user_convs, data);
   evas_object_del(data);
}

static void
_chat_conv_image(void *data __UNUSED__, Evas_Object *obj, Elm_Entry_Anchor_Info *ev)
{
   DBG("anchor: '%s' (%i, %i)", ev->name, ev->x, ev->y);
}

static void
_chat_conv_filter(void *data __UNUSED__, Evas_Object *obj __UNUSED__, char **str)
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
             //if (d > 3 && (!memcmp(http - 3, "&lt"
             eina_strbuf_append_length(buf, start, d);
          }
        end = strchr(http, ' ');
        if (!end) /* address goes to end of message */
          {
             len = strlen(http);
             snprintf(fmt, sizeof(fmt), "<a href=%%.%is>%%.%is</a><br>", len - 4, len - 4);
             eina_strbuf_append_printf(buf, fmt, http, http);
             DBG("ANCHOR: ");
             DBG(fmt, http);
             break;
          }
        len = end - http;
        snprintf(fmt, sizeof(fmt), "<a href=%%.%is>%%.%is</a>", len, len);
        eina_strbuf_append_printf(buf, fmt, http, http);
             DBG("ANCHOR: ");
             DBG(fmt, http);
        start = end;
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

static void
_chat_window_open(Contact *c)
{
   Evas_Object *parent_win, *win, *bg, *box, *convo, *entry;
   Evas_Object *topbox, *frame, *status, *close, *icon;

   win = eina_hash_find(c->list->user_convs, c->base->jid);
   if (win)
     {
        c->chat_window = win;
        c->chat_buffer = evas_object_data_get(win, "conv");
        c->status_line = evas_object_data_get(win, "status");
        return;
     }

   parent_win = elm_object_top_widget_get(
      elm_genlist_item_genlist_get(c->list_item));

   win = elm_win_add(parent_win, "chat-window", ELM_WIN_BASIC);
   elm_object_focus_allow_set(win, 0);
   elm_win_title_set(win, c->base->jid);
   evas_object_smart_callback_add(win, "delete,request", (Evas_Smart_Cb)_chat_window_close_cb, win);
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
   elm_entry_line_wrap_set(convo, ELM_WRAP_WORD);
   //elm_entry_line_wrap_set(convo, ELM_WRAP_MIXED); BROKEN as of 21 JULY
   elm_entry_text_filter_append(convo, _chat_conv_filter, NULL);
   evas_object_smart_callback_add(convo, "anchor,in", (Evas_Smart_Cb)_chat_conv_image, NULL);
   WEIGHT(convo, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   ALIGN(convo, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, convo);
   evas_object_show(convo);

   entry = elm_entry_add(win);
   elm_entry_single_line_set(entry, 1);
   elm_entry_scrollable_set(entry, 1);
   elm_object_focus(entry);
   WEIGHT(entry, EVAS_HINT_EXPAND, 0);
   ALIGN(entry, EVAS_HINT_FILL, 0);
   elm_box_pack_end(box, entry);
   evas_object_show(entry);

   evas_object_smart_callback_add(entry, "activated", _chat_window_send_cb, c);
   evas_object_event_callback_add(win, EVAS_CALLBACK_FREE, _chat_window_free_cb,
                                  c);
   evas_object_smart_callback_add(close, "clicked", (Evas_Smart_Cb)_chat_window_close_cb, win);

   eina_hash_add(c->list->user_convs, c->base->jid, win);
   evas_object_data_set(win, "conv", convo);
   evas_object_data_set(win, "status", status);
   evas_object_data_set(win, "list", c->list);

   c->chat_window = win;
   c->chat_buffer = convo;
   c->status_line = status;
}

static Eina_Bool
_event_iq_cb(Contact_List *cl, int type __UNUSED__, Shotgun_Event_Iq *ev)
{
   DBG("EVENT_IQ %d: %p", ev->type, ev->ev);
   switch(ev->type)
     {
      case SHOTGUN_IQ_EVENT_TYPE_ROSTER:
        {
           Shotgun_User *user;
           EINA_LIST_FREE(ev->ev, user)
             _do_something_with_user(cl, user);
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
        }
      default:
        ERR("WTF!");
     }
   return EINA_TRUE;
}

static Eina_Bool
_event_presence_cb(Contact_List *cl, int type __UNUSED__, Shotgun_Event_Presence *ev)
{
   Contact *c;
   Shotgun_Event_Presence *pres;
   char *jid, *p;
   Eina_List *l;
   static Elm_Genlist_Item_Class it = {
        .item_style = "double_label",
        .func = {
             .label_get = (GenlistItemLabelGetFunc)_it_label_get,
             .icon_get = (GenlistItemIconGetFunc)_it_icon_get,
             .state_get = (GenlistItemStateGetFunc)_it_state_get,
             .del = (GenlistItemDelFunc)_it_del
        }
   };

   p = strchr(ev->jid, '/');
   if (p) jid = strndupa(ev->jid, p - ev->jid);
   else jid = (char*)ev->jid;
   c = eina_hash_find(cl->users, jid);
   if (!c) return EINA_TRUE;

   if (!ev->status)
     {
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
             return EINA_TRUE;
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
             c->list_item = elm_genlist_item_append(cl->list, &it, c, NULL,
                                                   ELM_GENLIST_ITEM_NONE, NULL, NULL);
             if (ev->vcard) shotgun_iq_vcard_get(ev->account, c->base->jid);
          }
        else
          elm_genlist_item_update(c->list_item);
     }
   return EINA_TRUE;
}

static Eina_Bool
_event_message_cb(void *data, int type __UNUSED__, void *event)
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
     _chat_window_open(c);

   from = c->base->name ? : c->base->jid;
   if (msg->msg)
     _chat_message_insert(c, from, msg->msg);
   if (msg->status)
     _chat_message_status(c, msg);

   return EINA_TRUE;
}

static void
_contact_dbl_click_cb(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *ev)
{
   Elm_Genlist_Item *it = ev;
   Contact *c;

   c = elm_genlist_item_data_get(it);
   if (c->chat_window)
     {
        elm_win_raise(c->chat_window);
        return;
     }

   _chat_window_open(c);
}

static void
_close_btn_cb(void *data, Evas_Object *obj __UNUSED__, void *ev __UNUSED__)
{
   evas_object_del((Evas_Object *)data);
}

static void
_contact_list_free_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *ev __UNUSED__)
{
   Contact_List *cl = data;

   ecore_event_handler_del(cl->event_handlers.iq);
   ecore_event_handler_del(cl->event_handlers.presence);
   ecore_event_handler_del(cl->event_handlers.message);

   eina_hash_free(cl->users);
   eina_hash_free(cl->user_convs);

   free(cl);
}

#if 0
static void
_setup_extension(void)
{
   struct stat st;

   if (!stat("./shotgun.edj", &st))
     elm_theme_extension_add(NULL, "./shotgun.edj");
   else if (!stat("ui/shotgun.edj", &st))
     elm_theme_extension_add(NULL, "ui/shotgun.edj");
   else exit(1); /* FIXME: ~/.config/shotgun/etc, /usr/share/shotgun/etc */
}
#endif

void
contact_list_new(void)
{
   Evas_Object *win, *bg, *box, *list, *btn;
   Contact_List *cldata;

   //_setup_extension();

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

   cldata = calloc(1, sizeof(Contact_List));

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
                                  _contact_dbl_click_cb, cldata);
   evas_object_smart_callback_add(btn, "clicked", _close_btn_cb, win);
   evas_object_event_callback_add(win, EVAS_CALLBACK_FREE,
                                  _contact_list_free_cb, cldata);

   cldata->win = win;
   cldata->list = list;

   cldata->users = eina_hash_string_superfast_new((Eina_Free_Cb)_contact_free);
   cldata->user_convs = eina_hash_string_superfast_new(NULL);

   cldata->event_handlers.iq = ecore_event_handler_add(SHOTGUN_EVENT_IQ,
                                                       (Ecore_Event_Handler_Cb)_event_iq_cb, cldata);
   cldata->event_handlers.presence =
      ecore_event_handler_add(SHOTGUN_EVENT_PRESENCE, (Ecore_Event_Handler_Cb)_event_presence_cb,
                              cldata);
   cldata->event_handlers.message =
      ecore_event_handler_add(SHOTGUN_EVENT_MESSAGE, (Ecore_Event_Handler_Cb)_event_message_cb,
                              cldata);

   evas_object_data_set(win, "contact-list", cldata);

   evas_object_resize(win, 300, 700);
   evas_object_show(win);
}
