#include "ui.h"

int ui_log_dom = -1;
static Ecore_Event_Handler *dh = NULL;
static Ecore_Event_Handler *ch = NULL;

static Eina_Bool
con_state(void *d __UNUSED__, int type __UNUSED__, Shotgun_Auth *auth __UNUSED__)
{
   INF("Connection state++");
   return ECORE_CALLBACK_RENEW;
}


static Eina_Bool
disc(Contact_List *cl, int type __UNUSED__, Shotgun_Auth *auth __UNUSED__)
{
   Eina_List *l;
   Contact *c;
   INF("Disconnected");
   if (!cl)
     {
        ecore_main_loop_quit();
        return ECORE_CALLBACK_RENEW;
     }
   EINA_LIST_FOREACH(cl->users_list, l, c)
     contact_presence_clear(c);
   if (cl->pager)
     {
        evas_object_del(cl->pager);
        cl->pager_entries = eina_list_free(cl->pager_entries);
        cl->pager = NULL;
        cl->pager_state = 0;
     }
   shotgun_connect(cl->account);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
con(void *data, int type __UNUSED__, Shotgun_Auth *auth)
{
   Contact_List *cl = data;
   Shotgun_Settings *ss;
   INF("Connected!");
   /* don't mess up already-created list */
   if (!cl)
     {
        shotgun_presence_set(auth, SHOTGUN_USER_STATUS_CHAT, "testing SHOTGUN!", 1);
        ss = ui_eet_settings_get(auth);
        cl = contact_list_new(auth, ss);
        free(ss);
        if (!cl->settings.disable_reconnect)
          ecore_event_handler_data_set(dh, cl);
        ecore_event_handler_data_set(ch, cl);
        logging_dir_create(cl);
        if (cl->settings.allowed_image_age) ui_eet_idler_start(cl);
#ifdef HAVE_DBUS
        ui_dbus_init(cl);
#endif
#ifdef HAVE_AZY
        ui_azy_init(cl);
        if (ui_azy_connect(cl))
          ecore_timer_add(24 * 60 * 60, (Ecore_Task_Cb)ui_azy_connect, cl);
#endif
     }
   shotgun_iq_roster_get(auth);
   shotgun_presence_send(auth);

   return ECORE_CALLBACK_RENEW;
}

static void
_setup_extension(void)
{
   elm_theme_extension_add(NULL, PACKAGE_DATA_DIR "/default.edj");
}

int
main(int argc, char *argv[])
{
   char *pass;
   Shotgun_Auth *auth;
   char *getpass_x(const char *prompt);

   eina_init();
   ecore_app_args_set(argc, (const char**)argv);
   shotgun_init();
   elm_init(argc, argv);

   ui_log_dom = eina_log_domain_register("shotgun_ui", EINA_COLOR_LIGHTRED);
   eina_log_domain_level_set("shotgun_ui", EINA_LOG_LEVEL_DBG);
   if (!ecore_con_ssl_available_get())
     {
        CRI("SSL support is required in ecore!");
        exit(1);
     }
   _setup_extension();
   //eina_log_domain_level_set("ecore_con", EINA_LOG_LEVEL_DBG);
   ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA, (Ecore_Event_Handler_Cb)chat_image_data, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE, (Ecore_Event_Handler_Cb)chat_image_complete, NULL);
   ch = ecore_event_handler_add(SHOTGUN_EVENT_CONNECT, (Ecore_Event_Handler_Cb)con, NULL);
   ecore_event_handler_add(SHOTGUN_EVENT_CONNECTION_STATE, (Ecore_Event_Handler_Cb)con_state, NULL);
   dh = ecore_event_handler_add(SHOTGUN_EVENT_DISCONNECT, (Ecore_Event_Handler_Cb)disc, NULL);

   if (argc < 3)
     auth = ui_eet_auth_get(NULL, NULL);
   else if (argc == 3)
     auth = ui_eet_auth_get(argv[1], argv[2]);
   else
     auth = shotgun_new(argv[1], argv[2], argv[3]);

   if (!auth)
     {
        fprintf(stderr, "Usage: %s [server] [username] [domain]\n", argv[0]);
        fprintf(stderr, "Usage example: %s talk.google.com my_username gmail.com\n", argv[0]);
        fprintf(stderr, "Usage example (with saved account): %s\n", argv[0]);
        return 1;
     }

   if (!shotgun_password_get(auth))
     {
        pass = getpass_x("Password: ");
        if (!pass)
          {
             ERR("No password entered!");
             return 1;
          }
        shotgun_password_set(auth, pass);
     }
   if (!ui_eet_init(auth))
     {
        CRI("Could not initialize eet backend!");
        return 1;
     }
   shotgun_connect(auth);
   ecore_main_loop_begin();

   elm_shutdown();
   ui_eet_shutdown(auth);

   return 0;
}
