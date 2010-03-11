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

ZEND_FUNCTION(mb_SearchBasic)
{
	zval *cb = NULL;
	char *uin = NULL;
	char *proto = NULL;
	long  pl = 0;
	long  ul = 0;
	mb_event* mbe = (mb_event*)(((sPHPENV*)SG(server_context))->c_param);
	sACKSync* ack;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssz",&proto,&pl,&uin,&ul,&cb)==FAILURE || 
		cb->type != IS_STRING || !pl || !ul){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!zend_is_callable(cb,0,NULL)){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ack = (sACKSync*)my_malloc(sizeof(sACKSync));
	if(!ack){RETURN_FALSE}
	memset(ack,0,sizeof(sACKSync));

	if(!(ack->php = mbe->php))goto Error;

	ack->lType = ACKTYPE_SEARCH;
	ack->hContact = NULL;

	strncpy(ack->pszFunction,cb->value.str.val,sizeof(ack->pszFunction)-1);
	strncpy(ack->pszProtocol,proto,15);

	if(!g_slist.Add(ack)){goto Error;}

	ack->hProcess = (HANDLE)CallProtoService(proto,PS_BASICSEARCH,0,(LPARAM)uin);
	if(!ack->hProcess){
		g_slist.Del(ack);
		goto Error;
	}
	RETURN_LONG(1);
Error:
	if(ack)my_memfree(ack);
	RETURN_FALSE;
}

ZEND_FUNCTION(mb_SearchByEmail)
{
	zval *cb=NULL;
	char *email=NULL;
	char *proto=NULL;
	long  el = 0;
	long  pl = 0;
	mb_event* mbe = (mb_event*)(((sPHPENV*)SG(server_context))->c_param);
	sACKSync* ack;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssz",&proto,&pl,&email,&el,&cb)==FAILURE || 
		cb->type != IS_STRING || !pl){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!zend_is_callable(cb,0,NULL)){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ack = (sACKSync*)my_malloc(sizeof(sACKSync));
	if(!ack){RETURN_FALSE}
	memset(ack,0,sizeof(sACKSync));

	if(!(ack->php = mbe->php))goto Error;

	ack->lType = ACKTYPE_SEARCH;
	ack->hContact = NULL;

	strncpy(ack->pszFunction,cb->value.str.val,sizeof(ack->pszFunction)-1);
	strncpy(ack->pszProtocol,proto,15);

	if(!g_slist.Add(ack)){goto Error;}

	ack->hProcess = (HANDLE)CallProtoService(proto,PS_SEARCHBYEMAIL,0,(LPARAM)email);
	if(!ack->hProcess){
		g_slist.Del(ack);
		goto Error;
	}
	RETURN_LONG(1);
Error:
	if(ack)my_memfree(ack);
	RETURN_FALSE;
}

ZEND_FUNCTION(mb_SearchByName)
{
	zval *cb=NULL;
	char *proto=NULL;
	long  pl = 0;
	long  tl = 0;
	PROTOSEARCHBYNAME psbn={0};
	sACKSync* ack;

	mb_event* mbe = (mb_event*)(((sPHPENV*)SG(server_context))->c_param);

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "szsss",&proto,&pl,&cb,
		&psbn.pszNick,&tl,&psbn.pszFirstName,&tl,&psbn.pszLastName,&tl)==FAILURE || cb->type != IS_STRING || !pl){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!zend_is_callable(cb,0,NULL)){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ack = (sACKSync*)my_malloc(sizeof(sACKSync));
	if(!ack){RETURN_FALSE}
	memset(ack,0,sizeof(sACKSync));

	if(!(ack->php = mbe->php))goto Error;

	ack->lType = ACKTYPE_SEARCH;
	ack->hContact = NULL;

	strncpy(ack->pszFunction,cb->value.str.val,sizeof(ack->pszFunction)-1);
	strncpy(ack->pszProtocol,proto,15);

	if(!g_slist.Add(ack)){goto Error;}

	ack->hProcess = (HANDLE)CallProtoService(proto,PS_SEARCHBYNAME,0,(LPARAM)&psbn);
	if(!ack->hProcess){
		g_slist.Del(ack);
		goto Error;
	}
	RETURN_LONG(1);
Error:
	if(ack)my_memfree(ack);
	RETURN_FALSE;
}