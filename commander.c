// Sebastian tries to write basic lxpanal plugins this example is from: (last
// time read on 28th dec 2020)
// https://wiki.lxde.org/en/How_to_write_plugins_for_LXPanel

/* build with:
gcc -Wall `pkg-config --cflags gtk+-2.0 lxpanel` -shared -fPIC commanderplugin.c
-o commander.so `pkg-config --libs lxpanel`

and copy to

/usr/lib/lxpanel/plugins

then restart the panel with:

lxpanelctl restart

make sure your file is copied, the panel has restarted

then add the new plugin to lxpanel
 */

#include <stdio.h>

#include <gtk-2.0/gtk/gtk.h>
#include <lxpanel/plugin.h>
//#include "./cmdspawn.h"


//////////////////// SPAWN CODE BEGIN
//#include <gtk-2.0/gtk/gtk.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <strings.h>
#include <string.h>
#include <poll.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>

/* the plugin's id – an instance of this struct
   is what will be assigned to 'priv' */
typedef struct {
    gint iMyId;
    gint idx;
    GtkWidget* textfield; /* Textwidget displaying the result*/
    guint timeout_value; /* Timeout every $value seconds updates the display*/
    guint timer; /* Timer for periodic udates */
    char* buffer; /* String buffer to hold results*/
    char* commandString; /* not used, set and run this command periodically*/
    config_setting_t* settings; /*lxpanels stores the settings for you, doesn't work for me*/
    /* because "commander.so is not a lxpanel plugin" */
    LXPanel *panel;
} CommanderPlugin;


// This spawn command originates from Xfce4's Genmon plugin and is Licensed under GPL
// 
/**********************************************************************/
char *genmon_Spawn (char **argv, int wait)
/**********************************************************************/
 /* Spawn a command and capture its output from stdout or stderr */
 /* Return allocated string on success, otherwise NULL */
{
    enum { OUT, ERR, OUT_ERR };
    enum { RD, WR, RD_WR };

    int             aaiPipe[OUT_ERR][RD_WR];
    pid_t           pid;
    struct pollfd   aoPoll[OUT_ERR];
    int             status;
    int             i, j, k;
    char           *str = NULL;

    if (!(*argv)) {
        fprintf (stderr, "Spawn() error: No parameters passed!\n");
        return (NULL);
    }
    for (i = 0; i < OUT_ERR; i++)
        pipe (aaiPipe[i]);
    switch (pid = fork ()) {
        case -1:
            perror ("fork()");
            for (i = 0; i < OUT_ERR; i++)
                for (j = 0; j < RD_WR; j++)
                    close (aaiPipe[i][j]);
            return (NULL);
        case 0:
            close(0); /* stdin is not used in child */
            /* Redirect stdout/stderr to associated pipe's write-ends */
            for (i = 0; i < OUT_ERR; i++) {
                j = i + 1; // stdout/stderr file descriptor
                close (j);
                k = dup2 (aaiPipe[i][WR], j);
                if (k != j) {
                    perror ("dup2()");
                    exit (-1);
                }
            }
        /* Execute the given command */
        execvp (argv[0], argv);
        perror (argv[0]);
        exit (-1);
    }

    for (i = 0; i < OUT_ERR; i++)
        close (aaiPipe[i][WR]); /* close write end of pipes in parent */

    /* Wait for child completion */
    if (wait == 1)
    {
        status = waitpid (pid, 0, 0);
        if (status == -1) {
            perror ("waitpid()");
            goto End;
        }

        /* Read stdout/stderr pipes' read-ends */
        for (i = 0; i < OUT_ERR; i++) {
            aoPoll[i].fd = aaiPipe[i][RD];
            aoPoll[i].events = POLLIN;
            aoPoll[i].revents = 0;
        }
        poll (aoPoll, OUT_ERR, ~0);
        for (i = 0; i < OUT_ERR; i++)
            if (aoPoll[i].revents & POLLIN)
                break;
        if (i == OUT_ERR)
            goto End;

        j = 0;
        while (1) {
            str = g_realloc (str, j + 256);
            if ((k = read (aaiPipe[i][RD], str + j, 255)) > 0)
                j += k;
            else
                break;
        }
        str[j] = 0;

        /* Remove trailing carriage return if any */
        i = strlen (str) - 1;
        if (i >= 0 && str[i] == '\n')
            str[i] = 0;
    }

    End:
    /* Close read end of pipes */
    for (i = 0; i < OUT_ERR; i++)
        close (aaiPipe[i][RD]);

    return (str);
}// Spawn()


/**********************************************************************/
char *genmon_SpawnCmd (const char *p_pcCmdLine, int wait)
/**********************************************************************/
 /* Parse a command line, spawn the command, and capture its output from stdout or stderr */
 /* Return allocated string on success, otherwise NULL */
{
    //printf("tried loading genmon_SpawnCmd\n");
    char          **argv;
    int             argc;
    GError         *error = NULL;
    char           *str;

    /* Split the commandline into an argv array */
    if (!g_shell_parse_argv (p_pcCmdLine, &argc, &argv, &error)) {
        char *first = g_strdup_printf (("Error in command \"%s\""), p_pcCmdLine);

        g_error_free (error);
        g_free (first);
        return (NULL);
    }

    /* Spawn the command and free allocated memory */
    str = genmon_Spawn (argv, wait);
    g_strfreev (argv);

    return (str);
}// SpawnCmd()

////////////////////7 SPAWN CODE END

// internal to the plugin source, not used by the 'priv' variable
static int iInstanceCount = 0;

// Continuing, let's define LXPanelPluginInit's 'new_instance' callback which
// creates a widget.


//update Value on every timeout
static gboolean run_cmd(CommanderPlugin * c) {
    //c->buffer = genmon_SpawnCmd("echo hi", 1);
    char* buf[1024];
    char* command_output;
    command_output = genmon_SpawnCmd("playerctl metadata --format \"{{ artist }}: {{ title }}\"", 1);
    //command_output = genmon_SpawnCmd(c->commandString, 1);
    int n = snprintf(buf, sizeof(buf), "%s\0", command_output);
    c->buffer = buf;
    //printf("helloo timeout\n");
    gtk_label_set_text(c->textfield, c->buffer);
    gtk_widget_set_size_request(c->textfield, n * 7 + 30, 25);

    return TRUE ;
}

GtkWidget *commander_constructor(LXPanel *panel, config_setting_t *settings) {
    /* panel is a pointer to the panel and
        settings is a pointer to the configuration data
        since we don't use it, we'll make sure it doesn't
        give us an error at compilation time */
    (void)panel;
    (void)settings;

    // allocate our private structure instance
    CommanderPlugin *pCommander = g_new0(CommanderPlugin, 1);

    // update the instance count
    pCommander->iMyId = ++iInstanceCount;
    pCommander->commandString = "";

    // make a label out of the ID
    char cIdBuf[1024] = {'\0'};

    int l = snprintf(cIdBuf, sizeof(cIdBuf),
                     "Command hasn't run yet %d", pCommander->iMyId);

    char* output_of_program;
    printf("before loading genmon_SpawnCmd\n");
    pCommander->buffer = "Command hasn't run yet\0"; // genmon_SpawnCmd("echo 'hihihihi'", 1);

    l = snprintf(cIdBuf, sizeof(cIdBuf),
                     "%s", pCommander->buffer);



    if (l > 1024 || l < 0) {
        printf("something bad has happend with the Monitor/Commander "
               "plugin\n");
    } else {
        printf(
            "Commander/Monitor it is good enough for me,\nwritten %d bytes\n",
            l);
    }


    // place private textfield of Plugin in GtkWidget
    GtkWidget *p = gtk_event_box_new();
    pCommander->textfield = gtk_label_new(pCommander->buffer);
    

    // our widget doesn't have a window...
    // it is usually illegal to call gtk_widget_set_has_window() from
    // application but for GtkEventBox it doesn't hurt

    gtk_widget_set_has_window(p, FALSE);

    // bind private structure to the widget assuming it should be freed using
    lxpanel_plugin_set_data(p, pCommander, g_free);

    gtk_container_add(GTK_CONTAINER(p), pCommander->textfield);

    // set the size we want
    gtk_widget_set_size_request(pCommander->textfield, 30 * 7, 25);
    gtk_container_add(GTK_CONTAINER(p), pCommander->textfield);
    gtk_widget_show(pCommander->textfield);
    pCommander->timer = g_timeout_add_seconds((guint)1, (GSourceFunc) run_cmd, (gpointer) pCommander);
    

    // success!!!
    return p;
}

/* static gboolean applyConfig(gpointer user_data) { */
/*     CommanderPlugin *p = lxpanel_plugin_get_data(user_data); */
/*     config_group_set_string(p->settings, "Command", p->commandString); */
/*     config_group_set_int(p->settings, "Timeout", p->timeout_value); */
/*     return False; */
/* } */

/* static GtkWidget *config(LXPanel *panel, GtkWidget *p) { */
/*     CommanderPlugin *cp = lxpanel_plugin_get_data(p); */
/*     return lxpanel_generic_config_dlg(_("Commander"), */
/* 	    panel, applyConfig, p, */
/*             _("Command"), &cp->commandString, CONF_TYPE_STR, */
/* 	    _("Timeout"), &cp->timeout_value, CONF_TYPE_INT, */
/*             NULL); */
/* } */

FM_DEFINE_MODULE(lxpanel_gtk, commander)

/* Plugin descriptor. */
LXPanelPluginInit fm_module_init_lxpanel_gtk = {
    // assigning our functions to provided pointers.
    .name = "CommanderPlugin",
    .description = "Run a commander plugin.",
    .new_instance = commander_constructor,
    //    .config = config
};
