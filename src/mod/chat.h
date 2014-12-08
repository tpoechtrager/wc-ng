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


#ifndef STANDALONE
namespace mod {
namespace chat {
#endif //STANDALONE

enum
{
    C_CONNECT,
    C_CONNECTED,
    C_DISCONNECT,
    C_INVALID_PROTOCOL,
    C_INVALID_PASSWORD,
    C_CLIENT_CONNECT,
    C_CLIENT_DISCONNECT,
    C_CLIENT_RENAME,
    C_CHAT_MSG,
    C_SERVER_INFO,
    C_PING,
    C_PONG,
    C_TIME,
    C_AUTH_TOK
};

enum
{
    CS_NO_SSL,
    CS_USE_SSL,
    CS_DISC_SERVER_FULL,
    CS_DISC_IP_BANNED
};

static const int PROTOCOL_VER = 4;

static const size_t MAXDATA = 0xFFFF;
static const size_t MAXMSGLEN = 512u;
static const int INITTIMEOUT = 5000;
static const int SSLINITTIMEOUT = 3000;
static const int PINGTIMEOUT = 20000;
static const int PINGSENDTHRESHOLD = 5000;
static const int RECONNECTTHRESHOLD = 15000;
static const int NUMMAXCLIENTS = min<int>(MAXDATA/512, FD_SETSIZE-5);
static const int DEFAULTPORT = 6664;

static const int CONNECTRECVTIMEOUT = 3000;
static const char *const DEFAULTPREFIX = "\fs\f9[c]\fr";

static const char *const CIPHERLIST =
    "ECDH+AES256:DH+AESGCM:DH+AES256:ECDH+AES:DH+AES"
    "RSA+AESGCM:!aNULL:!eNULL:!NULL:!SHA1:!DSS:!RC4:!MD5";

void updatename();
void servconnect(bool connect = true);
inline void servdisconnect() { servconnect(false); }
void addrevokedcert(const char *digest);
void writecerts(stream *f);
void setupbinds();
void init();

struct cmdopt
{
    typedef void (*cmdfun)(const char *val);

    cmdfun fun;
    const char *aliases[10];
    const char *helptext;

    mod::strtool names()
    {
        mod::strtool n;

        loopi(sizeofarray(aliases))
        {
            const char *alias = aliases[i];
            if (!alias) break;
            n << "-" << alias << ", ";
        }

        n -= 2;
        return n;
    }

    int run(const char *arg, const char *val = "")
    {
        loopi(sizeofarray(aliases))
        {
            const char *alias = aliases[i];
            size_t len;

            if (!alias)
                return 0;

            len = strlen(alias);

            if (!strncmp(arg, alias, len))
            {
                if (arg[len] && arg[len] != '=')
                    return 0;

                if (arg[len] == '=')
                {
                    val = &arg[len+1];

                    if (!*val)
                    {
                        conoutf(CON_ERROR, "missing value for "
                                           "argument '-%s'", alias);
                        return -1;
                    }
                }

                fun(val);
                return 1;
            }
        }

        return false;
    }
};

struct certificate_t
{
    mod::strtool hash;
    mod::strtool host;

    const char *hashtype()
    {
        switch (hash.length())
        {
            case 47: return "md5";
            case 59: return "sha1";
            case 83: return "sha224";
            case 95: return "sha256";
            case 143: return "sha384";
            case 191: return "sha512";
            default: return NULL;
        }
    }

    template<class STR>
    bool operator==(const STR &h) const { return hash == h; }
    template<class STR>
    bool operator!=(const STR &h) const { return hash != h; }

    bool operator==(const certificate_t &c) const { return *this == c.hash; }
    bool operator!=(const certificate_t &c) const { return *this != c.hash; }

    template<class STR1, class STR2 = const char*>
    certificate_t(const STR1 &hash, const STR2 &host = "")
      : hash(hash), host(host) {}
};

#ifndef STANDALONE
} // namespace chat
} // namespace mod
#endif //STANDALONE
