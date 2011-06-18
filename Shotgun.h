#ifndef SHOTGUN_H
#define SHOTGUN_H

extern int SHOTGUN_EVENT_IQ;
extern int SHOTGUN_EVENT_MESSAGE;
extern int SHOTGUN_EVENT_PRESENCE;

typedef struct Shotgun_Auth Shotgun_Auth;

typedef enum
{
   SHOTGUN_IQ_EVENT_TYPE_UNKNOWN,
   SHOTGUN_IQ_EVENT_TYPE_ROSTER
} Shotgun_Iq_Event_Type;

typedef enum
{
   SHOTGUN_USER_SUBSCRIPTION_NONE,
   SHOTGUN_USER_SUBSCRIPTION_TO,
   SHOTGUN_USER_SUBSCRIPTION_FROM,
   SHOTGUN_USER_SUBSCRIPTION_BOTH
} Shotgun_User_Subscription;

typedef struct
{
   const char *jid;
   const char *name;
   Shotgun_User_Subscription subscription;
   Shotgun_Auth *account;
} Shotgun_User;

typedef struct
{
   const char *user;
   char *msg;
   Shotgun_Auth *account;
} Shotgun_Message;

typedef struct
{
   Shotgun_Iq_Event_Type type;
   void *ev;
   Shotgun_Auth *account;
} Shotgun_Event_Iq;

#ifdef __cplusplus
extern "C" {
#endif

Eina_Bool shotgun_iq_roster_get(Shotgun_Auth *auth);
Shotgun_Message *shotgun_message_new(Shotgun_Auth *auth);

#ifdef __cplusplus
}
#endif

#endif
