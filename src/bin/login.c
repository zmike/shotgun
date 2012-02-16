#include "ui.h"

static void _login(Login_Window *lw);

#define LOGIN_FRAME(TITLE) \
   fr = elm_frame_add(win); \
   elm_object_text_set(fr, TITLE); \
   WEIGHT(fr, EVAS_HINT_EXPAND, 0); \
   FILL(fr); \
   elm_box_pack_end(lw->box, fr); \
\
   obj = elm_entry_add(win); \
   elm_entry_bounce_set(obj, EINA_FALSE, EINA_FALSE); \
   elm_entry_single_line_set(obj, EINA_TRUE); \
   elm_entry_scrollable_set(obj, EINA_FALSE); \
   EXPAND(obj); \
   FILL(obj); \
   elm_object_content_set(fr, obj); \
   evas_object_show(obj); \
   evas_object_show(fr)

static void
_login_window_key(Login_Window *lw, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, Evas_Event_Key_Down *ev)
{
   //DBG("%s", ev->keyname);
   if (!strcmp(ev->keyname, "Escape"))
     {
        if (!elm_flip_front_visible_get(lw->flip))
          settings_toggle((UI_WIN*)lw, NULL, NULL);
     }
   else if (!strcmp(ev->keyname, "q"))
     evas_object_del(lw->win);
   else if ((!strcmp(ev->keyname, "Return")) || (!strcmp(ev->keyname, "KP_Enter")))
     _login(lw);

}


static void
_login_window_focus(Login_Window *lw, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   const char *txt;

   txt = elm_entry_entry_get(lw->server);
   if ((!txt) || (!txt[0]))
     {
        elm_object_focus_set(lw->server, EINA_TRUE);
        return;
     }
   txt = elm_entry_entry_get(lw->domain);
   if ((!txt) || (!txt[0]))
     {
        elm_object_focus_set(lw->domain, EINA_TRUE);
        return;
     }
   txt = elm_entry_entry_get(lw->username);
   if ((!txt) || (!txt[0]))
     {
        elm_object_focus_set(lw->username, EINA_TRUE);
        return;
     }
   elm_object_focus_set(lw->password, EINA_TRUE);
}

static void
_login_window_free(Login_Window *lw, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   evas_object_key_ungrab(lw->win, "Return", 0, 0);
   evas_object_key_ungrab(lw->win, "KP_Enter", 0, 0);
   evas_object_event_callback_del_full(lw->win, EVAS_CALLBACK_KEY_DOWN, (Evas_Object_Event_Cb)_login_window_key, lw);
   evas_object_event_callback_del_full(lw->win, EVAS_CALLBACK_FREE, (Evas_Object_Event_Cb)_login_window_free, lw);
   evas_object_smart_callback_del(lw->win, "focus,in", (Evas_Smart_Cb)_login_window_focus);
   ecore_event_handler_del(lw->evh);
   ecore_event_handler_del(lw->evh2);
   evas_object_del(lw->box);
   if (lw->spinner) ecore_animator_del(lw->spinner);

   if (!shotgun_connection_state_get(lw->account))
     {/* quit directly from login screen */
        ui_eet_settings_set(lw->account, lw->settings);
        ui_eet_shutdown(lw->account);
        shotgun_free(lw->account);
     }
   free(lw);
}

static Eina_Bool
_login_complete(Login_Window *lw, int type __UNUSED__, Shotgun_Auth *auth __UNUSED__)
{
   _login_window_free(lw, NULL, NULL, NULL);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_login_state(Login_Window *lw, int type __UNUSED__, Shotgun_Auth *auth)
{
   const char *text;
   switch (shotgun_connection_state_get(auth))
     {
       case SHOTGUN_CONNECTION_STATE_TLS:
         text = "Performing SSL/TLS handshake...";
         break;
       case SHOTGUN_CONNECTION_STATE_FEATURES:
         text = "Determining Server features...";
         break;
       case SHOTGUN_CONNECTION_STATE_SASL:
         text = "SASL login...";
         break;
       case SHOTGUN_CONNECTION_STATE_BIND:
         text = "Binding resource...";
         break;
       case SHOTGUN_CONNECTION_STATE_SESSION:
         text = "Initiating new session...";
         break;
       case SHOTGUN_CONNECTION_STATE_FINALIZING:
         text = "Finishing up...";
       default:
         break;
     }
   elm_object_text_set(lw->label, text);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_login_spin(Login_Window *lw)
{
   int x, y, w, h;

   evas_object_geometry_get(lw->icon, &x, &y, &w, &h);
   evas_map_util_rotate(lw->icon_map, 1.5, x + (w / 2), y + (h / 2));
   return EINA_TRUE;
}

static void
_login(Login_Window *lw)
{
   const char *domain, *svr_name, *user, *pw;
   char *d, *s, *u, *p;
   int argc;
   Shotgun_Auth *auth = NULL;
   d = s = u = p = NULL;

   svr_name = elm_entry_entry_get(lw->server);
   if (svr_name && (!svr_name[0])) svr_name = NULL;
   domain = elm_entry_entry_get(lw->domain);
   if (domain && (!domain[0])) domain = NULL;
   user = elm_entry_entry_get(lw->username);
   if (user && (!user[0])) user = NULL;
   pw = elm_entry_entry_get(lw->password);
   if (pw && (!pw[0])) pw = NULL;
   if (domain) d = elm_entry_markup_to_utf8(domain);
   if (svr_name) s = elm_entry_markup_to_utf8(svr_name);
   if (user) u = elm_entry_markup_to_utf8(user);
   if (pw) p = elm_entry_markup_to_utf8(pw);

   argc = (!!s) + (!!d) + (!!u);
   switch (argc)
     {
      case 0:
        /* ERROR */
        break;
      case 1:
        if (!u)
          {
             /* ERROR */
             break;
          }
        auth = ui_eet_auth_get(u, NULL);
        break;
      case 2:
        if (!u)
          {
             /* ERROR */
             break;
          }
        if (d)
          auth = ui_eet_auth_get(u, d);
        else
          auth = ui_eet_auth_get(u, NULL);
        if (auth && s) /* possibly changed address */
          shotgun_servername_set(auth, s);
        break;
      case 3:
        if (!u)
          {
             /* ERROR */
             break;
          }
        if (d)
          auth = ui_eet_auth_get(u, d);
        if (!auth)
          auth = ui_eet_auth_get(u, NULL);
        if (auth && s) /* possibly changed address */
          shotgun_servername_set(auth, s);
        if (s && d && (!auth))
          auth = shotgun_new(s, u, d);
      default:
        break;
     }
   if (!auth)
     {
        /* ERROR */

        free(d);
        free(s);
        free(u);
        free(p);
        return;
     }

   if (p) shotgun_password_set(auth, p);

   free(d);
   free(s);
   free(u);
   free(p);
   if ((!shotgun_username_get(auth)) ||
       (!shotgun_password_get(auth)) ||
       (!shotgun_domain_get(auth)) ||
       (!shotgun_servername_get(auth)))
     {
        /* ERROR */
        return;
     }
   shotgun_data_set(auth, shotgun_data_get(lw->account));
   shotgun_settings_set(auth, shotgun_settings_get(lw->account));
   shotgun_settings_set(lw->account, NULL);
   shotgun_free(lw->account);
   lw->account = auth;
   elm_object_tree_unfocusable_set(lw->box, EINA_TRUE);
   elm_object_text_set(lw->label, "Connecting");
   lw->icon_map = evas_map_new(4);
   evas_map_util_points_populate_from_object(lw->icon_map, lw->icon);
   evas_object_map_set(lw->icon, lw->icon_map);
   evas_object_map_enable_set(lw->icon, EINA_TRUE);
   lw->spinner = ecore_animator_add((Ecore_Task_Cb)_login_spin, lw);
   shotgun_ssl_verify_set(auth, ssl_verify);
   shotgun_connect(auth);
}

void
login_new(void)
{
   Evas_Object *win, *obj, *fr, *ic;
   Login_Window *lw;

   lw = calloc(1, sizeof(Login_Window));
   lw->account = shotgun_new(NULL, NULL, NULL);
   lw->evh = ecore_event_handler_add(SHOTGUN_EVENT_CONNECTION_STATE, (Ecore_Event_Handler_Cb)_login_state, lw);
   lw->evh2 = ecore_event_handler_add(SHOTGUN_EVENT_CONNECT, (Ecore_Event_Handler_Cb)_login_complete, lw);
   lw->type = 1;

   if (!ui_eet_init(lw->account))
     {
        CRI("Could not initialize eet backend!");
        ecore_main_loop_quit();
        return;
     }

   ui_win_init((UI_WIN*)lw);
   win = lw->win;

   elm_win_title_set(win, "Login");
   //1 | evas_object_key_grab(win, "Tab", 0, 0, 1); /* worst warn_unused ever. */
   1 | evas_object_key_grab(win, "Return", 0, 0, 1); /* worst warn_unused ever. */
   1 | evas_object_key_grab(win, "KP_Enter", 0, 0, 1); /* worst warn_unused ever. */
   evas_object_event_callback_add(win, EVAS_CALLBACK_FREE, (Evas_Object_Event_Cb)_login_window_free, lw);
   evas_object_event_callback_add(win, EVAS_CALLBACK_KEY_DOWN, (Evas_Object_Event_Cb)_login_window_key, lw);
   evas_object_smart_callback_add(win, "focus,in", (Evas_Smart_Cb)_login_window_focus, lw);

   ic = elm_icon_add(win);
   elm_icon_standard_set(ic, "forward");
   evas_object_show(ic);
   obj = elm_button_add(win);
   elm_object_tooltip_text_set(obj, "Settings");
   elm_object_tooltip_window_mode_set(obj, EINA_TRUE);
   elm_object_content_set(obj, ic);
   ALIGN(obj, 1, 0);
   elm_box_pack_end(lw->box, obj);
   evas_object_smart_callback_add(obj, "clicked", (Evas_Smart_Cb)settings_toggle, lw);
   evas_object_show(obj);

   lw->icon = obj = elm_icon_add(win);
   EXPAND(obj);
   FILL(obj);
   elm_icon_scale_set(obj, EINA_TRUE, EINA_FALSE);
   elm_icon_prescale_set(obj, 5);
   elm_box_pack_end(lw->box, obj);
   elm_icon_standard_set(obj, "shotgun/userunknown");
   evas_object_show(obj);

   lw->label = obj = elm_label_add(win);
   elm_box_pack_end(lw->box, obj);
   evas_object_show(obj);

   LOGIN_FRAME("Server:");
   elm_entry_entry_set(obj, "talk.google.com");
   elm_entry_select_all(obj);
   lw->server = obj;
   LOGIN_FRAME("Domain:");
   elm_entry_entry_set(obj, "gmail.com");
   lw->domain = obj;
   LOGIN_FRAME("Username:");
   lw->username = obj;
   LOGIN_FRAME("Password:");
   lw->password = obj;
   elm_entry_password_set(obj, EINA_TRUE);
   elm_object_focus_set(lw->server, EINA_TRUE);

   evas_object_resize(win, 300, 600);
   if (!lw->settings->settings_exist)
     elm_win_center(win, EINA_TRUE, EINA_TRUE);
   evas_object_show(win);
   return;
}
