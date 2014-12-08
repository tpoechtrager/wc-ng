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
using namespace mod;

void strtool::trim(bool front, strtool_size_t max, const char *trimchars)
{
    while (max--)
    {
        char c = front ? getfirst() : getlast();
        const char *p = trimchars;
        bool found = false;

        while (*p)
        {
            if (*p == c)
            {
                if (front)
                    popfirst();
                else
                    pop();

                found = true;
                break;
            }

            p++;
        }

        if (!found)
            break;
    }
}

void strtool::vfmt(const char *fmt, va_list v)
{
#ifdef STRTOOL_USE_REF_COUNTER
    checkreference();
#endif //STRTOOL_USE_REF_COUNTER

    va_list vlist;
    va_copy(vlist, v);

    if (autostorage)
    {
        strtool_size_t guessedlen = strlen(fmt)*2;

        if (capacity() < guessedlen)
            growbuf(guessedlen);
    }

    do
    {
        autogrow();

        int res = vsnprintf(p, left(), fmt, v);

        if (res < 0 || (strtool_size_t)res >=/*NULL byte*/ left())
        {
            if (autogrow(true, NULL, res > 0 ? res : 0))
            {
                va_end(v);
                va_copy(v, vlist);
                continue;
            }

            return;
        }

        p += res;

        va_end(v);
        break;
    } while(true);

    va_end(vlist);
}

void strtool::fmt(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfmt(fmt, args);
    va_end(args);
}

void strtool::join(const strtool **strs, strtool_size_t n, const char *delim)
{
#ifdef STRTOOL_USE_REF_COUNTER
    checkreference();
#endif //STRTOOL_USE_REF_COUNTER

    strtool_size_t delimlen = delim ? strlen(delim) : 0;
    const strtool **end = strs + n;

    if (isautostorage())
    {
        strtool_size_t totallen = 0;

        for(const strtool **p = strs; p < end; p++)
        {
            const strtool &str = **p;
            totallen += str.length();
        }

        totallen += delimlen*n - delimlen;

        autogrow(false, NULL, totallen);
    }

    for(const strtool **p = strs; p < end; p++)
    {
        const strtool &str = **p;

        append(str);

        if (delimlen  && p != end-1)
            append(delim, delimlen);
    }
}

strtool_size_t strtool::split(const char *word, strtool **strs, bool stripempty)
{
    if (!word || !*word || empty())
        return 0;

#ifdef STRTOOL_USE_REF_COUNTER
    checkreference();
#endif //STRTOOL_USE_REF_COUNTER

    strtool_size_t n = count(word);

    *strs = new strtool[++n];

    if (n == 1)
    {
         **strs = *this;
         return n;
    }

    finalize();

    const char *last = begin();
    strtool_size_t worldlen = strlen(word);

    size_t stripped = 0;

    loopai(n)
    {
        const char *p = find(word, last);

        if (!p)
            p = end();

        strtool &str = (*strs)[i-stripped];
        str.copy(last, p-last);

        last = p+worldlen;

        if (stripempty && str.empty())
        {
            stripped++;
            continue;
        }
    }

    return n-stripped;
}

strtool_size_t strtool::tokenize(const char *token, strtool **strs)
{
    if (!token || !*token || empty())
        return 0;

#ifdef STRTOOL_USE_REF_COUNTER
    checkreference();
#endif //STRTOOL_USE_REF_COUNTER

    char buf[512];
    char *safebuf;

    if (copystr(buf, sizeof(buf)))
        safebuf = buf;
    else
        safebuf = newstr();

#ifndef _WIN32
    char *saveptr;
#endif //_WIN32

    vector<const char*> tmp;

    char *p = NULL;
    char s[2];

    *s = *token++;
    s[1] = '\0';

#ifdef _WIN32
    // strtok is thread-safe according to
    // http://msdn.microsoft.com/en-us/library/2c8d19sb(v=vs.90).aspx
    // and strtok_s doesn't exist on windows xp
    #undef strtok_r
    #define strtok_r(str, delim, saveptr) strtok(str, delim)
#endif //_WIN32

    while ((p = strtok_r(p ? NULL : safebuf, s, &saveptr)))
    {
        tmp.add(p);

        if (*s)
            *s = *token++;
        else
            break;
    }

#ifdef _WIN32
#undef strtok_r
#endif //_WIN32

    const strtool_size_t n = (strtool_size_t)tmp.length();
    *strs = new strtool[n];

    loopv(tmp)
        (*strs)[i] = tmp[i];

    if (safebuf != buf)
        delete[] safebuf;

    return n;
}

int strtool::compare(const char *in) const
{
    const char *sp;
    const char *p = begin();
    const char *e = end();

    if (p == e)
    {
        if (!*in)
            return 0;

        return 1;
    }

    sp = in;

    #define CHECK \
        if (*in++ != *p++) return 1; \
        if (p == e) break

    do
    {
        CHECK; CHECK;
        CHECK; CHECK;
        CHECK; CHECK;
        CHECK; CHECK;
#if defined(_M_X64) || defined(__x86_64__)
        CHECK; CHECK;
        CHECK; CHECK;
        CHECK; CHECK;
        CHECK; CHECK;
#endif // x86_64
    } while (true);

    if (*in)
        return 1;

    return strtool_size_t(in-sp) == length() ? 0 : 1;
}

int strtool::compare(const char *in, strtool_size_t n) const
{
    if (n > length())
        return 1;

    const char *p = begin();
    const char *e = p+n;

    do
    {
        CHECK; CHECK;
        CHECK; CHECK;
        CHECK; CHECK;
        CHECK; CHECK;
#if defined(_M_X64) || defined(__x86_64__)
        CHECK; CHECK;
        CHECK; CHECK;
        CHECK; CHECK;
        CHECK; CHECK;
#endif // x86_64
    } while (true);

    #undef CHECK

    return 0;
}

bool strtool::endswith(const char *str)
{
    strtool_size_t slen = strlen(str);
    strtool_size_t stlen = length();
    strtool_size_t tmp = stlen-slen;
    if (tmp > stlen) return false;
    return !memcmp(getrawbuf()+tmp, str, slen);
}

char *strtool::newstr()
{
    strtool_size_t len = length();
    char *buf = new char[len+1];
    memcpy(buf, getrawbuf(), len);
    buf[len] = '\0';
    return buf;
}

const char *strtool::copystr(char *buf, strtool_size_t len)
{
    if (length() >= len)
        return NULL;

    strtool s(buf, len);
    s = *this;
    return s.str();
}

strtool strtool::substr(strtool_size_t pstart, strtool_size_t len)
{
    STRTOOL_ASSERT(pstart < length());

    strtool_size_t maxlen = length()-pstart;

    if (len == (strtool_size_t)-1 || len > maxlen)
        len = maxlen;

    strtool tmp;
    tmp.copy(buf+pstart, len);

    return tmp;
}

strtool &strtool::replace(const char *str, const char *with, strtool_size_t max)
{
#ifdef STRTOOL_USE_REF_COUNTER
    checkreference();
#endif //STRTOOL_USE_REF_COUNTER

    strtool_size_t slen = strlen(str);
    strtool tmp(size);

    while (max--)
    {
        const char *pos = find(str);

        if (!pos)
            break;

        strtool_size_t offset = pos-buf;

        tmp.copy(buf, offset);
        tmp.append(with);
        tmp.append(buf+offset+slen, length()-offset-slen);

        swap(tmp);
        tmp.clear();
    }

    return *this;
}

void strtool::growbuf(strtool_size_t n)
{
    STRTOOL_ASSERT(autostorage && "no auto storage");

    if (!n)
        return;

#ifdef STRTOOL_USE_REF_COUNTER
    checkreference();
#endif //STRTOOL_USE_REF_COUNTER

    strtool_size_t oldsize = size;
    strtool_size_t offset = oldsize-left();
    char *oldbuf = buf;

    n += offset;
    size = n;

#ifdef STRTOOL_USE_REF_COUNTER
    strtool_ref_counter_t oldrc = getreferencecount();
#endif //STRTOOL_USE_REF_COUNTER

    // allocate the reference counter and the string buffer at once
    char *newbuf = new char[ADJUST_STRTOOL_BUFSIZE(size)];

    p = buf = ADJUST_STRTOOL_BUFFER(newbuf, +);
    p += offset;

#ifdef STRTOOL_USE_REF_COUNTER
    strtool_ref_counter_t *referencecount = getreferencecounter();
    // check for alignment (debug macro)
    ASSERT(!((size_t)referencecount % sizeof(strtool_ref_counter_t)));
    *referencecount = oldrc;
#endif //STRTOOL_USE_REF_COUNTER

    if (oldbuf)
    {
        memcpy(buf, oldbuf, offset);
        delete[] ADJUST_STRTOOL_BUFFER(oldbuf, -);
    }
}

void strtool::doautogrow(const char *p, strtool_size_t max,
                         strtool_size_t *slen)
{
    static const strtool_size_t MINSIZE = 16;
    static const bool GROWEXACTSIZE = false;

    strtool_size_t min = 0;

    if (p)
    {
        min = strlen(p)+1;

        if (slen)
            *slen = min-1;
    }

    if (max && (min > ++max || !min))
        min = max;

    if (min && left() >= min)
        return;

    strtool_size_t n;

    if (!GROWEXACTSIZE)
    {
        n = size*2;

        if (!n)
            n = MINSIZE;

        while (n < min)
            n *= 2;
    }
    else
    {
        n = size;
        n += min-left();
    }

    growbuf(n);
}

void strtool::resize()
{
    if (!autostorage)
        return;

    strtool_size_t len = length();

    if (!len)
        return;

#ifdef STRTOOL_USE_REF_COUNTER
    strtool_ref_counter_t oldrc = getreferencecount();
#endif //STRTOOL_USE_REF_COUNTER

    // allocate the reference counter and the string buffer at once
    char *newbuf = new char[ADJUST_STRTOOL_BUFSIZE(len+1)/*\0*/];

#ifdef STRTOOL_USE_REF_COUNTER
    strtool_ref_counter_t *referencecount = getreferencecounter();
    // check for alignment (debug macro)
    ASSERT(!((size_t)referencecount % sizeof(strtool_ref_counter_t)));
    *referencecount = oldrc;
#endif //STRTOOL_USE_REF_COUNTER

    newbuf = ADJUST_STRTOOL_BUFFER(newbuf, +);

    memcpy(newbuf, buf, len);
    delete[] ADJUST_STRTOOL_BUFFER(buf, -);

    p = newbuf+(p-buf);
    buf = newbuf;
}

const char *strtool::convert2utf8()
{
    strtool tmp(length()*4);

    int n = encodeutf8((uchar *)tmp.buf, tmp.left(),
                       (const uchar *)buf,
                       length());

    tmp.p += (strtool_size_t)n;

    swap(tmp);

    return getbuf();
}

const char *strtool::convert2utf8(strtool &dst) const
{
    dst.clear();

    if (dst.autostorage && dst.size < length()*4)
        dst.growbuf(length()*4);

    int n = encodeutf8((uchar *)dst.buf, dst.left(),
                       (const uchar *)buf,
                       length());

    dst.p += (strtool_size_t)n;

    return dst.getbuf();
}

const char *strtool::convert2utf8(const char *src)
{
    checkreference();
    clear();

    strtool_size_t len = strlen(src);

    if (autostorage && size < len*4)
        growbuf(len*4);

    p += encodeutf8((uchar *)buf, left(), (const uchar *)src, len);

    return getbuf();
}

const char *strtool::convert2cube()
{
    strtool tmp(length());
    tmp.p += decodeutf8((uchar *)tmp.buf, tmp.left(), (const uchar *)buf, length());
    swap(tmp);
    return getbuf();
}

const char *strtool::convert2cube(strtool &dst) const
{
    dst.clear();

    if (dst.autostorage && dst.size < length())
        dst.growbuf(length());

    dst.p += decodeutf8((uchar *)dst.buf, dst.left(), (const uchar *)buf, length());

    return dst.getbuf();
}

void strtool::fmtmillis(ullong milliseconds, bool skipzero)
{
    int days = (milliseconds/(1000 * 60 * 60 * 24));
    int hours = (milliseconds/(1000 * 60 * 60)) % 24;
    int mins = (milliseconds/(1000 * 60)) % 60;
    int seconds = (milliseconds/1000) % 60;
    int millis = milliseconds % 1000;

    #define fmtval(val, specifier) \
    do { \
        if (!skipzero || val) \
            fmt("%d" specifier " ", val); \
    } while (0)

    fmtval(days, "d");
    fmtval(hours, "h");
    fmtval(mins, "m");
    fmtval(seconds, "s");
    fmtval(millis, "ms");

    pop();

    #undef fmtval
}

#ifdef STRTOOL_USE_REF_COUNTER
void strtool::checkreference(strtool_size_t n)
{
    if (getreferencecount() > 0)
    {
        char buf[sizeof(strtool)];
        strtool *tmp = new (buf) strtool();

        strtool_size_t len = length();

        // tmp->doautogrow(NULL, len+n+1, NULL);

        tmp->growbuf(len+n+1);
        tmp->copy(getrawbuf(), len);

        decrementreferencecounter();

        // take over the new buffer and *don't* call the destructor of tmp
        memcpy(this, tmp, sizeof(strtool));
    }
}
#endif //STRTOOL_USE_REF_COUNTER
