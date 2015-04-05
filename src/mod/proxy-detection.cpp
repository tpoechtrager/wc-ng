/***********************************************************************
 *  WC-NG - Cube 2: Sauerbraten Modification                           *
 *  Copyright (C) 2015 by Thomas Poechtrager                           *
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

#include "game.h"

namespace mod {
namespace proxydetection {

using ipbuf = network::ipbuf<>;

constexpr struct
{
    const char *const URL;
    const char *const CACERT;
    const bool COMPRESSED;
    const bool BINARY;
} PROXY_DETECTION_LIST_MIRROR =
{
    endiancond("https://wc-ng.sauerworld.org/proxy-list.bin.le.compressed",
               "https://wc-ng.sauerworld.org/proxy-list.bin.be.compressed"),
    "data/CA/wc-ng-ca.crt", true, true
};

static network::ipbuf<1, 1024 > blacklistedservers;
static network::ipbuf<4, 10240> ips;
static bool updating;
static bool inited;
extern int detectproxyusers;

static inline bool ipbuffull()
{
    constexpr uint32_t MAXIPRANGES = ips.maxsize()/10u;
    return ips.getsize()+blacklistedservers.getsize() >= MAXIPRANGES;
}

INLINE // force this function inline with -flto
ipbuf::addrc_t addiprange(const ipmask &ip, int bits, ipbuf::addflags_t flags)
{
    bool ok;
    if (ipbuffull()) return ipbuf::addrc_t::ERR_IPBUF_FULL;
    if (flags&ipbuf::addflags_t::BLACKLISTED_SERVER) ok = blacklistedservers.addip(ip, 0, bits);
    ok = ips.addip(ip, 0, bits);
    return ok ? ipbuf::addrc_t::OK : ipbuf::addrc_t::ERR;
}

static void updatecallback(uint requestid, void *cbdata, bool ok,
                           int responsecode, void *data, size_t datalen)
{
    void *udata;
    updating = false;

    if (!ok)
    {
        if (responsecode == http::SSL_ERROR) erroroutf_r("proxy detection list: can't verify ssl certificate");
        else erroroutf_r("proxy detection list: server unreachable");
        return;
    }

    if (responsecode != 200)
    {
        erroroutf_r("proxy detection list mirror replied with an "
                    "invalid http response code '%d'", responsecode);
        return;
    }

    if (PROXY_DETECTION_LIST_MIRROR.COMPRESSED)
    {
        if (uncompress(data, datalen, &udata, datalen, 10*1024*1024, true) != UNCOMPRESS_OK)
        {
            erroroutf_r("proxy detection list: can't uncompress data");
            return;
        }
        data = udata;
    }

    ips.clear();
    blacklistedservers.clear();

    if (PROXY_DETECTION_LIST_MIRROR.BINARY) network::parsebin(data, datalen, addiprange);
    else network::parsetext((char*)data, datalen, addiprange);

    strtool b1, b2;

    conoutf("received proxy detection list (%s range%s, %s ips)",
            makenumreadable(ips.getsize(), b1), plural(ips.getsize()),
            makenumreadable(ips.gethostcount(), b2));

    if (PROXY_DETECTION_LIST_MIRROR.COMPRESSED) free(udata);
}

static void updatelist()
{
    if (updating)
    {
        erroroutf_r("already downloading proxy detection list");
        return;
    }

    auto *httpreq = new http::request_t;

    httpreq->request = PROXY_DETECTION_LIST_MIRROR.URL;
    if (PROXY_DETECTION_LIST_MIRROR.CACERT) httpreq->cacert = PROXY_DETECTION_LIST_MIRROR.CACERT;
    httpreq->callback = updatecallback;
    httpreq->expecteddatalen = 500*1024;
    updating = true;

    if (http::request(httpreq) == http::UNABLE_TO_PROCESS_REQUEST_NOW)
    {
        conoutf("can't download proxy detection list");
        updating = false;
    }
}

bool isblacklistedserver(const ENetAddress &addr)
{
    if (!detectproxyusers) return false;
    return !!blacklistedservers.findip(addr.host);
}

bool isproxy(uint32_t ip)
{
    if (!detectproxyusers) return false;
    return !!ips.findip(ip);
}

bool usesproxy(void *client)
{
    fpsent *d = (fpsent*)client;
    return d->extinfo && isproxy(d->extinfo->ip.ui32);
}

static void varchanged()
{
    if (!inited) return;
    if (detectproxyusers) updatelist();
}

void init()
{
    if (detectproxyusers) updatelist();
    inited = true;
}

void isproxy_(const char *addr)
{
    if (!detectproxyusers)
    {
        conoutf("'/isproxy' only works with '/detectproxyusers 1'");
        return;
    }
    ipmask ip(addr);
    if (!ip.ok() || ip.getbitcount() < 24)
    {
        intret(0);
        return;
    }
    intret(isproxy(ip.ip));
}

static void listproxydetectionips(const char *s)
{
    struct args
    {
        const char *s;
        int i{};
        args(const char *s) : s(s) {}
    } a(s);

    auto showproxyip = [](const ipmask &ip, char, args &a)
    {
        a.i++;
        string ipstr;
        ip.print(ipstr);
        if (!*a.s || strstr(ipstr, a.s)) conoutf("%d. %s", a.i, ipstr);
        return true;
    };

    ips.loopips<args&>(showproxyip, a);
}

COMMAND(listproxydetectionips, "s");
COMMANDN(isproxy, isproxy_, "s");
COMMANDN(updateproxydetectionlist, updatelist, "");
MODVARFP(detectproxyusers, 0, 0, 1, varchanged());

} // namespace proxydetection
} // namespace mod
