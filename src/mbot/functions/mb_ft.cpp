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

ZEND_FUNCTION(mb_FileInitSend)
{
	char *desc=NULL,*file=NULL;
	long  dl=0,fl=0,cid=0,param=0;
	zval* cb = NULL;
	sACKSync* ack = NULL;
	char *files[2]={0};

	mb_event* mbe = (mb_event*)((sPHPENV*)SG(server_context))->c_param;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lsszl",&cid,&desc,&dl,&file,&fl,&cb,&param) == FAILURE || !cid || !dl || !fl){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}else if(!mbe->php){
		PHPWSE("You can't use this function here!");
	}else if(!zend_is_callable(cb,0,NULL)){
		PHPWSE("$cb is expected to be a valid callback function!");
	}

	ack = (sACKSync*)my_malloc(sizeof(sACKSync));
	if(!ack){RETURN_FALSE}
	memset(ack,0,sizeof(sACKSync));

	ack->lType = ACKTYPE_FILE;
	ack->hContact = (HANDLE)cid;
	ack->php = mbe->php;
	ack->lParam = param;
	strncpy(ack->pszFunction,cb->value.str.val,sizeof(ack->pszFunction)-1);

	files[0] = file;
	ack->hProcess = (HANDLE)CallContactService((HANDLE)cid,PSS_FILE,(WPARAM)desc,(LPARAM)files);
	if(!ack->hProcess)goto Error;
	g_slist.Add(ack);

	RETURN_LONG((long)ack);
Error:
	free(ack);
	RETURN_FALSE;
}

ZEND_FUNCTION(mb_FileGetInfo)
{
	long fth;
	sACKSync* as;
	sFileInfo* fi;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll",&fth) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!fth){
		goto Error;
	}

	try{
		g_slist.Lock();
		as = (sACKSync*)g_slist.m_head;
		while(as)
		{
			if((long)as == fth)
			{
				fi = (sFileInfo*)as->pszProtocol;
				if(array_init(return_value)==FAILURE){
					as = NULL;
					break;
				}
				//array($num_files,$cur_file,$tot_bytes,$tot_sent,$cur_size,$cur_sent,$cur_time);
				add_index_long(return_value,0,fi->numFiles);
				add_index_long(return_value,1,fi->curFile);
				add_index_long(return_value,2,fi->bytesTotal);
				add_index_long(return_value,3,fi->bytesDone);
				add_index_long(return_value,4,fi->curSize);
				add_index_long(return_value,5,fi->curDone);
				add_index_long(return_value,6,fi->curTime);
				break;
			}
			as = (sACKSync*)as->next;
		}
	}catch(...){
		as = NULL;
	}

	g_slist.Unlock();

	if(as){
		return;
	}
Error:
	RETURN_FALSE;
}

ZEND_FUNCTION(mb_FileCancel)
{
	long fth,hp;
	sACKSync* as;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&fth) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!fth){
		goto Error;
	}

	try{
		g_slist.Lock();
		as = (sACKSync*)g_slist.m_head;
		while(as){
			if((long)as == fth){
				hp = (long)as->hProcess;
				break;
			}
			as = (sACKSync*)as->next;
		}
	}catch(...){
		as = NULL;
	}

	g_slist.Unlock();
	

	if(as){
		CallContactService((HANDLE)as->hContact,PSS_FILECANCEL,(WPARAM)hp,0);
		RETURN_TRUE;
	}
Error:
	RETURN_FALSE;
}

ZEND_FUNCTION(mb_FileDeny)
{
	char *desc=NULL;
	long fid=0,dl=0,cid=0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lls",&cid,&fid,&desc,&dl) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	if(!fid || !cid){
		RETURN_FALSE;
	}
	RETURN_LONG(CallContactService((HANDLE)cid,PSS_FILEDENY,(WPARAM)fid,(LPARAM)desc));
}

ZEND_FUNCTION(mb_FileAccept)
{
	char *dst=NULL;
	long  dl;
	long  cid;
	long  fth;
	long  param=0;
	char  resume=0;
	zval* cb;
	sACKSync* ack = NULL;
	PROTOFILERESUME pfr;

	mb_event* mbe = (mb_event*)((sPHPENV*)SG(server_context))->c_param;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llsz|cl",&fth,&cid,&dst,&dl,&cb,&resume,&param) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}else if(!mbe->php){
		PHPWSE("You can't use this function here!");
	}else if(!zend_is_callable(cb,0,NULL)){
		PHPWSE("$cb is expected to be a valid callback function!");
	}

	if(!fth){RETURN_FALSE}

	ack = (sACKSync*)my_malloc(sizeof(sACKSync));
	if(!ack){RETURN_FALSE}
	memset(ack,0,sizeof(sACKSync));

	ack->lType = ACKTYPE_FILE;
	ack->hContact = (HANDLE)cid;
	ack->php = mbe->php;
	ack->lParam = param;
	strncpy(ack->pszFunction,cb->value.str.val,sizeof(ack->pszFunction)-1);

	try{
		if(resume == 0){
			ack->hProcess = (HANDLE)CallContactService((HANDLE)cid,PSS_FILEALLOW,(WPARAM)fth,(LPARAM)dst);
		}else{
			pfr.action = resume;
			pfr.szFilename = dst;
			ack->hProcess = (HANDLE)CallContactService((HANDLE)cid,PS_FILERESUME,(WPARAM)fth,(LPARAM)&pfr);
		}
	}catch(...){
		ack->hProcess = NULL;
	}

	if(ack->hProcess){
		g_slist.Add(ack);
		RETURN_LONG((long)ack);
	}

	if(ack)my_memfree(ack);
	RETURN_FALSE;
}

ZEND_FUNCTION(mb_FileStore)
{
	CCSDATA* css;
	PROTORECVEVENT* prr;
	long result = 0;
	char* proto=NULL;
	char* szDesc=NULL;
	char* szFile=NULL;
	DBEVENTINFO dbei={0};

	mb_event* mbe = (mb_event*)((sPHPENV*)SG(server_context))->c_param;
	css = (CCSDATA*)mbe->lParam;

	if(mbe->event != MBT_FILEIN || !css || !css->lParam){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}else if(mbe->lFlags & MBOT_FLAG_STORED){
		RETURN_FALSE;
	}

	
	prr = (PROTORECVEVENT*)css->lParam;
	proto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)css->hContact,0);

	szFile = prr->szMessage + sizeof(DWORD);
	szDesc = szFile + strlen(szFile) + 1;

	dbei.cbSize = sizeof(dbei);
	dbei.cbBlob = sizeof(DWORD) + strlen(szFile) + strlen(szDesc) + 2;
	dbei.pBlob = (PBYTE)prr->szMessage;
	dbei.eventType = (unsigned short)EVENTTYPE_FILE;
	dbei.timestamp = time(0);
	dbei.szModule = proto;
	dbei.flags = DBEF_READ;
	
	result = CallService(MS_DB_EVENT_ADD,(WPARAM)css->hContact,(LPARAM)&dbei);
	if(result){
		mbe->lFlags |= MBOT_FLAG_STORED;
	}

	RETURN_LONG(*((long*)dbei.pBlob));
}