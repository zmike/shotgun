#include <sasl.h>

#include "shotgun_private.h"

/*
static sasl_callback_t callbacks[] =
{
  {SASL_CB_GETREALM, NULL, NULL},
  {SASL_CB_AUTHNAME, NULL, NULL},
  {SASL_CB_USER, NULL, NULL},
  {SASL_CB_PASS, NULL, NULL},
  {SASL_CB_LIST_END, NULL, NULL}
};
*/

static char gtalk_authtypes[] = "PLAIN X-GOOGLE-TOKEN X-OAUTH2";

static int _init = 0;
char *getpass_x(const char *prompt);

static int
sasl_cb_domain(Shotgun_Auth *auth, int id __UNUSED__, const char **sup __UNUSED__, const char **domain)
{
   *domain = auth->from;
   return SASL_OK;
}

static int
sasl_cb_name(Shotgun_Auth *auth, int id __UNUSED__, const char **name, unsigned int *len)
{
   *name = auth->from;
   if (len) *len = eina_stringshare_strlen(auth->from);
   return SASL_OK;
}

static int
sasl_cb_pass(sasl_conn_t *conn __UNUSED__, Shotgun_Auth *auth, int id __UNUSED__, sasl_secret_t **secret)
{
   char *pass;
   size_t len;

   pass = getpass_x("Password: ");
   if (!pass)
     {
        ERR("No password entered!");
        return SASL_BADPARAM;
     }
   len = strlen(pass);
   auth->secret = malloc(sizeof(sasl_secret_t) + len); /* couldn't just make it a const char *...*/
   strcpy((char*)auth->secret->data, pass);
   auth->secret->len = len;
   *secret = auth->secret;
   return SASL_OK;
}

char *
sasl_init(Shotgun_Auth *auth)
{
   sasl_interact_t *client_interact = NULL; /*< apparently sasl crashes without this */
   const char *out, *mechusing = NULL;
   unsigned outlen;
   sasl_callback_t cbs[4] =
{
   {SASL_CB_GETREALM, sasl_cb_domain, auth},
   {SASL_CB_AUTHNAME, sasl_cb_name, auth},
   //{SASL_CB_USER, NULL, NULL},
   {SASL_CB_PASS, sasl_cb_pass, auth},
   {SASL_CB_LIST_END, NULL, NULL},
};

   if (!_init)
     {
        //if (sasl_client_init(callbacks))
        if (sasl_client_init(NULL))
          return NULL;
     }
   _init++;

   memcpy(&auth->callbacks, cbs, sizeof(sasl_callback_t) * 4);
   sasl_client_new("xmpp", "talk.google.com", NULL, NULL, auth->callbacks, 0, &auth->sasl);
   sasl_client_start(auth->sasl, gtalk_authtypes, &client_interact, &out, &outlen, &mechusing);
   if (!out) return NULL;

   return shotgun_base64_encode(out, outlen);
}
