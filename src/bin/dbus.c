#include "ui.h"
#ifdef HAVE_DBUS
#include <E_DBus.h>

#ifdef HAVE_NOTIFY
#include <E_Notify.h>
#endif

static DBusMessage *
_dbus_quit_cb(E_DBus_Object *obj, DBusMessage *msg)
{
   Contact_List *cl = e_dbus_object_data_get(obj);
   shotgun_disconnect(cl->account);
   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_dbus_list_get_cb(E_DBus_Object *obj, DBusMessage *msg)
{
   Contact_List *cl = e_dbus_object_data_get(obj);
   Eina_List *l;
   Contact *c;
   DBusMessage *ret;
   DBusMessageIter iter, arr;

   ret = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(ret, &iter);
   dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &arr);

   EINA_LIST_FOREACH(cl->users_list, l, c)
     {
        if (c->cur && c->cur->status && c->base->subscription)
          dbus_message_iter_append_basic(&arr, DBUS_TYPE_STRING, &c->base->jid);
     }
   dbus_message_iter_close_container(&iter, &arr);
   return ret;
}

static DBusMessage *
_dbus_list_get_all_cb(E_DBus_Object *obj, DBusMessage *msg)
{
   Contact_List *cl = e_dbus_object_data_get(obj);
   Eina_List *l;
   Contact *c;
   DBusMessage *ret;
   DBusMessageIter iter, arr;

   ret = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(ret, &iter);
   dbus_message_iter_open_container(&iter, 'a', "s", &arr);

   EINA_LIST_FOREACH(cl->users_list, l, c)
     dbus_message_iter_append_basic(&arr, 's', &c->base->jid);
   dbus_message_iter_close_container(&iter, &arr);
   return ret;
}

static DBusMessage *
_dbus_contact_status_cb(E_DBus_Object *obj, DBusMessage *msg)
{
   Contact_List *cl = e_dbus_object_data_get(obj);
   DBusMessageIter iter;
   DBusMessage *reply;
   Contact *c;
   char *name, *s;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &name);
   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   if ((!name) || (!name[0])) goto error;
   s = strchr(name, '/');
   if (s) name = strndupa(name, s - name);
   c = eina_hash_find(cl->users, name);
   if (!c) goto error;

   dbus_message_iter_append_basic(&iter, 's', &c->description);
   dbus_message_iter_append_basic(&iter, 'u', (uintptr_t*)c->status);
   dbus_message_iter_append_basic(&iter, 'i', (intptr_t*)c->priority);
   return reply;
error:
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, NULL);
   return reply;
}

static DBusMessage *
_dbus_contact_send_cb(E_DBus_Object *obj, DBusMessage *msg)
{
   Contact_List *cl = e_dbus_object_data_get(obj);
   DBusMessageIter iter;
   DBusMessage *reply;
   Contact *c;
   char *name, *s;
   Shotgun_Message_Status st;
   Eina_Bool ret = EINA_FALSE;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &name);
   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   if ((!name) || (!name[0])) goto error;
   s = strchr(name, '/');
   if (s) name = strndupa(name, s - name);
   c = eina_hash_find(cl->users, name);
   if (!c) goto error;
   dbus_message_iter_get_basic(&iter, &s);
   if (!s) goto error;
   dbus_message_iter_get_basic(&iter, &st);
   ret = shotgun_message_send(c->base->account, c->cur ? c->cur->jid : c->base->jid, s, st);
error:
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_BOOLEAN, &ret);
   return reply;
}

static DBusMessage *
_dbus_contact_icon_cb(E_DBus_Object *obj, DBusMessage *msg)
{
   Contact_List *cl = e_dbus_object_data_get(obj);
   DBusMessage *reply;

   reply = dbus_message_new_method_return(msg);
   return reply; /* get icon name from eet file */
}

void
ui_dbus_init(Contact_List *cl)
{
   E_DBus_Interface *iface;

   e_dbus_init();
#ifdef HAVE_NOTIFY
   e_notification_init();
#endif
   cl->dbus = e_dbus_bus_get(DBUS_BUS_SESSION);
   e_dbus_request_name(cl->dbus, "org.shotgun", 0, NULL, NULL);
   cl->dbus_object = e_dbus_object_add(cl->dbus, "/org/shotgun/remote", cl);
   iface = e_dbus_interface_new("org.shotgun.core");
   e_dbus_object_interface_attach(cl->dbus_object, iface);
   e_dbus_interface_unref(iface);

   e_dbus_interface_method_add(iface, "quit", "", "", _dbus_quit_cb);

   iface = e_dbus_interface_new("org.shotgun.list");
   e_dbus_object_interface_attach(cl->dbus_object, iface);
   e_dbus_interface_unref(iface);

   e_dbus_interface_method_add(iface, "get", "", "as", _dbus_list_get_cb);
   e_dbus_interface_method_add(iface, "get_all", "", "as", _dbus_list_get_all_cb);

   iface = e_dbus_interface_new("org.shotgun.contact");
   e_dbus_object_interface_attach(cl->dbus_object, iface);
   e_dbus_interface_unref(iface);

   e_dbus_interface_method_add(iface, "status", "s", "sui", _dbus_contact_status_cb);
   e_dbus_interface_method_add(iface, "icon", "s", "s", _dbus_contact_icon_cb);
   e_dbus_interface_method_add(iface, "send", "ssu", "b", _dbus_contact_send_cb);
}

#ifdef HAVE_NOTIFY
void
ui_dbus_notify(const char *from, const char *msg)
{
   E_Notification *n;

   n = e_notification_full_new("SHOTGUN!", 0, NULL, from, msg, 5000);
   e_notification_send(n, NULL, NULL);
   e_notification_unref(n);
}
#endif

void
ui_dbus_shutdown(Contact_List *cl)
{
   if (!cl) return;
   if (cl->dbus_object) e_dbus_object_free(cl->dbus_object);
   if (cl->dbus) e_dbus_connection_close(cl->dbus);
   cl->dbus = NULL;
   cl->dbus_object = NULL;
#ifdef HAVE_NOTIFY
   e_notification_shutdown();
#endif
   e_dbus_shutdown();
}
#endif
