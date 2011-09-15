#ifndef __UI_H
#define __UI_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef alloca
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#elif defined __GNUC__
#define alloca __builtin_alloca
#elif defined _AIX
#define alloca __alloca
#else
#include <stddef.h>
void *alloca (size_t);
#endif
#endif

#include <Shotgun.h>
#include <Ecore.h>
#include <Ecore_Con.h>
#include <Elementary.h>
#ifdef HAVE_ECORE_X
# include <Ecore_X.h>
#endif
#ifdef HAVE_DBUS
# include <E_DBus.h>
#endif
#ifdef HAVE_AZY
# include <Azy.h>
#endif

#ifndef __UNUSED__
# define __UNUSED__ __attribute__((unused))
#endif

#ifndef strdupa
# define strdupa(str)       strcpy(alloca(strlen(str) + 1), str)
#endif

#ifndef strndupa
# define strndupa(str, len) strncpy(alloca(len + 1), str, len)
#endif

#define DBG(...)            EINA_LOG_DOM_DBG(ui_log_dom, __VA_ARGS__)
#define INF(...)            EINA_LOG_DOM_INFO(ui_log_dom, __VA_ARGS__)
#define WRN(...)            EINA_LOG_DOM_WARN(ui_log_dom, __VA_ARGS__)
#define ERR(...)            EINA_LOG_DOM_ERR(ui_log_dom, __VA_ARGS__)
#define CRI(...)            EINA_LOG_DOM_CRIT(ui_log_dom, __VA_ARGS__)

#define WEIGHT evas_object_size_hint_weight_set
#define ALIGN evas_object_size_hint_align_set

extern int ui_log_dom;

typedef struct Contact_List Contact_List;
typedef struct Contact Contact;

typedef void (*Contact_List_Item_Tooltip_Cb)(void *item, Elm_Tooltip_Item_Content_Cb func, const void *data, Evas_Smart_Cb del_cb);
typedef Eina_Bool (*Contact_List_Item_Tooltip_Resize_Cb)(void *item, Eina_Bool set);
typedef void *(*Contact_List_At_XY_Item_Get)(void *list, Evas_Coord x, Evas_Coord y, int *ret);

struct Contact_List
{
   Evas_Object *win; /* window */
   Evas_Object *box; /* main box */
   Evas_Object *list; /* list/grid object */
   Evas_Object *status_entry; /* entry for user's status */

   Evas_Object *pager; /* pager for user add wizard */
   Eina_List *pager_entries; /* entry in pager */
   Eina_Bool pager_state : 1; /* 0 for first page, 1 for second */

   Eina_List *users_list; /* list of all contacts */
   Eina_Hash *users; /* hash of jid<->Contact */
   Eina_Hash *user_convs; /* hash of jid<->Contact->win */
   Eina_Hash *images; /* hash of img_url<->Image */
   Ecore_Timer *status_timer; /* timer for sending text in status_entry */

   Eina_Bool mode : 1; /* 0 for list, 1 for grid */
   Eina_Bool view : 1; /* 0 for regular, 1 for offlines */
#ifdef HAVE_DBUS
   E_DBus_Connection *dbus;
   E_DBus_Object *dbus_object;
#endif

   /* fps for doing stuff to both list and grid views with the same function */
   Ecore_Data_Cb list_item_parent_get[2];
   Ecore_Data_Cb list_selected_item_get[2];
   Contact_List_At_XY_Item_Get list_at_xy_item_get[2];
   Ecore_Cb list_item_del[2];
   Ecore_Cb list_item_update[2];
   Contact_List_Item_Tooltip_Cb list_item_tooltip_add[2];
   Contact_List_Item_Tooltip_Resize_Cb list_item_tooltip_resize[2];

   struct {
        Ecore_Event_Handler *iq;
        Ecore_Event_Handler *presence;
        Ecore_Event_Handler *message;
   } event_handlers;
   Shotgun_Auth *account; /* user's account */
};

struct Contact
{
   Shotgun_User *base;
   Shotgun_User_Info *info;
   Shotgun_Event_Presence *cur; /* the current presence; should NOT be in plist */
   Eina_List *plist; /* list of presences with lower priority than cur */

   /* the next 3 are just convenience pointers to the user's current X */
   Shotgun_User_Status status; /* user's current status */
   int priority; /* user's current priority */
   const char *description; /* user's current status message */

   const char *force_resource; /* always send to this resource if set */
   const char *last_conv; /* entire conversation with user to keep conversations fluid when windows are opened/closed */
   const char *tooltip_label; /* label for contact list item tooltip */
   void *list_item; /* the grid/list item object */
   Evas_Object *chat_window; /* the chat window for the contact (if open) */
   Evas_Object *chat_buffer; /* chat buffer of the conversation */
   Evas_Object *chat_input; /* input entry for the conversation */
   Evas_Object *chat_jid_menu; /* menu object for the submenu in the chat window */
   Evas_Object *status_line; /* status entry inside the frame at top */
   Contact_List *list; /* the owner list */
   Eina_Bool tooltip_changed : 1; /* when set, tooltip_label will be re-created */
   Eina_Bool ignore_resource : 1; /* when set, priority will be ignored and messages will be sent to all resources */
};

typedef struct
{
   Ecore_Con_Url *url;
   Eina_Binbuf *buf;
   const char *addr;
   Contact_List *cl;
} Image;

Contact_List *contact_list_new(Shotgun_Auth *auth);
void contact_list_user_add(Contact_List *cl, Contact *c);
void contact_list_user_del(Contact *c, Shotgun_Event_Presence *ev);

void chat_window_new(Contact *c);
void chat_message_status(Contact *c, Shotgun_Event_Message *msg);
void chat_message_insert(Contact *c, const char *from, const char *msg, Eina_Bool me);


void char_image_add(Contact_List *cl, const char *url);
void chat_image_free(Image *i);
void chat_conv_image_show(Evas_Object *convo, Evas_Object *obj, Elm_Entry_Anchor_Info *ev);
void chat_conv_image_hide(Evas_Object *convo __UNUSED__, Evas_Object *obj, Elm_Entry_Anchor_Info *ev);
Eina_Bool chat_image_data(void *d __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Url_Data *ev);
Eina_Bool chat_image_complete(void *d __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Url_Complete *ev);

const char *contact_name_get(Contact *c);
void contact_jids_menu_del(Contact *c, const char *jid);
void contact_free(Contact *c);
Contact *do_something_with_user(Contact_List *cl, Shotgun_User *user);
void contact_subscription_set(Contact *c, Shotgun_Presence_Type type, Shotgun_User_Subscription sub);

Eina_Bool ui_eet_init(Shotgun_Auth *auth);
void ui_eet_dummy_add(const char *url);
Eina_Bool ui_eet_dummy_check(const char *url);
void ui_eet_image_add(const char *url, Eina_Binbuf *buf);
Eina_Binbuf *ui_eet_image_get(const char *url);
void ui_eet_shutdown(Shotgun_Auth *auth);
Shotgun_Auth *ui_eet_auth_get(void);
void ui_eet_auth_set(Shotgun_Auth *auth, Eina_Bool store_pw, Eina_Bool use_auth);
void ui_eet_userinfo_add(Shotgun_Auth *auth, Shotgun_User_Info *info);
Shotgun_User_Info *ui_eet_userinfo_get(Shotgun_Auth *auth, const char *jid);

#ifdef HAVE_DBUS
void ui_dbus_signal_message_self(Contact_List *cl, const char *jid, const char *s);
void ui_dbus_signal_message(Contact_List *cl, Contact *c, Shotgun_Event_Message *msg);
void ui_dbus_signal_status_self(Contact_List *cl);
void ui_dbus_init(Contact_List *cl);
# ifdef HAVE_NOTIFY
void ui_dbus_notify(const char *from, const char *msg);
# endif
#endif

#ifdef HAVE_AZY
void ui_azy_init(Contact_List *cl);
void ui_azy_connect(Contact_List *cl);
void ui_azy_shutdown(Contact_List *cl);
#endif

Eina_Bool event_iq_cb(Contact_List *cl, int type __UNUSED__, Shotgun_Event_Iq *ev);
Eina_Bool event_presence_cb(Contact_List *cl, int type __UNUSED__, Shotgun_Event_Presence *ev);
Eina_Bool event_message_cb(Contact_List *cl, int type __UNUSED__, Shotgun_Event_Message *msg);

const char *util_configdir_get(void);
Eina_Bool util_configdir_create(void);

#endif /* __UI_H */
