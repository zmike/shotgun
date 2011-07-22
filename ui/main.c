#include "ui/ui.h"
#include "Shotgun.h"

int ui_log_dom = -1;

int
main(int argc, char *argv[])
{
   char *pass;
   Shotgun_Auth *auth;
   char *getpass_x(const char *prompt);

   if (argc != 3)
     {
        fprintf(stderr, "Usage: %s [username] [domain]\n", argv[0]);
        return 1;
     }

   eina_init();
   shotgun_init();
   
   ui_log_dom = eina_log_domain_register("shotgun_ui", EINA_COLOR_LIGHTRED);
   eina_log_domain_level_set("shotgun_ui", EINA_LOG_LEVEL_DBG);
   eina_log_domain_level_set("shotgun", EINA_LOG_LEVEL_DBG);
   //eina_log_domain_level_set("ecore_con", EINA_LOG_LEVEL_DBG);

   auth = shotgun_new(argv[1], argv[2]);
   pass = getpass_x("Password: ");
   if (!pass)
     {
        ERR("No password entered!");
        return 1;
     }
   shotgun_password_set(auth, pass);
   shotgun_gchat_connect(auth);
   contact_list_new(argc, argv);
   ecore_main_loop_begin();

   return 0;
}
