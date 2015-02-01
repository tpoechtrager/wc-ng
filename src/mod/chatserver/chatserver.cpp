/***********************************************************************
 *  WC-NG - Cube 2: Sauerbraten Modification                           *
 *  Copyright (C) 2014, 2015 by Thomas Poechtrager                     *
 *  t.poechtrager@gmail.com                                            *
 *                                                                     *
 *  This program is free software; you can redistribute it and/or      *
 *  modify it under the terms of the GNU General Public License        *
 *  as published by the Free Software Foundation; either version 2     *
 *  of the License, or (at your option) any later version.             *
 *                                                                     *
 *  This program is distributed in the hope that it will be useful,    *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the      *
 *  GNU General Public License for more details.                       *
 *                                                                     *
 *  You should have received a copy of the GNU General Public License  *
 *  along with this program; if not, write to the Free Software        *
 *  Foundation, Inc.,                                                  *
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.      *
 ***********************************************************************/

/***********************************************************************
 *  _OpenSSL Exception_                                                *
 *                                                                     *
 *  In addition, as a special exception, the copyright holder gives    *
 *  permission to link the code of this program with any version of    *
 *  the OpenSSL library which is distributed under a license identical *
 *  to that listed in the included LICENSE.OpenSSL.txt file, and       *
 *  distribute linked combinations including the two.                  *
 *                                                                     *
 *  You must obey the GNU General Public License in all respects for   *
 *  all of the code used other than OpenSSL.                           *
 *                                                                     *
 *  If you modify this file, you may extend this exception to your     *
 *  version of the file, but you are not obligated to do so.           *
 *                                                                     *
 *  If you do not wish to do so, delete this exception statement       *
 *  from your version.                                                 *
 ***********************************************************************/

#include "sfree.h"

#ifdef _WIN32
#define sched_yield() Sleep(0)
#else
#include <sched.h>
#include <termios.h>
#endif

#include "game.h"
#include <signal.h>

#include <enet/time.h>
#include "../chat.h"
#include <SDL.h>
#include "../thread.h"

#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/err.h>
static SSL_CTX *ctx;
VAR(HAVEOPENSSL, 1, 1, 1);
VARN(usessl, useopenssl, 0, 0, 1);
SVAR(cipherlist, CIPHERLIST);
SVAR(certificate, "server.crt");
SVAR(privatekey, "server.key");
VAR(forwardsecrecy, 0, 1, 1);

using mod::strtool;

static atomic<bool> shutdownrequest;
static FILE *logfd = stdout;
static ENetSocket serversocket = ENET_SOCKET_NULL;
static enet_uint32 millis;
static time_t starttime;

SVAR(serverip, "0.0.0.0");
VAR(serverport, 0, -1, 0xFFFF);
SVAR(serverpass, "");
VAR(serverpassprompt, 0, 0, 1);
VAR(forktobackground, 0, 0, 1);
VAR(maxclients, 2, 32, NUMMAXCLIENTS);
VAR(maxclientsperip, 1, 8, NUMMAXCLIENTS);
VAR(supportoldclients, 0, 0, 1);
VAR(useauthtoken, 0, 1, 1);
SVAR(logfile, "");

struct client_t
{
    ENetAddress addr;
    ENetSocket socket;
    SSL *ssl;

    struct asyncsslaccept
    {
        sdlthread *thread;
        atomic<bool> canjoin;
        atomic<bool> ready;
        atomic<bool> exit;
        atomic<bool> timeout;
        atomic<bool> success;
        asyncsslaccept() :
            thread() {}
    } asyncsslaccept;

    bool isauthed;
    strtool authtoken;
    enet_uint32 lastrecv;

    int id;
    int protocol;
    time_t conntime;
    strtool name;
    char ip[16];

    struct server
    {
        time_t conntime;
        ENetAddress addr;
        int cn;
        strtool title;
        server() : conntime(), addr(), cn() {}
    } server;

    const char *idstr()
    {
        static string buf;
        strtool s(buf, sizeof(buf));
        s << "id: " << id << " (socket: " <<  socket << " ip: " << ip << ")";
        return s.str();
    }

    bool operator==(const ENetAddress &addr)
    {
        return this->addr.host == addr.host &&
               this->addr.port == addr.port;
    }

    client_t() :
        addr(), socket(), isauthed(), lastrecv(),
        id(), protocol(), conntime(), ip() {}

    ~client_t()
    {
        if (!useopenssl)
            return;

        authtoken.secureclear();
        name.secureclear();
        server.title.secureclear();

        authtoken.~strtool();
        server.title.~strtool();
        name.~strtool();

        szeromemory(this, sizeof(*this));
    }
};

static vector<client_t*> clients;
static vector<ipmask> bans;

static void addban(const char *ip) { bans.add(ipmask(ip)); }
COMMAND(addban, "s");

static char *getpassword(const char *prompt)
{
    char password[1024];
    fputs(prompt, stdout);
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    DWORD omode;
    if (!GetConsoleMode(h, &omode)) abort();
    if (!SetConsoleMode(h, omode & (~ENABLE_ECHO_INPUT))) abort();
#else
    struct termios oflags, nflags;
    tcgetattr(fileno(stdin), &oflags);
    nflags = oflags;
    nflags.c_lflag &= ~ECHO;
    nflags.c_lflag |= ECHONL;
    if (tcsetattr(fileno(stdin), TCSANOW, &nflags)) abort();
#endif
    size_t pwlength = sizeof(password);
    loopi(sizeof(password))
    {
        int c = getchar();
        if (c == EOF || c == '\n' || c == '\r')
        {
            pwlength = i;
            break;
        }
        password[i] = c&0xFF;
    }
#ifdef _WIN32
    if (!SetConsoleMode(h, omode)) abort();
#else
    if (tcsetattr(fileno(stdin), TCSANOW, &oflags)) abort();
#endif
    char *pw = newstring(password, pwlength);
    szeromemory(password, pwlength);
    return pw;
}

static void passwordprompt()
{
    char *pw = NULL;
    again:;
    if (pw) delete[] pw;
    pw = getpassword("server password: ");
    if (!*pw)
    {
        conoutf(CON_ERROR, "zero length password");
        goto again;
    }
    setsvar("serverpass", pw);
    delete[] pw;
    pw = getpassword("re-enter server password: ");
    if (strcmp(serverpass, pw))
    {
        conoutf(CON_ERROR, "passwords do not match");
        goto again;
    }
}

static sdlmutex ssllock;

static int sslaccept(void *d)
{
    client_t *c = (client_t*)d;
    enet_uint32 start = enet_time_get();
    enet_uint32 millis, left, w;
    int rv = -1;
    int n;

    ENetSocketSet sockset;
    ENetSocket maxsock = c->socket+1;

    c->asyncsslaccept.ready = true;

    if (enet_socket_set_option(c->socket, ENET_SOCKOPT_NONBLOCK, 1) < 0)
        goto ret;

    rv = 0;

    do
    {
        if (rv < 0)
        {
            millis = enet_time_get() - start;

            if (millis >= (enet_uint32)SSLINITTIMEOUT)
            {
                c->asyncsslaccept.timeout = true;
                break;
            }

            left = SSLINITTIMEOUT - millis;
            w = min(10u, left);

            ENET_SOCKETSET_EMPTY(sockset);
            ENET_SOCKETSET_ADD(sockset, c->socket);

            switch (SSL_get_error(c->ssl, rv))
            {
                case SSL_ERROR_WANT_READ:
                    n = enet_socketset_select(maxsock, &sockset, NULL, w);
                    break;
                case SSL_ERROR_WANT_WRITE:
                    n = enet_socketset_select(maxsock, NULL, &sockset, w);
                    break;
                default:
                    goto ret;
            }

            if (n < 0)
                break;
        }

        SDL_Mutex_Locker ml(ssllock);
        rv = SSL_accept(c->ssl);
    } while (rv < 0 && !shutdownrequest);

    ret:;

    if (enet_socket_set_option(c->socket, ENET_SOCKOPT_NONBLOCK, 0) < 0)
        rv = -1;

    c->asyncsslaccept.success = (rv == 1);
    c->asyncsslaccept.canjoin = true;

    return 0;
}

template<class T>
static bool send(client_t &c, T &p)
{
    assert((size_t)p.len <= MAXDATA);
    if (useopenssl)
    {
        assert(c.asyncsslaccept.success);
        SDL_Mutex_Locker ml(ssllock);
        return SSL_write(c.ssl, p.buf, p.len) == p.len;
    }
    else
    {
        ENetBuffer buf = INIT_ENET_BUFFER(p.buf, (size_t)p.len);
        return enet_socket_send(c.socket, NULL, &buf, 1) == p.len;
    }
}

template<class T>
static bool send(ENetSocket socket, T &p)
{
    assert((size_t)p.len <= MAXDATA);
    ENetBuffer buf = INIT_ENET_BUFFER(p.buf, (size_t)p.len);
    return enet_socket_send(socket, NULL, &buf, 1) == p.len;
}


template<class T>
static void sendauthed(client_t &exclude, T &p)
{
    loopv(clients)
    {
        client_t &c = *clients[i];
        if (c.isauthed && c.id != exclude.id) send(c, p);
    }
}

template<class T>
static void sendauthed(T &p)
{
    loopv(clients)
    {
        client_t &c = *clients[i];
        if (c.isauthed) send(c, p);
    }
}

static int getclientcount(enet_uint32 ip)
{
    int c = 0;
    loopv(clients) if (clients[i]->addr.host == ip) c++;
    return c;
}

static bool isbanned(enet_uint32 ip)
{
    loopv(bans) if (bans[i].check(ip)) return true;
    return false;
}

static void purgeclient(int n, bool expected = false)
{
    client_t &c = *clients[n];
    conoutf("purging client: %s", c.idstr());

    if (c.isauthed)
    {
        uchar buf[32];
        ucharbuf p(buf, sizeof(buf));
        putint(p, C_CLIENT_DISCONNECT);
        putint(p, c.id);
        putint(p, expected ? 1 : 0);
        sendauthed(c, p);
    }

    if (useopenssl)
    {
        if (c.asyncsslaccept.thread)
        {
            if (!c.asyncsslaccept.canjoin)
                c.asyncsslaccept.exit = true;

            delete c.asyncsslaccept.thread;
        }
        SDL_Mutex_Locker ml(ssllock);
        SSL_free(c.ssl);
    }

    enet_socket_destroy(c.socket);
    delete clients[n];
    clients.remove(n);
}

static client_t *getclient(int id)
{
    loopv(clients)
    {
        client_t *c = clients[i];
        if (c->id == id) return c;
    }
    return NULL;
}

static void sendclient(ucharbuf &p, client_t &c)
{
    putint(p, C_CLIENT_CONNECT);
    putint(p, c.id);
    p.put((uchar*)&c.addr.host, 3);
    p.put(0xFF);
    putint(p, c.conntime);
    sendstring(c.name, p);
    putint(p, 0);
}

static void sendserverinfo(ucharbuf &p, client_t &c, bool fakedisc = false)
{
    if (fakedisc)
    {
        // fake disconnect in case the client
        // sends multiple connects in a row
        putint(p, C_SERVER_INFO);
        putint(p, c.id);
        putint(p, 0);
    }

    putint(p, C_SERVER_INFO);
    putint(p, c.id);
    putint(p, c.server.addr.host);

    if (c.server.addr.host)
    {
        putint(p, c.server.addr.port);
        putint(p, c.server.conntime);
        putint(p, c.server.cn);
        sendstring(c.server.title, p);
    }
    else
    {
        c.server.addr.host = 0;
        c.server.title.clear();
    }
}

static void printtime()
{
    if (!logfd || logfd == stdout) return;
    string buf;
    time_t t = time(NULL);
    size_t n = strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S]: ", localtime(&t));
    fwrite(buf, n, 1, logfd);
}

void fatal(const char *fmt, ...)
{
    if (logfd)
    {
        va_list args;
        va_start(args, fmt);
        printtime();
        fprintf(logfd, "fatal: ");
        vfprintf(logfd, fmt, args);
        fputc('\n', logfd);
        va_end(args);
    }
    _exit(EXIT_FAILURE);
}

void conoutfv(int type, const char *fmt, va_list args)
{
    if (!logfd) return;
    printtime();
    switch (type) case CON_ERROR: fprintf(logfd, "error: ");
    vfprintf(logfd, fmt, args);
    fputc('\n', logfd);
    fflush(logfd);
}

void conoutf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    conoutfv(CON_INFO, fmt, args);
    va_end(args);
}

void conoutf(int type, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    conoutfv(type, fmt, args);
    va_end(args);
}

static void checktimeout()
{
    time_t now = time(NULL);
    loopvrev(clients)
    {
        client_t &c = *clients[i];
        if (!c.isauthed && now-c.conntime >= INITTIMEOUT/1000)
        {
            if (c.asyncsslaccept.thread) continue;
            conoutf("init timeout: %d", c.id);
            purgeclient(i);
        }
        else if (millis-c.lastrecv >= (enet_uint32)PINGTIMEOUT)
        {
            conoutf("ping timeout: %d", c.id);
            purgeclient(i);
        }
    }
}

static void checkclients()
{
    ENetSocketSet readset;
    ENetSocket maxsock = serversocket;

    ENET_SOCKETSET_EMPTY(readset);
    ENET_SOCKETSET_ADD(readset, serversocket);

    loopvrev(clients)
    {
        client_t &c = *clients[i];

        if (c.asyncsslaccept.thread && c.asyncsslaccept.canjoin)
        {
            delete c.asyncsslaccept.thread;
            c.asyncsslaccept.thread = NULL;

            if (!c.asyncsslaccept.success)
            {
                conoutf("client %s (%d): ssl/tls handshake failed%s",
                        "", c.id, c.asyncsslaccept.timeout ? " (timeout)" : "");
                purgeclient(i);
                continue;
            }

            strtool cipher = SSL_get_cipher(c.ssl);
            cipher.lowerstring();

            conoutf("%s: ssl/tls handshake succeeded, cipher: %s",
                    c.idstr(), cipher.str());
        }

        ENET_SOCKETSET_ADD(readset, c.socket);
        maxsock = max(maxsock, c.socket);
    }

    if (enet_socketset_select(maxsock, &readset, NULL, 1000) <= 0)
    {
        millis = enet_time_get();
        checktimeout();
        return;
    }

    millis = enet_time_get();
    checktimeout();

    if (ENET_SOCKETSET_CHECK(readset, serversocket))
    {
        ENetAddress address;
        ENetSocket clientsocket = enet_socket_accept(serversocket, &address);

        if (clientsocket != ENET_SOCKET_NULL)
        {
            string ip;
            if (enet_address_get_host_ip(&address, ip, sizeof(ip)) < 0)
                ip[0] = '\0';

            uchar sendbuf[512];
            ucharbuf s(sendbuf, sizeof(sendbuf));
            SDEALLOC(sendbuf, useopenssl);

            bool banned = isbanned(address.host);
            bool mc = clients.length() >= maxclients;
            bool mcpip = !mc && getclientcount(address.host) >= maxclientsperip;

            if (banned || mc || mcpip)
            {
                if (!supportoldclients)
                {
                    putint(s, banned ? CS_DISC_IP_BANNED : CS_DISC_SERVER_FULL);
                    send(clientsocket, s);
                }
                enet_socket_destroy(clientsocket);
                if (banned) conoutf("%s: ip is banned", ip);
                else conoutf("%s: max clients%s", ip, mcpip ? " per ip" : "");
            }
            else
            {
                client_t *c = new client_t;

                c->addr = address;
                c->socket = clientsocket;
                copystring(c->ip, ip, sizeof(c->ip));

                if (useopenssl)
                {
                    c->ssl = SSL_new(ctx);

                    if (!c->ssl)
                        fatal("SSL_new() failed");

                    if (!SSL_set_cipher_list(c->ssl, cipherlist))
                        fatal("invalid cipher list");

                    SSL_set_fd(c->ssl, c->socket);
                    s.put(1);
                }
                else
                {
                    s.put(0);
                }

                c->lastrecv = millis;
                c->conntime = time(NULL);
                c->id = 0;

                while (getclient(c->id)) c->id++;
                clients.add(c);

                if (useauthtoken && ::serverpass[0])
                {
                    c->authtoken.growbuf(sizeof(ullong));
                    ullong authtok;
                    authtok = (ullong)time(NULL) + address.port + address.host;
                    authtok *= ((millis&0xFF) ^ c->id);
                    c->authtoken << authtok;
                    putint(s, C_AUTH_TOK);
                    sendstring(c->authtoken, s);
                }

                send(c->socket, s);
                conoutf("new client: %s", c->idstr());

                if (useopenssl)
                {
                    ENET_SOCKETSET_REMOVE(readset, c->socket);
                    c->asyncsslaccept.thread = new sdlthread(sslaccept, c);

                    while (!c->asyncsslaccept.ready)
                        sched_yield();
                }
            }
        }
    }

    loopvrev(clients)
    {
        client_t &c = *clients[i];

        if (useopenssl && (c.asyncsslaccept.thread || !c.asyncsslaccept.success))
            continue;

        if (ENET_SOCKETSET_CHECK(readset, c.socket))
        {
            uchar recvbuf[MAXDATA];
            uchar sendbuf[MAXDATA];

            int len;

            // TODO: Track length to avoid zero'ing 128 KB.
            SDEALLOC(recvbuf, useopenssl);
            SDEALLOC(sendbuf, useopenssl);

            if (useopenssl)
            {
                SDL_Mutex_Locker ml(ssllock);
                len = SSL_read(c.ssl, recvbuf, MAXDATA);
            }
            else
            {
                ENetBuffer buf = INIT_ENET_BUFFER(recvbuf, MAXDATA);
                len = enet_socket_receive(c.socket, NULL, &buf, 1);
            }

            c.lastrecv = millis;

            if (len <= 0)
            {
                conoutf("client %d: disconnected", c.id);
                purgeclient(i);
                continue;
            }
            else if (len == 0) continue;

            ucharbuf p(recvbuf, len);
            ucharbuf s(sendbuf, MAXDATA);

            if (!c.isauthed)
            {
                switch (getint(p))
                {
                    case C_CONNECT:
                    {
                        c.protocol = getint(p);
                        bool pwok = false;

                        if ((c.protocol != PROTOCOL_VER && !supportoldclients) ||
                            (c.protocol < 2 || c.protocol > PROTOCOL_VER))
                        {
                            putint(s, C_INVALID_PROTOCOL);
                            putint(s, PROTOCOL_VER);
                            send(c, s);
                            conoutf("client %s: outdated protocol (%d!=%d)",
                                    c.idstr(), c.protocol, PROTOCOL_VER);
                            purgeclient(i);
                            break;
                        }

                        getfilteredstring(c.name, p, false, MAXNAMELEN, true);

                        if (c.protocol >= 3)
                        {
                            string serverpwhash;
                            string clientpwhash;
                            strtool salt;

                            getstring(salt, p, 1024);
                            getstring(clientpwhash, p);

                            salt += c.authtoken;

                            if (::serverpass[0])
                            {
                                if (hashstring(::serverpass, salt.str(), serverpwhash))
                                    pwok = !strcmp(serverpwhash, clientpwhash);
                            }
                            else
                            {
                                // no server password set
                                pwok = true;
                            }

                            if (useopenssl && c.authtoken)
                                salt.secureclear();
                        }
                        else
                        {
                            strtool clientpass;
                            getstring(clientpass, p, 1024, true);
                            pwok = !::serverpass[0] || clientpass == ::serverpass;
                            if (useopenssl) clientpass.secureclear();
                        }

                        // clear temporary data
                        if (useopenssl)
                            c.authtoken.secureclear();

                        if (!pwok)
                        {
                            putint(s, C_INVALID_PASSWORD);
                            send(c, s);
                            conoutf("client %d: invalid password", c.id);
                            purgeclient(i);
                            break;
                        }

                        putint(s, C_CONNECTED);
                        putint(s, c.id);
                        putint(s, C_TIME);
                        putint(s, time(NULL));

                        c.isauthed = true;

                        loopv(clients)
                        {
                            client_t &c = *clients[i];

                            if (!c.isauthed)
                                continue;

                            sendclient(s, c);

                            if (c.server.addr.host)
                                sendserverinfo(s, c);
                        }

                        send(c, s);

                        s.len = 0;
                        sendclient(s, c);
                        sendauthed(c, s);
                        break;
                    }
                    default:
                        conoutf("unexpected message (before auth) "
                                "from client: %s ", c.idstr());
                        purgeclient(i);
                }

                continue;
            }

            while (p.remaining())
            {
                s.len = 0;

                switch (int type=getint(p))
                {
                    case C_CHAT_MSG:
                    {
                        strtool msg;
                        getint(p);
                        getfilteredstring(msg, p, true, MAXMSGLEN, true);
                        putint(s, C_CHAT_MSG);
                        putint(s, c.id);
                        putint(s, 0);
                        sendstring(msg, s);
                        sendauthed(c, s);
                        msg.secureclear();
                        break;
                    }
                    case C_CLIENT_RENAME:
                    {
                        strtool old = c.name;
                        getfilteredstring(c.name, p, false, MAXNAMELEN, true);
                        if (old != c.name)
                        {
                            putint(s, C_CLIENT_RENAME);
                            putint(s, c.id);
                            sendstring(c.name, s);
                            sendauthed(s);
                        }
                        old.secureclear();
                        break;
                    }
                    case C_DISCONNECT:
                        purgeclient(i, true);
                        goto nextclient;
                    case C_SERVER_INFO:
                    {
                        bool hadserver = c.server.addr.host!=0;
                        c.server.addr.host = (enet_uint32)getint(p);
                        if (c.server.addr.host)
                        {
                            c.server.addr.port = (enet_uint16)getint(p);
                            c.server.conntime = time(NULL)-getint(p);
                            c.server.cn = getint(p);
                            getfilteredstring(c.server.title, p, true, 25, true);
                        }
                        else
                        {
                            c.server.addr.host = 0;
                            c.server.title.clear();
                        }
                        sendserverinfo(s, c, hadserver && c.server.addr.host);
                        sendauthed(s);
                        break;
                    }
                    case C_PING:
                    {
                        putint(s, C_PONG);
                        putint(s, getint(p));
                        putint(s, getint(p));
                        send(c, s);
                        break;
                    }
                    default:
                        conoutf("invalid packet type from client: "
                                "%s (type: %d)", c.idstr(), type);
                        purgeclient(i);
                        goto nextclient;
                }
            }
        }
        nextclient:;
    }
}

static void setupserver()
{
    ENetAddress address;

    if (serverport == -1)
        serverport = DEFAULTPORT;

    address.host = ENET_HOST_ANY;
    address.port = serverport&0xFFFF;

    if (serverport != address.port)
        fatal("invalid server port");

    if (enet_address_set_host(&address, serverip)<0)
        fatal("failed to resolve server address: %s", serverip);

    serversocket = enet_socket_create(ENET_SOCKET_TYPE_STREAM);

    if (serversocket == ENET_SOCKET_NULL ||
        enet_socket_set_option(serversocket, ENET_SOCKOPT_REUSEADDR, 1) < 0 ||
        enet_socket_bind(serversocket, &address) < 0 ||
        enet_socket_listen(serversocket, -1) < 0)
    {
        fatal("failed to create server socket");
    }

    if (enet_socket_set_option(serversocket, ENET_SOCKOPT_NONBLOCK, 1) < 0)
        fatal("failed to make server socket non-blocking");

    if (useopenssl)
    {
        SSL_library_init();
        SSL_load_error_strings();

        ctx = SSL_CTX_new(TLSv1_2_server_method());

        if (!ctx)
        {
            sslinitfail:;
            ERR_print_errors_fp(stderr);
            fatal("ssl init failed");
        }

        if (forwardsecrecy)
        {
            EC_KEY *ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);

            if (!ecdh)
                goto sslinitfail;

            if (!SSL_CTX_set_options(ctx, SSL_OP_SINGLE_ECDH_USE))
                goto sslinitfail;

            if (!SSL_CTX_set_tmp_ecdh(ctx, ecdh))
                goto sslinitfail;

            EC_KEY_free(ecdh);
        }

        // Disable TLS v1.0, SSLv2, SSLv3 and prefer the server's
        // cipher preference.

        static const int OPTS = SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3|SSL_OP_NO_TLSv1|
                                SSL_OP_CIPHER_SERVER_PREFERENCE;

        if (!SSL_CTX_set_options(ctx, OPTS))
            goto sslinitfail;

        conoutf("*** reading certificate (%s / %s) ***", certificate, privatekey);

        if (!SSL_CTX_load_verify_locations(ctx, certificate, privatekey))
            goto sslinitfail;

        if (!SSL_CTX_set_default_verify_paths(ctx))
            goto sslinitfail;

        if (!SSL_CTX_use_certificate_file(ctx, certificate, SSL_FILETYPE_PEM))
            goto sslinitfail;

        if (!SSL_CTX_use_PrivateKey_file(ctx, privatekey, SSL_FILETYPE_PEM))
            goto sslinitfail;

        if (!SSL_CTX_check_private_key(ctx))
        {
            conoutf(CON_ERROR, "private key does not match the public "
                               "certificate");
            goto sslinitfail;
        }
    }

    enet_time_set(0);

    starttime = time(NULL);
    conoutf("*** started chat server on %s:%d ***", serverip, serverport);
}

static void signalhandler(int sig)
{
    shutdownrequest = true;
}

static void cmdopt_help(const char *val);
static void cmdopt_pass(const char *val) { setsvar("serverpass", val); }
static void cmdopt_passprompt(const char *val) { serverpassprompt = 1; }
static void cmdopt_authtoken(const char *val) { useauthtoken = atoi(val); }
static void cmdopt_ip(const char *val) { setsvar("serverip", val); }
static void cmdopt_port(const char *val) { serverport = atoi(val); }
static void cmdopt_daemon(const char *val) { forktobackground = 1; }
static void cmdopt_usessl(const char *val) { useopenssl = 1; }
static void cmdopt_nopfs(const char *val) { forwardsecrecy = 0; }

static void cmdopt_maxclients(const char *val)
{
    maxclients = min(atoi(val), NUMMAXCLIENTS);
}

static void cmdopt_maxclientsperip(const char *val)
{
    maxclientsperip = min(atoi(val), NUMMAXCLIENTS);
}

static void cmdopt_opensslversion(const char *)
{
    conoutf("openssl version: %s", SSLeay_version(SSLEAY_VERSION));
    exit(EXIT_SUCCESS);
}

const char *const helptext[] =
{
    "server password",
    "prompt for a server password on startup",
    "use auth tokens",
    "ip address to listen on",
    "port to listen on",
    "fork to background",
    "use ssl encryption",
    "disable forward secrecy",
    "max clients",
    "max clients per ip",
    "show openssl version",
    "help"
};

static cmdopt cmdopts[] =
{
    { cmdopt_pass,            { "pw", "pass", "pw"        }, helptext[0]  },
    { cmdopt_passprompt,      { "pwprompt", "pwp"         }, helptext[1]  },
    { cmdopt_authtoken,       { "useauthtoken", "uat"     }, helptext[2]  },
    { cmdopt_ip,              { "ip", "ipaddr", "i"       }, helptext[3]  },
    { cmdopt_port,            { "port", "serverport", "p" }, helptext[4]  },
    { cmdopt_daemon,          { "daemon", "d"             }, helptext[5]  },
    { cmdopt_usessl,          { "usessl", "us"            }, helptext[6]  },
    { cmdopt_nopfs,           { "no-pfs", "npfs",         }, helptext[7]  },
    { cmdopt_maxclients,      { "maxclients", "mc"        }, helptext[8]  },
    { cmdopt_maxclientsperip, { "maxclientsperip", "mcpi" }, helptext[9]  },
    { cmdopt_opensslversion,  { "openssl-version", "ov"   }, helptext[10] },
    { cmdopt_help,            { "help", "h"               }, helptext[11] },
};

static void cmdopt_help(const char *val)
{
    conoutf("Options: ");
    strtool text;

    for (auto &opt : cmdopts)
    {
        strtool names = opt.names();
        text << " " << names;
        if (opt.helptext)
        {
            text.add(' ', 30 - text.length());
            text << opt.helptext;
        }
        conoutf("%s", text.str());
        text.clear();
    }

    exit(EXIT_SUCCESS);
}

static void parsecommandopts(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        const char *arg = argv[i];
        const char *val = "";
        while (*arg == '-') ++arg;

        if (i+1 < argc && argv[i+1][0] != '-')
            val = argv[++i];

        for (auto &opt : cmdopts)
        {
            switch (opt.run(arg, val))
            {
                case 1: goto next;
                case -1: goto error;
            }
        }

        conoutf(CON_ERROR, "unknown argument: '%s'", argv[i]);
        conoutf(CON_ERROR, "see '%s -help' for more", *argv);

        error:;
        exit(EXIT_FAILURE);

        next:;
    }
}

#undef main
int main(int argc, char **argv)
{
    if (enet_initialize() < 0) fatal("unable to initialize enet");
    atexit(enet_deinitialize);

    execfile("wc_chat_server.cfg");
    parsecommandopts(argc, argv);

    if (supportoldclients && useauthtoken)
    {
        conoutf(CON_ERROR, "'supportoldclients 1' is not compatible with "
                           "'useauthoken 1'");
        return 1;
    }

    if (serverpassprompt)
        passwordprompt();

    FILE *log = logfd;

    if (logfile[0])
    {
        log = fopen(logfile, "a+");
        if (!log)
        {
            conoutf(CON_ERROR, "cannot open '%s' for writing", logfile);
            return 1;
        }
    }

    setupserver();

    signal(SIGINT, signalhandler);
    signal(SIGTERM, signalhandler);

#ifndef _WIN32
    if (forktobackground)
    {
        conoutf("*** forking to background ***");
        switch (fork())
        {
            case 0:
                close(STDIN_FILENO);
                close(STDOUT_FILENO);
                close(STDERR_FILENO);
                if (log == stdout) log = NULL;
                break;
            case -1:
                fatal("fork() failed");
            default:
                _exit(0);
        }
    }
#endif //!_WIN32

    logfd = log;

    while (!shutdownrequest)
        checkclients();

    conoutf("received shutdown request");

    loopvrev(clients)
        purgeclient(i);

    if (useopenssl)
        SSL_CTX_free(ctx);

    if (logfd && logfd != stdout)
        fclose(logfd);

    return 0;
}
