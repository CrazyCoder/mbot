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

ZEND_FUNCTION(mb_MsgSetBody)
{
	char*	body = NULL;
	char*	tmp;
	long	bl = 0;
	sPHPENV* ctx = (sPHPENV*)SG(server_context);
	mb_event* mbe = (mb_event*)(ctx->c_param);

	if((mbe->event!=MBT_PRERECV && mbe->event!=MBT_SEND && mbe->event != MBT_COMMAND && mbe->event != MBT_IRC_RAW) ||
		zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&body,&bl) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(mbe->event == MBT_IRC_RAW)
	{
		mmi.mmi_free(*((char**)mbe->p4));
		if(mbe->p3 == NULL && (bl < 2 || body[bl-2]!='\r'))bl += 2;

		tmp = (char*)mmi.mmi_malloc(bl + 1);

		*((char**)mbe->p4) = tmp;

		if(tmp){
			memcpy(tmp,body,bl);
			if(mbe->p3 == NULL){
				*(short*)&tmp[bl-2]='\n\r';
			}
			tmp[bl]=0;
			RETURN_LONG(1);
		}else{
			RETURN_FALSE;
		}
	}
	else if(mbe->t1 = MBE_SSTRING)
	{
		string* cs = (string*)mbe->p1;
		*cs = body;
		mbe->lFlags |= MBOT_FLAG_BODY_CHANGED;
	}

	RETURN_LONG(1);
}

ZEND_FUNCTION(mb_MsgSend)
{
	char*	body = NULL,*proto = NULL;
	long	bl = 0,cid = 0,hp = 0,result = 0,param = 0;
	zval*	cb = NULL;
	zend_bool log = 0;
	zend_bool unicode = 0;
	sACKSync* ack = NULL;
	mb_event* mbe = (mb_event*)(((sPHPENV*)SG(server_context))->c_param);

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls|bz!lb",&cid,&body,&bl,&log,&cb,&param,&unicode) == FAILURE
		|| cid==NULL || bl==0){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(cb && !zend_is_callable(cb,0,NULL)){
		PHP_FALSE_AND_ERRORS("$cb is expected to be a valid callback function!");
	}

	proto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)cid,0);
	if(!proto)
	{
		RETURN_FALSE;
	}

	if(cb)
	{
		ack = (sACKSync*)my_malloc(sizeof(sACKSync));
		if(!ack){RETURN_FALSE}

		memset(ack,0,sizeof(sACKSync));

		if(!(ack->php = mbe->php)){
			goto Error;
		}

		ack->lType = ACKTYPE_MESSAGE;
		ack->hContact = (HANDLE)cid;
		ack->lParam = param;

		strncpy(ack->pszFunction,cb->value.str.val,sizeof(ack->pszFunction)-1);
		strncpy(ack->pszProtocol,proto,15);

		if(!g_slist.Add(ack)){
			goto Error;
		}

		ack->hProcess = (HANDLE)CallContactService((HANDLE)cid,(unicode)?(PSS_MESSAGE "W"):(PSS_MESSAGE),(WPARAM)0x800000,(LPARAM)body);
		if(!ack->hProcess || ack->hProcess == (HANDLE)(-1))
		{
			g_slist.Del(ack);
			goto Error;
		}
	}
	else
	{
		if(!CallContactService((HANDLE)cid,(unicode)?(PSS_MESSAGE "W"):(PSS_MESSAGE),(WPARAM)0x800000,(LPARAM)body))
		{
			goto Error;
		}
	}

	if(log)
	{
		DBEVENTINFO dbei = {sizeof(dbei),0};
		dbei.cbBlob = bl + 1;
		dbei.pBlob = (PBYTE)body;
		dbei.eventType = EVENTTYPE_MESSAGE;
		dbei.timestamp = time(0);
		dbei.szModule = proto;
		dbei.flags = DBEF_SENT;
		bl = CallService(MS_DB_EVENT_ADD,(WPARAM)cid,(LPARAM)&dbei);

		if(ack && !param){
			ack->lParam = bl;
		}
	}else{
		bl = 1;
	}

	RETURN_LONG(bl);
Error:
	if(ack){
		my_memfree(ack);
	}
	RETURN_FALSE;
}