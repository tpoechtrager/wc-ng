/***********************************************************************
 *  WC-NG - Cube 2: Sauerbraten Modification                           *
 *  Copyright (C) 2014, 2015, 2019 by Thomas Poechtrager               *
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
#include <maxminddb.h>

/*
 * Compatibility stuff.
 * It's not possible to get rid of it at the moment.
 * It would require lots of changes in this client
 * and server mods too.
 */
extern const char *GeoIP_country_name[256];
extern const char *GeoIP_country_code[256];

namespace mod {
namespace geoip
{
    struct country_t
    {
        char *name;
        char *code;

        bool operator==(const strtool &name)
        {
            return name == this->name;
        }
    };

    static vector<country_t> countries;

    static country_t getoraddcountry(strtool &name, strtool &code)
    {
        loopv(countries) if (countries[i] == name) return countries[i];
        return countries.add({newstring(name.str(), name.length()),
                              newstring(code.str(), code.length())});
    }

    static const char *GEOIP_DB = "data/GeoLite2-Country.mmdb";
    static const char *GEOIP_UPDATE_TMP = "data/GeoLite2-Country.update.mmdb";

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

    static struct
    {
        const char *const URL;
        const char *const CACERT;
    } DB_MIRRORS[] =
    {
        // This is the last publicly available GeoLite2 Country database.
        // https://forum.matomo.org/t/maxmind-is-changing-access-to-free-geolite2-databases/35439/3
        { "https://wc-ng.mooo.com/geoip-database/GeoLite2-Country.mmdb.gz" }
    };


    static const int UPDATE_CHECK_INTERVAL = 7*60*60*24;

    static MMDB_s mmdb;
    static bool mmdb_opened = false;

    static sdlmutex geoipmutex;

    bool loaddatabase(const char *db)
    {
        SDL_Mutex_Locker m(geoipmutex);

        if (mmdb_opened)
        {
            MMDB_close(&mmdb);
            mmdb_opened = false;
        }

        int status = MMDB_open(db, MMDB_MODE_MMAP, &mmdb);

        if (status == MMDB_SUCCESS)
        {
            mmdb_opened = true;
            return true;
        }

        return false;
    }

    void closedatabase()
    {
        SDL_Mutex_Locker m(geoipmutex);
        if (!mmdb_opened) return;
        MMDB_close(&mmdb);
        mmdb_opened = false;
    }

    bool country(uint ip, const char **country, const char **countrycode)
    {
        if (country) *country = NULL;
        if (countrycode) *countrycode = NULL;

        SDL_Mutex_Locker m(geoipmutex);
        if (!mmdb_opened) return false;

        struct sockaddr saddr;
        struct sockaddr_in *saddr_in = (struct sockaddr_in *)&saddr;
        memset(&saddr, 0, sizeof(saddr));
        saddr_in->sin_family = AF_INET;
        saddr_in->sin_addr = *(struct in_addr *)&ip;

        int error;
        MMDB_lookup_result_s result = MMDB_lookup_sockaddr(&mmdb, &saddr, &error);
        if (error != MMDB_SUCCESS || !result.found_entry) return false;

        MMDB_entry_data_s entry_data;

        error = MMDB_get_value(&result.entry, &entry_data, "country", "names", "en", NULL);
        if (error != MMDB_SUCCESS || !entry_data.has_data) return false;
        if (entry_data.type != MMDB_DATA_TYPE_UTF8_STRING) return false;

        strtool nametmp;
        strtool codetmp;

        nametmp.append(entry_data.utf8_string, entry_data.data_size);

        error = MMDB_get_value(&result.entry, &entry_data, "country", "iso_code", NULL);
        if (error != MMDB_SUCCESS || !entry_data.has_data) return false;
        if (entry_data.type != MMDB_DATA_TYPE_UTF8_STRING) return false;

        codetmp.append(entry_data.utf8_string, entry_data.data_size);

        country_t c = getoraddcountry(nametmp, codetmp);

        if (country) *country = c.name;
        if (countrycode) *countrycode = c.code;

        return true;
    }

    static_assert(sizeofarray(GeoIP_country_code) == sizeofarray(GeoIP_country_name), "");

    constexpr const char *continents[][2] =
    {
        { "AF", "Africa" },
        { "AS", "Asia" },
        { "EU", "Europe" },
        { "NA", "North America" },
        { "SA", "South America" },
        { "OC", "Ocenia" }
    };

    bool iscontinent(const char *name)
    {
        for (auto continent : continents) if (!strcmp(continent[1], name)) return true;
        return false;
    }

    bool staticcontinent(const char *nscontinentcode, const char **continent, const char **continentcode)
    {
        for (auto &c : continents)
        {
            if (*(const uint16_t*)c[0] == *(const uint16_t*)nscontinentcode)
            {
                if (continent) *continent = c[1];
                if (continentcode) *continentcode = c[0];
                return true;
            }
        }
        if (continent) *continent = NULL;
        if (continentcode) *continentcode = NULL;
        return false;
    }

    bool staticcountry(const char *nscountrycode, const char **country, const char **countrycode)
    {
        if (nscountrycode[0] == '-') goto ignore;
        loopi(sizeofarray(GeoIP_country_code))
        {
            const char *scountrycode = GeoIP_country_code[i];
            if (!scountrycode) break;
            if (*(const uint16_t*)scountrycode == *(const uint16_t*)nscountrycode)
            {
                if (country) *country = GeoIP_country_name[i];
                if (countrycode) *countrycode = GeoIP_country_code[i];
                return true;
            }
        }
        ignore:;
        if (country) *country = NULL;
        if (countrycode) *countrycode = NULL;
        return false;
    }

    void lookupcountry(const extinfo::playerv2 *ep, const char *&country, const char *&countrycode)
    {
        if (ep->ip.ui32 != uint32_t(-1))
        {
            if (geoip::country(ep->ip.ui32, &country, &countrycode)) return;
        }
        if (ep->ext.havecountrycode())
        {
            if (ep->ext.iscontinentcode())
            {
                char tmp[2]{(char)toupper(ep->ext.countrycode[0]), (char)toupper(ep->ext.countrycode[1])};
                if (staticcontinent(tmp, &country, &countrycode)) return;
            }
            else if (staticcountry(ep->ext.countrycode, &country, &countrycode)) return;
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

    void init()
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

    void deinit()
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
        if (!totalsize) return true;

        int &guispawned = info[3];

        int percentdl = downloaded == totalsize ? 100 : (100.0f/totalsize)*downloaded;

        defformatstring(code, "if (! (=s (getalias \"geoip_download_status_update\") \"\")) [ geoip_download_status_update %d %d %d %d ]",
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
                            DB_MIRRORS[mirror].URL, responsecode);
                }
            }
            else if (!silent)
            {
                if (responsecode == http::SSL_ERROR)
                    erroroutf_r("error updating geoip database: geoip database mirror (%s): can't verify ssl certificate", DB_MIRRORS[mirror].URL);
                else
                    erroroutf_r("error updating geoip database: geoip database mirror (%s): server not responding", DB_MIRRORS[mirror].URL);
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

        httpreq->request = DB_MIRRORS[mirror].URL;

        if (DB_MIRRORS[mirror].CACERT)
            httpreq->cacert = DB_MIRRORS[mirror].CACERT;

        httpreq->headers = headers;

        httpreq->expecteddatalen = 800*1024;
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
