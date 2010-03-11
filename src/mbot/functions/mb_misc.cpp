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
#include "../window.h"

int CtrlACPI(HANDLE hATKACPI,int code, int hasArg, int arg)
{
	unsigned long bytes = 0;
	long inbuf[5];
	struct cmbuf {
		short cmds[2];
		long cm2;
	} cbuf;
	long outbuf[192];
	int ret;
	
	cbuf.cmds[0] = 0;
	cbuf.cmds[1] = 4;
	cbuf.cm2 = arg;
	inbuf[0] = 2;
	inbuf[1] = code;
	inbuf[2] = hasArg;
	inbuf[3] = 8 * hasArg;
	inbuf[4] = (long)&cbuf;

	ret = DeviceIoControl(hATKACPI, 0x222404, inbuf, sizeof(inbuf),
		outbuf, sizeof(outbuf), &bytes, NULL);
	return ret;
}

ZEND_FUNCTION(mb_AsusExt)
{
	int code,arg,harg;
	HANDLE hATKACPI;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lll",&code,&harg,&arg) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	hATKACPI = CreateFile("\\\\.\\ATKACPI",GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,0, NULL);

	if(!hATKACPI){
		PHPWSE("This computer does not support this extension!");
	}

	code = CtrlACPI(hATKACPI,code,harg,arg);
	CloseHandle(hATKACPI);
	RETURN_LONG(code);
}

ZEND_FUNCTION(mb_ConsoleShow)
{
	extern long lConTopMost;
	zend_bool show = 1;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b",&show) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(hDialog){
		ShowWindow(hDialog,(show)?(SW_SHOW):(SW_HIDE));
		if(show){
			SetWindowPos(hDialog,lConTopMost?HWND_TOPMOST:0,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
		}
	}
}

ZEND_FUNCTION(mb_ConsoleClear)
{
	if(hConsole){
		MBotConsoleClear();
	}
}

void __stdcall mbb_echo(void* txt)
{
	MBotConsoleAppendText(((char*)txt)+1,*((char*)txt));
	my_memfree((void*)txt);
}

ZEND_FUNCTION(mb_Echo)
{
	char*	txt=NULL;
	char*	tmp=NULL;
	long	tl=0;
	zend_bool rtf = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|b",&txt,&tl,&rtf) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	tmp = (char*)my_malloc(tl + 2);
	if(tmp){
		memcpy(tmp+1,txt,tl+1);
		*tmp = rtf;
		if(CallFunctionAsync(mbb_echo,tmp)==FALSE){
			my_memfree(tmp);
			RETURN_FALSE;
		}
		RETURN_TRUE;
	}else{
		RETURN_FALSE;
	}
}

ZEND_FUNCTION(mb_MsgBox)
{
	char*	body=NULL;
	char*	caption=NULL;
	long	bl=0,cl=0,type=0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|sl",&body,&bl,&caption,&cl,&type) == FAILURE || !bl){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	RETURN_LONG(MessageBox(NULL,body,(cl)?(caption):(NULL),type | MB_TOPMOST));
}

ZEND_FUNCTION(mb_CListEventAdd)
{
	long	cid = 0;
	long	ico = 0;
	zval	*cb = NULL;
	char	*info = NULL;
	long	il = 0;
	long	param = 0;
	CLISTEVENT cle = {0};
	sCLESync* mfs;

	mb_event* mbe = (mb_event*)(((sPHPENV*)SG(server_context))->c_param);

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lsz|ll",&cid,&info,&il,&cb,&param,&ico)==FAILURE ||  cb->type != IS_STRING){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!zend_is_callable(cb,0,NULL)){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	mfs = (sCLESync*)my_malloc(sizeof(sCLESync));
	if(!mfs){
		RETURN_FALSE;
	}
	memset(mfs,0,sizeof(sCLESync));

	if(!mbe->php){goto Error;}
	mfs->php = mbe->php;

	mfs->pParam = (void*)param;
	strncpy(mfs->pszFunction,cb->value.str.val,sizeof(mfs->pszFunction));

	cle.cbSize = sizeof(cle);
	cle.hIcon = (ico)?((HICON)ico):hMBotIcon; 
	cle.pszTooltip = info;
	cle.hContact = (HANDLE)cid;
	cle.pszService = MS_MBOT_CLISTEVENT;
	cle.hDbEvent = (HANDLE)mfs;
	cle.lParam = (LPARAM)mfs;

	CallService(MS_CLIST_ADDEVENT,0,(LPARAM)&cle);
	RETURN_TRUE;
Error:
	if(mfs){
		my_memfree(mfs);
	}
	RETURN_FALSE;
}