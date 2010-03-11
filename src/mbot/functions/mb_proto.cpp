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

//protocols
ZEND_FUNCTION(mb_PGetMyStatus)
{
	char*	proto=NULL;
	long	pl=0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&proto,&pl) == FAILURE || !pl){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	pl = CallProtoService(proto,PS_GETSTATUS,0,0);
	RETURN_LONG(pl);
}

ZEND_FUNCTION(mb_PSetMyStatus)
{
	long  status=0,pl=0;
	char* proto=NULL;

	sPHPENV* ctx = (sPHPENV*)SG(server_context);
	mb_event* mbe = (mb_event*)(ctx->c_param);
	if(mbe->event == MBT_NEWMYSTATUS){
		RETURN_FALSE;
	}

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",&proto,&pl,&status) == FAILURE || !status || !proto){
		PHP_FALSE_AND_ERROR;
	}
	RETURN_LONG(CallProtoService(proto,PS_SETSTATUS,status,0)==0);
}
ZEND_FUNCTION(mb_PSetMyAwayMsg)
{
	long  ml=0,pl=0,status=0;
	char* proto=NULL,*msg=NULL;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|l",&proto,&pl,&msg,&ml,&status) == FAILURE || !proto || !pl){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!status){
		status = CallProtoService(proto,PS_GETSTATUS,0,0);
	}
	RETVAL_LONG(CallProtoService(proto,PS_SETAWAYMSG,status,(ml)?((LPARAM)msg):(NULL))==0);
	return;
}

ZEND_FUNCTION(mb_PGetCaps)
{
	long  pl=0;
	long  flag=0;
	char* proto=NULL;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",&proto,&pl,&flag) == FAILURE || !pl){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	RETVAL_LONG(CallProtoService(proto,PS_GETCAPS,flag,0));
	return;
}