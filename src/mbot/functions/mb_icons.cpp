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
#include "../functions.h"

///////////////////////////////
//icons
///////////////////////////////
const static char* mb_lsi_def_icons[]={0,IDI_APPLICATION,IDI_ASTERISK,
		IDI_ERROR,IDI_EXCLAMATION,IDI_HAND,IDI_INFORMATION,IDI_QUESTION,IDI_WARNING,IDI_WINLOGO,0};

ZEND_FUNCTION(mb_IconLoadSys)
{
	HANDLE hIcon = NULL;
	long iid=0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&iid) == FAILURE || iid<1 || iid>9){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	RETURN_LONG((long)LoadIcon(NULL,mb_lsi_def_icons[iid]));
}
ZEND_FUNCTION(mb_IconLoadSkin)
{
	long iid=0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&iid) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	RETURN_LONG((long)LoadSkinnedIcon((int)iid));
}
ZEND_FUNCTION(mb_IconLoadSkinnedProto)
{
	char* proto=NULL;
	long iid=0,pl=0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",&proto,&pl,&iid) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	RETURN_LONG((long)LoadSkinnedProtoIcon((const char*)(*proto)?(proto):(NULL),(int)iid));
}
ZEND_FUNCTION(mb_IconLoadProto)
{
	char* proto=NULL;
	long iid=0,pl=0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",&proto,&pl,&iid) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	//
	RETURN_LONG((long)CallProtoService((const char*)proto,PS_LOADICON,(int)iid | PLIF_SMALL,0));
}

ZEND_FUNCTION(mb_IconDestroy)
{
	long iid=0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&iid) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	try{
		DestroyIcon((HICON)iid);
	}catch(...){
		RETURN_FALSE;
	}
	RETURN_TRUE;
}