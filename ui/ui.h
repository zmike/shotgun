#ifndef __UI_H
#define __UI_H

#include <Shotgun.h>
#include <Ecore.h>

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

void contact_list_new(void);

#endif /* __UI_H */
