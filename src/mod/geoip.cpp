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

#include "game.h"
#include LIB_GEOIP_HEADER

namespace mod {
namespace geoip
{
    static const int GEOIP_CHARSET = GEOIP_CHARSET_ISO_8859_1;

    static const char *GEOIP_DB = "data/GeoIP.dat";
    static const char *GEOIP_UPDATE_TMP = "data/GeoIP.update.dat";

    static void checkcfgfile(const char *var, int& varval, int& prevarval)
    {
        const char *currentcfg = getcurrentcsfilenamep();

        if (!currentcfg || strcmp(game::wcconfig(), currentcfg))
        {
            varval = prevarval;
            erroroutf_r("value of '%s' can't be changed", var);
            return;
        }

        prevarval = varval;
    }

    #define CHANGE_RO_GEOIP_VARVAL(name, val) name = val; prev ## name = name
    #define CHECK_GEOIP_RO_VAR(var) checkcfgfile(#var, var, prev ## var)
    #define RO_GEOIP_VAR(name, min, cur, max) static int prev ## name = cur; MODVARFP(name, min, cur, max, CHECK_GEOIP_RO_VAR(name))

    RO_GEOIP_VAR(wcgeoipdblastmodified, 0, 0, 0x7FFFFFFF);
    RO_GEOIP_VAR(wcgeoipdblastupdatecheck, 0, 0, 0x7FFFFFFF);
    MODVARP(wcgeoipautoupdatecheck, 0, 1, 1);

    static const char *const DB_MIRRORS[] =
    {
        "http://geolite.maxmind.com/download/geoip/database/GeoLiteCountry/GeoIP.dat.gz"
    };

    static const int UPDATE_CHECK_INTERVAL = 1*60*60*24;

    static GeoIP *geoip = NULL;
    static sdlmutex geoipmutex;

    bool loaddatabase(const char *db)
    {
        SDL_Mutex_Locker m(geoipmutex);

        if (geoip)
            GeoIP_delete(geoip);

        geoip = GeoIP_open(db, GEOIP_MEMORY_CACHE);

        if (geoip)
        {
            geoip->charset = GEOIP_CHARSET;
            return true;
        }

        return false;
    }

    void closedatabase()
    {
        SDL_Mutex_Locker m(geoipmutex);

        if (geoip)
            GeoIP_delete(geoip);

        geoip = NULL;
    }

    const char *country(const char *ip)
    {
        SDL_Mutex_Locker m(geoipmutex);

        if (!geoip)
            return NULL;

        return GeoIP_country_name_by_addr(geoip, ip);
    }

    const char *country(uint ip)
    {
        SDL_Mutex_Locker m(geoipmutex);

        if (!geoip)
            return NULL;

        return GeoIP_country_name_by_ipnum(geoip, ENET_NET_TO_HOST_32(ip));
    }

    const char *countrycode(const char *ip)
    {
        SDL_Mutex_Locker m(geoipmutex);

        if (!geoip)
            return NULL;

        return GeoIP_country_code_by_addr(geoip, ip);
    }

    const char *countrycode(uint ip)
    {
        SDL_Mutex_Locker m(geoipmutex);

        if (!geoip)
            return NULL;

        return GeoIP_country_code_by_ipnum(geoip, ENET_NET_TO_HOST_32(ip));
    }

    static_assert(sizeofarray(GeoIP_country_code) == sizeofarray(GeoIP_country_name), "");

    const char *staticcountry(size_t index)
    {
        if (index >= sizeofarray(GeoIP_country_name)) return NULL;
        return GeoIP_country_name[index];
    }

    const char *staticcountrycode(const char *countrycode, size_t *index)
    {
        if (*index) *index = 0;
        assert(countrycode[0] && countrycode[1]);
        for (const char *scountrycode : GeoIP_country_code)
        {
            if (!scountrycode) break;
            if (*(uint16_t*)scountrycode == *(uint16_t*)countrycode) return scountrycode;
            if (index) ++*index;
        }
        return NULL;
    }

    void lookupcountry(const extinfo::playerv2 *ep, const char *&country, const char *&countrycode)
    {
        if (ep->ip.ui32 != uint32_t(-1))
        {
            countrycode = geoip::countrycode(ep->ip.ui32);
            if (countrycode)
            {
                country = geoip::country(ep->ip.ui32);
                return;
            }
        }
        if (ep->ext.havecountrycode())
        {
            size_t index;
            countrycode = staticcountrycode(ep->ext.countrycode, &index);
            if (countrycode)
            {
                country = staticcountry(index);
                return;
            }
        }
        country = NULL;
        countrycode = NULL;
    }

    static bool completeupdate();
    static void updatedatabase(bool silent = false, int mirror = 0);

    static void downloaddatabase(void *, uint)
    {
        CHANGE_RO_GEOIP_VARVAL(wcgeoipdblastmodified, 0);
        updatedatabase();
    }

    static void delayupdatecheck(void *, uint)
    {
        if (!wcgeoipautoupdatecheck)
            return;

        if (wcgeoipdblastupdatecheck <= 0 || time(NULL)-wcgeoipdblastupdatecheck >= UPDATE_CHECK_INTERVAL)
            updatedatabase(true);
    }

    void startup()
    {
        if (fileexists(findfile(GEOIP_UPDATE_TMP, "r"), "r"))
            completeupdate();

        const char *db = findfile(GEOIP_DB, "r");

        if (!fileexists(db, "r"))
        {
            downloaddb:;
            addtimerfunction(0, true, &downloaddatabase, NULL); // make sure httpproxy is loaded first
            return;
        }

        const char *homedir = gethomedir();

        if (*homedir && strstr(db, homedir) != db)
        {
            string tmp;
            strtool p(tmp, sizeof(tmp));

            p = db;
            p.fixpathdiv();

            conoutf("\f3attempting to remove %s because it is in the wrong path...", p.getbuf());

            if (remove(p.getbuf()) != 0)
            {
                conoutf("couldn't remove %s", p.getbuf());
                conoutf("your geoip database file is in the wrong place! it must be in your given home directory");
                conoutf("please remove %s from your sauerbraten installation directory and restart the client", p.getbuf());
#ifdef WIN32
                conoutf("starting the client once with administrator rights may also help");
#endif
            }
            else
            {
                db = findfile(GEOIP_DB, "r");
                if (!fileexists(db, "r"))
                    goto downloaddb;
            }
        }

        if (!loaddatabase(db))
            erroroutf_r("couldn't load geoip database");

        addtimerfunction(0, true, &delayupdatecheck, NULL); // make sure geoipdblastmodified is set first
    }

    void shutdown()
    {
        closedatabase();
    }

    static bool completeupdate()
    {
        closedatabase();

        string tmpfile, prevfile;

        copystring(prevfile, findfile(GEOIP_UPDATE_TMP, "r"));
        copystring(tmpfile, prevfile);

        *(strrchr(prevfile, '/')+1) = 0;
        concatstring(prevfile, strrchr(GEOIP_DB, '/')+1);

        path(prevfile);
        remove(prevfile);

        path(tmpfile);

        if (rename(tmpfile, prevfile) != 0)
        {
            conoutf("error updating geoip database: couldn't rename %s to %s", tmpfile, prevfile);
            return false;
        }

        return true;
    }

    static bool downloadingdb = false;

    static int contentcallback(uint requestid, int *info, void *data, size_t datalen)
    {
        int &messageshown = info[2];

        if (!messageshown)
        {
            conoutf_r("downloading new geoip database, please be patient...");
            messageshown = 1;
        }

        return 1;
    }

    static bool statuscallback(uint requestid, int *info, size_t totalsize, size_t downloaded, ullong mselapsed)
    {
        if (!totalsize)
            return true;

        int &guispawned = info[3];

        int percentdl = downloaded == totalsize ? 100 : (100.0f/totalsize)*downloaded;

        defformatstring(code)("if (! (=s (getalias \"geoip_download_status_update\") \"\")) [ geoip_download_status_update %d %d %d %d ]",
                              (int)totalsize, (int)downloaded, percentdl, (int)mselapsed);
        execute_r(code);

        if (!guispawned)
        {
            execute_r("if (! (=s (getalias \"show_geoip_download_status_gui\") \"\")) [ show_geoip_download_status_gui ]");
            guispawned = 1;
        }

        return true;
    }

    static void downloadcallback(uint requestid, int *info, bool ok, int responsecode, void *data, size_t datalen)
    {
        downloadingdb = false;
        stream *f = NULL;
        void *udata = NULL; size_t ulen;
        const char *homedir;

        int &silent = info[0];
        int &mirror = info[1];

        if (!ok || !datalen || responsecode != 200)
        {
            if (ok)
            {
                if (responsecode == 304)
                {
                    CHANGE_RO_GEOIP_VARVAL(wcgeoipdblastupdatecheck, time(NULL));

                    if (!silent)
                        conoutf("geoip database is up to date");

                    goto cleanup;
                }

                if (!silent)
                {
                    conoutf("error updating geoip database: geoip database mirror (%s) answered with invalid response code '%d'",
                            DB_MIRRORS[mirror], responsecode);
                }
            }
            else if (!silent)
            {
                conoutf("error updating geoip database: geoip database mirror (%s) isn't answering",
                        DB_MIRRORS[mirror]);
            }

            if (++mirror < (int)sizeofarray(DB_MIRRORS))
            {
                if (!silent)
                    conoutf("trying next geoip database mirror...");

                updatedatabase(!!silent, mirror);
                goto cleanup;
            }

            goto error;
        }

        if (uncompress(data, datalen, &udata, ulen, 10*1024*1024) != UNCOMPRESS_OK)
            goto error;

        homedir = gethomedir();

        if (*homedir)
        {
            string tmp;
            strtool dir(tmp, sizeof(tmp));

            dir = homedir;
            dir += PATHDIV;
            dir += GEOIP_UPDATE_TMP;

            dir.popuntil('/');
            dir.fixpathdiv();

            dir.finalize();

            if (!fileexists(dir.getrawbuf(), "r") && !createdir(dir.getrawbuf()))
            {
                conoutf("error updating geoip database: couldn't create (%s) directory", dir.getrawbuf());
                goto cleanup;
            }
        }

        f = openrawfile(GEOIP_UPDATE_TMP, "wb");

        if (!f)
        {
            conoutf("error updating geoip database: couldn't open (%s) for writing", GEOIP_UPDATE_TMP);
            goto cleanup;
        }

        if (f->write(udata, ulen) != ulen)
        {
            remove(GEOIP_UPDATE_TMP);
            goto error;
        }

        f->close();

        if (!completeupdate())
            goto error;

        if (loaddatabase(findfile(GEOIP_DB, "r")))
        {
            time_t lastmodified = http::getfiletime(requestid);

            if (!lastmodified || lastmodified > 0x7FFFFFFF)
            {
                erroroutf_r("geoip database mirror server sent invalid last modified date");
                CHANGE_RO_GEOIP_VARVAL(wcgeoipdblastmodified, 0);
            }
            else
                CHANGE_RO_GEOIP_VARVAL(wcgeoipdblastmodified, (int)lastmodified);

            CHANGE_RO_GEOIP_VARVAL(wcgeoipdblastupdatecheck, time(NULL));

            conoutf("geoip database update complete");
            goto cleanup;
        }

        error:;
        if (!silent)
            erroroutf_r("geoip database update failed");

        cleanup:;
        free(udata);
        delete f;
    }

    static void freecallbackdata(void *data)
    {
        delete[] (int*)data;
    }

    static void updatedatabase(bool silent, int mirror)
    {
        if (downloadingdb)
        {
            if (!silent)
                erroroutf_r("already checking for geoip database updates");

            return;
        }

        vector<http::header_t> headers;

        if (wcgeoipdblastmodified)
        {
            string &header = headers.add().header;
            time_t lastmodified = wcgeoipdblastmodified;
            struct tm *tm = gmtime(&lastmodified);
            strftime(header, sizeof(header), "If-Modified-Since: %a, %d %b %Y %H:%M:%S GMT", tm);
        }

        int *info = new int[4];

        info[0] = silent;
        info[1] = mirror;
        info[2] = 0;
        info[3] = 0;

        auto *httpreq = new http::request_t;

        httpreq->request = DB_MIRRORS[mirror];
        httpreq->headers = headers;

        httpreq->callbackdata = info;
        httpreq->callback = (http::callback_t)downloadcallback;
        httpreq->contentcallback = (http::contentcallback_t)contentcallback;
        httpreq->statuscallback = (http::statuscallback_t)statuscallback;
        httpreq->freedatacallback = freecallbackdata;

        if (!http::request(httpreq))
        {
            if (!silent)
                erroroutf_r("can't check for geoip updates: http is queue is full");

            return;
        }

        downloadingdb = true;

        if (!mirror && !silent)
            conoutf_r("checking for geoip database updates...");
    }

    ICOMMAND(wcupdategeoipdatabase, "", (), updatedatabase());

} // namespace geoip
} // namespace mod
