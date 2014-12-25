/***********************************************************************
 *  WC-NG - Cube 2: Sauerbraten Modification                           *
 *  Copyright (C) 2014 by Thomas Poechtrager                           *
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

#include "game.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

namespace mod {
namespace chat {

static ENetSocket sock = ENET_SOCKET_NULL;
static bool opensslinited = false;

static struct sslconn
{
    X509 *cert;
    SSL *ssl;
    SSL_CTX *ctx;

    enum
    {
        UNTRUSTED,
        ISSUED_BY_TRUSTED_CA,
        REVOKED_CERTIFICATE,
        TRUSTED_CERTIFICATE
    } truststatus;

    void free()
    {
        if (ssl)
        {
            SSL_free(ssl);
            ssl = NULL;
        }
        if (cert)
        {
            X509_free(cert);
            cert = NULL;
        }
    }
    void freectx()
    {
        if (ctx)
        {
            SSL_CTX_free(ctx);
            ctx = NULL;
        }
    }
    sslconn() : cert(), ssl(), ctx(), truststatus(UNTRUSTED) {}
    ~sslconn() { freectx(); }
} sslconn;

struct connectdata_t
{
    strtool server;
    strtool nickname;
    strtool password;
    strtool cipherlist;

    connectdata_t() {}

    connectdata_t(const connectdata_t &cd)
    {
        copystrtool(server, cd.server);
        copystrtool(nickname, cd.nickname);
        copystrtool(password, cd.password);
        copystrtool(cipherlist, cd.cipherlist);
    }
} connectdata;

struct client_t;
static vector<client_t*> clients;

struct client_t
{
    int id;
    union
    {
        uint32_t ui32;
        uint8_t ui[4];
    } ip;
    time_t conntime;
    strtool name;
    const char *country;
    const char *countrycode;

    struct server
    {
        time_t conntime;
        ENetAddress addr;
        int cn;
        strtool title;
        const char *country;
        const char *countrycode;

        server() : conntime(), addr(), cn(),
                   country(), countrycode() {}
    } server;

    strtool getname()
    {
        strtool tmp;
        tmp << name;
        loopv(clients)
        {
            client_t *c = clients[i];
            if (id != c->id && c->name == name)
            {
                tmp << " (" << id << ")";
                break;
            }
        }
        return tmp;
    }

    client_t() : id(), ip(), conntime() {}
};

//
// This chat is used from different threads,
// therefore it requires "some" locking.
//

static bool inited = false;
static int pingsalt = 0;

static atomic<bool> isconnecting(false);
static atomic<bool> isconnected(false);
static atomic<bool> connectdatahaschanged(false);
static atomic<bool> shutdown(false);
static atomic<int> myid(0);
static atomic<uint> myping(0);
static atomic<bool> needserverinfo(false);
static atomic<bool> showconnects(true);
static atomic<bool> showservconnects(true);
static atomic<int> timeoffset(0);
static strtool prefix;
static sdlmutex prefixmutex;
static sdlmutex sendmutex;
static sdlmutex sockmutex;
static sdlmutex connectdatamutex;
static sdlmutex clientmutex;

static sdlmutex sslmutex;
static atomic<bool> usessl(false);
static vector<certificate_t> validcerts;
static vector<certificate_t> revokedcerts;

static void updateconnectdata(bool reconnect = true);
static int chatthread(void *);
static sdlthread *thread = NULL;
static bool parsemessages(ucharbuf &p);
static void wcchatlist();
static void wcchatloopclients(const char *callbackargs, const char *callbackcode);
static void wcchatgoto(const char *client);
static void wcchatisconnected();
static void wcchatlatency(int *cubescript);
static void wcchat(const char *msg);
static void wcchatconnect();
static void wcchatdoautoconnect();
static void wcchatdisconnect();

static void wcchataddcert(const char *fingerprint, const char *host);
static void wcchatdelcert(const char *fingerprint);
static void wcchatdelcerts();
static void wcchatlistcerts();
static void wcchatshowcertdetails();
static void wcchataddrevokedcert(const char *fingerprint);

static void updateprefix(const char *newprefix)
{
    SDL_Mutex_Locker m(prefixmutex);
    prefix = newprefix;
}

static struct initprefix
{
    char dummy;
    initprefix() { updateprefix(DEFAULTPREFIX); }
} initprefx;

MODSVARFP(wcchatserver, "", updateconnectdata());
MODSVARFP(wcchatpassword, "", updateconnectdata());
MODVARFP(wcchatshowconnects, 0, 1, 1, showconnects = wcchatshowconnects!=0);
MODVARFP(wcchatshowservconnects, 0, 1, 1, showservconnects = wcchatshowservconnects!=0);
MODSVARFP(wcchatprefix, DEFAULTPREFIX, updateprefix(wcchatprefix));
MODSVARFP(wcchatbinds, "pc", setupbinds());
MODVARP(wcchatautoconnect, 0, 0, 1);

COMMAND(wcchataddcert, "ss");
COMMAND(wcchatdelcert, "s");
COMMAND(wcchatdelcerts, "");
COMMAND(wcchatlistcerts, "");
COMMAND(wcchatshowcertdetails, "");
COMMAND(wcchataddrevokedcert, "s");
MODSVARFP(wcchatcipherlist, CIPHERLIST, updateconnectdata());

COMMAND(wcchatlist, "");
COMMAND(wcchatloopclients, "ss");
COMMAND(wcchatisconnected, "");
COMMAND(wcchatlatency, "i");
COMMAND(wcchatgoto, "s");
COMMAND(wcchat, "s");
COMMAND(wcchatconnect, "");
COMMAND(wcchatdisconnect, "");

#define ADDMSG(con)                                                            \
do {                                                                           \
    strtool msg(512);                                                          \
    prefixmutex.lock();                                                        \
    msg << prefix << ' ';                                                      \
    prefixmutex.unlock();                                                      \
    va_list v;                                                                 \
    va_start(v, fmt);                                                          \
    msg.vfmt(fmt, v);                                                          \
    va_end(v);                                                                 \
    addthreadmessage(msg, con);                                                \
} while (0)

static void consoleoutf(int con, const char *fmt, ...)
{
    ADDMSG(con);
}

static void consoleoutf(const char *fmt, ...)
{
    ADDMSG(CON_INFO);
}

static bool calccerthash(const X509 *cert, const char *hashtype, strtool &hash)
{
    uint len;
    uchar rhash[EVP_MAX_MD_SIZE];
    const EVP_MD *digest = EVP_get_digestbyname(hashtype);
    if (!digest) return false;
    if (!X509_digest(cert, digest, rhash, &len)) return false;
    hash.clear();
    loopi(len) hash.fmt("%.2X:", rhash[i]);
    if (len) hash.pop();
    return true;
}

static bool isrevokedcert(X509 *cert, strtool &error)
{
    strtool hash;

    for (auto &revokedcert : revokedcerts)
    {
        if (!calccerthash(cert, revokedcert.hashtype(), hash))
            continue;

        if (revokedcert == hash)
        {
            error = "this server's certificate has been revoked";
            return true;
        }
    }

    return false;
}

static bool verifybyhash(X509 *cert, const char *host, strtool &error)
{
    strtool hash;
    strtool hosts;

    for (auto &validcert : validcerts)
    {
        if (!calccerthash(cert, validcert.hashtype(), hash))
            continue;

        if (validcert == hash)
        {
            if (validcert.host.empty() || validcert.host == host)
                return true;

            hosts << validcert.host << ", ";
        }
    }

    if (hosts)
    {
        hosts -= 2;

        error << "certificate not valid for current hostname, "
                 "hostname should be " << hosts;
    }

    return false;
}

#include "chat-ca-certs.h"

static bool loadcacerts(vector<X509*> &cacerts)
{
    for (auto &cacert : PUBLIC_CA_CERTS)
    {
        BIO *bio = BIO_new_mem_buf((void*)cacert.data, -1);
        if (!bio) continue;
        X509 *cert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
        if (cert)
        {
            strtool error;
            if (!isrevokedcert(cert, error)) cacerts.add(cert);
            else X509_free(cert);
        }
        BIO_free(bio);
    }
    return !cacerts.empty();
}

static bool verifybyca(X509 *cert, X509 *cacert, const char *host, strtool &error)
{
    SDL_Mutex_Locker m(sslmutex);
    int v = 0;

    X509_STORE *store = NULL;
    X509_STORE_CTX *storectx = NULL;
    OpenSSL_add_all_algorithms();

    if (!(store = X509_STORE_new()))
        goto fail;

    if (!X509_STORE_add_cert(store, cacert))
        goto fail;

    if (!(storectx = X509_STORE_CTX_new()))
        goto fail;

    static const int OPTS =
        X509_V_FLAG_X509_STRICT|X509_V_FLAG_CHECK_SS_SIGNATURE|
        X509_V_FLAG_POLICY_CHECK;

    if (!X509_STORE_set_flags(store, OPTS))
        goto fail;

    if (!X509_STORE_CTX_init(storectx, store, cert, NULL))
        goto fail;

    v = X509_verify_cert(storectx);

    if (v == 1)
    {
        int pos = -1;
        X509_NAME *subject = X509_get_subject_name(cert);

        for (;;)
        {
            pos = X509_NAME_get_index_by_NID(subject, NID_commonName, pos);
            if (pos == -1) break;

            X509_NAME_ENTRY *e = X509_NAME_get_entry(subject, pos);
            BIO *b = BIO_new(BIO_s_mem());
            if (!b) continue;

            string cn = "";
            if (ASN1_STRING_print_ex(b, e->value, 0))
                cn[BIO_read(b, cn, sizeof(cn)-1)] = '\0';

            BIO_free(b);

            // check for wildcard
            if (!strncmp(cn, "*.", 2))
            {
                strtool commonname = cn;
                strtool hostname = host;

                // allow *.domain.org but not *.org
                if (commonname.count('.') < 2) continue;

                // disallow ip addresses
                if (isipaddress(hostname)) continue;

                const char *want = commonname.str()+2;
                const char *given = host;

                if (hostname.count('.') > commonname.count('.')-1)
                    given = hostname.find('.')+1;

                if (!strcmp(want, given)) goto hostok;
            }
            else if (!strcmp(host, cn)) goto hostok;
        }

        error = "certificate not valid for current host";
        v = 0;
        hostok:;
    }

    fail:;
    if (store) X509_STORE_free(store);
    if (storectx) X509_STORE_CTX_free(storectx);

    return v == 1;
}

static bool issuedbytrustedca(X509 *cert, const char *host, strtool &error)
{
    SDL_Mutex_Locker m(sslmutex);
    vector<X509*> cacerts;
    bool retval = false;
    loadcacerts(cacerts);

    for (auto *cacert : cacerts)
    {
        if (verifybyca(cert, cacert, host, error))
        {
            retval = true;
            break;
        }
    }

    while (!cacerts.empty()) X509_free(cacerts.pop());
    return retval;
}

static void showcertdetails(X509 *cert)
{
    SDL_Mutex_Locker m(sslmutex);
    if (!cert) return;

    const char *truststatus = NULL;
    strtool fingerprint;

    if (cert == sslconn.cert)
    {
        switch (sslconn.truststatus)
        {
            case sslconn::ISSUED_BY_TRUSTED_CA:
                truststatus = "issued by a trusted CA";
                break;
            case sslconn::TRUSTED_CERTIFICATE:
                truststatus = "trusted certificate";
                break;
            default:
                truststatus = "unknown";
                break;
        }
    }

    char *issuer = X509_NAME_oneline(X509_get_issuer_name(cert), NULL, 0);
    char *subject = X509_NAME_oneline(X509_get_subject_name(cert), NULL, 0);

    string d1 = "", d2 = "";
    BIO *s1 = BIO_new(BIO_s_mem());
    BIO *s2 = BIO_new(BIO_s_mem());

    if (s1 && ASN1_TIME_print(s1, X509_get_notBefore(cert)))
        d1[BIO_read(s1, d1, sizeof(d1)-1)] = '\0';

    if (s2 && ASN1_TIME_print(s2, X509_get_notAfter(cert)))
        d2[BIO_read(s2, d2, sizeof(d2)-1)] = '\0';

    string sigalg = "";
    OBJ_obj2txt(sigalg, sizeof(sigalg), cert->sig_alg->algorithm, 0);
    sigalg[sizeof(sigalg)-1] = '\0';

    calccerthash(cert, "sha256", fingerprint);

    consoleoutf("certificate details:");
    if (truststatus) consoleoutf(" trust status: %s", truststatus);
    consoleoutf(" issuer: %s", issuer);
    consoleoutf(" subject: %s", subject);
    consoleoutf(" not valid before: %s, not valid after: %s", d1, d2);
    consoleoutf(" signature algorithm: %s", sigalg);
    consoleoutf(" sha256 fingerprint: %s", fingerprint.str());

    OPENSSL_free(issuer);
    OPENSSL_free(subject);

    if (s1) BIO_free(s1);
    if (s2) BIO_free(s2);
}

template<class T>
static bool send(T &p)
{
    SDL_Mutex_Locker m1(sendmutex);
    SDL_Mutex_Locker m2(sockmutex);
    if (sock == ENET_SOCKET_NULL) return false;
    if (usessl)
    {
        SDL_Mutex_Locker m(sslmutex);
        if (sslconn.ssl) return SSL_write(sslconn.ssl, p.buf, p.len) == p.len;
        else return false;
    }
    else
    {
        ENetBuffer buf = INIT_ENET_BUFFER(p.buf, (size_t)p.len);
        return enet_socket_send(sock, NULL, &buf, 1) == p.len;
    }
}

static int recv(void *data, size_t len)
{
    SDL_Mutex_Locker m1(sendmutex);
    SDL_Mutex_Locker m2(sockmutex);
    if (sock == ENET_SOCKET_NULL) return false;
    if (usessl)
    {
        SDL_Mutex_Locker m(sslmutex);
        return SSL_read(sslconn.ssl, data, len);
    }
    else
    {
        ENetBuffer buf = INIT_ENET_BUFFER(data, len);
        return enet_socket_receive(sock, NULL, &buf, 1);
    }
}

static inline void sendping()
{
    uchar sendbuf[128];
    ucharbuf p(sendbuf, sizeof(sendbuf));
    putint(p, C_PING);
    union { ullong ull; int i[2]; } x;
    x.ull = getsaltedmicroseconds(false /*newsalt*/, -1, &pingsalt);
    putint(p, x.i[0]);
    putint(p, x.i[1]);
    send(p);
}

static inline void updateconnectdata(bool reconnect)
{
    if (!game::player1->name[0]) return;
    if (!ismainthread()) abort();
    connectdatamutex.lock();
    connectdata.nickname = game::player1->name;
    connectdata.server = wcchatserver;
    connectdata.password = wcchatpassword;
    connectdata.cipherlist = wcchatcipherlist;
    if (reconnect) connectdatahaschanged = true;
    connectdatamutex.unlock();
    wcchatdoautoconnect();
}

void updatename()
{
    updateconnectdata(false);

    if (isconnected)
    {
        uchar sendbuf[1024];
        ucharbuf p(sendbuf, sizeof(sendbuf));
        putint(p, C_CLIENT_RENAME);
        sendstring(game::player1->name, p);
        send(p);
    }
}

void servconnect(bool connect)
{
    if (isconnected)
    {
        uchar sendbuf[1024];
        ucharbuf p(sendbuf, sizeof(sendbuf));
        putint(p, C_SERVER_INFO);
        putint(p, connect ? curpeer->address.host : 0);
        if (connect)
        {
            putint(p, curpeer->address.port);
            putint(p, (totalmillis-connmillis)/1000);
            putint(p, game::player1->clientnum);
            sendstring(game::servinfo, p);
        }
        send(p);
    }
    needserverinfo = false;
}

static inline client_t *getclient(int id)
{
    loopv(clients)
    {
        client_t *c = clients[i];
        if (c->id == id) return c;
    }
    return NULL;
}

static int findclients(const char *find, vector<client_t*> &matches, bool m = false)
{
    strtool client = find;
    strtool tmp;
    client.lowerstring();

    loopv(clients)
    {
        client_t *c = clients[i];

        tmp.copy(c->name);
        tmp.lowerstring();

        if (tmp.find(client)) matches.add(c);
    }

    if (matches.empty())
    {
        consoleoutf("no matching client found");
        return 0;
    }
    else if (!m && matches.length() > 1)
    {
        consoleoutf("multiple clients found, please be more specific");
        return 0;
    }

    return matches.length();
}

static bool connect(const char *host, int port)
{
    SDL_Mutex_Locker m(sockmutex);

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    sslmutex.lock();
    if (!opensslinited)
    {
        SSL_library_init();
        SSL_load_error_strings();
        sslconn.ctx = SSL_CTX_new(TLSv1_2_client_method());
        static const int OPTS = SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3|SSL_OP_NO_TLSv1;
        if (!sslconn.ctx || !SSL_CTX_set_options(sslconn.ctx, OPTS)) abort();
        opensslinited = true;
    }
    sslmutex.unlock();

    consoleoutf("connecting to server (%s:%d)", host, port);

    sock = enet_socket_create(ENET_SOCKET_TYPE_STREAM);

    if (sock == ENET_SOCKET_NULL)
    {
        consoleoutf("unable create socket");
        return false;
    }

    if (enet_address_set_host(&address, host) < 0)
    {
        enet_socket_destroy(sock);
        sock = ENET_SOCKET_NULL;
        consoleoutf("unable to resolve %s", host);
        return false;
    }

    if (enet_socket_connect(sock, &address) < 0)
    {
        enet_socket_destroy(sock);
        sock = ENET_SOCKET_NULL;
        consoleoutf("unable to connect to %s:%d", host, port);
        return false;
    }

    return true;
}

static void disconnect(bool msg = true)
{
    assert(!ismainthread());
    ucharbuf p;
    putint(p, C_DISCONNECT);
    send(p);
    SDL_Mutex_Locker m1(sockmutex);
    SDL_Mutex_Locker m2(clientmutex);
    clients.deletecontents();
    if (msg && (isconnected || isconnecting))
        consoleoutf("disconnecting from server");
    myid = 0;
    myping = 0;
    isconnected = false;
    isconnecting = false;
    needserverinfo = false;
    timeoffset = 0;
    if (sock == ENET_SOCKET_NULL) return;
    enet_socket_destroy(sock);
    sock = ENET_SOCKET_NULL;
    usessl = false;
    SDL_Mutex_Locker m(sslmutex);
    sslconn.free();
}

static int init(const char *host, int port, const char *nickname,
                const char *password, bool forcessl)
{
    isconnecting = true;
    if (!connect(host, port)) return false;

    auto doconnectselect = [&]() -> bool
    {
        ENetSocketSet sockset;
        ENET_SOCKETSET_EMPTY(sockset);
        ENET_SOCKETSET_ADD(sockset, sock);

        if (enet_socketset_select(sock, &sockset, NULL, CONNECTRECVTIMEOUT) <= 0 ||
           !ENET_SOCKETSET_CHECK(sockset, sock))
        {
            consoleoutf("unable to connect: receive timeout");
            return false;
        }

        return true;
    };
    if (!doconnectselect()) return 0;

    uniqueptr<uchar*> recvbuf(new uchar[MAXDATA], true);
    ENetBuffer ebuf = INIT_ENET_BUFFER(recvbuf, MAXDATA);
    int len = enet_socket_receive(sock, NULL, &ebuf, 1);
    strtool authtok;

    if (len <= 0) return false;

    switch (int type=recvbuf[0])
    {
        case CS_NO_SSL:
        case CS_USE_SSL:
        {
            usessl = (type == CS_USE_SSL);
            break;
        }
        case CS_DISC_SERVER_FULL:
        case CS_DISC_IP_BANNED:
        {
            const char *const discreason[] = { "server full", "ip is banned" };
            consoleoutf("unable to connect: %s", discreason[type-2]);
            return false;
        }
        default: return false;
    }

    if (len > 1)
    {
        ucharbuf p(&recvbuf[1], len-1);

        while (p.remaining()) switch (getint(p))
        {
            case C_AUTH_TOK:
            {
                // this is to prevent the re-use of password hashses
                getstring(authtok, p, 1024);
                break;
            }
            default: return false;
        }
    }

    if (usessl)
    {
        SDL_Mutex_Locker m(sslmutex);

        if (false)
        {
            sslinitfail:;
            consoleoutf("ssl init failed");
            return 0;
        }

        sslconn.ssl = SSL_new(sslconn.ctx);
        if (!sslconn.ssl) goto sslinitfail;

        {
            SDL_Mutex_Locker ml(connectdatamutex);

            if (!SSL_set_cipher_list(sslconn.ssl, connectdata.cipherlist.str()))
            {
                consoleoutf("invalid cipher list!");
                goto sslinitfail;
            }
        }

        SSL_set_fd(sslconn.ssl, sock);

        int rv;
        if ((rv = SSL_connect(sslconn.ssl)) <= 0)
        {
            consoleoutf("ssl/tls handshake failed: %s",
                        ERR_error_string(SSL_get_error(sslconn.ssl, rv), NULL));
            ERR_print_errors_fp(stderr);
            return 0;
        }

        strtool cipher = SSL_get_cipher(sslconn.ssl);
        cipher.lowerstring();

        consoleoutf("encryption: %s", cipher.str());

        if (!(sslconn.cert = SSL_get_peer_certificate(sslconn.ssl)))
        {
            consoleoutf("failed to get certificate");
            return 0;
        }

        time_t t = time(NULL);
        strtool error;
        int v;

        v = X509_cmp_time(X509_get_notBefore(sslconn.cert), &t);

        if (v > 0)
        {
            error = "certificate not yet valid";
            goto certfail;
        }

        v = X509_cmp_time(X509_get_notAfter(sslconn.cert), &t);

        if (v < 0)
        {
            error = "certificate has expired";
            goto certfail;
        }

        if (isrevokedcert(sslconn.cert, error))
        {
            sslconn.truststatus = sslconn::REVOKED_CERTIFICATE;
            goto certfail;
        }

        if (issuedbytrustedca(sslconn.cert, host, error))
        {
            sslconn.truststatus = sslconn::ISSUED_BY_TRUSTED_CA;
        }
        else if (verifybyhash(sslconn.cert, host, error))
        {
            sslconn.truststatus = sslconn::TRUSTED_CERTIFICATE;
        }
        else
        {
            if (!error)
            {
                strtool hash;

                if (!calccerthash(sslconn.cert, "sha1", hash))
                    hash = "unknown";

                error << "unknown certificate! sha1 fingerprint: " << hash;
            }

            sslconn.truststatus = sslconn::UNTRUSTED;
            goto certfail;
        }

        if (false)
        {
            certfail:;
            consoleoutf("certificate verification failed (%s)", error.str());
            return 0;
        }
    }

    if (!usessl)
    {
        consoleoutf("server does not use encryption");

        if (forcessl)
        {
            consoleoutf(CON_ERROR, "\fs\f3error:\fr expected ssl connection "
                                   "(you may be a victim of a MITM attack)");
            return 0;
        }
    }

    uniqueptr<uchar*> buf(new uchar[MAXDATA], true);
    ucharbuf p(buf, MAXDATA);

    putint(p, C_CONNECT);
    putint(p, PROTOCOL_VER);
    sendstring(nickname, p);

    if (password[0])
    {
        strtool salt;
        string hash;

        {
            ullong prev = 0;
            ullong ns;

            do {
                while ((ns = getnanoseconds() % 0xFFFF) == prev)
                    SDL_Delay(0);

                salt << ns;
                prev = ns;
            } while (salt.length() < 224);
        }

        while (salt.length() <= 256)
            salt.add('A');

        salt += authtok;

        if (!hashstring(password, salt.str(), hash))
            return 0;

        salt -= authtok.length();

        sendstring(salt, p);
        sendstring(hash, p);
    }
    else
    {
        p.put('\0');
        p.put('\0');
    }

    if (!send(p)) return 0;

    if (!doconnectselect()) return 0;
    len = recv(recvbuf, MAXDATA);

    if (len > 0)
    {
        ucharbuf p(recvbuf, len);

        switch (int type=getint(p))
        {
            case C_CONNECTED:
            {
                myid = getint(p);
                isconnected = true;
                isconnecting = false;
                needserverinfo = true;
                consoleoutf("connected to server (id: %d)", myid.load());
                break;
            }
            case C_INVALID_PROTOCOL:
                consoleoutf(CON_ERROR, "unable to connect: \f3protocol version mismatch");
                return -1;
            case C_INVALID_PASSWORD:
                consoleoutf(CON_ERROR, "unable to connect: \f3invalid password");
                return -1;
            default:
                consoleoutf(CON_ERROR, "\f3invalid message type: %d", type);
                return 0;
        }

        if (p.remaining()) parsemessages(p);
    }

    if (len <= 0 || p.overread())
    {
        disconnect();
        return 0;
    }

    return 1;
}

static bool parsemessages(ucharbuf &p)
{
    SDL_Mutex_Locker m(clientmutex);

    while (p.remaining()) switch (int type=getint(p))
    {
        case C_CLIENT_CONNECT:
        {
            client_t c;
            c.id = getint(p);
            p.get((uchar*)&c.ip.ui32, 4);
            c.conntime = getint(p)+timeoffset;
            getfilteredstring(c.name, p, false, MAXNAMELEN);
            if (!c.name) c.name = "unnamed";
            c.server.conntime = getint(p);
            if (c.server.conntime)
            {
                c.server.addr.host = (enet_uint32)getint(p);
                c.server.addr.port = (enet_uint16)getint(p);
                c.server.country = geoip::country(c.server.addr.host);
                c.server.countrycode = geoip::countrycode(c.server.addr.host);
            }
            if (getclient(c.id) || clients.length() >= NUMMAXCLIENTS) break;
            c.country = geoip::country(c.ip.ui32);
            c.countrycode = geoip::countrycode(c.ip.ui32);
            clients.add(new client_t(c));
            if (!showconnects || c.id == myid) break;
            consoleoutf("connect: %s (%s)", c.getname().str(),
                        c.countrycode ? c.countrycode : "??");
            break;
        }
        case C_CLIENT_DISCONNECT:
        {
            int id = getint(p);
            /*int expected =*/ getint(p);
            if (id == myid) break;
            loopvrev(clients)
            {
                client_t *c = clients[i];
                if (c->id == id)
                {
                    if (showconnects)
                        consoleoutf("disconnect: %s", c->getname().str());
                    delete clients.remove(i);
                    break;
                }
            }
            break;
        }
        case C_CHAT_MSG:
        {
            strtool msg;
            client_t *c = getclient(getint(p));
            getint(p);
            getfilteredstring(msg, p, true, MAXMSGLEN);
            if (!c || c->id == myid) break;
            consoleoutf(CON_CHAT, "\f1%s: \f0%s", c->getname().str(), msg.str());
            break;
        }
        case C_CLIENT_RENAME:
        {
            strtool newname;
            client_t *c = getclient(getint(p));
            getfilteredstring(newname, p, false, MAXNAMELEN);
            if (!c || c->name == newname) break;
            strtool oldname = c->getname();
            c->name.swap(newname);
            if (c->id != myid)
            {
                consoleoutf("\fs\f1%s\fr is now known as \fs\f1%s\fr",
                            oldname.str(), c->getname().str());
            }
            break;
        }
        case C_SERVER_INFO:
        {
            client_t *c = getclient(getint(p));
            string ipbuf;
            if (!c) return false;
            enet_uint32 ip = (enet_uint32)getint(p);
            if (ip)
            {
                c->server.addr.host = ip;
                c->server.addr.port = (enet_uint16)getint(p);
                c->server.conntime = getint(p)+timeoffset;
                c->server.cn = getint(p);
                getfilteredstring(c->server.title, p, true, 25);
                if (!showservconnects || c->id == myid) break;
                consoleoutf("\fs\f0%s\fr connected to server \fs\f0%s\fr "
                            "(\fs\f0%s\fr:\fs\f0%d\fr)", c->getname().str(),
                            c->server.title ? c->server.title.str() : "<no title>",
                            resolve(ip, ipbuf), c->server.addr.port);
            }
            else
            {
                if (showservconnects && c->id != myid && c->server.addr.host)
                {
                    consoleoutf("\fs\f0%s\fr disconnected from server \fs\f0%s\fr "
                                "(\fs\f0%s\fr:\fs\f0%d\fr)", c->getname().str(),
                                c->server.title ? c->server.title.str() : "<no title>",
                                resolve(c->server.addr.host, ipbuf), c->server.addr.port);
                }
                c->server.addr.host = 0;
                c->server.addr.port = 0;
                c->server.title.clear();
            }
            break;
        }
        case C_TIME:
        {
            timeoffset = time(NULL)-getint(p);
            break;
        }
        case C_PONG:
        {
            union { ullong ull; int i[2]; } x;
            x.i[0] = getint(p);
            x.i[1] = getint(p);
            myping = getsaltedmicroseconds(false /*newsalt*/, x.ull, &pingsalt);
            break;
        }
        default:
            consoleoutf(CON_ERROR, "\f3invalid message type: %d", type);
            return false;
    }

    return true;
}

static int chatthread(void *)
{
    connectdata_t cd;
    strtool *strs;
    ullong millis = 0;
    ullong lastrecv = 0;
    ullong lastping = 0;
    bool forcessl = false;
    int rv;

    reconnect:;

    connectdatamutex.lock();
    cd = connectdata;
    connectdatahaschanged = false;
    connectdatamutex.unlock();

    strs = NULL;

    const char *server;
    int port;
    size_t n;

    if (!cd.server.compare("ssl:", 4))
    {
        forcessl = true;
        cd.server.remove(0, 4);
    }

    if ((n = cd.server.split(":", &strs)) < 1) goto delay;

    server = strs[0].str();
    port = n >= 2 ? atoi(strs[1].str()) : DEFAULTPORT;

    rv = init(server, port, cd.nickname.str(), cd.password.str(), forcessl);

    delete[] strs;

    if (rv <= 0)
    {
        disconnect(rv >= 0);
        /*if (rv == 0)*/ goto delay;
    }

    //
    // Must provide our own random functions as
    // seedMT() and randomMT() are not thread safe.
    //

    getsaltedmicroseconds(true /*newsalt*/, -1, &pingsalt,
      [](uint seed){ RAND_seed(&seed, sizeof(seed)); },
      [](){uint t = 0; RAND_pseudo_bytes((uchar*)&t, sizeof(t)); return t;});

    lastrecv = getmilliseconds();
    lastping = 0;

    while (!shutdown)
    {
        millis = getmilliseconds();

        if (/*rv != -1 &&*/ !isconnected)
        {
            delay:;
            auto s = getmilliseconds();

            while (!shutdown && !connectdatahaschanged)
            {
                if (getmilliseconds()-s >= (ullong)RECONNECTTHRESHOLD)
                {
                    consoleoutf("connecting to server "
                                          "(type '/wcchatdisconnect' to stop "
                                          "reconnecting)");
                    goto reconnect;
                }

                SDL_Delay(10);
            }

            if (shutdown) goto shutdown;
        }

        if (connectdatahaschanged)
        {
            consoleoutf("server connect data has changed - reconnecting");
            disconnect(isconnected.load());
            goto reconnect;
        }

        ENetSocketSet sockset;
        ENET_SOCKETSET_EMPTY(sockset);
        ENET_SOCKETSET_ADD(sockset, sock);

        if (enet_socketset_select(sock, &sockset, NULL, 50) > 0)
        {
            if (ENET_SOCKETSET_CHECK(sockset, sock))
            {
                uchar buf[MAXDATA];
                int len = recv(buf, sizeof(buf));
                lastrecv = millis;

                if (len <= 0)
                {
                    disconnect();
                    goto delay;
                }

                ucharbuf p(buf, len);

                if (!parsemessages(p))
                {
                    consoleoutf("disc parsemessages()");
                    disconnect();
                    goto delay;
                }
            }
        }

        if (millis-lastrecv >= (ullong)PINGTIMEOUT)
        {
            consoleoutf("ping timeout");
            disconnect();
            goto delay;
        }
        else if (!lastping || millis-lastping >= (ullong)PINGSENDTHRESHOLD)
        {
            sendping();
            lastping = millis;
        }
    }

    shutdown:;
    disconnect();
    return 0;
}

static inline bool checkconnection(bool msg = true)
{
    if (!isconnected)
    {
        if (msg) consoleoutf("not connected to chat server");
        return false;
    }
    return true;
}

static void wcchatlist()
{
    if (!checkconnection()) return;
    SDL_Mutex_Locker m(clientmutex);

    strtool str;

    strtool time1;
    strtool time2;
    strtool srv;
    string ipbuf;

    time_t now = 0;

    auto fmtseconds = [&](strtool &s, time_t t)
    {
        s.clear();
        if (!now) now = time(NULL);
        s.fmtseconds(now-t);
        if (!s) s = "0s";
    };

    loopv(clients)
    {
        client_t *c = clients[i];
        fmtseconds(time1, c->conntime);

        srv.clear();
        auto *s = &c->server;

        if (s->addr.host)
        {
            resolve(s->addr.host, ipbuf);
            fmtseconds(time2, s->conntime);
            srv << "  is playing on: " << (s->title ? s->title.str() : "<no title>")
                << " (" << ipbuf << ":" << (uint)s->addr.port << ")  (" << time2 << ")";
        }

        consoleoutf("[%d/%d] id: %d  name: %s  country: %s  (%s)%s",
                    i+1, clients.length(), c->id, c->name.str(),
                    c->country ? c->country : "unknown",
                    time1.str(), srv.str());
    }
}

static void wcchatgoto(const char *client)
{
    if (!checkconnection()) return;
    SDL_Mutex_Locker m(clientmutex);

    vector<client_t*> matches;
    if (!findclients(client, matches)) return;

    client_t *c = matches[0];
    auto *s = &c->server;

    if (!s->addr.host)
    {
        consoleoutf("%s is not connected to any server",
                    c->getname().str());
        return;
    }

    if (curpeer && addressequal(curpeer->address, s->addr))
    {
        fpsent *d = game::getclient(s->cn);
        if (!d && game::player1->clientnum == s->cn) d = game::player1;
        if (d && c->name == d->name)
        {
            consoleoutf("you are playing on the same server");
            return;
        }
    }

    string ipbuf;
    resolve(s->addr.host, ipbuf);
    defformatstring(connect)("connect %s %d", ipbuf, s->addr.port);
    execute(connect);
}

static void wcchatisconnected()
{
    intret(isconnected);
}

static void wcchatlatency(int *cubescript)
{
    if (!checkconnection(!*cubescript))
    {
        if (*cubescript) floatret(0);
        return;
    }
    float latency = myping.load()/1000.0f;
    if (*cubescript) floatret(latency);
    else consoleoutf("latency: %.2f ms", latency);
}

static void wcchat(const char *msg)
{
    if (!checkconnection()) return;

    if (!msg[0])
    {
        erroroutf_r("empty message");
        return;
    }

    if (strlen(msg) > 200)
    {
        erroroutf_r("message too long");
        return;
    }

    uchar sendbuf[MAXDATA];
    ucharbuf p(sendbuf, sizeof(sendbuf));

    putint(p, C_CHAT_MSG);
    putint(p, -1);
    sendstring(msg, p);
    send(p);

    SDL_Mutex_Locker m(clientmutex);

    client_t *c = getclient(myid);
    if (c) consoleoutf(CON_CHAT, "\f1%s: \f0%s", c->getname().str(), msg);

    if (clients.length() <= 1)
        consoleoutf("info: no other clients connected");
}

static void wcchatconnect()
{
    if (!wcchatserver[0])
    {
        consoleoutf(CON_ERROR, "'/wcchatserver' must be set! "
                   "(/wcchatserver [ssl:]<host>[:<port>])");
        return;
    }
    if (isconnecting)
    {
        consoleoutf(CON_ERROR, "already connecting to server");
        return;
    }
    if (isconnected)
    {
        consoleoutf(CON_ERROR, "already connected to server");
        return;
    }
    if (thread) return;

    static bool eventinstalled = false;

    if (!eventinstalled)
    {
        event::xinstall(event::SHUTDOWN, event::l2f([](){ wcchatdisconnect(); }));

        event::xinstall(event::FRAME, event::l2f([](){
            if (needserverinfo && game::fullyconnected && curpeer)
                servconnect(true);
        }));

        eventinstalled = true;
    }

    connectdatahaschanged = false;
    thread = new sdlthread(chatthread);
}

static inline void wcchatdoautoconnect()
{
    if (wcchatautoconnect && inited && !thread && !isconnecting && !isconnected)
    {
        {
            SDL_Mutex_Locker m(connectdatamutex);
            if (!connectdata.server) return;
        }

        wcchatconnect();
    }
}

static void wcchatdisconnect()
{
    if (!thread) return;
    shutdown = true;
    delete thread;
    thread = NULL;
    assert(!isconnecting && !isconnected);
    shutdown = false;
}

static bool checkfingerprint(strtool &hash)
{
    size_t length = hash.length();
    size_t sepcount = hash.count(":");
    switch (length)
    {
        case 47:  if (sepcount == 15) goto valid; break; // MD5
        case 59:  if (sepcount == 19) goto valid; break; // SHA1
        case 83:  if (sepcount == 27) goto valid; break; // SHA224
        case 95:  if (sepcount == 31) goto valid; break; // SHA256
        case 143: if (sepcount == 47) goto valid; break; // SHA384
        case 191: if (sepcount == 63) goto valid; break; // SHA512
    }
    erroroutf_r("invalid fingerprint");
    return false;
    valid:;
    hash.upperstring();
    return true;
}

static void wcchataddcert(const char *hash, const char *host)
{
    strtool h = hash;
    if (!checkfingerprint(h)) return;
    SDL_Mutex_Locker m(sslmutex);
    for (auto &cert : validcerts)
    {
        if (cert != hash) continue;
        if (cert.host.empty() || cert.host == host) return;
    }
    validcerts.add(certificate_t(h, host));
}

static void wcchatdelcert(const char *hash)
{
    SDL_Mutex_Locker m(sslmutex);
    bool ok = false;

    if (strlen(hash) == 59)
    {
        int len = validcerts.length();
        validcerts.removeobj(hash);

        if (len != validcerts.length())
            ok = true;
    }
    else
    {
        int i = atoi(hash)-1;
        if (validcerts.inrange(i))
        {
            validcerts.remove(i);
            ok = true;
        }
    }

    if (ok) conoutf("certificate removed");
    else erroroutf_r("invalid certificate");
}

static void wcchatdelcerts()
{
    SDL_Mutex_Locker m(sslmutex);
    validcerts.shrink(0);
    conoutf("certificates cleared");
}

static void wcchatlistcerts()
{
    SDL_Mutex_Locker m(sslmutex);
    if (validcerts.empty())
    {
        conoutf("no certificates added");
        return;
    }
    loopv(validcerts)
    {
        certificate_t &cert = validcerts[i];
        strtool hash = cert.hash;

        if (hash.length() > 59)
        {
            hash.remove(59, hash.length()-59);
            hash += "...";
        }

        conoutf("%d. %s fingerprint: %s  hostname: %s",
                i+1, cert.hashtype(), hash.str(),
                cert.host ? cert.host.str() : "*");
    }
}

static void wcchatloopclients(const char *callbackargs, const char *callbackcode)
{
    uint callbackid;
    uint scriptcallbackid = event::install(event::INTERNAL_CALLBACK, callbackargs, callbackcode, &callbackid);
    uint cbevent = callbackid<<16|event::INTERNAL_CALLBACK;

    if (!scriptcallbackid) return;

#ifdef ENABLE_IPS
    constexpr const char *args = "siisssiisssuuuuuu";
#else
    constexpr const char *args = "siisssiisssuu";
#endif

    time_t now = time(NULL);

    clientmutex.lock();

    for (client_t *c : clients)
    {
        struct client_t::server &s = c->server;
        string sip;

        if (!s.addr.host || enet_address_get_host_ip(&s.addr, sip, sizeof(sip)) < 0)
            memcpy(sip, "0.0.0.0", 8);

        event::run(cbevent, args, c->getname().str(), c->id, int(now-c->conntime), c->country ? c->country : "",
                                  c->countrycode ? c->country : "",
                                  s.title.str(), s.cn, int(now-s.conntime),
                                  s.country ? s.country : "", s.countrycode ? s.countrycode : "",
                                  sip, s.addr.host, s.addr.port,
                                  c->ip.ui[0], c->ip.ui[1], c->ip.ui[2], c->ip.ui[3] /* 0xff */);
    }

    // name id sec-connected country countrycode
    // servername cn sec-connected servercountry servercountrycode hostname host(int) port

    clientmutex.unlock();

    event::uninstall(scriptcallbackid);
}

static void wcchatshowcertdetails()
{
    SDL_Mutex_Locker m(sslmutex);

    if (!opensslinited)
    {
        SSL_library_init();
        opensslinited = true;
    }

    vector<X509*> cacerts;
    loadcacerts(cacerts);
    consoleoutf("");

    loopv(cacerts)
    {
        consoleoutf("root certificate %d/%d:", i+1, cacerts.length());
        showcertdetails(cacerts[i]);
        consoleoutf("");
    }

    while (!cacerts.empty()) X509_free(cacerts.pop());

    if (isconnected && usessl)
    {
        consoleoutf("certificate of current server:");
        showcertdetails(sslconn.cert);
    }
}

static void wcchataddrevokedcert(const char *hash)
{
    strtool h = hash;
    if (!checkfingerprint(h)) return;
    SDL_Mutex_Locker m(sslmutex);
    if (revokedcerts.find(h) >= 0) return;
    revokedcerts.add(h);
}

void addrevokedcert(const char *hash)
{
    wcchataddrevokedcert(hash);
}

void writecerts(stream *f)
{
    SDL_Mutex_Locker m(sslmutex);

    auto writecerts = [&](vector<certificate_t> &certs, const char *desc)
    {
        if (certs.empty())
            return;

        f->printf("\n// %s%schat certificates\n", desc, desc[0] ? " " : "");

        for (auto &cert : certs)
        {
            if (cert.host)
            {
                f->printf("wcchatadd%scert %s %s\n",
                          desc, escapestring(cert.hash.str()),
                          escapestring(cert.host.str()));
                continue;
            }

            f->printf("wcchatadd%scert %s\n",
                      desc, escapestring(cert.hash.str()));
        }
    };

    writecerts(validcerts, "");
    writecerts(revokedcerts, "revoked");
}

void setupbinds()
{
    constexpr const char *action =
        "inputcommand \"\" [wcchat $commandbuf] \"^fs^f9[global chat]^fr\"";

    auto setupbind = [&](int key)
    {
        defformatstring(buf)("bind %c [ %s ];", key, action);
        execute(buf);
    };

    delbinds(action);

    const char *keys = wcchatbinds;
    while (*keys) setupbind(*keys++);
}

void init()
{
    inited = true;
    wcchatdoautoconnect();
}

} // namespace chat
} // namespace mod
