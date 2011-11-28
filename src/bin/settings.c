#include "ui.h"

#define SETTINGS_FRAME(LABEL) do { \
   fr = elm_frame_add(cl->win); \
   EXPAND(fr); \
   FILL(fr); \
   elm_object_text_set(fr, LABEL); \
   elm_box_pack_end(box, fr); \
   evas_object_show(fr); \
\
   frbox = elm_box_add(cl->win); \
   EXPAND(frbox); \
   FILL(frbox); \
   elm_object_content_set(fr, frbox); \
   evas_object_show(frbox); \
} while (0)

#define SETTINGS_CHECK(LABEL, POINTER, TOOLTIP) do { \
   ck = elm_check_add(cl->win); \
   EXPAND(ck); \
   FILL(ck); \
   elm_object_text_set(ck, LABEL); \
   elm_check_state_pointer_set(ck, &cl->settings.POINTER); \
   elm_object_tooltip_text_set(ck, TOOLTIP); \
   elm_tooltip_size_restrict_disable(ck, EINA_TRUE); \
   elm_box_pack_end(frbox, ck); \
   evas_object_show(ck); \
} while (0)

#define SETTINGS_SLIDER(LABEL, TOOLTIP, UNITS, MAX, CB) do { \
   sl = elm_slider_add(cl->win); \
   EXPAND(sl); \
   FILL(sl); \
   elm_slider_unit_format_set(sl, UNITS); \
   elm_slider_min_max_set(sl, 0, MAX); \
   elm_object_text_set(sl, LABEL); \
   elm_object_tooltip_text_set(sl, TOOLTIP); \
   elm_tooltip_size_restrict_disable(sl, EINA_TRUE); \
   evas_object_smart_callback_add(sl, "delay,changed", (Evas_Smart_Cb)_settings_ ##CB## _change, cl); \
   elm_box_pack_end(frbox, sl); \
   evas_object_show(sl); \
} while (0)

static void
_settings_image_size_change(Contact_List *cl, Evas_Object *obj, void *event_info __UNUSED__)
{
   cl->settings.allowed_image_size = elm_slider_value_get(obj);
}

static void
_settings_image_age_change(Contact_List *cl, Evas_Object *obj, void *event_info __UNUSED__)
{
   cl->settings.allowed_image_age = elm_slider_value_get(obj);
}

static void
_settings_logging_change(Contact_List *cl, Evas_Object *obj, void *event_info __UNUSED__)
{
   Eina_List *l, *ll;
   const char *dir;
   Chat_Window *cw;
   Contact *c;

   if (!elm_check_state_get(obj))
     {
        /* close existing logs */
        EINA_LIST_FOREACH(cl->users_list, l, c)
          logging_contact_file_close(c);
        return;
     }

   dir = logging_dir_get();
   if (!dir[0]) logging_dir_create(cl);
   EINA_LIST_FOREACH(cl->chat_wins, l, cw)
     {
        EINA_LIST_FOREACH(cw->contacts, ll, c)
          /* open logs for all open chats */
          logging_contact_file_refresh(c);
     }
}

void
settings_new(Contact_List *cl)
{
   Evas_Object *scr, *ic, *back, *box, *ck, *fr, *frbox, *sl;
   int init;

   cl->settings_box = box = elm_box_add(cl->win);
   EXPAND(box);
   FILL(box);
   elm_flip_content_back_set(cl->flip, box);
   evas_object_show(box);

   ic = elm_icon_add(cl->win);
   elm_icon_standard_set(ic, "back");
   evas_object_show(ic);
   back = elm_button_add(cl->win);
   elm_object_content_set(back, ic);
   elm_object_tooltip_text_set(back, "Return to contact list");
   elm_tooltip_size_restrict_disable(back, EINA_TRUE);
   WEIGHT(back, 0, 0);
   ALIGN(back, 0, 0);
   elm_box_pack_end(box, back);
   evas_object_smart_callback_add(back, "clicked", (Evas_Smart_Cb)settings_toggle, cl);
   evas_object_show(back);

   scr = elm_scroller_add(cl->win);
   EXPAND(scr);
   FILL(scr);
   elm_box_pack_end(box, scr);

   box = elm_box_add(cl->win);
   EXPAND(box);
   FILL(box);
   evas_object_show(box);

   elm_object_content_set(scr, box);
   elm_scroller_bounce_set(scr, EINA_FALSE, EINA_FALSE);
   elm_scroller_policy_set(scr, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
   evas_object_show(scr);

   SETTINGS_FRAME("Account");
   SETTINGS_CHECK("Save account info", enable_account_info, "Remember account name and password");
   SETTINGS_CHECK("Remember last account", enable_last_account, "Automatically sign in with current account on next run");

   SETTINGS_FRAME("Appearance");
   SETTINGS_CHECK("Enable single window mode", enable_illume, "Use a single window for the application - REQUIRES RESTART (embedded friendly)");

#ifdef HAVE_NOTIFY
   SETTINGS_FRAME("DBus");
   SETTINGS_CHECK("Disable notifications", disable_notify, "Disables use of notification popups");
#endif

   init = ecore_con_url_init();
   if (init)
     SETTINGS_FRAME("Images");
   else
     SETTINGS_FRAME("Images (DISABLED: REQUIRES CURL SUPPORT IN ECORE)");
   SETTINGS_CHECK("Disable automatic image fetching", disable_image_fetch, "Disables background fetching of images");
   if (init)
     {
        SETTINGS_SLIDER("Max image age", "Number of days to save linked images on disk before deleting them",
                        "%1.0f days", 60, image_age);
        SETTINGS_SLIDER("Max image memory", "Total size of images to keep in memory",
                        "%1.0f MB", 512, image_size);
     }
   else
     {
        elm_check_state_set(ck, EINA_TRUE);
        elm_object_disabled_set(ck, EINA_TRUE);
     }

   SETTINGS_FRAME("Messages");
   SETTINGS_CHECK("Focus chat window on message", enable_chat_focus, "Focus chat window whenever message is received");
   SETTINGS_CHECK("Promote contact on message", enable_chat_promote, "Move contact to top of list when message is received");
   SETTINGS_CHECK("Always select new chat tabs", enable_chat_newselect, "When a message is received which would open a new tab, make that tab active");
   SETTINGS_CHECK("Log messages to disk", enable_logging, "All messages sent or received will appear in ~/.config/shotgun/logs");
   evas_object_smart_callback_add(ck, "changed", (Evas_Smart_Cb)_settings_logging_change, cl);
}

void
settings_toggle(Contact_List *cl, Evas_Object *obj __UNUSED__, void *event_info)
{
   if ((!cl->image_cleaner) && cl->settings.allowed_image_age)
     ui_eet_idler_start(cl);
   chat_image_cleanup(cl);
   IF_ILLUME
     {
        if (elm_flip_front_get(cl->flip))
          elm_object_text_set(cl->illume_frame, "Settings");
        else
          elm_object_text_set(cl->illume_frame, "Contacts");
     }
   if (event_info) elm_toolbar_item_selected_set(event_info, EINA_FALSE);
   elm_flip_go(cl->flip, ELM_FLIP_ROTATE_Y_CENTER_AXIS);
}
