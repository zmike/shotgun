#include "ui.h"

static void
_contact_list_free_cb(Contact_List *cl, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *ev __UNUSED__)
{
   ecore_event_handler_del(cl->event_handlers.iq);
   ecore_event_handler_del(cl->event_handlers.presence);
   ecore_event_handler_del(cl->event_handlers.message);

   eina_hash_free(cl->users);
   eina_hash_free(cl->images);
   eina_hash_free(cl->user_convs);
   cl->users_list = eina_list_free(cl->users_list);

   free(cl);
}

static void
_contact_list_close(Contact_List *cl, Evas_Object *obj __UNUSED__, void *ev  __UNUSED__)
{
   evas_object_del(cl->win);
}

static void
_contact_list_click_cb(Contact_List *cl, Evas_Object *obj __UNUSED__, void *ev)
{
   Contact *c;

   c = cl->list_item_contact_get[cl->mode](ev);
   if (c->chat_window)
     {
        elm_win_raise(c->chat_window);
        return;
     }

   chat_window_new(c);
}

static char *
_it_label_get_grid(Contact *c, Evas_Object *obj __UNUSED__, const char *part __UNUSED__)
{
   if (c->base->name)
     return strdup(c->base->name);
   if (c->info && c->info->full_name)
     return strdup(c->info->full_name);
   return strdup(c->base->jid);
}

static char *
_it_label_get_list(Contact *c, Evas_Object *obj __UNUSED__, const char *part)
{
   if (!strcmp(part, "elm.text"))
     {
        if (c->base->name)
          return strdup(c->base->name);
        if (c->info && c->info->full_name)
          return strdup(c->info->full_name);
        return strdup(c->base->jid);
     }
   else if (!strcmp(part, "elm.text.sub"))
     {
        char *buf;
        const char *status;

        switch(c->status)
          {
           case SHOTGUN_USER_STATUS_NORMAL:
              status = "Normal";
              break;
           case SHOTGUN_USER_STATUS_AWAY:
              status = "Away";
              break;
           case SHOTGUN_USER_STATUS_CHAT:
              status = "Chat";
              break;
           case SHOTGUN_USER_STATUS_DND:
              status = "Busy";
              break;
           case SHOTGUN_USER_STATUS_XA:
              status = "Very Away";
              break;
           case SHOTGUN_USER_STATUS_NONE:
              status = "Offline?";
              break;
           default:
              status = "What the fuck aren't we handling?";
          }

        if (!c->description)
          return strdup(status);
        asprintf(&buf, "%s: %s", status, c->description);
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
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 0, 0);

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
_contact_list_list_add(Contact_List *cl)
{
   Evas_Object *list;

   cl->list = list = elm_genlist_add(cl->win);
   cl->mode = EINA_FALSE;
   elm_genlist_always_select_mode_set(list, EINA_FALSE);
   WEIGHT(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   ALIGN(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(cl->box, list);
   evas_object_show(list);
   evas_object_smart_callback_add(list, "clicked,double",
                                  (Evas_Smart_Cb)_contact_list_click_cb, cl);
}

static void
_contact_list_grid_add(Contact_List *cl)
{
   Evas_Object *grid;

   cl->list = grid = elm_gengrid_add(cl->win);
   cl->mode = EINA_TRUE;
   elm_gengrid_always_select_mode_set(grid, EINA_FALSE);
   elm_gengrid_item_size_set(grid, 75, 100);

   WEIGHT(grid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   ALIGN(grid, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(cl->box, grid);
   evas_object_show(grid);
   evas_object_smart_callback_add(grid, "clicked,double",
                                  (Evas_Smart_Cb)_contact_list_click_cb, cl);
}

static void
_contact_list_mode_toggle(Contact_List *cl, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Eina_List *l;
   Contact *c;
   evas_object_del(cl->list);
   if (cl->mode)
     _contact_list_list_add(cl);
   else
     _contact_list_grid_add(cl);
   EINA_LIST_FOREACH(cl->users_list, l, c)
     {
        if ((c->base->subscription > SHOTGUN_USER_SUBSCRIPTION_NONE) && c->status)
          contact_list_user_add(cl, c);
     }
}

void
contact_list_user_add(Contact_List *cl, Contact *c)
{
   static Elm_Genlist_Item_Class glit = {
        .item_style = "double_label",
        .func = {
             .label_get = (GenlistItemLabelGetFunc)_it_label_get_list,
             .icon_get = (GenlistItemIconGetFunc)_it_icon_get,
             .state_get = (GenlistItemStateGetFunc)_it_state_get,
             .del = (GenlistItemDelFunc)_it_del
        }
   };
   static Elm_Gengrid_Item_Class ggit = {
        .item_style = "default",
        .func = {
             .label_get = (GridItemLabelGetFunc)_it_label_get_grid,
             .icon_get = (GridItemIconGetFunc)_it_icon_get,
             .state_get = (GridItemStateGetFunc)_it_state_get,
             .del = (GridItemDelFunc)_it_del
        }
   };
   if (c->list->mode)
     c->list_item = elm_gengrid_item_append(cl->list, &ggit, c, NULL, NULL);
   else
     c->list_item = elm_genlist_item_append(cl->list, &glit, c, NULL,
                                                     ELM_GENLIST_ITEM_NONE, NULL, NULL);
}

void
contact_list_user_del(Contact *c, Shotgun_Event_Presence *ev)
{
   Eina_List *l;
   Shotgun_Event_Presence *pres;
   if (!c->plist)
     {
        shotgun_event_presence_free(c->cur);
        if (c->list_item)
          {
             INF("Removing user %s", c->base->jid);
             c->list->list_item_del[c->list->mode](c->list_item);
          }
        c->list_item = NULL;
        c->cur = NULL;
        return;
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
   c->list->list_item_update[c->list->mode](c->list_item);
}

void
contact_list_new(void)
{
   Evas_Object *win, *obj, *box, *menu;
   Elm_Toolbar_Item *it;
   Contact_List *cl;

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

   cl = calloc(1, sizeof(Contact_List));

   cl->win = win = elm_win_add(NULL, "Shotgun - Contacts", ELM_WIN_BASIC);
   elm_win_title_set(win, "Shotgun - Contacts");
   elm_win_autodel_set(win, 1);

   obj = elm_bg_add(win);
   WEIGHT(obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, obj);
   evas_object_show(obj);

   cl->box = box = elm_box_add(win);
   WEIGHT(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, box);
   evas_object_show(box);

   obj = elm_toolbar_add(win);
   elm_toolbar_mode_shrink_set(obj, ELM_TOOLBAR_SHRINK_MENU);
   ALIGN(obj, EVAS_HINT_FILL, 0);
   elm_toolbar_align_set(obj, 0);
   it = elm_toolbar_item_append(obj, NULL, "Shotgun", NULL, NULL);
   elm_toolbar_item_menu_set(it, 1);
   elm_toolbar_menu_parent_set(obj, win);
   menu = elm_toolbar_item_menu_get(it);
   elm_menu_item_add(menu, NULL, "refresh", "Toggle View Mode", (Evas_Smart_Cb)_contact_list_mode_toggle, cl);
   elm_menu_item_add(menu, NULL, "close", "Quit", (Evas_Smart_Cb)_contact_list_close, cl);
   elm_box_pack_end(box, obj);
   evas_object_show(obj);

   cl->list_item_contact_get[0] = (Ecore_Data_Cb)elm_genlist_item_data_get;
   cl->list_item_contact_get[1] = (Ecore_Data_Cb)elm_gengrid_item_data_get;
   cl->list_item_parent_get[0] = (Ecore_Data_Cb)elm_genlist_item_genlist_get;
   cl->list_item_parent_get[1] = (Ecore_Data_Cb)elm_gengrid_item_gengrid_get;
   cl->list_item_del[0] = (Ecore_Cb)elm_genlist_item_del;
   cl->list_item_del[1] = (Ecore_Cb)elm_gengrid_item_del;
   cl->list_item_update[0] = (Ecore_Cb)elm_genlist_item_update;
   cl->list_item_update[1] = (Ecore_Cb)elm_gengrid_item_update;

   _contact_list_list_add(cl);

   evas_object_event_callback_add(win, EVAS_CALLBACK_FREE,
                                  (Evas_Object_Event_Cb)_contact_list_free_cb, cl);

   cl->users = eina_hash_string_superfast_new((Eina_Free_Cb)contact_free);
   cl->user_convs = eina_hash_string_superfast_new((Eina_Free_Cb)evas_object_del);
   cl->images = eina_hash_string_superfast_new((Eina_Free_Cb)chat_image_free);

   cl->event_handlers.iq = ecore_event_handler_add(SHOTGUN_EVENT_IQ,
                                                       (Ecore_Event_Handler_Cb)event_iq_cb, cl);
   cl->event_handlers.presence =
      ecore_event_handler_add(SHOTGUN_EVENT_PRESENCE, (Ecore_Event_Handler_Cb)event_presence_cb,
                              cl);
   cl->event_handlers.message =
      ecore_event_handler_add(SHOTGUN_EVENT_MESSAGE, (Ecore_Event_Handler_Cb)event_message_cb,
                              cl);

   evas_object_data_set(win, "contact-list", cl);

   evas_object_resize(win, 300, 700);
   evas_object_show(win);
}
