#ifndef SHOTGUN_PRIVATE_H
#define SHOTGUN_PRIVATE_H

#ifndef __UNUSED__
# define __UNUSED__ __attribute__((unused))
#endif

typedef enum
{
   SHOTGUN_STATE_NONE,
   SHOTGUN_STATE_FEATURES,
   SHOTGUN_STATE_TLS,
   SHOTGUN_STATE_INIT
} Shotgun_State;

typedef struct
{
   const char *from;
   struct
   {
      Eina_Bool starttls : 1;
      Eina_Bool sasl : 1;
   } features;
   Shotgun_State state;
} Shotgun_Auth;

extern int shotgun_log_dom;

#define DBG(...)            EINA_LOG_DOM_DBG(shotgun_log_dom, __VA_ARGS__)
#define INF(...)           EINA_LOG_DOM_INFO(shotgun_log_dom, __VA_ARGS__)
#define WRN(...)           EINA_LOG_DOM_WARN(shotgun_log_dom, __VA_ARGS__)
#define ERR(...)            EINA_LOG_DOM_ERR(shotgun_log_dom, __VA_ARGS__)
#define CRI(...)            EINA_LOG_DOM_CRIT(shotgun_log_dom, __VA_ARGS__)
#endif
