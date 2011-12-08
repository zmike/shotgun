#include "ui.h"


static Evas_Object *
_chat_conv_image_provider(Image *i, Evas_Object *obj __UNUSED__, Evas_Object *tt)
{
   Evas_Object *ret, *ic;
   int w, h, cw, ch;
   DBG("(i=%p)", i);
   if ((!i) || (!i->buf)) goto error;

   w = h = cw = ch = 0;
   ic = elm_icon_add(tt);
   if (!elm_icon_memfile_set(ic, eina_binbuf_string_get(i->buf), eina_binbuf_length_get(i->buf), NULL, NULL))
     {
        /* an unloadable image is a useless image! */
        i->cl->image_list = eina_list_remove(i->cl->image_list, i);
        eina_hash_del_by_key(i->cl->images, ecore_con_url_url_get(i->url));
        evas_object_del(ic);
        goto error;
     }
   evas_object_show(ic);
   ret = elm_box_add(tt);
   elm_box_homogeneous_set(ret, EINA_FALSE);
   EXPAND(ret);
   FILL(ret);
   elm_box_pack_end(ret, ic);

   elm_win_screen_size_get(tt, NULL, NULL, &cw, &ch);
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
   ret = elm_label_add(tt);
   elm_object_text_set(ret, "Left click link to open in BROWSER<ps>"
                           "Right click link to copy to clipboard");
   return ret;
}

static int
_chat_image_sort_cb(Image *a, Image *b)
{
   long long diff;
   diff = a->timestamp - b->timestamp;
   if (diff < 0) return -1;
   if (!diff) return 0;
   return 1;
}

void
chat_conv_image_show(Contact *c, Evas_Object *obj, Elm_Entry_Anchor_Info *ev)
{
   Image *i = NULL;

   if (!c) return;
   i = eina_hash_find(c->list->images, ev->name);
   if (!i) return;
   elm_object_tooltip_content_cb_set(obj, (Elm_Tooltip_Content_Cb)_chat_conv_image_provider, i, NULL);
   elm_object_tooltip_window_mode_set(obj, EINA_TRUE);
   elm_object_tooltip_show(obj);
   DBG("anchor in: '%s' (%i, %i)", ev->name, ev->x, ev->y);
}

void
chat_conv_image_hide(Contact *c, Evas_Object *obj, Elm_Entry_Anchor_Info *ev)
{
   Image *i = NULL;

   if (!c) return;
   i = eina_hash_find(c->list->images, ev->name);
   if (!i) return;
   elm_object_tooltip_unset(obj);
   DBG("anchor out: '%s' (%i, %i)", ev->name, ev->x, ev->y);
}

void
chat_image_add(Contact_List *cl, const char *url)
{
   Image *i;
   unsigned long long t;

   t = (unsigned long long)ecore_time_unix_get();

   i = eina_hash_find(cl->images, url);
   if (i)
     {
        if (i->buf)
          {
             i->timestamp = t;
             ui_eet_image_ping(url, i->timestamp);
          }
        else
          {
             /* randomly deleted during cache pruning */
             i->buf = ui_eet_image_get(url, t);
             cl->image_size += eina_binbuf_length_get(i->buf);
          }
        chat_image_cleanup(i->cl);
        return;
     }
   if (ui_eet_dummy_check(url)) return;
   if (cl->settings->disable_image_fetch) return;
   i = calloc(1, sizeof(Image));
   i->buf = ui_eet_image_get(url, t);
   if (i->buf)
     cl->image_size += eina_binbuf_length_get(i->buf);
   else
     {
        i->url = ecore_con_url_new(url);
        ecore_con_url_data_set(i->url, i);
        ecore_con_url_get(i->url);
     }
   i->cl = cl;
   i->addr = url;
   eina_hash_add(cl->images, url, i);
   cl->image_list = eina_list_sorted_insert(cl->image_list, (Eina_Compare_Cb)_chat_image_sort_cb, i);
   chat_image_cleanup(i->cl);
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
   if (!i->buf) i->buf = eina_binbuf_manage_new_length(ev->data, ev->size);
   else eina_binbuf_append_length(i->buf, ev->data, ev->size);
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
        i->cl->image_list = eina_list_remove(i->cl->image_list, i);
        eina_hash_del_by_key(i->cl->images, ecore_con_url_url_get(ev->url_con));
        return ECORE_CALLBACK_RENEW;
     }
   if (ev->status != 200)
     {
        i->cl->image_list = eina_list_remove(i->cl->image_list, i);
        eina_hash_del_by_key(i->cl->images, ecore_con_url_url_get(ev->url_con));
        return ECORE_CALLBACK_RENEW;
     }
   i->timestamp = (unsigned long long)ecore_time_unix_get();
   if (ui_eet_image_add(i->addr, i->buf, i->timestamp) == 1)
     i->cl->image_size += eina_binbuf_length_get(i->buf);
   chat_image_cleanup(i->cl);
   return ECORE_CALLBACK_RENEW;
}

void
chat_image_cleanup(Contact_List *cl)
{
   Image *i;
   Eina_List *l, *l2;

   if (!cl->settings->allowed_image_size) return;
   if (cl->settings->allowed_image_size < (cl->image_size / 1024 / 1024)) return;
   EINA_LIST_FOREACH_SAFE(cl->image_list, l, l2, i)
     {
        if (!i->buf) continue;
        /* only free the buffers here to avoid having to deal with multiple list entries */
        cl->image_size -= eina_binbuf_length_get(i->buf);
        eina_binbuf_free(i->buf);
        i->buf = NULL;
        if (cl->settings->allowed_image_size < (cl->image_size / 1024 / 1024))
          return;
     }
}
