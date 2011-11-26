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

void
settings_new(Contact_List *cl)
{
   Evas_Object *scr, *ic, *back, *box, *ck, *fr, *frbox;

   cl->settings_box = box = elm_box_add(cl->win);
   EXPAND(box);
   FILL(box);
   elm_win_resize_object_add(cl->win, box);
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
   elm_flip_content_back_set(cl->flip, box);

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

#ifdef HAVE_NOTIFY
   SETTINGS_FRAME("DBus");
   SETTINGS_CHECK("Disable notifications", disable_notify, "Disables use of notification popups");
#endif

   SETTINGS_FRAME("Messages");
   SETTINGS_CHECK("Focus chat window on message", enable_chat_focus, "Focus chat window whenever message is received");
   SETTINGS_CHECK("Promote contact on message", enable_chat_promote, "Move contact to top of list when message is received");
   SETTINGS_CHECK("Always select new chat tabs", enable_chat_newselect, "When a message is received which would open a new tab, make that tab active");
}

void
settings_toggle(Contact_List *cl, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   elm_flip_go(cl->flip, ELM_FLIP_ROTATE_Y_CENTER_AXIS);
}
