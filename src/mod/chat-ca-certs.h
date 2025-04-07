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
        * SHA:  24:81:5F:96:80:43:8C:C8:2F:B4:24:9A:3D:46:D5:61:68:E4:51:D8
        * MD5:  3E:03:C6:22:89:89:FA:01:8F:32:4A:93:AE:02:1F:83
        * VALID NOT BEFORE: Apr  7 16:44:50 2025 GMT
        * VALID UNTIL:      Apr  3 16:44:50 2040 GMT
        * SIG ALG: ecdsa-with-SHA256
        */


        "TP-ECDSA",

        "-----BEGIN CERTIFICATE-----\n"
        "MIICZjCCAeugAwIBAgIUZyDygDynvbj/oDggAJbKdRmS7tgwCgYIKoZIzj0EAwIw\n"
        "dTELMAkGA1UEBhMCQVQxEDAOBgNVBAgMB0F1c3RyaWExDzANBgNVBAoMBlRlc3RD\n"
        "QTEbMBkGA1UEAwwSVGhvbWFzIFBvZWNodHJhZ2VyMSYwJAYJKoZIhvcNAQkBFhd0\n"
        "LnBvZWNodHJhZ2VyQGdtYWlsLmNvbTAeFw0yNTA0MDcxNjQ0NTBaFw00MDA0MDMx\n"
        "NjQ0NTBaMHUxCzAJBgNVBAYTAkFUMRAwDgYDVQQIDAdBdXN0cmlhMQ8wDQYDVQQK\n"
        "DAZUZXN0Q0ExGzAZBgNVBAMMElRob21hcyBQb2VjaHRyYWdlcjEmMCQGCSqGSIb3\n"
        "DQEJARYXdC5wb2VjaHRyYWdlckBnbWFpbC5jb20wdjAQBgcqhkjOPQIBBgUrgQQA\n"
        "IgNiAAQI1uAs2GXeTSh8oEBqyLsxhjVmRdSzZtgExo6lCoCSjSfrB6bApzmCkTiY\n"
        "Ctl3GigLZqQ4NVoHOdwyEZDa1Q43ZJLty4moE2knl1uqM9c3Ys7NammidgtmY2Pl\n"
        "uBKIPUSjPDA6MAwGA1UdEwQFMAMBAf8wCwYDVR0PBAQDAgEGMB0GA1UdDgQWBBS8\n"
        "q0DumVFM85oqbSVaXWNApwLuSjAKBggqhkjOPQQDAgNpADBmAjEAtapM/ny165Yb\n"
        "OkPTRRO3aZdQTZLWiHs1mMetMP9/FjRfGIVfn/pVeoefaWCcpB41AjEA8FtxZn2e\n"
        "GXP2N0L54Q0sFuZqkvo8iePhnhQ4miDJq9o4HQLIYNA/7VsScJ9oEixA\n"
        "-----END CERTIFICATE-----"
    },
    {
        /*
        * SHA:  50:4E:B2:F0:9C:A2:15:CE:3A:3A:6C:BE:7E:3E:A6:41:BE:F7:55:89
        * MD5:  B5:54:EE:ED:3B:34:BD:62:29:34:3A:66:85:A5:A3:F3
        * VALID NOT BEFORE: Apr  7 16:44:50 2025 GMT
        * VALID UNTIL:      Apr  3 16:44:50 2040 GMT
        * SIG ALG: sha256WithRSAEncryption
        */


        "TP-RSA",

        "-----BEGIN CERTIFICATE-----\n"
        "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\n"
        "sv5POJSwIM3ajsBGlzqIbK4LuNKxcx8R\n"
        "-----END CERTIFICATE-----"
    }
};
