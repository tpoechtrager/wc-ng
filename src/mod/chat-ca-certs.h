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

static struct
{
    const char *const ca;
    const char *const data;
} PUBLIC_CA_CERTS[] =
{
    {
        /*
         * SHA: DF:1F:D4:5B:5E:11:62:B4:09:1D:36:E7:5A:26:B1:17:E9:7A:FC:4B
         * MD5: 94:FE:7C:B4:D0:DB:AF:EC:80:30:DB:51:50:04:90:A0
         * VALID NOT BEFORE: Sep  6 12:19:53 2014 GMT
         * VALID UNTIL:      Sep  3 12:19:53 2024 GMT
         * SIG ALG: ecdsa-with-SHA256
         */

        "TP-ECDSA",

        "-----BEGIN CERTIFICATE-----\n"
        "MIICLDCCAdGgAwIBAgIJANSQ6bLWjjGCMAoGCCqGSM49BAMCMHIxCzAJBgNVBAYT\n"
        "AkFUMRAwDgYDVQQIDAdBdXN0cmlhMQowCAYDVQQKDAEtMR0wGwYDVQQDDBRUaG9t\n"
        "YXMgUMODwrZjaHRyYWdlcjEmMCQGCSqGSIb3DQEJARYXdC5wb2VjaHRyYWdlckBn\n"
        "bWFpbC5jb20wHhcNMTQwOTA2MTIxOTUzWhcNMjQwOTAzMTIxOTUzWjByMQswCQYD\n"
        "VQQGEwJBVDEQMA4GA1UECAwHQXVzdHJpYTEKMAgGA1UECgwBLTEdMBsGA1UEAwwU\n"
        "VGhvbWFzIFDDg8K2Y2h0cmFnZXIxJjAkBgkqhkiG9w0BCQEWF3QucG9lY2h0cmFn\n"
        "ZXJAZ21haWwuY29tMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEZNwnY79V9O7M\n"
        "6HlcCYG+4gwVDBCXacjYpwqjbot+Jd5dWsXP4SXBrJvatDPOaaUAs4L6Oa0xXlce\n"
        "UU5u4H9SWKNQME4wHQYDVR0OBBYEFNrV2AL/Z9IbBEFLZ8Ay965GdXf/MB8GA1Ud\n"
        "IwQYMBaAFNrV2AL/Z9IbBEFLZ8Ay965GdXf/MAwGA1UdEwQFMAMBAf8wCgYIKoZI\n"
        "zj0EAwIDSQAwRgIhAMyVir7+7rTUnDlpbarTBagLzGDSQzckElAFk/hw3PHBAiEA\n"
        "uL7Il+6ccolvX4IcBJP2ybKWtp/rdRPfz3x0c+f8Spo=\n"
        "-----END CERTIFICATE-----"
    },
    {
        /*
         * SHA: 98:84:0C:B1:BD:AB:31:57:01:D2:EB:4D:44:C0:CB:82:13:41:CB:EF
         * MD5: DE:D7:4E:A3:C1:F0:0E:6A:5E:16:DE:9B:63:36:3A:F2
         * VALID NOT BEFORE: Sep  6 12:10:59 2014 GMT
         * VALID UNTIL:      Sep  3 12:10:59 2024 GMT
         * SIG ALG: sha256WithRSAEncryption
         */

        "TP-RSA",

        "-----BEGIN CERTIFICATE-----\n"
        "MIIJtzCCBZ+gAwIBAgIJAPxVWctO078qMA0GCSqGSIb3DQEBCwUAMHIxCzAJBgNV\n"
        "BAYTAkFUMRAwDgYDVQQIDAdBdXN0cmlhMQowCAYDVQQKDAEtMR0wGwYDVQQDDBRU\n"
        "aG9tYXMgUMODwrZjaHRyYWdlcjEmMCQGCSqGSIb3DQEJARYXdC5wb2VjaHRyYWdl\n"
        "ckBnbWFpbC5jb20wHhcNMTQwOTA2MTIxMDU5WhcNMjQwOTAzMTIxMDU5WjByMQsw\n"
        "CQYDVQQGEwJBVDEQMA4GA1UECAwHQXVzdHJpYTEKMAgGA1UECgwBLTEdMBsGA1UE\n"
        "AwwUVGhvbWFzIFDDg8K2Y2h0cmFnZXIxJjAkBgkqhkiG9w0BCQEWF3QucG9lY2h0\n"
        "cmFnZXJAZ21haWwuY29tMIIEIjANBgkqhkiG9w0BAQEFAAOCBA8AMIIECgKCBAEA\n"
        "yV0Z/0o58Q+D9cGBD1KYXkF+3pSN2awx7XpZmVgw97Zc9RBhz5O3+r9HNLP7VL/I\n"
        "b7LUw0U4m7V8FIfXviBGDUhGBA+32w2h2vkszBQ4vBr5x2hmgb4CymDaJzHEnnJ0\n"
        "NkL6tEwGc9d1XyG8X2EraD4gC5tt0rnGU4JNLVc1rPSwweyZUNGho1L1o6eeJIKw\n"
        "bj1nYHOByOnCn7Pbe+88ZJ+fSHbd0Zdomx5grAWOdBqR/XuElH/li0QIi4PHryN3\n"
        "5kTTt3AYWaOAaC7RFeoGKrwg9Dh1uLyCtglBNUXA2V/blaauygVYx1evZPTd5Bbk\n"
        "Wn/PMubHajuxUmisw/psVCdO03P9tHOUwkur7OBm0fAbSV2Wt/GuAdUwgoMCc4d+\n"
        "2NPYtlgGwFUT7PGxykgEx5Wb0Il+YfQqW/sdkAsxBIGtstvHq28aEPFOfwfbXue3\n"
        "HxOHDAon6pGst5OTt4/KbqWxv8JIaQ2iu8VJENGP2ka5xMU7JfTgWFdK0Me5Vvr2\n"
        "cCsFfAookueEwr6k910VdGKHAPhhovamVaLrfzklYQWqdp447YEwxgQQrCwYKqlb\n"
        "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\n"
        "-----END CERTIFICATE-----\n"
    }
};
