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
    
    $Revision: 410 $ -  $Date: 2008-01-13 13:28:55 +1030 (Sun, 13 Jan 2008) $ $Author: mlampard $

    This daemon listens on localhost port 15550 for client connections,
    and arbitrates LCD display.  Allows for multiple simultaneous clients.
    Client screens can be cycled through by pressing the 'L1' key.
*/
#ifdef __cplusplus
extern "C"
{
#endif

#define G15_WIDTH 160
#define G15_HEIGHT 43

#define G15_BUFSIZE 6880
#define G15DAEMON_VERSION g15daemon_version()

#define G15_PIXELBUF 0
#define G15_TEXTBUF 1
#define G15_WBMPBUF 2
#define G15_G15RBUF 3
#define G15_SHMRBUF 4

/* client / server commands - see README.devel for details on use */
 #define G15DAEMON_KEY_HANDLER 0x10
 #define G15DAEMON_MKEYLEDS 0x20
 #define G15DAEMON_CONTRAST 0x40
 #define G15DAEMON_BACKLIGHT 0x80
 #define G15DAEMON_KB_BACKLIGHT 0x8
 #define G15DAEMON_GET_KEYSTATE 'k'
 #define G15DAEMON_SWITCH_PRIORITIES 'p'
 #define G15DAEMON_IS_FOREGROUND 'v'
 #define G15DAEMON_IS_USER_SELECTED 'u'
 #define G15DAEMON_NEVER_SELECT 'n' 

const char *g15daemon_version();

/* open a new connection to the g15daemon.  returns an fd to be used with g15_send & g15_recv */
/* screentype should be either 0 (graphic/pixelbuffer) or 1 (textbuffer). only Graphic buffers are
   supported in this version */
int new_g15_screen(int screentype);

/* close connection - just calls close() */
int g15_close_screen(int sock);

/* these two functions operate in the same way as send & recv, except they wont return 
 * until _all_ len bytes are sent or received, or an error occurs */
int g15_send(int sock, char *buf, unsigned int len);
int g15_recv(int sock, char *buf, unsigned int len);

/* send a command (defined above) to the daemon.  any replies from the daemon are returned */
unsigned long g15_send_cmd (int sock, unsigned char command, unsigned char value);
/* receive an oob byte from the daemon, used internally by g15_send_cmd, but useful elsewhere */
#define G15_FOREGROUND_SENT_OOB 1
int g15_recv_oob_answer(int sock);
#ifdef __cplusplus
}
#endif
