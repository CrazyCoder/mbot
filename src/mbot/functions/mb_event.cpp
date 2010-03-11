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

/***********************************
 * history
 **********************************/
ZEND_FUNCTION(mb_EventFindFirst)
{
	long cid=0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&cid) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	RETURN_LONG(CallService(MS_DB_EVENT_FINDFIRST,(WPARAM)cid,0));
}
ZEND_FUNCTION(mb_EventFindFirstUnread)
{
	long cid=0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&cid) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	RETURN_LONG(CallService(MS_DB_EVENT_FINDFIRSTUNREAD,(WPARAM)cid,0));
}
ZEND_FUNCTION(mb_EventFindLast)
{
	long cid=0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&cid) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	RETURN_LONG(CallService(MS_DB_EVENT_FINDLAST,(WPARAM)cid,0));
}
ZEND_FUNCTION(mb_EventFindNext)
{
	long hid=0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&hid) == FAILURE || !hid){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	RETURN_LONG(CallService(MS_DB_EVENT_FINDNEXT,(WPARAM)hid,0));
}
ZEND_FUNCTION(mb_EventFindPrev)
{
	long hid=0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&hid) == FAILURE || !hid){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	RETURN_LONG(CallService(MS_DB_EVENT_FINDPREV,(WPARAM)hid,0));
}
ZEND_FUNCTION(mb_EventGetCount)
{
	long cid=0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&cid) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	cid = CallService(MS_DB_EVENT_GETCOUNT,(WPARAM)cid,0);
	if(cid == -1){
		RETURN_FALSE;
	}else{
		RETURN_LONG(cid);
	}
}
ZEND_FUNCTION(mb_EventDel)
{
	long hid=0,cid=-1;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|l",&hid,&cid) == FAILURE || !hid){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(cid == -1){
		cid = CallService(MS_DB_EVENT_GETCONTACT,hid,0);
		if(cid == -1){
			RETURN_NULL();
		}
	}
	RETURN_LONG(CallService(MS_DB_EVENT_DELETE,cid,hid)!=-1);
}
ZEND_FUNCTION(mb_EventAdd)
{
	DBEVENTINFO dbei = {sizeof(dbei),0};
	char* proto=NULL,*body=NULL;
	long hid=0,cid=0,pl=0,bl=0,type=EVENTTYPE_MESSAGE,ts=0;
	long flags=DBEF_SENT;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sls|lll",&proto,&pl,&cid,&body,&bl,&type,&flags,&ts) == FAILURE || !proto){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	flags &= ~(0x00000001);

	dbei.cbBlob = bl + 1;
	dbei.pBlob = (PBYTE)body;
	dbei.eventType = (unsigned short)type;
	dbei.timestamp = (ts)?(ts):(time(0));
	dbei.szModule = proto;
	dbei.flags = flags;

	RETURN_LONG(CallService(MS_DB_EVENT_ADD,(WPARAM)cid,(LPARAM)&dbei));
}
ZEND_FUNCTION(mb_EventGetData)
{
	long hid=0;
	unsigned long mbLen = 0;
	char* buffer = NULL;
	wchar_t *wbuff = NULL;
	long req_buff = NULL;
	zend_bool unicode = 0;

	DBEVENTINFO dbei = {0};

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|b",&hid,&unicode) == FAILURE || !hid){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	req_buff = CallService(MS_DB_EVENT_GETBLOBSIZE,(WPARAM)hid,0);
	if(req_buff < 0){
		RETURN_FALSE;
	}

	buffer = (char*)emalloc(req_buff + 1);
	if(!buffer){
		RETURN_FALSE;
	}
	buffer[req_buff]='\0';

	dbei.cbSize = sizeof(dbei);
	dbei.cbBlob = req_buff;
	dbei.pBlob = (PBYTE)buffer;

	if(CallService(MS_DB_EVENT_GET, (WPARAM)hid, (LPARAM)&dbei)!=0){
		efree(buffer);
		RETURN_FALSE;
	}

	//module,type,timestamp,flags,value
	if(array_init(return_value)==FAILURE){
		efree(buffer);
		RETURN_FALSE;
	}

	add_index_string(return_value, 0, dbei.szModule, 1);
	add_index_long(return_value, 1, dbei.eventType);
	add_index_long(return_value, 2, dbei.timestamp);
	add_index_long(return_value, 3, dbei.flags);

	mbLen = strlen(buffer);

	if(unicode){
		//add the ascii body
		add_index_stringl(return_value, 4, buffer, mbLen, 1);
		//if possible add the UNICODE body
		if(((mbLen + 1) * 3) <= dbei.cbBlob){
			add_index_stringl(return_value, 5, (buffer + mbLen + 1), mbLen * 2 , 1);
		}else{//if not add empty string
			wbuff = (wchar_t*)emalloc((mbLen + 1) * sizeof(wchar_t));
			if(wbuff){
				for(unsigned i=0;i<mbLen;i++){
					wbuff[i] = buffer[i];
				}
				add_index_stringl(return_value, 5, (char*)wbuff, mbLen * 2 , 0);
			}else{
				add_index_null(return_value, 5);
			}
		}
	}else{
		add_index_stringl(return_value, 4, buffer, mbLen, 0);
	}
	return;
}
ZEND_FUNCTION(mb_EventMarkRead)
{
	long hid=0,cid=(-1);
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|l",&hid,&cid) == FAILURE || !hid){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(cid == (-1)){
		cid = CallService(MS_DB_EVENT_GETCONTACT,hid,0);
	}

	if(cid == (-1)){
		RETURN_FALSE;
	}
	
	RETURN_LONG(CallService(MS_DB_EVENT_MARKREAD,cid,hid)!=-1);
}