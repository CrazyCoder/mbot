Miranda Scripting Plugin (MSP - mBot) v0.0.3.6

This is a fork of the original MSP plug-in available at:
http://addons.miranda-im.org/details.php?action=viewfile&id=1584

Current version includes support for Miranda 0.8.x and 0.9.x and
also fixes the console window appearing when starting Miranda with
mBot enabled on some systems.

/dist folder contains the recent binary of mbot and also the most
recent binary of PHP from 5.2.x branch. All the binaries are linked
statically with the C++ runtime and have no external dependencies.

To install this version simply copy the contents of the /dist folder
into your Miranda installation directory preserving the folders.

If you want to compile it yourself, see docs/build.txt for instructions.


CHANGES:

0.0.3.6
 - static linking with C++ runtime (/MT), no MSVCR*.dll required
 - MSVS 2008 SP1 project (vc9)
 - PHP 5.2.13 binary with most extensions disabled to save size
 - fixed console window appearing on Miranda start
 - Miranda 0.8.x and 0.9.x compatibility
