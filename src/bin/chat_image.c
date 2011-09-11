#include "ui.h"


static Evas_Object *
_chat_conv_image_provider(Image *i, Evas_Object *obj, Evas_Object *tt)
{
   Evas_Object *ret, *ic, *win = elm_object_top_widget_get(obj);
   int w, h, cw, ch;
   DBG("(i=%p,win=%p)", i, win);
   if ((!i) || (!i->buf)) goto error;

   w = h = cw = ch = 0;
   ic = elm_icon_add(tt);
   if (!elm_icon_memfile_set(ic, eina_binbuf_string_get(i->buf), eina_binbuf_length_get(i->buf), NULL, NULL))
     {
        /* an unloadable image is a useless image! */
        eina_hash_del_by_key(i->cl->images, ecore_con_url_url_get(i->url));
        evas_object_del(ic);
        goto error;
     }
   evas_object_show(ic);
   ret = elm_box_add(tt);
   WEIGHT(ret, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   ALIGN(ret, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(ret, ic);

#ifdef HAVE_ECORE_X
   Ecore_X_Window xwin;
   xwin = elm_win_xwindow_get(elm_object_top_widget_get(obj));
   xwin = ecore_x_window_root_get(xwin);
   ecore_x_randr_screen_current_size_get(xwin, &cw, &ch, NULL, NULL);
#else
   evas_object_geometry_get(elm_object_top_widget_get(obj), NULL, NULL, &cw, &ch);
#endif
   elm_icon_size_get(ic, &w, &h);
   elm_icon_scale_set(ic, 0, 0);
   if (elm_icon_animated_available_get(ic))
     {
        elm_icon_animated_set(ic, EINA_TRUE);
        elm_icon_animated_play_set(ic, EINA_TRUE);
     }
   {
      float sc = 0;
      if ((float)w / (float)cw >= 0.6)
        sc = ((float)cw * 0.6) / (float)w;
      else if ((float)h / (float)ch >= 0.6)
        sc = ((float)ch * 0.6) / (float)h;
      if (sc) elm_object_scale_set(ic, sc);
   }
   ic = elm_label_add(tt);
   elm_object_text_set(ic, "Left click link to open in BROWSER<ps>"
                           "Right click link to copy to clipboard");
   elm_box_pack_end(ret, ic);
   evas_object_show(ic);
   return ret;
error:
   ret = elm_bg_add(tt);
   elm_bg_color_set(ret, 0, 0, 0);
   ret = elm_label_add(tt); /* FIXME: loading image or something ? */
   elm_object_text_set(ret, "Image could not be loaded!");
   return ret;
}

void
chat_conv_image_show(Evas_Object *convo, Evas_Object *obj, Elm_Entry_Anchor_Info *ev)
{
   Evas_Object *win = elm_object_top_widget_get(convo);
   Image *i = NULL;
   Contact *c = evas_object_data_get(win, "contact");

   if (!c) return;
   i = eina_hash_find(c->list->images, ev->name);
   if (!i) return;
   elm_object_tooltip_content_cb_set(convo, (Elm_Tooltip_Content_Cb)_chat_conv_image_provider, i, NULL);
   elm_tooltip_size_restrict_disable(obj, EINA_TRUE);
   elm_object_tooltip_style_set(obj, "transparent");
   elm_object_tooltip_show(obj);
   DBG("anchor in: '%s' (%i, %i)", ev->name, ev->x, ev->y);
}

void
chat_conv_image_hide(Evas_Object *convo, Evas_Object *obj, Elm_Entry_Anchor_Info *ev)
{
   Evas_Object *win = elm_object_top_widget_get(convo);
   Image *i = NULL;
   Contact *c = evas_object_data_get(win, "contact");

   if (!c) return;
   i = eina_hash_find(c->list->images, ev->name);
   if (!i) return;
   elm_object_tooltip_unset(obj);
   DBG("anchor out: '%s' (%i, %i)", ev->name, ev->x, ev->y);
}

void
char_image_add(Contact_List *cl, const char *url)
{
   Image *i;

   i = eina_hash_find(cl->images, url);
   if (i) return;
   if (ui_eet_dummy_check(url)) return;
   i = calloc(1, sizeof(Image));
   i->buf = ui_eet_image_get(url);
   if (!i->buf)
     {
        i->url = ecore_con_url_new(url);
        ecore_con_url_data_set(i->url, i);
        ecore_con_url_get(i->url);
     }
   i->cl = cl;
   i->addr = url;
   eina_hash_add(cl->images, url, i);
}

void
chat_image_free(Image *i)
{
   if (!i) return;
   if (i->url) ecore_con_url_free(i->url);
   if (i->buf) eina_binbuf_free(i->buf);
   eina_stringshare_del(i->addr);
   free(i);
}

Eina_Bool
chat_image_data(void *d __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Url_Data *ev)
{
   Image *i = ecore_con_url_data_get(ev->url_con);

   //DBG("Received %i bytes of image: %s", ev->size, ecore_con_url_url_get(ev->url_con));
   if (!i->buf) i->buf = eina_binbuf_new();
   eina_binbuf_append_length(i->buf, &ev->data[0], ev->size);
   return ECORE_CALLBACK_RENEW;
}

Eina_Bool
chat_image_complete(void *d __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Url_Complete *ev)
{
   Image *i = ecore_con_url_data_get(ev->url_con);
   const Eina_List *headers, *l;
   const char *h;
   DBG("%i code for image: %s", ev->status, ecore_con_url_url_get(ev->url_con));
   headers = ecore_con_url_response_headers_get(ev->url_con);
   EINA_LIST_FOREACH(headers, l, h)
     {
        if (strncasecmp(h, "content-type: ", sizeof("content-type: ") - 1)) continue;
        h += sizeof("content-type: ") - 1;

        if (!strncasecmp(h, "image/", 6)) break;
        ui_eet_dummy_add(ecore_con_url_url_get(ev->url_con));
        eina_hash_del_by_key(i->cl->images, ecore_con_url_url_get(ev->url_con));
        return ECORE_CALLBACK_RENEW;
     }
   if (ev->status != 200)
     {
        eina_hash_del_by_key(i->cl->images, ecore_con_url_url_get(ev->url_con));
        return ECORE_CALLBACK_RENEW;
     }
   ui_eet_image_add(i->addr, i->buf);
   return ECORE_CALLBACK_RENEW;
}

