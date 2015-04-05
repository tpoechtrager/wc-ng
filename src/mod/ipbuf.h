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

#ifndef __IPBUF_H__
#define __IPBUF_H__

namespace mod {
namespace network {

constexpr uint32_t HOSTCOUNT[33]
{
    0xFFFFFFFD, 2147483646, 1073741822, 536870910,
    268435454, 134217726, 67108862, 33554430,
    16777214, 8388606, 4194302, 2097150,
    1048574, 524286, 262142, 131070, 65534,
    32766, 16382, 8190, 4094, 2046, 1022, 510,
    254, 12, 62, 30, 14, 6, 2, 0, 1
};

constexpr int minsubnet(uint32_t n, int i = 0)
{
    return n >= HOSTCOUNT[i] ? i : minsubnet(n, i+1);
}

template<class T>
struct ipaddr
{
    ipmask ip;
    T data;
};

// ATTENTION: GLOBAL must be set to false when this class
// is not declared as a static or global symbol

template<uint8_t NODESIZE = 2, int MAXNODEENTRIES = -1,
         class DATATYPE = char, bool MUSTFREEDATA = false,
         bool GLOBAL = true>
class ipbuf
{
public:
    typedef ipaddr<DATATYPE> ipaddress;
    typedef vector<ipaddress, GLOBAL> ipaddressvec;
    typedef vector<ipaddress*> ipaddresspvec;

    // types for parsetext() and parsebin()

    enum addrc_t : int
    {
        OK,
        ERR,
        ERR_IPBUF_FULL
    };

    enum addflags_t : int
    {
        NONE = 0x0,
        BLACKLISTED_SERVER = 0x1
    };

    typedef addrc_t (*addfun_t)(const ipmask &ip, int bits, addflags_t flags);

    static constexpr uint32_t maxsize() { return 0x100u*NODESIZE*MAXNODEENTRIES; }
    static constexpr int minsubnet() { return ::mod::network::minsubnet((((1u<<(32-8))-2)/NODESIZE)); }

    bool addip(const ipmask &ip, DATATYPE data = DATATYPE(), int bits = -1)
    {
        constexpr int minbitcount = minsubnet();
        if (bits == -1) bits = ip.getbitcount();
        if (bits < minbitcount) return false;
        auto &iv = getvector(ip.ip);
        if (MAXNODEENTRIES != -1 && iv.length() >= MAXNODEENTRIES) return false;
        auto &ipa = iv.add();
        ipa.ip = ip;
        ipa.data = data;
        size++;
        hosts += ip.gethostcount();
        return true;
    }

    uint delip(const uint32_t ip, vector<ipmask> &removedips)
    {
        uint count = 0;
        auto &iv = getvector(ip);

        loopvrev(iv)
        {
            if (iv[i].ip.check(ip))
            {
                auto ipa = iv.remove(i);

                if (MUSTFREEDATA)
                    freedata(ipa.data);

                removedips.add(ipa.ip);

                count++;
                size--;
                hosts -= ipa.ip.gethostcount();
            }
        }

        return count;
    }

    INLINE ipmask *findip(const uint32_t ip, DATATYPE *data = NULL, ipaddress **addr = NULL, int *start = NULL)
    {
        auto &iv = getvector(ip);

        for (int i = (start ? *start : 0); i < iv.length(); i++)
        {
            if (iv[i].ip.check(ip))
            {
                if (data) *data = iv[i].data;
                if (addr) *addr = &iv[i];
                if (start) *start = i+1;
                return &iv[i].ip;
            }
        }

        return NULL;
    }

    uint findips(const uint32_t ip, ipaddresspvec &result)
    {
        int start = 0;

        do
        {
            ipaddress *ia;
            if (!findip(ip, NULL, NULL, &ia, &start)) break;
            result.add(ia);
        } while (true);

        return result.length();
    }

    uint findips(bool (*compare)(DATATYPE data, void *comparedata), void *comparedata, ipaddresspvec &result)
    {
        loopi(sizeofarray(ips))
        {
            auto &node = ips[i];

            loopj(NODESIZE)
            {
                auto &iv = node[j];
                loopvj(iv) if (compare(iv[j].datatype, iv[j].data, comparedata)) result.add(&iv[j]);
            }
        }

        return result.length();
    }

    template<class T>
    void loopips(bool (*callback)(const ipmask &ip, DATATYPE data, T cbdata), T cbdata = T())
    {
        loopi(sizeofarray(ips))
        {
            auto &node = ips[i];

            loopj(NODESIZE)
            {
                auto &iv = node[j];
                loopvj(iv) if (!callback(iv[j].ip, iv[j].data, cbdata)) return;
            }
        }
    }

    uint getallips(ipaddresspvec &result)
    {
        if (isempty()) return 0;
        if ((uint32_t)result.capacity() < getsize()) result.growbuf(getsize()+1);

        loopi(sizeofarray(ips))
        {
            auto &node = ips[i];
            loopj(NODESIZE)
            {
                auto &iv = node[j];
                loopvj(iv) result.add(&iv[j]);
            }
        }

        return result.length();
    }


    bool isempty() { return size == 0; }
    uint32_t getsize() { return size; }
    uint64_t gethostcount() { return hosts; }

    void clear()
    {
        loopi(sizeofarray(ips))
        {
            auto &node = ips[i];
            loopj(NODESIZE)
            {
                auto &iv = node[j];
                if (MUSTFREEDATA) loopvj(iv) freedata(iv[j].data);
                iv.setsize(0);
            }
        }

        size = 0;
        hosts = 0;
    }

    void swap(ipbuf &in)
    {
        uchar tmp[sizeof(*this)];
        memcpy(tmp, &in, sizeof(*this));
        memcpy(&in, this, sizeof(*this));
        memcpy(this, tmp, sizeof(*this));
    }

    ipbuf() { if (!GLOBAL) memset(this, 0, sizeof(*this)); }
    ~ipbuf() { clear(); }

private:
    template<class T>
    void freedata(T data) { delete data; }
    void freedata(void *data) { free(data); }
    void freedata(char) {}
    void freedata(int) {}

    INLINE ipaddressvec &getvector(const uint32_t ip)
    {
        return ips[ip&0xFF][((ip>>8)&0xFF)%NODESIZE];
    }

    ipaddressvec ips[0x100][NODESIZE];
    uint32_t size;
    uint64_t hosts;  // ranges may overlap
};

void parsetext(char *iplist, size_t datalen, ipbuf<>::addfun_t addfun);
void parsebin(const void *iplist, size_t datalen, ipbuf<>::addfun_t addfun);

} // namespace network
} // namespace mod

#endif // __IPBUF_H__
