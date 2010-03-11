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

/******************************************
 * popups                                 *
 ******************************************/

void MB_Popup(const char* title,const char* error,unsigned long flags,unsigned long timeout)
{
	MIRANDASYSTRAYNOTIFY mbp={0};
	mbp.cbSize = sizeof(MIRANDASYSTRAYNOTIFY);
	mbp.szProto = "MBot";
	mbp.szInfoTitle = (char*)title;
	mbp.szInfo = (char*)error;
	mbp.dwInfoFlags = flags;
	mbp.uTimeout = timeout;

	if(flags == NIIF_ERROR && (DBGetContactSettingByte(NULL,MBOT,"SWOnError",0))){
		ShowWindow(hDialog,SW_SHOW);
		SetWindowPos(hDialog,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
	}

	CallService(MS_CLIST_SYSTRAY_NOTIFY, (WPARAM)NULL,(LPARAM) &mbp);
}

void WINAPI MBotPUHelperThread(void* a)
{
	POPUPDATAEX* pde = (POPUPDATAEX*)a;

	if(pde)
	{
		try{
			PUAddPopUpEx(pde);
		}catch(...){
			pde->lchIcon = NULL;
		}
		my_memfree(pde);
	}
}

ZEND_FUNCTION(mb_PUMsg)
{
	char*	txt=NULL;
	long	tl=0,type=0;
	POPUPDATAEX *pd;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l",&txt,&tl,&type) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!(pd = (POPUPDATAEX*)my_malloc(sizeof(POPUPDATAEX)))){
		RETURN_FALSE;
	}

	memset(pd,0,sizeof(POPUPDATAEX));
	pd->iSeconds = 0;

	if(type == 2){
		pd->lchIcon = (HICON)hMBotIcon;
	}else{
		pd->lchIcon = (type == 2)?LoadIcon(NULL,IDI_ASTERISK):LoadIcon(NULL,IDI_INFORMATION);
	}
	
	strncpy(pd->lpzText,txt,sizeof(pd->lpzText));
	strcpy(pd->lpzContactName,"MBot");
	if(CallFunctionAsync(MBotPUHelperThread,pd)==FALSE){
		my_memfree(pd);
		RETURN_FALSE;
	}
	RETURN_TRUE;
}
ZEND_FUNCTION(mb_PUAdd)
{
	char *cname,*txt;
	long  cid=0,cl=0,tl=0,bgc=0,txc=0,hico=NULL;
	POPUPDATAEX *pd;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lss|lll",
		&cid,&cname,&cl,&txt,&tl,&bgc,&txc,&hico) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!(pd = (POPUPDATAEX*)my_malloc(sizeof(POPUPDATAEX)))){
		PHP_FALSE_AND_WARNS("Could not allocate memory!");
	}

	memset(pd,0,sizeof(POPUPDATAEX));
	pd->iSeconds = 0;

	pd->lchContact = (HANDLE)cid;
	pd->colorText = txc;
	pd->colorBack = bgc;
	pd->lchIcon = (hico)?((HICON)hico):((HICON)hMBotIcon);
	strncpy(pd->lpzContactName,cname,sizeof(pd->lpzContactName)-1);
	strncpy(pd->lpzText,txt,sizeof(pd->lpzText)-1);

	if(CallFunctionAsync(MBotPUHelperThread,pd)==FALSE){
		my_memfree(pd);
		RETURN_FALSE;
	}
	RETURN_TRUE;
}

ZEND_FUNCTION(mb_PUAddEx)
{
	long  cid=0,cl=0,tl=0,bgc=0,txc=0,delay=0,param=0,hico=0;
	char* cname=NULL,*txt=NULL;
	zval* cb = NULL;
	sMFSync* mfs = NULL;
	POPUPDATAEX *pd;

	mb_event* mbe = (mb_event*)(((sPHPENV*)SG(server_context))->c_param);

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lss|llllzl",
		&cid,&cname,&cl,&txt,&tl,&bgc,&txc,&hico,&delay,&cb,&param) == FAILURE || (cb && cb->type != IS_STRING)){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(cb && !zend_is_callable(cb,0,NULL)){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!(pd = (POPUPDATAEX*)my_malloc(sizeof(POPUPDATAEX)))){
		RETURN_FALSE;
	}

	memset(pd,0,sizeof(POPUPDATAEX));

	if(cb)
	{
		mfs = (sMFSync*)my_malloc(sizeof(sMFSync));
		if(!mfs)goto Error;
		memset(mfs,0,sizeof(sMFSync));

		if(!mbe->php){goto Error;}
		mfs->php = mbe->php;

		strncpy(mfs->pszFunction,cb->value.str.val,sizeof(mfs->pszFunction));
		mfs->pParam = (void*)param;

		pd->PluginWindowProc = (WNDPROC)help_popup_wndproc;
		pd->PluginData = mfs;
	}

	pd->iSeconds = delay;
	pd->lchContact = (HANDLE)cid;
	pd->colorText = txc;
	pd->colorBack = bgc;
	pd->lchIcon = (hico)?((HICON)hico):((HICON)hMBotIcon);
	strncpy(pd->lpzContactName,cname,sizeof(pd->lpzContactName)-1);
	strncpy(pd->lpzText,txt,sizeof(pd->lpzText)-1);

	if(CallFunctionAsync(MBotPUHelperThread,pd)==FALSE){
		goto Error;
	}
	RETURN_TRUE;
Error:
	my_memfree(pd);
	if(mfs)my_memfree(mfs);
	RETURN_FALSE;
}

ZEND_FUNCTION(mb_PUSystem)
{
	char* body;
	char* title;
	unsigned long  tl=0,bl=0,flags=0,timeout=5;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|ll",&title,&tl,&body,&bl,&flags,&timeout) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!bl || !tl){
		RETURN_FALSE;
	}

	MB_Popup(title,body,flags,timeout * 1000);
	RETURN_TRUE;
}