/*

Miranda Scripting Plugin for Miranda-IM
Copyright 2004-2006 Piotr Pawluczuk (www.pawluczuk.info)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <cXmldoc.h>

#define MBOT_CFG_STATUS "Status"
static const char* config_table[]={MBOT_CFG_STATUS,NULL};
int mbot_our_own(const char* s);
const char* mbot_replace_with_our_own(const char* s);

struct php_entry
{
	const char* name;
	const char* def_val;
	unsigned long str;
};

extern char pszCmdTag[8];
extern char pszPhpTag[8];
extern long lCmdTagLen;
extern long lPhpTagLen;
extern long lErrorLog;
extern cXmlDoc cSettings;

int php_generate_ini();

#endif //_CONFIG_H_