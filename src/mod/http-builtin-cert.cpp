/***********************************************************************
 *  WC-NG - Cube 2: Sauerbraten Modification                           *
 *  Copyright (C) 2020 by Thomas Poechtrager                           *
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

//#include "cube.h"
#include <curl/curl.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

namespace mod {
namespace http
{

static const char builtin_wc_ng_certificate[] =
R"(-----BEGIN CERTIFICATE-----
MIIJmTCCBYGgAwIBAgIJAL+4Ea/9V0JSMA0GCSqGSIb3DQEBCwUAMGMxCzAJBgNV
BAYTAkFUMRAwDgYDVQQIDAdBdXN0cmlhMRowGAYDVQQKDBFEYW5nZXJvdXMgTW9u
a2V5czEmMCQGCSqGSIb3DQEJARYXdC5wb2VjaHRyYWdlckBnbWFpbC5jb20wHhcN
MTQxMjA4MTIzMDQ5WhcNMzQxMjAzMTIzMDQ5WjBjMQswCQYDVQQGEwJBVDEQMA4G
A1UECAwHQXVzdHJpYTEaMBgGA1UECgwRRGFuZ2Vyb3VzIE1vbmtleXMxJjAkBgkq
hkiG9w0BCQEWF3QucG9lY2h0cmFnZXJAZ21haWwuY29tMIIEIjANBgkqhkiG9w0B
AQEFAAOCBA8AMIIECgKCBAEAvJ/ixyQtZH/J1MtSL2cVVOn1CfZudalKckU5Cj+w
kbOmiEmNo8YAxlw6s92LL9QfS8Cxp8AePpm8lrsTuvaQ8aRePr/hreCFjI5rBjfm
dk/gj3BMRwl3/w+19x8DLoVEKy8n3NCR2g0mq5OEUSyRQlian0NvaMSCuaNAbLMN
spChD+azxWRcV5JUqURPYANXxLOuX+iULDSwm4fP8Ddt7OZLh/mUNw4NeXOpMDBh
jcsHlrwfh/CjGKE+tR4gq5iCUpIPIJPVwCem764XCeOBreU6grWV+SbdrakcBPir
KsOjphopcWqebXzZwyZuT7issOZUIIIp5tQ2VsjefN3X3UJ1YspXJtBnW12IVMRo
NQ4poY9BZH4o8XoUagt4nY4u4FVPnXGM5RahK01tug2XhI6uFeiGaG2CuIoajUgH
6dqvu0gIpao6oUXDOIXphi/S/nJp6z2TiNVnqB569OWo5H/jpTGNECrq+6p9Pyt6
OCA9f2A6E5YBIkscKgXofdudXvkHB7q2KFJ4fXvs4+iGqgP8jlm0fICHS28+OAW4
EWcBgiHEPwL9hyJxZBS2hI3W1NPWSMBGFqg04NGJ917CxR756/9sSrOdQbrVtBfr
gQuMf+va7zHpqdsGRG6N3o4/gPYRjUcpOMFNdrMfRkJf5bcVGns/wD47auZB6oz2
iFyFlbMTaCZ0HpbEsNIKA7ZbBCeRLo9N17Qr+uVjVVVDqpQD8lZtjvNrMFTnuz0y
mxMvz8hyEN5JEyyOemIJuiTs59X5KAz0v/sESo5M+507DLJ1iNMHHExobB/xc/bW
5pr65D91/yuG7OsC9vMydau5WVIYoQ49vEdVrrFhQjEpaE9ZDVeFv2xYcjhWJnkg
HBj7ybyM+7Wf96UY0cS6FnSQcs487u7SrGoXW+2MY30jlXdZNLTh4m8m/ew6helV
oJaQ71B+IvRmmrsEElenYwhSnzqIQMQQrobWCH6UC4W+xGMSG+ul6+1ygRO6x+N0
cUsHawxDtCdnGDCJjeJkPacnylfwu/zlJXdqkUsBRyWRMdKxRQNDgQwaSwyX818j
eDoGsnQ3AvPcBKTtMvKDnfBXiQyj/nkyxrtAPD04T4t3i05u7fVUoyrD7muNgqol
lMEcUF/x1Lt8N5akvOjOIvYJEMdoIWl96sk8dTtpA/VWJ9iztgoVUwHcb9Ji4U1M
mIcJv5hbkkAo+uxNhbrtTe7BxCP83XU+2wsAMG/6d0MikFk9oLCiHcnxQSkIYDno
KL6S7U+a8xagJ+R4OjEfVR1UdjTPKv2p1zUkFwWOPvTvLWXDd7nfrLwea73kmqu1
LXYM389plxqzFdNLcYQTwO+amlyPsr7NxjR+W/PD7FQ6zQIDAQABo1AwTjAdBgNV
HQ4EFgQUacMflnw+UNVb9ITSApDmdQq9u8QwHwYDVR0jBBgwFoAUacMflnw+UNVb
9ITSApDmdQq9u8QwDAYDVR0TBAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCBAEASrTS
yqOQOg7ZQl9UMAc0ArgqFPoI5tD+a9tGgo4c9PYpthrnUsOQBwRysDZGkVj8Cwzg
QFvZLXjrwfY1lr5eYPBl0tOOpgcX6fcdY80L8e1hHc8dgrvi0FJefzQzbc8JmNBE
/QPotrqaTt15Subso74euOrGJVAuaGPwUSl49/eDvgUdTupZg/PiX1N3plkwOFMw
BSjqFxdYA6wNpvmQNh0gk+idFBUcb1D6RHei7MCVTL1vbvZWQ/kZpr4AUZ3zWFbl
dJYQksDQRGjGtci2CAdupRFpRWalR6Z8OK8iZolUVJyQ3S2cwD/LwkmhvBqCFwrU
0zih+F6XCuptdwdXPDkYO7oMTJOEG83tPmSBEBLKAlW5blTRLmuzhX0fQa8M4E51
R6RrR+EKejSnQJWVKQHIOJa7EHKuyY0vXa1AzijMI8WNEYklyVBZBe58bBYB4djZ
j07goiUKNJQOFiuG1QMbBrXmc1P/iC5z4/hd9JUcbsfOrsXQM8irxqaTlx1hDUq8
T2RVjISbk9dJsBdtXQ1k6qG7eFmhc30AtyOCY4rxp+aKWJxwWQogl5HtVXUifYjd
Svxy15CI/BexM4OX39wfR7LoQGDdnaKUszdTHRcL1I0rL3ijL18G3nwl/oK3hVD3
qKLAG4evepQbOcazU2W0JVKvLBFNOuipGglqHbqZxdwGZtqSMgKJaljz5yeTmBJh
3Id94dEAguAAblLEuwmwlTSHoZWeZrf5eWbFJd4aXLCGChgmJx1ZW6dbC2uzS3wp
jG5kPFAwhMY7XhpXROX1lDZuFkcxyL3V45cCDYcyeE1FM1UXFkhE28xYqqrFWNv8
vEft/1t5sUFNfWF9ivBDVtgUhc41je7Kf8U++KO/9IF6M4qzihu2AWVEfiwWue84
vGoBU5z2EML3G6Jx5m2Jk87Klb3rQmPAxcIMIIIJOoQfMDogLY/amdOk4iqutjwB
6hRLpfdcH+RcKNUQCAvM0IVCXfYV2UZ7JVyDZFILgdpEWdxeK1No3qoYFeJwDzOD
iu4ugw5mDOd8BBIPI9Mngafd44qqjSuy8m8ZfEPLjLWsJ6rqK1onUx2crkpHNnGd
ge4dEv08TCU1tbXRI1i3ytRJBxURheRYEVjwac7IWxXYrkfUZgSEAirGLwKfYggE
7T3QREY5bii6qoOu5uzJHdi2h3qff1YQGDwDWSezGXf76U5tyllZlSlX4VnAKKb2
S0fE0NlmZGhZkJZnFf3nGQDn/RUYLbovqxvnm6OI7yfmnk+B6ZZW8MHtIv7vxBoo
ol5WYVP9ipxGRMLDKlb4StN1FSEGTZ7TsFTJnRKozk0gXpVfUcasu7sW6jnj/WfV
rT76DN69Pkxh09v/QQ==
-----END CERTIFICATE-----
")";

CURLcode handlebuiltincert(CURL *curl, void *sslctx, void *parm)
{
    (void)curl;
    (void)parm;

    CURLcode rv = CURLE_ABORTED_BY_CALLBACK;

    BIO *cbio = BIO_new_mem_buf(builtin_wc_ng_certificate, sizeof(builtin_wc_ng_certificate));
    X509_STORE  *cts = SSL_CTX_get_cert_store((SSL_CTX *)sslctx);
    int i;
    STACK_OF(X509_INFO) *inf;

    if(!cts || !cbio) return rv;

    inf = PEM_X509_INFO_read_bio(cbio, NULL, NULL, NULL);

    if(!inf)
    {
        BIO_free(cbio);
        return rv;
    }

    for(i = 0; i < sk_X509_INFO_num(inf); i++)
    {
        X509_INFO *itmp = sk_X509_INFO_value(inf, i);
        if (itmp->x509) X509_STORE_add_cert(cts, itmp->x509);

        if (itmp->crl) X509_STORE_add_crl(cts, itmp->crl);
    }

    sk_X509_INFO_pop_free(inf, X509_INFO_free);
    BIO_free(cbio);
 
    rv = CURLE_OK;
    return rv;
} 

} // namespace http
} // namespace mod
