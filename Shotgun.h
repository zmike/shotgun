#ifndef SHOTGUN_H
#define SHOTGUN_H

extern int SHOTGUN_EVENT_IQ; /* Shotgun_Event_Iq */
extern int SHOTGUN_EVENT_MESSAGE; /* Shotgun_Event_Message */
extern int SHOTGUN_EVENT_PRESENCE; /* NYI */

typedef struct Shotgun_Auth Shotgun_Auth;

typedef enum
{
   SHOTGUN_IQ_EVENT_TYPE_UNKNOWN,
   SHOTGUN_IQ_EVENT_TYPE_ROSTER /* Eina_List *Shotgun_User */
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
   SHOTGUN_USER_STATUS_AWAY,
   SHOTGUN_USER_STATUS_CHAT,
   SHOTGUN_USER_STATUS_DND,
   SHOTGUN_USER_STATUS_XA /* eXtended Away */
} Shotgun_User_Status;

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
   char *msg;
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
   Shotgun_User_Status status;

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
Eina_Bool shotgun_message_send(Shotgun_Auth *auth, const char *to, const char *msg);
Eina_Bool shotgun_presence_set(Shotgun_Auth *auth, Shotgun_User_Status st, const char *desc);

#ifdef __cplusplus
}
#endif

#endif
