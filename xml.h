#ifndef SHOTGUN_XML_H
#define SHOTGUN_XML_H

#include "shotgun_private.h"

#ifdef __cplusplus
extern "C" {
#endif

char *xml_stream_init_create(const char *from, const char *to, const char *lang, int64_t *len);
Eina_Bool xml_stream_init_read(Shotgun_Auth *auth, char *xml, size_t size);
const char *xml_starttls_write(int64_t *size);
Eina_Bool xml_starttls_read(char *xml, size_t size);

#ifdef __cplusplus
}
#endif

#endif
