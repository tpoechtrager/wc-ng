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

#include <cube.h>

namespace mod {
namespace network {

void parsetext(char *iplist, size_t datalen, ipbuf<>::addfun_t addfun)
{
    const char *curip;
    bool blacklistedserver;
    bool end;
    const char *endp = iplist+datalen;
    uchar buf[sizeof(ipmask)];
    ipmask *ip;
    int bits;
    bool compact = false;

    auto iscomment = [](int token)
    {
        switch (token)
        {
            case '#': case ' ': case '\t': return true;
            default: return false;
        }
    };

    auto skipuntil = [&](int token, const bool stripcomments, const bool killtoken)
    {
        if (!stripcomments) while (*iplist && *iplist != token) iplist++;
        else
        {
            while (*iplist && *iplist != '\n' && !iscomment(*iplist)) iplist++;
            if (iscomment(*iplist))
            {
                *iplist++ = '\0';
                while (*iplist && *iplist != '\n') iplist++;
            }
        }
        end = !*iplist;
        if (killtoken) *iplist = '\0';
    };

    while (*iplist)
    {
        curip = iplist;
        switch (*iplist)
        {
            case '#': case ' ': case '\t': skipuntil('\n', false, false); goto next;
            case ':':
            {
                if (!strncmp(iplist, ":compact", 8)) compact = true;
                else if (!strncmp(iplist, ":std", 4)) compact = false;
                skipuntil('\n', false, false);
                goto next;
            }
        }
        skipuntil('\n', true, true);
        if (iplist == curip) goto next; // empty line
        switch (*curip)
        {
            // blacklisted server entry?
            case 'f': blacklistedserver = endp-curip > 4 && !memcmp(curip, "fi:\0", 4); curip += 4; break;
            default: blacklistedserver = false;
        }
        if (compact)
        {
            uint64_t compactip = strtoull(curip, NULL, 10);
            uint32_t ipnum = ENET_HOST_TO_NET_32(compactip/0x40);
            uint32_t bits = compactip%0x40;
            uint32_t mask;
            if (bits > 32) goto next;
            mask = bits ? ENET_HOST_TO_NET_32(0xFFFFFFFF << (32-bits)) : 0xFFFFFFFF;
            ip = new (buf) ipmask{ipnum, mask};
        }
        else ip = new (buf) ipmask(curip);
        if (ip->ok())
        {
            bits = ip->getbitcount();
            if (!blacklistedserver && bits >= 25)
            {
                ip->setbitcount(24);
                bits = 24;
            }
            auto flags = blacklistedserver ? ipbuf<>::addflags_t::BLACKLISTED_SERVER : ipbuf<>::addflags_t::NONE;
            if (addfun(*ip, bits, flags) == ipbuf<>::addrc_t::ERR_IPBUF_FULL) break;
        }
        next:;
        if (!end) iplist++; // \n
    }
}

void parsebin(const void *iplist, size_t datalen, ipbuf<>::addfun_t addfun)
{
    if (!isaligned<uint32_t>(iplist)) return;

    const char *p = (const char*)iplist;
    const char *end = p+datalen;

    const struct header
    {
        uint32_t numips;
        uint32_t bsnumips;
    } *hdr = (const header*)p;

    if (datalen < sizeof(header)) return;
    p += sizeof(header);

    const ipmask *ip = (const ipmask*)p;
    const ipmask *ipe = ip+hdr->numips;

    if ((const char*)ipe > end) return;
    while (ip < ipe && addfun(*ip++, -1, ipbuf<>::addflags_t::NONE) != ipbuf<>::addrc_t::ERR_IPBUF_FULL);

    ipe = ip+hdr->bsnumips;
    if ((const char*)ipe > end) return;
    while (ip < ipe && addfun(*ip++, -1, ipbuf<>::addflags_t::BLACKLISTED_SERVER) != ipbuf<>::addrc_t::ERR_IPBUF_FULL);
}

} // namespace network
} // namespace mod