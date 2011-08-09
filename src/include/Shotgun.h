#ifndef SHOTGUN_H
#define SHOTGUN_H

#include <Eina.h>

#define SHOTGUN_DBUS_INTERFACE "org.shotgun"
#define SHOTGUN_DBUS_PATH "/org/shotgun/remote"
#define SHOTGUN_DBUS_METHOD_BASE "org.shotgun"
/**
 * DBUS API:
 *
 * @brief Disconnect Shotgun
 * void org.shotgun.core.quit(void)
 *
 * @brief Retrieve the string array of online contact JIDs (username@server.com)
 * @return An array of strings containing all online contacts from the user's contact list
 * Array<String> org.shotgun.list.get(void)
 *
 * @brief Retrieve the string array of all contact JIDs (username@server.com)
 * @return An array of strings containing all contacts from the user's contact list
 * Array<String> org.shotgun.list.get(void)
 *
 * @brief Retrieve the full status of a contact's current presence (based on priority)
 * @return The contact's status message (if set)
 * @param JID The contact's JID
 * @param st The contact's status on return
 * @param priority The contact's priority on return
 * String org.shotgun.contact.status(String JID, Shotgun_User_Status *st, int *priority)
 *
 * @brief Retrieve the contact's icon's eet key
 * @return The key to use for retrieving the eet key of the icon belonging to contact represented
 * by @p JID
 * @param JID The contact's JID
 * String org.shotgun.contact.icon(String JID)
 *
 * @brief Send a message and message status to a contact
 * @return TRUE on successful send, else FALSE
 * @param JID The contact's JID
 * @param msg The message to send
 * @param st The (optional) #Shotgun_Message_Status to set along with the message
 * Bool org.shotgun.contact.send(String JID, String msg, Shotgun_Message_Status st)
 *
 * @brief Send a message and message status to a contact, echoing the message to
 * the contact's chat window
 * @return TRUE on successful send, else FALSE
 * @param JID The contact's JID
 * @param msg The message to send
 * @param st The (optional) #Shotgun_Message_Status to set along with the message
 * Bool org.shotgun.contact.send_echo(String JID, String msg, Shotgun_Message_Status st)
*/

extern int SHOTGUN_EVENT_CONNECT; /* Shotgun_Auth */
extern int SHOTGUN_EVENT_CONNECTION_STATE;
extern int SHOTGUN_EVENT_DISCONNECT; /* Shotgun_Auth */
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

typedef enum
{
   SHOTGUN_CONNECTION_STATE_NONE,
   SHOTGUN_CONNECTION_STATE_TLS,
   SHOTGUN_CONNECTION_STATE_FEATURES,
   SHOTGUN_CONNECTION_STATE_SASL,
   SHOTGUN_CONNECTION_STATE_BIND,
   SHOTGUN_CONNECTION_STATE_CONNECTING,
   SHOTGUN_CONNECTION_STATE_CONNECTED
} Shotgun_Connection_State;

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
        const char *sha1;
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
   const char *photo;
   char *description;
   int priority;
   Shotgun_User_Status status;
   Eina_Bool vcard : 1;

   Shotgun_Auth *account;
} Shotgun_Event_Presence;

#ifdef __cplusplus
extern "C" {
#endif

int shotgun_init(void);
Eina_Bool shotgun_connect(Shotgun_Auth *auth);
void shotgun_disconnect(Shotgun_Auth *auth);

Shotgun_Auth *shotgun_new(const char *svr_name, const char *username, const char *domain);
void shotgun_free(Shotgun_Auth *auth);
Shotgun_Connection_State shotgun_connection_state_get(Shotgun_Auth *auth);
const char *shotgun_username_get(Shotgun_Auth *auth);
const char *shotgun_password_get(Shotgun_Auth *auth);
const char *shotgun_domain_get(Shotgun_Auth *auth);
const char *shotgun_servername_get(Shotgun_Auth *auth);
const char *shotgun_jid_get(Shotgun_Auth *auth);
void shotgun_data_set(Shotgun_Auth *auth, void *data);
void *shotgun_data_get(Shotgun_Auth *auth);

/**
 * DOES NOT ALLOCATE FOR PASSWORD.
 */
void shotgun_password_set(Shotgun_Auth *auth, const char *password);
void shotgun_password_del(Shotgun_Auth *auth);

Eina_Bool shotgun_iq_roster_get(Shotgun_Auth *auth);
Eina_Bool shotgun_iq_vcard_get(Shotgun_Auth *auth, const char *user);

Eina_Bool shotgun_message_send(Shotgun_Auth *auth, const char *to, const char *msg, Shotgun_Message_Status status);

Shotgun_User_Status shotgun_presence_status_get(Shotgun_Auth *auth);
void shotgun_presence_status_set(Shotgun_Auth *auth, Shotgun_User_Status status);
int shotgun_presence_priority_get(Shotgun_Auth *auth);
void shotgun_presence_priority_set(Shotgun_Auth *auth, int priority);
const char *shotgun_presence_desc_get(Shotgun_Auth *auth);
void shotgun_presence_desc_set(Shotgun_Auth *auth, const char *desc);
void shotgun_presence_desc_manage(Shotgun_Auth *auth, char *desc);
void shotgun_presence_set(Shotgun_Auth *auth, Shotgun_User_Status st, const char *desc, int priority);
const char *shotgun_presence_get(Shotgun_Auth *auth, Shotgun_User_Status *st, int *priority);
Eina_Bool shotgun_presence_send(Shotgun_Auth *auth);

void shotgun_event_message_free(Shotgun_Event_Message *msg);
void shotgun_event_presence_free(Shotgun_Event_Presence *pres);
void shotgun_user_info_free(Shotgun_User_Info *info);
void shotgun_user_free(Shotgun_User *user);

#ifdef __cplusplus
}
#endif

#endif
