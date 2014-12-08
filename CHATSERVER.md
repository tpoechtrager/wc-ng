#### Step 1: Compile the chat server: ####

$ `cd src`  
... static libs (known to work):  
$ `make chatserver USESTATICLIBS=1`  
... or with your system libs (OpenSSL 1.0.x is required):  
$ `make chatserver`

... or simply use the pre-compiled binary from the client package (bin*/wc\_chat\_server{.exe}).

#### Step 2: Generate a certificate: ####

  * Self-signed:

      EC (recommended):  
      $ `openssl ecparam -out server.key -name prime256v1 -genkey`  
      $ `openssl req -new -key server.key -x509 -nodes -days 365 -out server.crt`
  
      RSA:  
      $ `openssl req -x509 -newkey rsa:4096 -keyout server.key -out server.crt -days 365`
  
      Get the certificate's fingerprint:
  
      $ `openssl x509 -in server.crt -fingerprint -sha224 -noout`
  
      MD5, SHA1, SHA224, SHA256, SHA384 and SHA512 are supported.
 

  * [TODO]

#### Step 3: Copy the chat server binary and the OpenSSL certificate onto your server: ####
  
$ `scp wc_chat_server ../doc/wc_chat_server.cfg server.key server.crt user@host.com:/home/user/chatserver/`

#### Step 4: SSH onto your server: ####

Generate a secure password:

$ `cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 32 | sed 1q` (or `pwgen -snB 32 1`)

Adjust wc_chat_server.cfg:

$ `vi wc_chat_server.cfg`

Start the chat server:

$ `./wc_chat_server -daemon`

Hint: To list all supported options, type: `./wc_chat_server -help`

#### Step 5: Deploy the password and certificate fingerprint to your mates: ####

Tell them to add the following to their autoexec.cfg:

    wcchatserver [ssl:]<hostname>[:<port>]
    wcchatautoconnect 1
    wcchatpassword <pw>
    wcchataddcert <fingerprint> [<hostname>] // only required for self-signed certificates

So this may look like:

    wcchatserver ssl:127.0.0.1 
    wcchatautoconnect 1  
    wcchatpassword eM7u4xSymAPeToSXFhANnk7mlg8vZhyV
    wcchataddcert 5F:41:8E:EA:C3:E5:DC:4D:83:50:6A:24:64:0E:34:B5:32:FC:18:1D 127.0.0.1

Hint: "ssl:" automatically disables unencrypted connections.

#### Certificate Revocation: ####

##### Method 1: ######

Tell your mates to add the following to their autoexec.cfg:  

* `wcchataddrevokedcert <fingerprint>`

Just removing the entry from autoexec.cfg is *not* safe,  
and will not have the desired effect.

##### Method 2: ######

Write an email to t.poechtrager@gmail.com (attach server.crt).

Also be certain that your email address *must* match to the certificate,  
otherwise I will deny the request.

Certificate revocation is realized through the update check.  
Certificate revocation cannot be undone.
