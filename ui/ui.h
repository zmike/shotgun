#ifndef __UI_H
#define __UI_H

#include <Shotgun.h>
#include <Ecore.h>
#include <Ecore_Con.h>
#include <Elementary.h>

#ifndef __UNUSED__
# define __UNUSED__ __attribute__((unused))
#endif

#define DBG(...)            EINA_LOG_DOM_DBG(ui_log_dom, __VA_ARGS__)
#define INF(...)            EINA_LOG_DOM_INFO(ui_log_dom, __VA_ARGS__)
#define WRN(...)            EINA_LOG_DOM_WARN(ui_log_dom, __VA_ARGS__)
#define ERR(...)            EINA_LOG_DOM_ERR(ui_log_dom, __VA_ARGS__)
#define CRI(...)            EINA_LOG_DOM_CRIT(ui_log_dom, __VA_ARGS__)

#define WEIGHT evas_object_size_hint_weight_set
#define ALIGN evas_object_size_hint_align_set

extern int ui_log_dom;

typedef struct
{
   Evas_Object *win;
   Evas_Object *list;

   Eina_Hash *users;
   Eina_Hash *user_convs;
   Eina_Hash *images;

   struct {
        Ecore_Event_Handler *iq;
        Ecore_Event_Handler *presence;
        Ecore_Event_Handler *message;
   } event_handlers;
} Contact_List;

typedef struct
{
   Shotgun_User *base;
   Shotgun_User_Info *info;
   Shotgun_Event_Presence *cur;
   Eina_List *plist;
   Shotgun_User_Status status;
   char *description;
   Elm_Genlist_Item *list_item;
   Evas_Object *chat_window;
   Evas_Object *chat_buffer;
   Evas_Object *status_line;
   Contact_List *list;
} Contact;

typedef struct
{
   Ecore_Con_Url *url;
   Eina_Binbuf *buf;
   Contact_List *cl;
} Image;

void contact_list_new(void);
void contact_list_user_add(Contact_List *cl, Contact *c);
void contact_list_user_del(Contact *c, Shotgun_Event_Presence *ev);

void chat_window_new(Contact *c);
void chat_message_status(Contact *c, Shotgun_Event_Message *msg);
void chat_message_insert(Contact *c, const char *from, const char *msg);


void char_image_add(Contact_List *cl, const char *url);
void chat_image_free(Image *i);
void chat_conv_image_show(Evas_Object *convo, Evas_Object *obj, Elm_Entry_Anchor_Info *ev);
void chat_conv_image_hide(Evas_Object *convo __UNUSED__, Evas_Object *obj, Elm_Entry_Anchor_Info *ev);
Eina_Bool chat_image_data(void *d __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Url_Data *ev);
Eina_Bool chat_image_complete(void *d __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Url_Complete *ev);

void contact_free(Contact *c);
void do_something_with_user(Contact_List *cl, Shotgun_User *user);

Eina_Bool event_iq_cb(Contact_List *cl, int type __UNUSED__, Shotgun_Event_Iq *ev);
Eina_Bool event_presence_cb(Contact_List *cl, int type __UNUSED__, Shotgun_Event_Presence *ev);
Eina_Bool event_message_cb(void *data, int type __UNUSED__, void *event);

#endif /* __UI_H */
