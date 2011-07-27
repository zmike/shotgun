#include "ui.h"


static Evas_Object *
_chat_conv_image_provider(Image *i, Evas_Object *obj, Evas_Object *tt)
{
   Evas_Object *ret, *win = elm_object_top_widget_get(obj);
   DBG("(i=%p,win=%p)", i, win);
   if ((!i) || (!i->buf)) goto error;

   ret = elm_icon_add(tt);
   if (!elm_icon_memfile_set(ret, eina_binbuf_string_get(i->buf), eina_binbuf_length_get(i->buf), NULL, NULL))
     {
        /* an unloadable image is a useless image! */
        eina_hash_del_by_key(i->cl->images, ecore_con_url_url_get(i->url));
        evas_object_del(ret);
        goto error;
     }
   elm_icon_scale_set(ret, 0, 0);
   return ret;
error:
   ret = elm_label_add(obj); /* FIXME: loading image or something ? */
   elm_object_text_set(ret, "Image could not be loaded!");
   return ret;
}

void
chat_conv_image_show(Evas_Object *convo, Evas_Object *obj, Elm_Entry_Anchor_Info *ev)
{
   Evas_Object *win = elm_object_top_widget_get(convo);
   Image *i = NULL;
   Contact *c = evas_object_data_get(win, "contact");

   if (c) i = eina_hash_find(c->list->images, ev->name);
   elm_object_tooltip_content_cb_set(convo, (Elm_Tooltip_Content_Cb)_chat_conv_image_provider, i, NULL);
   elm_tooltip_size_restrict_disable(obj, EINA_TRUE);
   elm_object_tooltip_style_set(obj, "transparent");
   elm_object_tooltip_show(obj);
   DBG("anchor in: '%s' (%i, %i)", ev->name, ev->x, ev->y);
}

void
chat_conv_image_hide(Evas_Object *convo __UNUSED__, Evas_Object *obj, Elm_Entry_Anchor_Info *ev)
{
   elm_object_tooltip_hide(obj);
   DBG("anchor out: '%s' (%i, %i)", ev->name, ev->x, ev->y);
}

void
char_image_add(Contact_List *cl, const char *url)
{
   Image *i;

   i = eina_hash_find(cl->images, url);
   if (i) return;
   i = calloc(1, sizeof(Image));
   i->url = ecore_con_url_new(url);
   ecore_con_url_data_set(i->url, i);
   i->cl = cl;

   ecore_con_url_get(i->url);
   eina_hash_add(cl->images, url, i);
}

void
chat_image_free(Image *i)
{
   if (!i) return;
   if (i->url) ecore_con_url_free(i->url);
   if (i->buf) eina_binbuf_free(i->buf);
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
   DBG("%i code for image: %s", ev->status, ecore_con_url_url_get(ev->url_con));
   if (ev->status != 200)
     { /* FIXME: update tooltips too */
        eina_hash_del_by_key(i->cl->images, ecore_con_url_url_get(ev->url_con));
        return ECORE_CALLBACK_RENEW;
     }

   return ECORE_CALLBACK_RENEW;
}

