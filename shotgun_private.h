#ifndef SHOTGUN_PRIVATE_H
#define SHOTGUN_PRIVATE_H

#ifndef __UNUSED__
# define __UNUSED__ __attribute__((unused))
#endif

#include <Ecore_Con.h>
#include "Shotgun.h"

#define DBG(...)            EINA_LOG_DOM_DBG(shotgun_log_dom, __VA_ARGS__)
#define INF(...)            EINA_LOG_DOM_INFO(shotgun_log_dom, __VA_ARGS__)
#define WRN(...)            EINA_LOG_DOM_WARN(shotgun_log_dom, __VA_ARGS__)
#define ERR(...)            EINA_LOG_DOM_ERR(shotgun_log_dom, __VA_ARGS__)
#define CRI(...)            EINA_LOG_DOM_CRIT(shotgun_log_dom, __VA_ARGS__)

typedef enum
{
   SHOTGUN_STATE_NONE,
   SHOTGUN_STATE_TLS,
   SHOTGUN_STATE_FEATURES,
   SHOTGUN_STATE_SASL,
   SHOTGUN_STATE_BIND,
   SHOTGUN_STATE_CONNECTING,
   SHOTGUN_STATE_CONNECTED
} Shotgun_State;

typedef enum
{
   SHOTGUN_DATA_TYPE_UNKNOWN,
   SHOTGUN_DATA_TYPE_MSG,
   SHOTGUN_DATA_TYPE_IQ,
   SHOTGUN_DATA_TYPE_PRES
} Shotgun_Data_Type;

typedef enum
{
   SHOTGUN_IQ_TYPE_GET,
   SHOTGUN_IQ_TYPE_SET,
   SHOTGUN_IQ_TYPE_RESULT,
   SHOTGUN_IQ_TYPE_ERROR
} Shotgun_Iq_Type;


/* pre-formatted xml */
typedef enum
{
   SHOTGUN_IQ_PRESET_BIND,
   SHOTGUN_IQ_PRESET_ROSTER
} Shotgun_Iq_Preset;

struct Shotgun_Auth
{
   const char *from; /* domain name of account */
   const char *user; /* username */
   const char *resource; /* identifier for "location" of user */
   const char *bind; /* full JID from xmpp:bind */
   const char *jid; /* bare JID */

   const char *pass; /* NOT ALLOCATED! */

   Eina_Strbuf *buf;

   Ecore_Con_Server *svr;

   struct
   {  /* this serves no real purpose */
      Eina_Bool starttls : 1;
      Eina_Bool sasl : 1;
   } features;
   Shotgun_State state;
};

extern int shotgun_log_dom;

#ifdef __cplusplus
extern "C" {
#endif
static inline void
shotgun_write(Ecore_Con_Server *svr, const void *data, size_t size)
{
   DBG("Sending:\n%s", (char*)data);
   ecore_con_server_send(svr, data, size);
}

static inline void
shotgun_fake_free(void *d __UNUSED__, void *d2 __UNUSED__)
{}

void shotgun_message_feed(Shotgun_Auth *auth, char *data, size_t size);
Shotgun_Event_Message *shotgun_message_new(Shotgun_Auth *auth);

void shotgun_iq_feed(Shotgun_Auth *auth, char *data, size_t size);

Shotgun_Event_Presence *shotgun_presence_new(Shotgun_Auth *auth);
void shotgun_presence_feed(Shotgun_Auth *auth, char *data, size_t size);

char *shotgun_base64_encode(const unsigned char *string, double len, size_t *size);
unsigned char *shotgun_base64_decode(const char *string, int len, size_t *size);

Eina_Bool shotgun_login_con(Shotgun_Auth *auth, int type, Ecore_Con_Event_Server_Add *ev);
void shotgun_login(Shotgun_Auth *auth, Ecore_Con_Event_Server_Data *ev);

#ifdef __cplusplus
}
#endif
#endif
