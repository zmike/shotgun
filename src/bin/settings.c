#include "ui.h"

void
settings_new(Contact_List *cl)
{
   Evas_Object *ic, *back, *box, *ck, *fr, *frbox;

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
   WEIGHT(back, 0, 0);
   ALIGN(back, 0, 0);
   elm_box_pack_end(box, back);
   evas_object_smart_callback_add(back, "clicked", (Evas_Smart_Cb)settings_toggle, cl);
   evas_object_show(back);

   fr = elm_frame_add(cl->win);
   EXPAND(fr);
   FILL(fr);
   elm_object_text_set(fr, "Account");
   elm_box_pack_end(box, fr);
   evas_object_show(fr);

   frbox = elm_box_add(cl->win);
   EXPAND(frbox);
   FILL(frbox);
   elm_object_content_set(fr, frbox);
   evas_object_show(frbox);

   ck = elm_check_add(cl->win);
   EXPAND(ck);
   FILL(ck);
   elm_object_text_set(ck, "Save account info");
   elm_check_state_pointer_set(ck, &cl->settings.enable_account_info);
   elm_box_pack_end(frbox, ck);
   evas_object_show(ck);

   ck = elm_check_add(cl->win);
   EXPAND(ck);
   FILL(ck);
   elm_object_text_set(ck, "Remember last account");
   elm_check_state_pointer_set(ck, &cl->settings.enable_last_account);
   elm_box_pack_end(frbox, ck);
   evas_object_show(ck);

#ifdef HAVE_NOTIFY
   fr = elm_frame_add(cl->win);
   EXPAND(fr);
   FILL(fr);
   elm_object_text_set(fr, "DBus");
   elm_box_pack_end(box, fr);
   evas_object_show(fr);

   frbox = elm_box_add(cl->win);
   EXPAND(frbox);
   FILL(frbox);
   elm_object_content_set(fr, frbox);
   evas_object_show(frbox);

   ck = elm_check_add(cl->win);
   EXPAND(ck);
   FILL(ck);
   elm_object_text_set(ck, "Disable notifications");
   elm_check_state_pointer_set(ck, &cl->settings.disable_notify);
   elm_box_pack_end(frbox, ck);
   evas_object_show(ck);
#endif

   fr = elm_frame_add(cl->win);
   EXPAND(fr);
   FILL(fr);
   elm_object_text_set(fr, "Messages");
   elm_box_pack_end(box, fr);
   evas_object_show(fr);

   frbox = elm_box_add(cl->win);
   EXPAND(frbox);
   FILL(frbox);
   elm_object_content_set(fr, frbox);
   evas_object_show(frbox);

   ck = elm_check_add(cl->win);
   EXPAND(ck);
   FILL(ck);
   elm_object_text_set(ck, "Focus chat window on message");
   elm_check_state_pointer_set(ck, &cl->settings.enable_chat_focus);
   elm_box_pack_end(frbox, ck);
   evas_object_show(ck);

   ck = elm_check_add(cl->win);
   EXPAND(ck);
   FILL(ck);
   elm_object_text_set(ck, "Promote contact on message");
   elm_check_state_pointer_set(ck, &cl->settings.enable_chat_promote);
   elm_box_pack_end(frbox, ck);
   evas_object_show(ck);

   elm_flip_content_back_set(cl->flip, box);
}

void
settings_toggle(Contact_List *cl, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   elm_flip_go(cl->flip, ELM_FLIP_ROTATE_Y_CENTER_AXIS);
}
