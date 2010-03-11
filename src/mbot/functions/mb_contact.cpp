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

ZEND_FUNCTION(mb_CIsOnList)
{
	long cid=0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&cid) == FAILURE || cid==0){
		PHP_FALSE_AND_ERROR;
	}
	RETURN_LONG(DBGetContactSettingByte((HANDLE)cid,"CList","NotOnList",0)==FALSE);
}

ZEND_FUNCTION(mb_CSendTypingInfo)
{
	long cid=0;
	char on=0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lb",&cid,&on) == FAILURE || cid==0){
		PHP_FALSE_AND_ERROR;
	}

	RETURN_LONG(CallContactService((HANDLE)cid,PSS_USERISTYPING,(WPARAM)cid,on)==0);
}

ZEND_FUNCTION(mb_CGetStatus)
{
	long cid = 0;
	char* proto = NULL;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&cid) == FAILURE || !cid){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	proto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)cid,0);
	if(!proto){
		RETURN_FALSE;
	}

	cid = DBGetContactSettingWord((HANDLE)cid,proto,"Status",0);
	RETURN_LONG(cid);
}

ZEND_FUNCTION(mb_CGetProto)
{
	long cid = 0;
	char* proto = NULL;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&cid) == FAILURE || !cid){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	proto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)cid,0);
	if(proto){
		RETURN_STRING(proto,1);
	}else{
		RETURN_FALSE;
	}
}

ZEND_FUNCTION(mb_CDelete)
{
	long cid=0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&cid) == FAILURE || !cid)
	{
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	RETURN_LONG(CallService(MS_DB_CONTACT_DELETE,(WPARAM)cid,0)==0);
}

ZEND_FUNCTION(mb_CGetDisplayName)
{
	long cid=0;
	char* tmp=NULL;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&cid) == FAILURE || !cid)
	{
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	tmp = (char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)cid,0);
	if(tmp){
		RETURN_STRING(tmp,1);
	}else{
		RETURN_FALSE;
	}
}

ZEND_FUNCTION(mb_CGetAwayMsg)
{
	long cid=0;
	zval *cb;
	char *proto;
	mb_event* mbe = (mb_event*)(((sPHPENV*)SG(server_context))->c_param);
	sACKSync* ack;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz",&cid,&cb)==FAILURE || cid==0 || cb->type != IS_STRING){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!zend_is_callable(cb,0,NULL)){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!(proto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,cid,0))){
		RETURN_FALSE;
	}

	ack = (sACKSync*)my_malloc(sizeof(sACKSync));
	if(!ack){RETURN_FALSE}
	memset(ack,0,sizeof(sACKSync));

	if(!(ack->php = mbe->php))goto Error;
	sman_incref(ack->php);

	ack->lType = ACKTYPE_AWAYMSG;
	ack->hContact = (HANDLE)cid;

	strncpy(ack->pszFunction,cb->value.str.val,sizeof(ack->pszFunction)-1);
	strncpy(ack->pszProtocol,proto,15);

	if(!g_slist.Add(ack)){goto Error;}

	ack->hProcess = (HANDLE)CallContactService((HANDLE)cid,PSS_GETAWAYMSG,0,0);
	if(!ack->hProcess){
		g_slist.Del(ack);
		goto Error;
	}
	RETURN_LONG(1);
Error:
	if(ack->php)sman_decref(ack->php);
	if(ack)my_memfree(ack);
	RETURN_FALSE;
}

int xx_enumcsettings(const char *szSetting,LPARAM lParam)
{
	cutMemf* mf = (cutMemf*)lParam;
	if(*szSetting){
		mf->write((void*)szSetting, strlen(szSetting)+1);
	}
	return 0;
}

ZEND_FUNCTION(mb_CSettingEnum)
{
	DBCONTACTENUMSETTINGS ecs = {0};
	cutMemf mf;
	long cid=0,ml=0;
	char* mod=NULL;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls",&cid,&mod,&ml) == FAILURE || !mod || !ml){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!mf.create(2048)){
		RETURN_FALSE;
	}

	ecs.pfnEnumProc = xx_enumcsettings;
	ecs.szModule = mod;
	ecs.lParam = (LPARAM)&mf;

	if(CallService(MS_DB_CONTACT_ENUMSETTINGS,cid,(LPARAM)&ecs)!=0){
		RETURN_FALSE;
	}
	mf.putc(0);
	if(array_init(return_value)==FAILURE){
		RETURN_FALSE;
	}

	mod = (char*)mf.getdata();
	ml = 0;
	while(*mod)
	{
		add_index_string(return_value,ml,mod,1);
		mod = mod + strlen(mod) + 1;
		ml++;
	}
	mf.close();
	return;
}

ZEND_FUNCTION(mb_CSetApparentMode)
{
	long cid=0,mode=0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll",&cid,&mode) == FAILURE || !cid){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	RETURN_LONG(CallContactService((HANDLE)cid,PSS_SETAPPARENTMODE,mode,0)==0);
}
ZEND_FUNCTION(mb_CAddNew)
{
	RETURN_LONG(CallService(MS_DB_CONTACT_ADD,0,0));
}
ZEND_FUNCTION(mb_CAddAuth)
{
	long aid=0;
	DBEVENTINFO dbei={0};
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&aid) == FAILURE || !aid){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	dbei.cbSize = sizeof(dbei);

	if(CallService(MS_DB_EVENT_GET,(WPARAM)aid,(LPARAM)&dbei)!=0){
		RETURN_FALSE;
	}
	RETURN_LONG(CallProtoService(dbei.szModule,PS_ADDTOLISTBYEVENT,0,(LPARAM)aid));
}
ZEND_FUNCTION(mb_CAddSearch)
{
	//MBT_SRESULT
	mb_event* mbe = (mb_event*)(((sPHPENV*)SG(server_context))->c_param);
	if(mbe->event != MBT_CALLBACK || mbe->t3 != MBE_SRESULT)
	{
		//PHPWARN();
		RETURN_FALSE;
	}
	else
	{
		PROTOSEARCHRESULT* sr = (PROTOSEARCHRESULT*)mbe->p3;
		ACKDATA* ack = (ACKDATA*)mbe->p2;

		RETURN_LONG(CallProtoService(ack->szModule,PS_ADDTOLIST,0,(LPARAM)sr));
	}
}
ZEND_FUNCTION(mb_CGetInfo)
{
	long cid=0,cf=0;
	CONTACTINFO ci={0};

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll",&cid,&cf) == FAILURE || !cid){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(cf < 1 || cf > 17){
		RETURN_FALSE;
	}

	ci.cbSize = sizeof(ci);
	ci.dwFlag = (BYTE)((0xFF)& cf);
	ci.hContact = (HANDLE)cid;
	ci.szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)cid,0);

	cid = CallService(MS_CONTACT_GETCONTACTINFO,0,(LPARAM)&ci);

	if(cid == 0)
	{
		if(ci.type == CNFT_ASCIIZ){
			RETURN_STRING(ci.pszVal,1);
		}else if(ci.type == CNFT_BYTE){
			RETURN_LONG(ci.bVal);
		}else if(ci.type == CNFT_WORD){
			RETURN_LONG(ci.wVal);
		}else{
			RETURN_LONG(ci.dVal);
		}

		if(cf >= 16)
		{
			MM_INTERFACE mmi={0};
			mmi.cbSize = sizeof(mmi);
			CallService(MS_SYSTEM_GET_MMI,0,(LPARAM)&mmi);
			mmi.mmi_free((void*)ci.pszVal);
		}
		return;
	}else{
		RETURN_FALSE;
	}
}
ZEND_FUNCTION(mb_CGetUIN)
{
	long cid = 0;
	char* proto = NULL;
	char* uin = NULL;
	DBVARIANT dbv;
	DBCONTACTGETSETTING cgs;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&cid) == FAILURE || cid==0){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	proto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)cid,0);
	if(!proto){
		RETURN_FALSE;
	}
	uin = (char*)CallProtoService(proto,PS_GETCAPS,PFLAG_UNIQUEIDSETTING,0);
	if(!uin){
		RETURN_FALSE;
	}

	cgs.szModule=proto;
	cgs.szSetting=uin;
	cgs.pValue=&dbv;
	if(CallService(MS_DB_CONTACT_GETSETTING,(WPARAM)cid,(LPARAM)&cgs)){
		RETURN_FALSE;
	}

	if(dbv.type==DBVT_BYTE){
		RETVAL_LONG(dbv.bVal);
	}else if(dbv.type==DBVT_WORD){
		RETVAL_LONG(dbv.wVal);
	}else if(dbv.type==DBVT_DWORD){
		RETVAL_LONG(dbv.dVal);
	}else if(dbv.type==DBVT_ASCIIZ){
		RETVAL_STRING(dbv.pszVal,1);
	}else if(dbv.type==DBVT_BLOB){
		RETVAL_STRINGL((char*)dbv.pbVal,dbv.cpbVal,1)
	}else{
		RETVAL_FALSE;
	}
	DBFreeVariant(&dbv);
	return;
}

ZEND_FUNCTION(mb_CSettingGet)
{
	char*	setting = NULL;
	char*	module = NULL;
	long	cid=0,sl=0,ml=0;
	DBVARIANT dbv;
	DBCONTACTGETSETTING cgs;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lss",&cid,&module,&ml,&setting,&sl) == FAILURE || !sl || !ml){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	cgs.szModule=module;
	cgs.szSetting=setting;
	cgs.pValue=&dbv;

	if(CallService(MS_DB_CONTACT_GETSETTING,(WPARAM)cid,(LPARAM)&cgs)){
		RETURN_FALSE;
	}

	if(dbv.type==DBVT_BYTE){
		RETVAL_LONG(dbv.bVal);
	}else if(dbv.type==DBVT_WORD){
		RETVAL_LONG(dbv.wVal);
	}else if(dbv.type==DBVT_DWORD){
		RETVAL_LONG(dbv.dVal);
	}else if(dbv.type==DBVT_ASCIIZ){
		RETVAL_STRING(dbv.pszVal,1);
	}else if(dbv.type==DBVT_BLOB){
		RETVAL_STRINGL((char*)dbv.pbVal,dbv.cpbVal,1)
	}else{
		RETVAL_FALSE;
	}
	DBFreeVariant(&dbv);
}
ZEND_FUNCTION(mb_CSettingDel)
{
	char*	setting = NULL;
	char*	module = NULL;
	long	cid=0,sl=0,ml=0;
	DBVARIANT dbv={0};
	DBCONTACTGETSETTING cgs;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lss",&cid,&module,&ml,&setting,&sl) == FAILURE || !sl || !ml){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}else{
		//MBT_CSCHANGED
		mb_event* mbe = (mb_event*)(((sPHPENV*)SG(server_context))->c_param);
		if(mbe->event == MBT_CSCHANGED && strcmp(setting,"Status")==0){
			RETURN_FALSE;
		}
	}

	cgs.szModule=module;
	cgs.szSetting=setting;
	cgs.pValue=&dbv;

	RETURN_LONG(DBDeleteContactSetting((HANDLE)cid,module,setting)==0);
}


ZEND_FUNCTION(mb_CSettingSet)
{
	char*	setting = NULL;
	char*	module = NULL;
	char*	value = NULL;
	char	buf[32]={0};
	long	cid=0,sl=0,ml=0,vl=0;
	DBVARIANT dbv ={0};
	DBCONTACTGETSETTING cgs;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lsss",&cid,&module,&ml,&setting,&sl,&value,&vl) == FAILURE ||
		!sl || !ml || !value){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}else{
		//MBT_CSCHANGED
		mb_event* mbe = (mb_event*)(((sPHPENV*)SG(server_context))->c_param);
		if(mbe->event == MBT_CSCHANGED && strcmp(setting,"Status")==0){
			RETURN_FALSE;
		}
	}


	cgs.szModule = module;
	cgs.szSetting = setting;
	cgs.pValue = &dbv;

	dbv.type = DBVT_ASCIIZ;
	dbv.pszVal = (char*)buf;
	dbv.cchVal = sizeof(buf);

	if(CallService(MS_DB_CONTACT_GETSETTINGSTATIC,(WPARAM)cid,(LPARAM)&cgs)){
		RETURN_FALSE;
	}

	if(dbv.type==DBVT_BYTE){
		RETVAL_LONG(DBWriteContactSettingByte((HANDLE)cid,module,setting,(BYTE)(strtoul(value,NULL,0)&0xff))==FALSE);
	}else if(dbv.type==DBVT_WORD){
		RETVAL_LONG(DBWriteContactSettingWord((HANDLE)cid,module,setting,(WORD)(strtoul(value,NULL,0)&0xffff))==FALSE);
	}else if(dbv.type==DBVT_DWORD){
		RETVAL_LONG(DBWriteContactSettingDword((HANDLE)cid,module,setting,(DWORD)(strtoul(value,NULL,0)))==FALSE);
	}else if(dbv.type==DBVT_ASCIIZ){
		RETVAL_LONG(DBWriteContactSettingString((HANDLE)cid,module,setting,value)==FALSE)
	}else if(dbv.type==DBVT_BLOB){
		DBCONTACTWRITESETTING cws;
		cws.szModule=module;
		cws.szSetting = mbot_replace_with_our_own(setting);
		cws.value.type=DBVT_BLOB;
		cws.value.pbVal=(BYTE*)value;
		cws.value.cpbVal = (WORD)(vl&0x00ffff);
		RETVAL_LONG(CallService(MS_DB_CONTACT_WRITESETTING,(WPARAM)cid,(LPARAM)&cws)==FALSE);
	}else{
		RETVAL_FALSE;
	}
}

ZEND_FUNCTION(mb_CSettingAdd)
{
	char*	setting = NULL;
	char*	module = NULL;
	char*	value = NULL;
	char	buf[32]={0};
	long	cid=0,sl=0,ml=0,vl=0,type=0;
	DBVARIANT dbv ={0};

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lslss",&cid,&module,&ml,&type,&setting,&sl,&value,&vl) == FAILURE ||
		!sl || !ml || !value){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}else{
		//MBT_CSCHANGED
		mb_event* mbe = (mb_event*)(((sPHPENV*)SG(server_context))->c_param);
		if(mbe->event == MBT_CSCHANGED && strcmp(setting,"Status")==0){
			RETURN_FALSE;
		}
	}

	if(type==DBVT_BYTE){
		RETVAL_LONG(DBWriteContactSettingByte((HANDLE)cid,module,setting,(BYTE)(strtoul(value,NULL,0)&0xff))==FALSE);
	}else if(type==DBVT_WORD){
		RETVAL_LONG(DBWriteContactSettingWord((HANDLE)cid,module,setting,(WORD)(strtoul(value,NULL,0)&0xffff))==FALSE);
	}else if(type==DBVT_DWORD){
		RETVAL_LONG(DBWriteContactSettingDword((HANDLE)cid,module,setting,(DWORD)(strtoul(value,NULL,0)))==FALSE);
	}else if(type==DBVT_ASCIIZ){
		RETVAL_LONG(DBWriteContactSettingString((HANDLE)cid,module,setting,value)==FALSE)
	}else if(type==DBVT_BLOB){
		DBCONTACTWRITESETTING cws;
		cws.szModule=module;
		cws.szSetting = mbot_replace_with_our_own(setting);
		cws.value.type=DBVT_BLOB;
		cws.value.pbVal=(BYTE*)value;
		cws.value.cpbVal = (WORD)(vl&0x00ffff);
		RETVAL_LONG(CallService(MS_DB_CONTACT_WRITESETTING,(WPARAM)cid,(LPARAM)&cws)==FALSE);
	}else{
		RETVAL_FALSE;
	}
}

ZEND_FUNCTION(mb_CFindFirst)
{
	long cid = CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	RETURN_LONG(cid);
}
ZEND_FUNCTION(mb_CFindNext)
{
	long cid = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&cid) == FAILURE || cid==NULL){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	cid = CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)cid,0);
	RETURN_LONG(cid);
}
ZEND_FUNCTION(mb_CFindByUIN)
{
	char*	proto=NULL;
	long	pl=0;
	zval*	uin;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",&proto,&pl,&uin) == FAILURE || !pl){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(uin->type == IS_STRING){
		RETURN_LONG((long)help_find_by_uin(proto,uin->value.str.val,0));
	}else if(uin->type == IS_LONG){
		RETURN_LONG((long)help_find_by_uin(proto,(const char*)uin->value.lval,1));
	}else{
		RETURN_FALSE;
	}
}