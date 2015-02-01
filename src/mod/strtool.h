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

namespace mod {

//
// use copy-on-write mechanism instead of copying the buffer every time
//

#define STRTOOL_USE_REF_COUNTER

//
// const references can be used without problems,
// as the buffer won't be modified.
//
// only problem may be write protected memory,
// but it's not possible to mark any members of
// strtool write protected, as they are all private.
//

#define STRTOOL_USE_CONST_REF

//
// enable strtool asserts.
//

//#if !defined(__OPTIMIZE__) || defined(_DEBUG) || defined(__DEBUG__)
#define ENABLE_STRTOOL_ASSERTS
//#endif //!__OPTIMIZE__ || _DEBUG || __DEBUG__

#undef isnumber

#ifdef STRTOOL_USE_REF_COUNTER
#define ADJUST_STRTOOL_BUFSIZE(size) (size + sizeof(strtool_ref_counter_t))
#define ADJUST_STRTOOL_BUFFER(p, op) (char*)(p op sizeof(strtool_ref_counter_t))
#else
#define ADJUST_STRTOOL_BUFSIZE(size) size
#define ADJUST_STRTOOL_BUFFER(p, op) p
// #pragma push_macro("checkreference")
#undef checkreference
#define checkreference(...) (void)(0)
#endif //STRTOOL_USE_REF_COUNTER


#ifdef ENABLE_STRTOOL_ASSERTS
#define STRTOOL_ASSERT assert
#else
#define STRTOOL_ASSERT(c) (void)(0)
#endif //ENABLE_STRTOOL_ASSERTS


#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4521)
#pragma warning (disable: 4522)
#endif //_MSC_VER

static inline bool isbool(bool) { return true; }
template<class T> static inline bool isbool(T) { return false; }

typedef uint32_t strtool_size_t;
typedef uint32_t strtool_ref_counter_t;

class strtool
{
public:
    void append(const char *c)
    {
        if (!autostorage && !left())
            return;

        strtool_size_t len = 0;
        strtool_size_t inlen = 0;

        checkreference(autostorage ? (inlen = strlen(c)) : 0);

        if (!autogrow(true, c, 0, &len))
            len = min(left(), inlen ? inlen : (strtool_size_t)strlen(c));

        memcpy(p, c, len);
        p += len;
    }

    void append(const char *c, strtool_size_t max)
    {
        if (!autostorage && !left())
            return;

        checkreference(max);
        strtool_size_t len;

        if (!autogrow(true, NULL, max))
            len = min(left(), max);
        else
            len = max;

        memcpy(p, c, len);
        p += len;
    }

    void append(const strtool &s) { append(s.getrawbuf(), s.length()); }

    strtool_size_t appenduntil(const char *c, const char *word)
    {
        const char *end = strstr(c, word);

        if (!end)
            return 0;

        strtool_size_t len =  end-c;
        append(c, len);
        return len;
    }

    void copy(const char *c)
    {
        clear();
        append(c);
    }

    void copy(const char *c, strtool_size_t max)
    {
        clear();
        append(c, max);
    }

    strtool_size_t copyuntil(const char *c, const char *word)
    {
        clear();
        return appenduntil(c, word);
    }

    void copy(const strtool &src)
    {
        clear();
        copy(src.getrawbuf(), src.length());
    }

    void add(char c)
    {
        checkreference();

        if (!left() && !autogrow())
            return;

        *p++ = c;
    }

    void add(char c, strtool_size_t count)
    {
        if (!count)
            return;

        checkreference();

        if (count <= left() || autogrow(true, NULL, count))
        {
            count = min(count, left());
            memset(p, c, count);
            p += count;
        }
    }

    void pop()
    {
        if (empty())
            return;

        checkreference();

        p--;
    }

    void popfirst()
    {
        if (empty())
            return;

        checkreference();

        memmove(buf, buf+1, length()-1);
        p--;
    }

    void popuntil(char c)
    {
        checkreference();

        while (size != left() && getlast() != c)
            pop();
    }

    void remove(strtool_size_t i, strtool_size_t count = 1)
    {
        if (empty())
            return;

        checkreference();

        char *offp = buf+i;
        char *offpm = offp+count;
        strtool_size_t tomove = end()-offpm;

        if (tomove)
        {
            STRTOOL_ASSERT((offp >= begin() && offp < end()) &&
                           (offpm >= begin() && offpm < end()));

            memmove(offp, offpm, tomove);
        }

        p -= count;

        STRTOOL_ASSERT(p >= buf && p < buf+size);
    }

    void erease(strtool_size_t i, strtool_size_t count = 1)
    {
        remove(i, count);
    }

    void trim(bool front = false, strtool_size_t max = -1,
              const char *trimchars = "\r\n");

    void removechr(char c)
    {
        checkreference();
        strtool_size_t end = length();

        for (strtool_size_t i = 0; i < end; i++)
        {
            if (buf[i] == c)
            {
                remove(i);
                i--;
                end--;
            }
        }
    }

    void erasechr(char c) { removechr(c); }

    void fixpathdiv()
    {
        checkreference();
        finalize();
        path(buf);
        p = buf + strlen(buf);
    }

    void upperstring()
    {
        if (empty())
            return;

        checkreference();

        loopai(length())
            buf[i] = toupper(buf[i]);
    }

    void lowerstring()
    {
        if (empty())
            return;

        checkreference();

        loopai(length())
            buf[i] = tolower(buf[i]);
    }

    bool isnumber() const
    {
        if (empty())
            return false;

        const char *c = begin();

        if (*c == '-' || *c == '+')
            c++;

        for (; c != end(); c++)
        {
            if (*c < '0' || *c > '9')
                return false;
        }

        return true;
    }

    template<class T>
    T tonumber(T nonnumval, T minval, T maxval)
    {
        static_assert(sizeof(T) <= sizeof(ullong), "");

        if (!isnumber())
            return nonnumval;

        const char *c = str();
        bool neg = false;

        if (*c == '-' || *c == '+')
        {
            neg = (*c == '-');
            c++;
        }

        T val;

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4800)
#pragma warning (disable: 4554)
#endif //_MSC_VER

        if (sizeof(T) == 8)
        {
            ullong v;
#ifdef _MSC_VER
            v = (T)_strtoui64(c, NULL, 10);
#else
            v = (T)strtoull(c, NULL, 10);
#endif //_MSC_VER
            val = isbool(T()) ? !!v : (T)v;
        }
        else
        {
            uint v = strtoul(c, NULL, 10);
            val = isbool(T()) ? !!v : (T)v;
        }

        if (!isbool(T()))
            val = (neg ? (val^T(-1))+1 : val);

#ifdef _MSC_VER
#pragma warning (pop)
#endif //_MSC_VER

        if (!isbool(T()))
            val = clamp(val, minval, maxval);

        return val;
    }

    const char *convert2utf8();
    const char *convert2utf8(strtool &dst) const;
    const char *convert2utf8(const char *src);
    const char *convert2cube();
    const char *convert2cube(strtool &dst) const;

    strtool_size_t count(char c) const
    {
        strtool_size_t n = 0;

        for (const char *p = begin(); p != end(); p++)
        {
            if (*p == c)
                n++;
        }

        return n;
    }

    strtool_size_t count(const char *c)
    {
        strtool_size_t n = 0;
        strtool_size_t findlen = strlen(c);
        const char *last = NULL;

        while ((last = find(c, last)))
        {
            last += findlen;
            n++;
        }

        return n;
    }

    void join(const strtool **strs, strtool_size_t n, const char *delim = NULL);

    strtool_size_t split(const char *word, strtool **strs,
                         bool stripempty = false);

    strtool_size_t tokenize(const char *token, strtool **strs);

    int compare(const char *in) const;
    int compare(const char *in, strtool_size_t n) const;
    bool endswith(const char *str);

    bool isautostorage() const { return autostorage!=0; }

    void swap(strtool &in)
    {
        // don't use ::swap()
        // this way avoids running the ctors
        checkreference();
        char tmp[sizeof(strtool)];
        memcpy(tmp, &in, sizeof(strtool));
        memcpy(&in, this, sizeof(strtool));
        memcpy(this, tmp, sizeof(strtool));
    }

    const char *find(const char *find, const char *last = NULL)
    {
        return strstr(last ? last : getbuf(), find);
    }

    const char *find(char c, const char *last = NULL)
    {
        char s[2];
        *s = c; s[1] = '\0';
        return find(s, last);
    }

    const char *find(strtool &in) { return find(in.getbuf()); }

    strtool_size_t capacity() const { return left(); }
    strtool_size_t length() const { return p-buf; }
    strtool_size_t left() const { return size-(p-buf); }
    strtool_size_t bufsize() const{ return size; }
    bool empty() const { return p == buf; }

    bool checkspace(strtool_size_t want) const { return capacity() > want; }

    void finalize()
    {
        if (!left())
        {
            autogrow();

            if (!left())
                p--;
        }

        if (!buf)
            return;

        *p = '\0';
    }

    void clear()
    {
        if (!buf)
            return;

        checkreference();

        p = buf;
    }

    void secureclear()
    {
        checkreference();

        if (buf)
        {
            memset(buf, '\0', length());
            *(volatile char*)buf = *(volatile char*)buf;
            p = buf;
        }
    }

    void fmt(const char *fmt, ...) PRINTFARGS(2, 3);
    void vfmt(const char *fmt, va_list v);

    void fmtmillis(ullong milliseconds, bool skipzero = true);

    void fmtseconds(uint seconds, bool skipzero = true)
    {
        fmtmillis(seconds*1000ULL, skipzero);
    }

    const char *getbuf()
    {
        if (empty())
        {
            // do not allocate memory for an empty string
            return "";
        }
        finalize();
        return buf;
    }

    const char *getstr() { return getbuf(); }
    const char *str() { return getbuf(); }
    const char *getrawbuf() const { return buf; }
    char getlast() const { return size != left() ? p[-1] : '\0'; }
    char getfirst() const { return size != left() ? *buf : '\0'; }

    void disown()
    {
#ifdef STRTOOL_USE_REF_COUNTER
        if (getreferencecount())
            decrementreferencecounter();
#endif //STRTOOL_USE_REF_COUNTER

        autostorage = true;
        buf = p = NULL; size = 0;
    }

    void growbuf(strtool_size_t n);
    void resize();

    char *newstr();
    const char *copystr(char *buf, strtool_size_t len);

    strtool substr(strtool_size_t pstart, strtool_size_t len = -1);

    strtool &replace(const char *str, const char *with,
                     strtool_size_t max = -1);

    // std::string compatible
    const char *begin() const { return buf; }
    const char *end() const { return p; }

    char front() const { return getfirst(); }
    char back() const { return getlast(); }

    const char *c_str() { return str(); }
    const char *data() { return getrawbuf(); }

    void push_back(char c) { add(c); }
    void pop_back() { pop(); }
    void pop_front() { popfirst(); }

    bool operator==(const strtool &in) const
    {
        if (length() != in.length())
            return false;

        return !memcmp(getrawbuf(), in.getrawbuf(), length());
    }

    bool operator==(const char *in) const { return !compare(in); }
    bool operator!=(const strtool &in) const { return !(*this == in); }
    bool operator!=(const char *in) const { return !!compare(in); }
    void operator+=(const strtool &in) { append(in); }
    void operator+=(const char *in) { append(in); }
    void operator+=(const char in) { add(in); }

#ifdef __MINGW32__
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
    #pragma GCC diagnostic push
#endif
    #pragma GCC diagnostic ignored "-Wformat"
    #pragma GCC diagnostic ignored "-Wformat-extra-args"
#endif //__MINGW32__

    strtool &operator<<(const strtool &in) { append(in); return *this; }
    strtool &operator<<(const char in) { add(in); return *this; }
    strtool &operator<<(const char *in) { append(in); return *this; }

    strtool &operator<<(int in) { fmt("%d", in); return *this; }
    strtool &operator<<(uint in) { fmt("%u", in); return *this; }
    strtool &operator<<(long in) { fmt("%ld", in); return *this; }
    strtool &operator<<(unsigned long in) { fmt("%lu", in); return *this; }
    strtool &operator<<(llong in) { fmt("%lld", in); return *this; }
    strtool &operator<<(ullong in) { fmt("%llu", in); return *this; }
    strtool &operator<<(float in) { fmt("%f", in); return *this; }

#ifdef __MINGW32__
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
    #pragma GCC diagnostic pop
#endif
#endif //__MINGW32__

    char operator--(int)
    {
        pop();
        return getlast();
    }

    char operator--()
    {
        popfirst();
        return getfirst();
    }

    void operator-=(strtool_size_t n)
    {
        n = min(n, length());
        loopai(n) pop();
    }

    strtool &operator=(const char *in)
    {
        copy(in);
        return *this;
    }

#ifdef STRTOOL_USE_REF_COUNTER
    strtool &operator=(strtool &in)
    {
        if (buf == in.buf)
            return *this;

        if (autostorage && in.autostorage)
        {
            in.incrementreferencecounter();

            this->~strtool();
            memcpy(this, &in, sizeof(strtool));

            return *this;
        }

        if (!in.empty())
            copy(in.getrawbuf(), in.length());

        return *this;
    }
#endif //STRTOOL_USE_REF_COUNTER

    strtool &operator=(const strtool &in)
    {
#ifdef STRTOOL_USE_REF_COUNTER
#ifdef STRTOOL_USE_CONST_REF
        // we don't modify the buffer, so it doesn't matter
        *this = (strtool&)in;
        return *this;
#endif //STRTOOL_USE_CONST_REF
#endif //STRTOOL_USE_REF_COUNTER

        if (!in.empty())
            copy(in.getrawbuf(), in.length());

        return *this;
    }

    const char *operator*() { return str(); }
    operator bool() const { return !empty(); }
    bool operator!() const { return empty(); }

    char &operator[](strtool_size_t i)
    {
        STRTOOL_ASSERT(i < length());
        checkreference();
        return buf[i];
    }

    strtool(strtool_size_t n = 0)
      : autostorage(true), size(0), p(NULL), buf(NULL)
    {
        growbuf(n);
    }

    strtool(const char *str)
      :  autostorage(true), size(0), p(NULL), buf(NULL)
    {
        copy(str);
    }

    strtool(const char *str, size_t len)
      :  autostorage(true), size(0), p(NULL), buf(NULL)
    {
        copy(str, len);
    }

    strtool(char *str, size_t len, bool c = true) 
      : autostorage(false), size(len), p(str), buf(str)
    {
        if (!c && len)
        {
            p += size;
            return;
        }

        clear();
    }

#ifdef STRTOOL_USE_REF_COUNTER
    strtool(strtool &str)
      : autostorage(true), size(0), p(NULL), buf(NULL)
    {
#ifdef STRTOOL_USE_REF_COUNTER
        if (str.autostorage)
        {
            str.incrementreferencecounter();
            memcpy(this, &str, sizeof(strtool));
            return;
        }
#endif //STRTOOL_USE_REF_COUNTER
        copy(str.getrawbuf(), str.length());
    }
#endif //STRTOOL_USE_REF_COUNTER

    strtool(const strtool &str)
      : autostorage(true), size(0), p(NULL), buf(NULL)
    {
#ifdef STRTOOL_USE_REF_COUNTER
#ifdef STRTOOL_USE_CONST_REF
        if (autostorage)
        {
            // we don't write to the buffer, so it doesn't matter
            ((strtool&)str).incrementreferencecounter();
            memcpy(this, (strtool*)&str, sizeof(strtool));
            return;
        }
#endif //STRTOOL_USE_CONST_REF
#endif //STRTOOL_USE_REF_COUNTER
        *this = str;
    }

    strtool(const strtool &str, bool conv2utf8)
      : autostorage(true), size(0), p(NULL), buf(NULL)
    {
        if (conv2utf8) str.convert2utf8(*this);
        else *this = str;
    }

    strtool(const char *str, bool conv2utf8)
      : autostorage(true), size(0), p(NULL), buf(NULL)
    {
        if (conv2utf8) convert2utf8(str);
        else *this = str;
    }

    ~strtool()
    {
        if (autostorage)
        {
#ifdef STRTOOL_USE_REF_COUNTER
            if (getreferencecount())
            {
                decrementreferencecounter();
                return;
            }
#endif //STRTOOL_USE_REF_COUNTER

            if (buf)
                delete[] ADJUST_STRTOOL_BUFFER(buf, -);
        }
        else if (buf)
        {
            finalize();
        }
    }

    strtool_ref_counter_t getreferencecount()
    {
#ifdef STRTOOL_USE_REF_COUNTER
        if (!autostorage || !buf)
            return 0;

        return *getreferencecounter();
#else
        return 0;
#endif //STRTOOL_USE_REF_COUNTER
    }

private:
    void doautogrow(const char *p, strtool_size_t max, strtool_size_t *slen);

    bool autogrow(bool force = false, const char *p = NULL,
                  strtool_size_t max = 0, strtool_size_t *slen = 0)
    {
        if ((left() && !force) || !autostorage)
            return false;

        doautogrow(p, max, slen);
        return true;
    }

#ifdef STRTOOL_USE_REF_COUNTER
    strtool_ref_counter_t *getreferencecounter()
    {
        strtool_ref_counter_t *referencecount = (strtool_ref_counter_t*)buf;
        return --referencecount;
    }

    strtool_ref_counter_t incrementreferencecounter()
    {
        if (!autostorage || !buf)
            return 0;

        strtool_ref_counter_t *referencecount = getreferencecounter();

        return ++(*referencecount);
    }

    strtool_ref_counter_t decrementreferencecounter()
    {
        if (!autostorage || !buf)
            return 0;

        strtool_ref_counter_t *referencecount = getreferencecounter();

        strtool_ref_counter_t rc = --(*referencecount);
        STRTOOL_ASSERT(rc < strtool_ref_counter_t(-1)/2);

        return rc;
    }
#endif //STRTOOL_USE_REF_COUNTER

#ifdef STRTOOL_USE_REF_COUNTER
#ifdef __OPTIMIZE_SIZE__
    __attribute__ ((noinline))
#endif //__OPTIMIZE_SIZE__
    void checkreference(strtool_size_t n = 0);
#endif //STRTOOL_USE_REF_COUNTER

    strtool_size_t autostorage, size;
    char *p, *buf;
};

// #ifndef STRTOOL_USE_REF_COUNTER
// #pragma pop_macro("checkreference")
// #endif //!STRTOOL_USE_REF_COUNTER

#ifdef STRTOOL_USE_REF_COUNTER
// explicitly copy the strtool buffer
static inline void copystrtool(strtool &dst, const strtool &src)
{
    // copy buffer without reference counting
    dst.copy(src);
}
#else
#define copystrtool(dst, src) do { dst = src; } while (0)
#endif //STRTOOL_USE_REF_COUNTER

#ifdef _MSC_VER
#pragma warning (pop)
#endif //_MSC_VER

} // namespace mod
