############################
# Contents
############################
1. EULA
2. Contact me
3. Required Components
4. Usage
5. Changelog

#####################################
# 1. EULA
#####################################

MBot ("the program") is provided as-is without any stated or implied warranty. By installing, copying, or otherwise using this program or its components, you agree to be bound by the terms, conditions, and the spirit of this agreement. If you do not agree, discontinue the use of the application and remove it from your computer or network. Neither the author, nor any contributors of this program are responsible for any direct or consequential damages arising from the use of this program or sceneries produced in whole or in part by this program.

Your copy of the program was distributed with the freeware license.
As freeware you are permitted to distribute this archive subject to the following conditions:

- The archive must be distributed without modification to the contents of the archive;
- Redistributing this archive with any files added, removed or modified is prohibited without the permission of the author;
- The inclusion of any individual file from this archive in another archive is prohibited without permission from the author;
- No charge may be made for this archive;
- The authors' rights and wishes concerning this archive must be respected;
- If you have paid a fee for this archive or any derivative of this archive, please contact the author!
- If you haven't downloaded this library at www.piopawlu.net please visit my page in order to let me know how many people are using this lib;

############################
# 2. Contact Me
############################
 Piotr Pawluczuk
 eml : piotrek@piopawlu.net
 www : http://www.piopawlu.net
 gsm : +48 503339974
 icq : 109468355
 gg  : 1634410

############################
# 3. Required components
############################
Everything you need to use MBot is php 5.1.4+ library, which is not enclosed due to its size. I'll provide a tiny version of php5ts.dll on my web site when I have some free time.

############################
# 4. Usage
############################
First of all you have to install a script which can be downloaded at www.miranda-im.org or copied from the /mbot/scripts directory. To install a script you should drag and drop the file into the CommandBox of MBot Console; Now, depending if the script includes a command or a service, you'll see the results or you'll have to use COMMAND TAG and the command to execute it; Default command tag is "m>", and a script tag is "?>";

############################
# 5. Changelog /dd.mm.yyyy/
############################

05.08.2006
- linked against latest php binaries (5.1.4)
- some minor bugs fixed
- no more junk file left on e:/ drive (shame on me, I left some debug code inside)

23.05.2005

- printing vars fixed
- IRC modules listed properly

20.05.2005

- quite a few new functions;
- script managment functions;
- new thread resource manager;
- no more 'you must not call php...' message;
- multidimensional, mixed, associative arrays support in mt_set/get var;
- array indexes are no longer broken (when using $keep_idx option for mt_setvar($name,$val,$create,$keep_idx));
- console window fixed;
- many minor changes;
- new constant MBOT_TIMESTAMP;
- IRC support;
- possibility of adding root menu items;
- possibility of calling php services from php;
- standard input dialog is now centered by default;
- mb_SelfSetInfo($desc); //sets script description, must NOT be used before mb_SelfRegister!
- new event MB_EVENT_CONFIG mbe_config(){}; a functon called from the mbot's config page;
- etc;


29.03.2005

- mb_SysCallProc function fixed;

26.03.2005

- HTTPD: POST finally fixed, max post length increased to 4 MB
- ASUS ATKACPI (eMail LED) support;
- PHP: File uploads enabled by default;

23.03.2005
 - HTTPD: AUTH error fixed;
 - some minor bugs fixed;
 - a few new functions;
 - WM_TIMER support for dialogs;

05.03.2005
 - File transfers in/out
 - Combo box support
 - Generic input box - mb_DlgGetString(...);
 - HTTPD: recursive access level configuration
 - Many bugs has been fixed;
 - WM_COMMAND, WM_NOTIFY callbacks* (optional);
 - HTTPD: $_SERVER variables now work correctly;
 - HTTPD: getenv(..); function fixed;
 - etc;

10.01.2005
 - mb_SysBeginThread, mb_SelfEnable, and a few other functions;
 - httpd authorization error fixed;

29.12.2004
 - mb_MenuAdd function fixed
 - quite a few new functions added, (hooking events, creating services, calling WINAPI, etc);
 - some bugs has been fixed
 - ability of calling services which take structures as parameters (for advanced users)
 - etc;

15.12.2004
 - httpd 'no output' bug fixed;
 - new functions mb_SysCallService, mb_SysCallProtoService, mb_SysCallContactService;
 - mt_getvar function fixed;
 - mt_delvar function fixed;
 - a few different bugs fixed, and probably introduced :-)

12.12.2004
 - dialog boxes
 - some bugs has been fixed

11.12.2004
 - scheduler parser fixed
 - http file sending fixed

10.12.2004
 - a few bugs fixed
 - a few new functions (pls refer to the manual)
 - mb_SchReg no longer takes $script as a param!
 - new event (MB_EVENT_FILE_IN)
 - etc;

09.12.2004
 - 100% CPU Load fixed;
 - new param $uin for the mbe_AwayMsgICQ event;

08.12.2004
 - dynamic script registration;
 - re-caching scripts on runtime;
 - new script manager;
 - new MIME system for the httpd;

07.12.2004
 - some bugs has been fixed
 - new functions
 - !!! new function names !!!

06.12.2004
 - built in simple httpd deamon (just GET/POST commands, yet it supports php ;])
 - another few bugs fixed, hopefully not too many introduced;

05.12.2004
 - new events
 - popups with callbacks
 - menu items with callbacks
 - clist events with callbacks
 - new functions (mb_GetCAwayMsg, mb_PUAddPopupEx, mb_EnumProtocols, mb_MenuAdd, etc)
 - several bug fixed
 - msvcr71.dll is no longer necessary (linked against msvcr70)
 - new bugs ;-)


28.11.2004
 - new events
 - task scheduling
 - new functions
 - chm help with examples
 - drag & drop script installing
 - bug fixes

21.11.2004
 - new events (command,startup,shutdown)
 - new functions
 - bug fixes

17.11.2004
 - new functions (events/status/awaymsg/etc;)
 - file caching
 - new events
 - some bugs fixed

14.11.2004
 - new functions
 - new events

14.11.2004
 - msg_in,msg_out script corrected
 - some bug fixes
 - some new features
 - handlers registering

13.11.2004
 - built in libphp

12.11.2004
 - new functions
 - new features

11.11.2004
 - initial release

######################################
# Copyright (C) 2004 Piotr Pawluczuk #
######################################