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

ZEND_FUNCTION(mb_IrcGetGuiDataIn)
{
	mb_event* mbe = (mb_event*)((sPHPENV*)SG(server_context))->c_param;
	if(mbe->event == MBT_IRC_IN)
	{
		GCEVENT* gce = (GCEVENT*)mbe->p4;
		if(array_init(return_value)==FAILURE)RETURN_FALSE;
		add_index_long(return_value,0,gce->pDest->iType);
		if(gce->pDest->pszID){add_index_string(return_value,1,(char*)gce->pDest->pszID,1);}else{add_index_unset(return_value,1);}
		if(gce->pszText){add_index_string(return_value,2,(char*)gce->pszText,1);}else{add_index_unset(return_value,2);}
		if(gce->pszNick){add_index_string(return_value,3,(char*)gce->pszNick,1);}else{add_index_unset(return_value,3);}
		if(gce->pszUID){add_index_string(return_value,4,(char*)gce->pszUID,1);}else{add_index_unset(return_value,4);}
		if(gce->pszStatus){add_index_string(return_value,5,(char*)gce->pszStatus,1);}else{add_index_unset(return_value,5);}
		if(gce->pszUserInfo){add_index_string(return_value,6,(char*)gce->pszUserInfo,1);}else{add_index_unset(return_value,6);}
		add_index_long(return_value,7,gce->bIsMe);
		add_index_long(return_value,8,(gce->dwFlags & GCEF_ADDTOLOG) != 0);
		add_index_long(return_value,9,gce->time);
		add_index_long(return_value,10,((WPARAM_GUI_IN*)mbe->wParam)->wParam);
		return;
	}
	else
	{
		PHPWSE("You cannot call this function here!");
	}
}

void help_strrealloc(zval* v,const char** out)
{
	if(v){
		mmi.mmi_free((void*)*out);
		if((v->type == IS_BOOL && !v->value.lval) || (v->type == IS_LONG && !v->value.lval)){
			*out = NULL;
		}else{
			convert_to_string(v);
			*out = (const char*)mmi.mmi_malloc(v->value.str.len+1);
			memcpy((void*)*out,v->value.str.val,v->value.str.len+1);
		}
	}
}

ZEND_FUNCTION(mb_IrcSetGuiDataIn)
{
	zval* itype = NULL;
	zval* pszID = NULL;
	zval* pszText = NULL;
	zval* pszNick = NULL;
	zval* pszUID = NULL;
	zval* pszStatus = NULL;
	zval* pszUserInfo = NULL;
	zval* bIsMe = NULL;
	zval* bAddToLog = NULL;
	zval* timestamp = NULL;
	long wParam = 0xfaaaffaa;

	mb_event* mbe = (mb_event*)((sPHPENV*)SG(server_context))->c_param;
	
	if(mbe->event != MBT_IRC_IN){
		PHPWSE("You cannot call this function here!");
	}
	//mb_IrcSetGuiDataIn($iType,$pszID,$pszText,$pszNick,$pszUID,$pszStatus,$pszUserInfo,$bIsMe,$bAddToLog,$timestamp,$wParam);

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z!|z!z!z!z!z!z!z!z!z!l",
		&itype,&pszID,&pszText,&pszNick,&pszUID,&pszStatus,&pszUserInfo,&bIsMe,&bAddToLog,&timestamp,&wParam) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	GCEVENT* gce = (GCEVENT*)mbe->p4;
	if(itype && itype->type == IS_LONG){
		gce->pDest->iType = itype->value.lval;
	}

	try{
		help_strrealloc(pszID,(const char**)&gce->pDest->pszID);
		help_strrealloc(pszText,&gce->pszText);
		help_strrealloc(pszNick,&gce->pszNick);
		help_strrealloc(pszUID,&gce->pszUID);
		help_strrealloc(pszStatus,&gce->pszStatus);
		help_strrealloc(pszUserInfo,&gce->pszUserInfo);
		help_strrealloc(pszNick,&gce->pszNick);
	}catch(...){
		PHPWSE("Oops... some data might have got corrupted!");
	}

	if(bIsMe && bIsMe->type == IS_LONG){
		gce->bIsMe = bIsMe->value.lval;
	}
	if(bAddToLog && bAddToLog->type == IS_LONG){
		gce->dwFlags = (bAddToLog->value.lval)?GCEF_ADDTOLOG:0;
	}
	if(timestamp && timestamp->type == IS_LONG){
		gce->time = timestamp->value.lval;
	}
	if(wParam != 0xfaaaffaa){
		((WPARAM_GUI_IN*)mbe->wParam)->wParam = wParam;
	}
	RETURN_TRUE;
}

ZEND_FUNCTION(mb_IrcSetGuiDataOut)
{
	//$iType,$pszID,$pszUID,$text
	zval* itype;
	zval* pszID;
	zval* pszUID;
	zval* text;

	mb_event* mbe = (mb_event*)((sPHPENV*)SG(server_context))->c_param;
	
	if(mbe->event != MBT_IRC_OUT){
		PHPWSE("You cannot call this function here!");
	}
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z!|z!z!z!",&itype,&pszID,&pszUID,&text) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	GCHOOK* gch = (GCHOOK*)mbe->p4;

	if(itype && itype->type == IS_LONG){
		gch->pDest->iType = itype->value.lval;
	}

	try{
		help_strrealloc(text,(const char**)&gch->pszText);
		help_strrealloc(pszUID,(const char**)&gch->pszUID);
		help_strrealloc(pszID,(const char**)&gch->pDest->pszID);
	}catch(...){
		PHPWSE("Oops... some data might have got corrupted!");
	}
	RETURN_TRUE;
}
ZEND_FUNCTION(mb_IrcInsertRawIn)
{
	char msg[256];
	char raw[530];
	char* module,*body;
	long  ml,bl;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",&module,&ml,&body,&bl) == FAILURE || !*module){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}else if(bl > 512){
		PHPWSE("You must not provide more than 512 bytes including CRLF for this function!");
	}

	memcpy(raw,body,bl);
	raw[bl]=0;

	_snprintf(msg,sizeof(msg)-1,"%s/InsertRawIn",module);
	RETURN_LONG(CallService(msg,NULL,(LPARAM)raw));
}
ZEND_FUNCTION(mb_IrcInsertRawOut)
{
	char msg[256];
	char raw[530];
	char* module,*body;
	long  ml,bl;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",&module,&ml,&body,&bl) == FAILURE || !*module){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}else if(bl > 512){
		PHPWSE("You must not provide more than 512 bytes including CRLF for this function!");
	}

	memcpy(raw,body,bl);
	if(raw[bl-2]!='\r'){
		if(raw[bl-1]=='\n'){
			raw[bl-1]='\r';
			raw[bl]='\n';
			raw[bl+1]=0;
		}else{
			raw[bl]='\r';
			raw[bl+1]='\n';
			raw[bl+2]=0;
		}
	}else{
		raw[bl]=0;
	}

	_snprintf(msg,sizeof(msg)-1,"%s/InsertRawOut",module);
	RETURN_LONG(CallService(msg,NULL,(LPARAM)raw));
}
ZEND_FUNCTION(mb_IrcInsertGuiIn)
{
	char* module;
	long ml;
	long itype = NULL;
	zval* pszID = NULL;
	zval* pszText = NULL;
	zval* pszNick = NULL;
	zval* pszUID = NULL;
	zval* pszStatus = NULL;
	zval* pszUserInfo = NULL;
	char  bIsMe = 0;
	char  bAddToLog = 0;
	long  timestamp = 0;
	long  wParam = 0;

	char msg[256];
	GCEVENT gce = {0};
	GCDEST gcd = {0};
	WPARAM_GUI_IN wpi = {0};

	mb_event* mbe = (mb_event*)((sPHPENV*)SG(server_context))->c_param;
	
	//mb_IrcInsertGuiIn($module,$iType,$pszID,$pszText,$pszNick,$pszUID,$pszStatus,$pszUserInfo,$bIsMe,$bAddToLog,$timestamp,$wParam);
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "slz!|z!z!z!z!z!bbll",&module,&ml,&itype,&pszID,
		&pszText,&pszNick,&pszUID,&pszStatus,&pszUserInfo,&bIsMe,&bAddToLog,&timestamp,&wParam) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	gce.pDest = &gcd;
	gcd.iType = itype;
	gcd.pszModule = module;
	if(pszID){
		convert_to_string(pszID);
		gcd.pszID = pszID->value.str.val;
	}
	gce.cbSize = sizeof(gce);
	gce.dwItemData = 0x80000000;
	gce.time = (timestamp)?timestamp:time(0);
	gce.bIsMe = bIsMe;
	gce.dwFlags = (bAddToLog)?GCEF_ADDTOLOG:0;

	if(pszUID){
		convert_to_string(pszUID);
		gce.pszUID = pszUID->value.str.val;
	}
	if(pszText){
		convert_to_string(pszText);
		gce.pszText = pszText->value.str.val;
	}
	if(pszNick){
		convert_to_string(pszNick);
		gce.pszNick = pszNick->value.str.val;
	}
	if(pszStatus){
		convert_to_string(pszStatus);
		gce.pszStatus = pszStatus->value.str.val;
	}
	if(pszUserInfo){
		convert_to_string(pszUserInfo);
		gce.pszUserInfo = pszUserInfo->value.str.val;
	}
	wpi.wParam = wParam;
	wpi.pszModule = module;

	_snprintf(msg,sizeof(msg)-1,"%s/InsertGuiIn",module);
	RETURN_LONG(CallService(msg,(WPARAM)&wpi,(LPARAM)&gce));
}

ZEND_FUNCTION(mb_IrcInsertGuiOut)
{
	//?> mb_IrcInsertGuiOut('IRC',0x0080,'#miranda',NULL,NULL);
	char* module;
	long iType = 0;
	long ml = 0;
	zval* pszText = NULL;
	zval* pszUID = NULL;
	zval* pszID = NULL;

	char msg[256];
	GCHOOK gch = {0};
	GCDEST gcd = {0};

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "slz!|z!z!",&module,&ml,&iType,&pszID,&pszUID,&pszText) == FAILURE || !*module){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}


	gch.pDest = &gcd;
	gcd.iType = iType;
	gcd.pszModule = module;
	gch.dwData = 0x80000000;
	if(pszID){
		convert_to_string(pszID);
		gch.pDest->pszID = pszID->value.str.val;
	}
	if(pszUID){
		convert_to_string(pszUID);
		gch.pszUID = pszUID->value.str.val;
	}
	if(pszText){
		convert_to_string(pszText);
		gch.pszText = pszText->value.str.val;
	}
	_snprintf(msg,sizeof(msg)-1,"%s/InsertGuiOut",module);
	RETURN_LONG(CallService(msg,(WPARAM)module,(LPARAM)&gch));
}

ZEND_FUNCTION(mb_IrcGetData)
{
	char* module;
	char* setting;
	char* channel = 0;
	long ml,sl,cl;
	char msg[256];

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|s",&module,&ml,&setting,&sl,&channel,&cl) == FAILURE || !*module){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	_snprintf(msg,sizeof(msg)-1,"%s/GetIrcData",module);
	setting = (char*)CallService(msg,(WPARAM)channel,(LPARAM)setting);
	if(setting){
		try{
			RETVAL_STRING(setting,1);
			mmi.mmi_free(setting);
			return;
		}catch(...){
			RETURN_FALSE;
		}
	}else{
		RETURN_FALSE;
	}
}
ZEND_FUNCTION(mb_IrcPostMessage)
{
	char* module;
	char* text;
	long ml,tl;

	char msg[256];
	GCHOOK gch = {0};
	GCDEST gcd = {0};

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",&module,&ml,&text,&tl) == FAILURE || !*module){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	gch.pDest = &gcd;
	gcd.iType = 1;
	gcd.pszModule = module;
	gcd.pszID = "Network Log";
	gch.dwData = 0x80000000;
	gch.pszText = text;
	_snprintf(msg,sizeof(msg)-1,"%s/InsertGuiOut",module);
	RETURN_LONG(CallService(msg,(WPARAM)module,(LPARAM)&gch));
}