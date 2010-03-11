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
//authorize
///////////////////////////////
ZEND_FUNCTION(mb_AuthGetInfo)
{
	sPHPENV* ctx = (sPHPENV*)SG(server_context);
	mb_event* mbe = (mb_event*)(ctx->c_param);
	char* inf = (char*)mbe->p3;

	if(mbe->t2 != MBE_EVENTID || (mbe->p2 != (void*)MB_EVENT_AUTH_IN) || mbe->t3!=MBE_CUSTOM){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(array_init(return_value)==FAILURE){
		RETURN_FALSE;
	}

	//blob is: uin(DWORD),hcontact(HANDLE),nick(ASCIIZ),first(ASCIIZ),last(ASCIIZ),email(ASCIIZ),reason(ASCIIZ)

	//nick
	inf += sizeof(DWORD) + sizeof(HANDLE);
	add_index_string(return_value,0,inf,1);
	//first
	inf += strlen(inf) + 1;
	add_index_string(return_value,1,inf,1);
	//last
	inf += strlen(inf) + 1;
	add_index_string(return_value,2,inf,1);
	//email
	inf += strlen(inf) + 1;
	add_index_string(return_value,3,inf,1);
	return;
}

ZEND_FUNCTION(mb_AuthDeny)
{
	sPHPENV* ctx = (sPHPENV*)SG(server_context);
	mb_event* mbe = (mb_event*)(ctx->c_param);
	char* reason=NULL;
	long rl=0,hid=0,cid=0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sll",&reason,&rl,&cid,&hid) == FAILURE ||
		!reason || !hid || !cid){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	RETURN_LONG((CallService(PS_AUTHDENY,(WPARAM)hid,(LPARAM)reason) == 0));
}

ZEND_FUNCTION(mb_AuthAccept)
{
	sPHPENV* ctx = (sPHPENV*)SG(server_context);
	mb_event* mbe = (mb_event*)(ctx->c_param);
	long hid=0,cid=0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll",&cid,&hid) == FAILURE || !hid || !cid){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	RETURN_LONG((CallContactService((HANDLE)cid,PS_AUTHALLOW,(WPARAM)hid,NULL) == 0));
}

ZEND_FUNCTION(mb_AuthStore)
{
	DBEVENTINFO dbei = {sizeof(dbei),0};
	sPHPENV* ctx = (sPHPENV*)SG(server_context);
	mb_event* mbe = (mb_event*)(ctx->c_param);
	CCSDATA* css = (CCSDATA*)mbe->lParam;
	PROTORECVEVENT* prr;
	long result = 0;
	char* proto = NULL;

	if(mbe->event != MBT_AUTHRECV || !css || !css->lParam){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}else if(mbe->lFlags & MBOT_FLAG_STORED){
		RETURN_FALSE;
	}

	prr = (PROTORECVEVENT*)css->lParam;
	proto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)css->hContact,0);

	dbei.cbBlob = prr->lParam;
	dbei.pBlob = (PBYTE)prr->szMessage;
	dbei.eventType = (unsigned short)EVENTTYPE_AUTHREQUEST;
	dbei.timestamp = time(0);
	dbei.szModule = proto;
	dbei.flags = DBEF_READ;

	result = CallService(MS_DB_EVENT_ADD,(WPARAM)css->hContact,(LPARAM)&dbei);
	if(result){
		mbe->lFlags |= MBOT_FLAG_STORED;
	}

	RETURN_LONG(result);
}