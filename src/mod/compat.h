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

#ifdef _MSC_VER
#define _ALLOW_KEYWORD_MACROS
#define constexpr
#if _MSC_VER < 1800
#ifndef va_copy
#define va_copy(a, b) do { a = b; } while (0)
#endif //!va_copy
#endif //_MSC_VER < 1800
#define popen _popen
#define pclose _pclose
#define __thread __declspec(thread)
#endif //_MSC_VER

#ifdef _WIN32
#define CURL_STATICLIB
#endif //_WIN32
