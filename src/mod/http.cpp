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

#include "cube.h"
#include <curl/curl.h>

namespace mod {
namespace http
{
    static const int THREADLIMIT = 10;
    static const int RECEIVELIMIT = 2*1024*1024;
    static const int RECEIVEPREALLOC = 100*1024;
    static const long CONNECTTIMEOUT = 5L;

    string defaultuseragent = {0};

    static inline void gendefaultuseragent()
    {
        static const char *os = getosstring();

        formatstring(defaultuseragent)("wc-ng/%s (%s)",
                     CLIENTVERSION.str().str(), os);
    }

    struct http_t;
    static vector<http_t*> requests;

    size_t contentcallback(void *data, size_t size, size_t nmemb, http_t *http);
    int requestthread(http_t *http);

    static vector<uint> availableids;

    static SDL_mutex *varlock = NULL;
    static string proxy = {0};
    static int speedlimit = 0;

    static void setproxy(const char *newproxy)
    {
        if (!varlock)
            abort();

        SDL_Mutex_Locker m(varlock);
        copystring(proxy, newproxy);
    }

    static void getproxy(char *buf, size_t size)
    {
        SDL_Mutex_Locker m(varlock);
        copystring(buf, proxy, size);
    }

    static void setspeedlimit(int limit)
    {
        SDL_Mutex_Locker m(varlock);
        speedlimit = limit*1024;
    }

    static void getspeedlimit(int& limit)
    {
        SDL_Mutex_Locker m(varlock);
        limit = speedlimit;
    }

    MODSVARFP(httpproxy, "", setproxy(httpproxy));
    MODVARFP(httpspeedlimit, 0, 0, 0x1FFFFF, setspeedlimit(httpspeedlimit));

    struct http_t
    {
        SDL_mutex *requestmutex;
        bool processed;
        bool wantuninstall;
        vector<char> data;
        SDL_Thread *thread;
        CURL *curl;
        CURLcode requestresult;
        long responsecode;
        bool requestok;
        uint requestid;
        int speedlimit;
        bool canuninstall;
        time_t lastmodified;
        ullong downloadstart;

        request_t *request;

        bool isprocessed()
        {
            SDL_Mutex_Locker m(requestmutex);
            return processed;
        }

        bool uninstallrequest()
        {
            SDL_Mutex_Locker m(requestmutex);
            return wantuninstall;
        }

        void process()
        {
            if (!(thread = SDL_CreateThread((int (*)(void *))requestthread, this)))
                fatal("failed to create http request thread");
        }

        uint getrequestid()
        {
            return requestid;
        }

        http_t(request_t *request) :
                  requestmutex(SDL_CreateMutex()), processed(false), wantuninstall(false), thread(NULL), curl(NULL),
                  canuninstall(true), lastmodified(0), downloadstart(0), request(request)
        {
            if (!requestmutex)
                fatal("failed to create request mutex");

            getspeedlimit(speedlimit);

            static uint nextrequestid = 0;
            requestid = !availableids.empty() ? availableids.pop() : ++nextrequestid;

            process();
        }

        ~http_t()
        {
            SDL_DestroyMutex(requestmutex);

            if (thread)
                SDL_WaitThread(thread, NULL);

            delete request;
            availableids.add(getrequestid());
        }
    };

    size_t contentcallback(void *data, size_t size, size_t nmemb, http_t *http)
    {
        request_t *req = http->request;

        if (req->callback && data && !http->uninstallrequest()) // no need to store data, if there is no callback anyway
        {
            if (!http->downloadstart)
                http->downloadstart = getmilliseconds();

            size_t realsize = size*nmemb;

            size_t size = min((int)realsize, RECEIVELIMIT-http->data.length());
            http->data.add(data, size);

            int speedlimit;
            getspeedlimit(speedlimit);

            if (http->speedlimit != speedlimit)
            {
                http->speedlimit = speedlimit;
                curl_easy_setopt(http->curl, CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t)http->speedlimit);
            }

            if (req->contentcallback)
            {
                http->data.add('\0');
                int v = req->contentcallback(http->getrequestid(), req->callbackdata, http->data.getbuf(), http->data.length()-1);
                http->data.pop();

                switch (v)
                {
                    case 0: return 0;
                    case 2: http->data.setsize(0);
                }
            }

            if (size == realsize)
                return realsize;
        }

        return 0;
    }

    int downloadstatuscallback(http_t *http, double downloadsize, double downloaded, double uploadsize, double uploaded)
    {
        request_t *req = http->request;
        ullong elapsed = http->downloadstart ? getmilliseconds()-http->downloadstart : 0;
        return req->statuscallback(http->getrequestid(), req->callbackdata, downloadsize, downloaded, elapsed) ? 0 : -1;
    }

    int requestthread(http_t *http)
    {
        request_t *req = http->request;
        http->curl = curl_easy_init();

        if (!http->curl)
            abort();

        curl_slist *headers = NULL;
        string proxy;

        #define setopt(OPT, VAL)                                                   \
        do                                                                         \
        {                                                                          \
            if (curl_easy_setopt(http->curl, OPT, VAL) != CURLE_OK)                \
            {                                                                      \
                conoutf_r("curl_easy_setopt() failed: %s:%d", __FILE__, __LINE__); \
                http->responsecode = QUERY_ABORTED;                                \
                goto error;                                                        \
            }                                                                      \
        } while (0)

        setopt(CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);
        setopt(CURLOPT_URL, req->request.str());
        setopt(CURLOPT_WRITEFUNCTION, &contentcallback);
        setopt(CURLOPT_WRITEDATA, http);
        setopt(CURLOPT_FOLLOWLOCATION, 1L); // follow redirects
        setopt(CURLOPT_FILETIME, 1L);
        setopt(CURLOPT_NOSIGNAL, 1L);

        setopt(CURLOPT_SSL_VERIFYPEER, 1L);

        if (req->cacert)
        {
            setopt(CURLOPT_CAPATH, "/dev/null");
            setopt(CURLOPT_CAINFO, req->cacert.str());
        }

        if (req->statuscallback)
        {
            setopt(CURLOPT_NOPROGRESS, 0L);
            setopt(CURLOPT_PROGRESSDATA, http);
            setopt(CURLOPT_PROGRESSFUNCTION, &downloadstatuscallback);
        }

        if (!req->headers.empty())
        {
             loopv(req->headers)
                headers = curl_slist_append(headers, req->headers[i].header);

             setopt(CURLOPT_HTTPHEADER, headers);
        }

        if (req->referer)
        {
            setopt(CURLOPT_REFERER, req->referer.str());
            setopt(CURLOPT_AUTOREFERER, 1L); // send referer
        }

        if (req->useragent)
            setopt(CURLOPT_USERAGENT, req->useragent.str());

        getproxy(proxy, sizeof(proxy));

        if (*proxy)
            setopt(CURLOPT_PROXY, proxy);

        if (http->speedlimit)
            setopt(CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t)http->speedlimit);

        http->data.reserve(RECEIVEPREALLOC); // try to avoid re-allocs in contentcallback

        http->requestresult = curl_easy_perform(http->curl);

        if (headers)
            curl_slist_free_all(headers);

        http->requestok = (http->requestresult == CURLE_OK ||
                           http->requestresult == CURLE_WRITE_ERROR); // happens if contentcallback returns 0 - but count it as ok

        switch (http->requestresult)
        {
            case CURLE_WRITE_ERROR:
                http->responsecode = QUERY_ABORTED;
                break;

            case CURLE_SSL_CONNECT_ERROR: case CURLE_SSL_ISSUER_ERROR:
            case CURLE_SSL_CRL_BADFILE:   case CURLE_SSL_CACERT:
                http->responsecode = SSL_ERROR;
                break;

            default:
                if (curl_easy_getinfo(http->curl, CURLINFO_HTTP_CODE, &http->responsecode) != CURLE_OK)
                    http->responsecode = INVALID_RESPONSE_CODE;
        }

        http->data.add('\0');
        http->data.resize(); // save memory

        long lastmodified;
        if (curl_easy_getinfo(http->curl, CURLINFO_FILETIME, &lastmodified) == CURLE_OK && lastmodified > 0)
            http->lastmodified = lastmodified;

        error:;

        curl_easy_cleanup(http->curl);
        http->curl = NULL;

        {
            SDL_Mutex_Locker m(http->requestmutex);
            http->processed = true;
        }

        return 0;
    }

    void slice()
    {
        loopv(requests) // always try to process them in the row they have been added
        {
            http_t *http = requests[i];

            if (http->isprocessed())
            {
                request_t *req = http->request;

                if (req->callback)
                {
                    http->canuninstall = false;

                    size_t length = http->requestok && http->data.length() ? http->data.length()-1 : 0;

                    req->callback(http->getrequestid(), req->callbackdata,
                                  http->requestok, http->responsecode,
                                  http->requestok ? http->data.getbuf() : NULL,
                                  length);

                    http->canuninstall  = true;
                }

                requests[i] = NULL;
                delete req;
            }
        }

        loopvrev(requests) if (!requests[i]) requests.remove(i);
    }

    time_t getfiletime(uint id)
    {
        loopv(requests)
        {
            http_t *http = requests[i];

            if (http && http->isprocessed() && http->getrequestid() == id)
                return http->lastmodified;
        }

        return 0;
    }

    void startup()
    {
        gendefaultuseragent();

        if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK)
            fatal("failed to init curl");

        if (!(varlock = SDL_CreateMutex()))
            abort();
    }

    void shutdown()
    {
        int maxwait = 250; // wait up to 250 msec until the requests will be killed forcefully

        while (!requests.empty())
        {
            static bool msglogged = false;

            if (!msglogged)
            {
                conoutf(CON_WARN, "waiting until all http requests have finished ...");
                msglogged = true;
            }

            uint start = getmilliseconds();
            uninstallrequest(requests[0]->getrequestid(), false, maxwait);
            maxwait -= min(int(getmilliseconds()-start), maxwait);
        }

        curl_global_cleanup();

        SDL_DestroyMutex(varlock);
        varlock = NULL;
    }

    uint request(request_t *request)
    {
        if (requests.length() >= THREADLIMIT)
        {
            delete request;
            return UNABLE_TO_PROCESS_REQUEST_NOW;
        }

        if (!request->useragent)
            request->useragent = defaultuseragent;

        if (!request->cacert)
            request->cacert = "data/CA/ca-bundle.crt";

        strtool cacert = findfile(path(request->cacert.str(), true), "rb");

        if (!fileexists(cacert.str(), "rb"))
        {
            erroroutf_r("%s(): '%s' does not exist", __func__, cacert.str());
            request->cacert.clear();
        }
        else request->cacert = cacert;

        http_t *http = new http_t(request);
        requests.add(http);

        return http->getrequestid();
    }

    bool uninstallrequest(uint id, bool async, uint maxwait)
    {
        http_t *http;
        bool retval = false;

        do
        {
            http = NULL;

            loopv(requests)
            {
                http = requests[i];

                if (!http->canuninstall)
                    return false;

                if (http->getrequestid() == id)
                {
                    retval = true;
                    break;
                }
            }

            if (http)
            {
                if (maxwait != WAIT_INFINITE && !maxwait--) // try to shutdown request gracefully first
                {
                    // timeout reached, kill thread forcefully
                    // only do this when quitting the whole client to avoid freezes

                    conoutf(CON_DEBUG, "timeout reached, killing http request (%d) forcefully", http->getrequestid());
                    SDL_KillThread(http->thread);
                    http->thread = NULL;
                    http->data.disown();
                    http->responsecode = QUERY_KILLED;
                    http->processed = true;
                    slice();

                    retval = true;
                    break;
                }

                {
                    SDL_Mutex_Locker m(http->requestmutex);
                    http->wantuninstall = true;
                }

                if (async)
                    break;

                slice();
                SDL_Delay(1);
            }

        } while (http);

        return retval;
    }

} // namespace http
} // namespace mod
