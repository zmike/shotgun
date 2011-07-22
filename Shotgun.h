#ifndef SHOTGUN_H
#define SHOTGUN_H

#include <Eina.h>

extern int SHOTGUN_EVENT_CONNECT; /* Shotgun_Auth */
extern int SHOTGUN_EVENT_IQ; /* Shotgun_Event_Iq */
extern int SHOTGUN_EVENT_MESSAGE; /* Shotgun_Event_Message */
extern int SHOTGUN_EVENT_PRESENCE; /* Shotgun_Event_Presence */

typedef struct Shotgun_Auth Shotgun_Auth;

typedef enum
{
   SHOTGUN_IQ_EVENT_TYPE_UNKNOWN,
   SHOTGUN_IQ_EVENT_TYPE_ROSTER, /* Eina_List *Shotgun_User */
   SHOTGUN_IQ_EVENT_TYPE_INFO, /* Shotgun_User_Info */
} Shotgun_Iq_Event_Type;

typedef enum
{
   SHOTGUN_USER_SUBSCRIPTION_NONE,
   SHOTGUN_USER_SUBSCRIPTION_FROM,
   SHOTGUN_USER_SUBSCRIPTION_TO,
   SHOTGUN_USER_SUBSCRIPTION_BOTH
} Shotgun_User_Subscription;

typedef enum
{
   SHOTGUN_USER_STATUS_NONE, /* unavailable */
   SHOTGUN_USER_STATUS_NORMAL,
   SHOTGUN_USER_STATUS_AWAY,
   SHOTGUN_USER_STATUS_CHAT,
   SHOTGUN_USER_STATUS_DND,
   SHOTGUN_USER_STATUS_XA /* eXtended Away */
} Shotgun_User_Status;

typedef enum
{
   SHOTGUN_MESSAGE_STATUS_NONE,
   SHOTGUN_MESSAGE_STATUS_ACTIVE,
   SHOTGUN_MESSAGE_STATUS_COMPOSING,
   SHOTGUN_MESSAGE_STATUS_PAUSED,
   SHOTGUN_MESSAGE_STATUS_INACTIVE,
   SHOTGUN_MESSAGE_STATUS_GONE
} Shotgun_Message_Status;

typedef struct
{
   const char *jid;
   const char *name; /* nickname (alias) */
   Shotgun_User_Subscription subscription;
   Shotgun_Auth *account;
} Shotgun_User;

typedef struct
{
   const char *jid;
   const char *full_name;
   struct
     {
        const char *type;
        void *data;
        size_t size;
     } photo;
} Shotgun_User_Info;

typedef struct
{
   const char *jid;
   char *msg;
   Shotgun_Message_Status status;
   Shotgun_Auth *account;
} Shotgun_Event_Message;

typedef struct
{
   Shotgun_Iq_Event_Type type;
   void *ev;
   Shotgun_Auth *account;
} Shotgun_Event_Iq;

typedef struct
{
   const char *jid;
   char *description;
   char *photo;
   int priority;
   Shotgun_User_Status status;
   Eina_Bool vcard : 1;

   Shotgun_Auth *account;
} Shotgun_Event_Presence;

#ifdef __cplusplus
extern "C" {
#endif

int shotgun_init(void);
Eina_Bool shotgun_gchat_connect(Shotgun_Auth *auth);

Shotgun_Auth *shotgun_new(const char *username, const char *domain);
/**
 * DOES NOT ALLOCATE FOR PASSWORD.
 */
void shotgun_password_set(Shotgun_Auth *auth, const char *password);
void shotgun_password_del(Shotgun_Auth *auth);

Eina_Bool shotgun_iq_roster_get(Shotgun_Auth *auth);
Eina_Bool shotgun_iq_vcard_get(Shotgun_Auth *auth, const char *user);

Eina_Bool shotgun_message_send(Shotgun_Auth *auth, const char *to, const char *msg, Shotgun_Message_Status status);
Eina_Bool shotgun_presence_set(Shotgun_Auth *auth, Shotgun_User_Status st, const char *desc);

void shotgun_event_message_free(Shotgun_Event_Message *msg);
void shotgun_event_presence_free(Shotgun_Event_Presence *pres);
void shotgun_user_info_free(Shotgun_User_Info *info);
void shotgun_user_free(Shotgun_User *user);

#ifdef __cplusplus
}
#endif

#endif
