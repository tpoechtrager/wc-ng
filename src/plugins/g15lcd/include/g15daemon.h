/*
    This file is part of g15daemon.

    g15daemon is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    g15daemon is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with g15daemon; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    
    (c) 2006-2008 Mike Lampard, Philip Lawatsch, and others
          
     $Revision: 441 $ -  $Date: 2008-01-22 20:39:21 +1030 (Tue, 22 Jan 2008) $ $Author: mlampard $
              
    This daemon listens on localhost port 15550 for client connections,
    and arbitrates LCD display.  Allows for multiple simultaneous clients.
    Client screens can be cycled through by pressing the 'L1' key.
*/
#ifndef G15DAEMON
#define G15DAEMON

#ifndef BLACKnWHITE
#define BLACK 1
#define WHITE 0
#define BLACKnWHITE 
#endif

#define LCD_WIDTH 160
#define LCD_HEIGHT 43
#define LCD_BUFSIZE 1048

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pthread.h>
#include <pwd.h>
#include <syslog.h> 

#define CLIENT_CMD_GET_KEYSTATE 'k'
#define CLIENT_CMD_SWITCH_PRIORITIES 'p'
#define CLIENT_CMD_NEVER_SELECT 'n'
#define CLIENT_CMD_IS_FOREGROUND 'v'
#define CLIENT_CMD_IS_USER_SELECTED 'u'
#define CLIENT_CMD_BACKLIGHT 0x80
#define CLIENT_CMD_KB_BACKLIGHT 0x8
#define CLIENT_CMD_CONTRAST 0x40
#define CLIENT_CMD_MKEY_LIGHTS 0x20
/* if the following CMD is sent from a client, G15Daemon will not send any MR or G? keypresses via uinput, 
 * all M&G keys must be handled by the client.  If the client dies or exits, normal functions resume. */
#define CLIENT_CMD_KEY_HANDLER 0x10

enum {
    /* plugin types - LCD plugins are provided with a lcd_t and keystates via EVENT when visible */
/* CORE plugins source and sink events, have no screen associated, and are not able to quit.
    by design they implement core functionality..CORE_LIBRARY implements graphic and other 
    functions for use by other plugins.
*/
    G15_PLUGIN_NONE = 0,
    G15_PLUGIN_LCD_CLIENT =1,
    G15_PLUGIN_CORE_KB_INPUT = 2,
    G15_PLUGIN_CORE_OS_KB = 3,
    G15_PLUGIN_LCD_SERVER = 4
};

enum {
    /* plugin RETURN values */
    G15_PLUGIN_QUIT = -1,
    G15_PLUGIN_OK = 0
};

enum {
    /* plugin EVENT types */
    G15_EVENT_KEYPRESS = 1,
    G15_EVENT_VISIBILITY_CHANGED,
    G15_EVENT_USER_FOREGROUND,
    G15_EVENT_MLED,
    G15_EVENT_BACKLIGHT,
    G15_EVENT_CONTRAST,
    G15_EVENT_REQ_PRIORITY,
    G15_EVENT_CYCLE_PRIORITY,
    G15_EVENT_EXITNOW,
    /* core event types */
    G15_COREVENT_KEYPRESS_IN,
    G15_COREVENT_KEYPRESS_OUT
};

enum {
    SCR_HIDDEN = 0,
    SCR_VISIBLE
};

/* plugin global or local */
enum {
    G15_PLUGIN_NONSHARED = 0,
    G15_PLUGIN_SHARED
};

typedef struct lcd_s 		lcd_t;
typedef struct g15daemon_s 	g15daemon_t;
typedef struct lcdnode_s 	lcdnode_t;

typedef struct plugin_event_s 	plugin_event_t;
typedef struct plugin_info_s 	plugin_info_t;
typedef struct plugin_s 	plugin_t;

typedef struct config_items_s	config_items_t;
typedef struct config_section_s config_section_t;
typedef struct configfile_s 	configfile_t;

typedef struct config_items_s
{
    config_items_t *next;
    config_items_t *prev;
    config_items_t *head;
    char *key;
    char *value;
} config_items_s;

typedef struct config_section_s
{
    config_section_t *head;
    config_section_t *next;
    char *sectionname;
    config_items_t *items;
}config_section_s;

typedef struct configfile_s
{
    config_section_t *sections;
}configfile_s;

typedef struct plugin_info_s 
{
    /* type - see above for valid defines*/
    int type;
    /* short name of the plugin - used only for logging at the moment */
    char *name;
    /* run thread - will be called every update_msecs milliseconds*/
    int *(*plugin_run) (void *);
    unsigned int update_msecs;
    /* plugin process to be called on close or NULL if there isnt one*/
    void *(*plugin_exit) (void *);
    /* plugin process to be called on EVENT (such as keypress)*/
    int *(*event_handler) (void *);
    /* init func if there is one else NULL*/
    int *(*plugin_init) (void *);
    char *filename;
} plugin_info_s;

typedef struct plugin_s 
{
    g15daemon_t *masterlist;
    unsigned int type;
    plugin_info_t *info;
    void *plugin_handle;
    void *args;
} plugin_s;

typedef struct lcd_s
{
    g15daemon_t *masterlist;
    int lcd_type;
    unsigned char buf[LCD_BUFSIZE];
    int max_x;
    int max_y;
    int connection;
    unsigned int backlight_state;
    unsigned int mkey_state;
    unsigned int contrast_state;
    unsigned int state_changed;
    /* set to 1 if user manually selected this screen 0 otherwise */
    unsigned int usr_foreground;
    /* set to 1 if screen is never to be user-selectable */
    unsigned int never_select;
    /* only used for plugins */
    plugin_t *g15plugin;
    
} lcd_s;


typedef struct plugin_event_s
{
    unsigned int event;
    unsigned long value;
    lcd_t *lcd;
} plugin_event_s;


struct lcdnode_s {
    g15daemon_t *list;
    lcdnode_t *prev;
    lcdnode_t *next;
    lcdnode_t *last_priority;
    lcd_t *lcd;
}lcdnode_s;

struct g15daemon_s
{
    lcdnode_t *head;
    lcdnode_t *tail;
    lcdnode_t *current;
    void *(*keyboard_handler)(void*);
    struct passwd *nobody;
    volatile unsigned long numclients;
    configfile_t *config;
    unsigned int kb_backlight_state; // master state
    unsigned int remote_keyhandler_sock;
}g15daemon_s;

pthread_mutex_t lcdlist_mutex;
pthread_mutex_t g15lib_mutex;

/* server hello */
#define SERV_HELO "G15 daemon HELLO"

#ifdef G15DAEMON_BUILD
/* internal g15daemon-only functions */
void g15daemon_init_refresh();
void g15daemon_quit_refresh();
int uf_write_buf_to_g15(lcd_t *lcd);
/* write a pbm format file 'filename' with image contained in 'buf' */
int uf_screendump_pbm(unsigned char *buf,char *filename);
int uf_read_keypresses(unsigned int *keypresses, unsigned int timeout);
/* return the pid of a running copy of g15daemon, else -1 */
int uf_return_running();
/* create a /var/run/g15daemon.pid file, returning 0 on success else -1 */
int uf_create_pidfile();
/* open & run all plugins in the given directory */
int g15_open_all_plugins(g15daemon_t *masterlist, char *plugin_directory);
/* linked lists */
g15daemon_t *ll_lcdlist_init();
void ll_lcdlist_destroy(g15daemon_t **masterlist);

/* open and parse config file */
int uf_conf_open(g15daemon_t *list, char *filename);
/* write the config file with all keys/sections */
int uf_conf_write(g15daemon_t *list,char *filename);
/* free all memory used by the config subsystem */
void uf_conf_free(g15daemon_t *list);
/* search the list for valid key called "key" in section named "section" return pointer to item or NULL */
config_items_t* uf_search_confitem(config_section_t *section, char *key);
/* generic handler for net clients */
int internal_generic_eventhandler(plugin_event_t *myevent);
#endif

/* the following functions are available for use by plugins */
/* send notification of LCD buffer update */
void g15daemon_send_refresh(lcd_t *lcd);
/* wait on notification of LCD buffer update */
void g15daemon_wait_refresh();
/* create a new section */
config_section_t *g15daemon_cfg_load_section(g15daemon_t *masterlist,char *name);
/* return string value from key in sectionname */
char* g15daemon_cfg_read_string(config_section_t *section, char *key, char *defaultval);
/* return float from key in sectionname */
double g15daemon_cfg_read_float(config_section_t *section, char *key, double defaultval);
/* return int from key in sectionname */
int g15daemon_cfg_read_int(config_section_t *section, char *key, int defaultval);
/* return bool as int from key in sectionname */
int g15daemon_cfg_read_bool(config_section_t *section, char *key, int defaultval);
/* add a new key, or update the value of an already existing key, or return -1 if section doesnt exist */
int g15daemon_cfg_write_string(config_section_t *section, char *key, char *val);
int g15daemon_cfg_write_float(config_section_t *section, char *key, double val);
int g15daemon_cfg_write_int(config_section_t *section, char *key, int val);
/* simply write value as On or Off depending on whether val>0 */
int g15daemon_cfg_write_bool(config_section_t *section, char *key, unsigned int val);
/* remoe a key/value pair from named section */
int g15daemon_cfg_remove_key(config_section_t *section, char *key);

/* send event to foreground client's eventlistener */
int g15daemon_send_event(void *caller, unsigned int event, unsigned long value);
/* open named plugin */
void * g15daemon_dlopen_plugin(char *name,unsigned int library);
/* close plugin with handle <handle> */
int g15daemon_dlclose_plugin(void *handle) ;
/* syslog wrapper */
int g15daemon_log (int priority, const char *fmt, ...);
/* cycle from displayed screen to next on list */
void g15daemon_lcdnode_cycle(g15daemon_t *masterlist);
/* add new screen */
lcdnode_t *g15daemon_lcdnode_add(g15daemon_t **masterlist) ;
/* remove screen */
void g15daemon_lcdnode_remove (lcdnode_t *oldnode);

/* handy function from xine_utils.c */
void *g15daemon_xmalloc(size_t size) ;
/* threadsafe sleep */
void g15daemon_sleep(int seconds);
/* threadsafe millisecond sleep */
int g15daemon_msleep(int milliseconds) ;
/* return current time in milliseconds */
unsigned int g15daemon_gettime_ms();
/* convert 1byte/pixel buffer to internal g15 format */
void g15daemon_convert_buf(lcd_t *lcd, unsigned char * orig_buf);

#endif
