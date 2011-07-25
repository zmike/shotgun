#include <sys/stat.h>
#include "ui.h"

void
contact_free(Contact *c)
{
   Shotgun_Event_Presence *pres;
   shotgun_event_presence_free(c->cur);
   EINA_LIST_FREE(c->plist, pres)
     shotgun_event_presence_free(pres);
   if (c->list_item)
     c->list->list_item_del[c->list->mode](c->list_item);
   if (c->chat_window)
     evas_object_del(c->chat_window);
   shotgun_user_free(c->base);
   shotgun_user_info_free(c->info);
   c->list->users_list = eina_list_remove(c->list->users_list, c);
   eina_stringshare_del(c->last_conv);
   free(c);
}

void
do_something_with_user(Contact_List *cl, Shotgun_User *user)
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
   cl->users_list = eina_list_append(cl->users_list, c);
}


