#include "ui.h"

void
ui_key_grab_set(UI_WIN *ui, const char *key, Eina_Bool enable)
{
   Evas *e;
   Evas_Modifier_Mask ctrl, shift, alt;

   e = evas_object_evas_get(ui->win);
   ctrl = evas_key_modifier_mask_get(e, "Control");
   shift = evas_key_modifier_mask_get(e, "Shift");
   alt = evas_key_modifier_mask_get(e, "Alt");
   if (enable)
     1 | evas_object_key_grab(ui->win, key, 0, ctrl | shift | alt, 1); /* worst warn_unused ever. */
   else
     evas_object_key_ungrab(ui->win, key, 0, ctrl | shift | alt);
}

void
ui_win_init(UI_WIN *ui)
{
   Evas_Object *obj, *fr, *box, *win;
   Evas *e;
   Evas_Modifier_Mask ctrl, shift, alt;

   settings_finagle(ui);
   win = ui->win = elm_win_add(NULL, "shotgun", ELM_WIN_BASIC);

   elm_win_autodel_set(win, 1);
   elm_win_screen_constrain_set(win, 1);
   e = evas_object_evas_get(win);
   ctrl = evas_key_modifier_mask_get(e, "Control");
   shift = evas_key_modifier_mask_get(e, "Shift");
   alt = evas_key_modifier_mask_get(e, "Alt");
   1 | evas_object_key_grab(win, "Escape", 0, ctrl | shift | alt, 1); /* worst warn_unused ever. */
   1 | evas_object_key_grab(win, "q", ctrl, shift | alt, 1); /* worst warn_unused ever. */

   obj = elm_bg_add(win);
   EXPAND(obj);
   FILL(obj);
   elm_win_resize_object_add(win, obj);
   evas_object_show(obj);

   if (ui->settings->enable_illume)
     {
        ui->illume_box = box = elm_box_add(win);
        elm_box_homogeneous_set(box, EINA_FALSE);
        elm_box_horizontal_set(box, EINA_TRUE);
        EXPAND(box);
        FILL(box);
        elm_win_resize_object_add(win, box);
        evas_object_show(box);

        ui->illume_frame = fr = elm_frame_add(win);
        EXPAND(fr);
        FILL(fr);
        if (ui->type)
          elm_object_text_set(fr, "Login");
        else
          elm_object_text_set(fr, "Contacts");
        elm_box_pack_end(box, fr);
     }

   ui->flip = elm_flip_add(win);
   EXPAND(ui->flip);
   FILL(ui->flip);

   IF_ILLUME(ui)
     {
        elm_object_content_set(fr, ui->flip);
        evas_object_show(fr);
     }
   else
     elm_win_resize_object_add(win, ui->flip);

   ui->box = box = elm_box_add(win);
   elm_box_homogeneous_set(box, EINA_FALSE);
   IF_ILLUME(ui)
     WEIGHT(box, 0, EVAS_HINT_EXPAND);
   else
     EXPAND(box);
   evas_object_show(box);

   elm_object_part_content_set(ui->flip, "front", box);

   evas_object_show(ui->flip);
}
