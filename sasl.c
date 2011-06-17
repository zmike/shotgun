#include "shotgun_private.h"

char *
sasl_init(Shotgun_Auth *auth)
{
   Eina_Binbuf *buf;
   char *ret;

   buf = eina_binbuf_new();

   eina_binbuf_append_char(buf, 0);
   eina_binbuf_append_length(buf, auth->user, eina_stringshare_strlen(auth->user));
   eina_binbuf_append_char(buf, 0);
   eina_binbuf_append_length(buf, auth->pass, strlen(auth->pass));
   ret = shotgun_base64_encode(eina_binbuf_string_get(buf), eina_binbuf_length_get(buf));
   eina_binbuf_free(buf);
   return ret;
}
