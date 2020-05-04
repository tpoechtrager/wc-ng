## Sauerbraten WC NG ##

## Latest Release: ##

You can download the latest release here: [wc-ng.mooo.com](https://wc-ng.mooo.com/).

### Installation: ###

* Client:
    * Windows:
        * Installation on Windows is quite easy, all you have to do is to run the installer.  
        * You don't need to backup anything, as the installer won't replace your original  
          client. You can run both, the original and our client at the same time.  
        * The installer attempts to find the installation path automatically, if it fails  
          (e.g. your sauer comes from svn, then point it to your sauerbraten installation path).  
        * Once installing is done, you'll have new shortcuts on your desktop and startmenu.  
        * Start the client by clicking on the "Sauerbraten_wc" shortcut.
    * Linux:
        * Open your preferred file manager (e.g. konqueror, dolphin, nautilus, etc.)  
          or terminal and browse to ~/.sauerbraten.  
        * Copy the doc, data, packages and plugins folders into the previous opened dir.  
        * Browse to the location of the original sauerbraten binary  
          (e.g. /usr/lib/games/sauerbraten/).  
        * Rename the binary to sauerbraten_orig and copy the wc executeable into this dir.  
    * OS X:
        * Open another finder window, go to your home folder and then browse to  
          Library / Application Support / sauerbraten.  
        * Copy the doc, data, packages and plugins folders from the opened wc.dmg into the  
          previous opened dir.  
        * Browse to the location of the original sauerbraten package, right click it,  
          choose "Show Package Contents" and browse to contents / MacOS.  
        * Rename (backup) the original file sauerbraten_u (OS X 10.6+) or sauerbraten_c (OS X 10.5)  
          to sauerbraten_orig and the wclient executeable from the opened wc.dmg into this dir.  
        * Now rename it to either sauerbraten_u (OS X 10.6+) or sauerbraten_c (OS X 10.5).  

* Menu:

    * Windows:
        * Open Windows Explorer and navigate to  
          C:\Users\<your username>\Documents\My Games\Sauerbraten or  
          startmenu->documents->My Games->Sauerbraten.  
        * Copy the content of the zip archive into this directory.  
        * Add exec config/main.cfg to your autoexec.cfg (create it, if it doesn't exit)
    * Linux:
        * Open your preferred file manager (e.g. konqueror, dolphin, nautilus, etc.)  
          or terminal and browse to ~/.sauerbraten.
        * Copy the content of the zip archive into this directory.  
        * Add exec config/main.cfg to your autoexec.cfg (create it, if it doesn't exit).
    * OS X:
        * Open finder and navigate to ~/Library/Application Support/sauerbraten  
          On 10.7+ you need to make the Library folder visible first, checkout: link  
        * Copy the content of the zip archive into the previous opened directory.  
        * Add exec config/main.cfg to your autoexec.cfg (create it, if it doesn't exit).

### Links: ###

* [Installation Instructions](http://wc-ng.sauerworld.org/builds/nightly/readme/#installation)
* [Client Documentation](http://wc-ng.sauerworld.org/builds/nightly/readme)
* [Chat Server](https://github.com/tpoechtrager/wc-ng/blob/master/CHATSERVER.md)
* [Build Instructions](https://github.com/tpoechtrager/wc-ng/blob/master/BUILD_INSTRUCTIONS.md)
* [Library Build Scripts](https://github.com/tpoechtrager/wc-ng/blob/master/src/scripts/lib-build-scripts/README.md)
* [ChangeLog](https://github.com/tpoechtrager/wc-ng/blob/master/ChangeLog)
* [Screenshots](http://wc-ng.sauerworld.org/builds/nightly/screens)

### Features: ###

* Global Chat:
    * Chat with your clanmates across servers
    * High encryption via OpenSSL
    * Easy server setup


* Demo Recorder:
    * Record demos directly via the client
    * Supports recording coop edit sessions as well as normal games

* Search Demo:
    * Search for player- names, frags, deaths, ... in demos


* Enhanced Demo Playback:
    * Jump forward in demos (/jumpto <mm>[<:ss>])
    * If available:
         * See Player- Frags, Deaths, Acc, ... in the Scoreboard
         * See the Server title in the Scoreboard
         * ... and more


* Enhanced Server Browser:
    * See the countries of the servers
    * See Players (including their stats, countries, ...)
    * Many filter options (available through the menu)


* Enhanced Scoreboard:
    * Including Player- Frags, Deaths, Accuracy, Country, ...


* Enhanced Gamehud:
    * Full configurable Stats Display
    * Ping and time left on the hud
    * Bandwidth statistics
    * On Screen Scoreboard


* Enhanced Scripting:
    * Event System with many builtin events
    * Script Vector: storing arrays is now an ease, no more need for "(at)"
    * Http Requests: You can do http requests with callbacks
    * Time Functions: Useful for filenames and other things
    * And many other new functionalities!


* And other useful things, such as:
    * Multiple Master Servers
    * Console History
    * Time Scaling
    * Proxy User Detection
    * Everything can be turned off and on
    * And much more (See our documentation for a full reference)!


### Credits: ###

Certain features were contributed by other people:

* Wouter "Aardappel" van Oortmerssen, Lee "eihrul" Salzman, and others:
    * Cube2: Sauerbraten
* [Pisto](http://github.com/pisto):
    * SDL2 port
* Hero, NemeX
    * Documentation improvements
    * Work on the ogros menu
* WahnFred
    * Some parts of the Build Scripts / Menu / Documentation
    * Time Left timer

I would also like to thank the following people for excessive testing
of pre-releases:

  * The DM Clan, Rosine, and others!

## Notes: ##

Library build scripts can be found [here](http://todo).

This product includes software developed by the OpenSSL Project  
for use in the OpenSSL Toolkit. ([https://www.openssl.org](https://www.openssl.org/))

## License: ##

WC-NG is distributed under the terms of the **GNU GPL v2**;

**OpenSSL Exception:**

In addition, as a special exception, the copyright holder gives permission to  
link the code of this program with any version of the OpenSSL library which is  
distributed under a license identical to that listed in the included  
LICENSE.OpenSSL.txt file, and distribute linked combinations including the two.  

You must obey the GNU General Public License in all respects for all of the  
code used other than OpenSSL.  

If you modify this file, you may extend this exception to your version of the  
file, but you are not obligated to do so.  

If you do not wish to do so, delete this exception statement from your version.

**COPYING Exception:**

Original Authors: Thomas Pöchtrager;  
Closed Source Software: An Anti-Cheat Client (nothing else intended);  

The original authors of this software are excepted from the GPLv2 license terms.  
This means, they are allowed to use sent in patches in closed source software.  

This exception is only valid for the original WC-NG repository.
