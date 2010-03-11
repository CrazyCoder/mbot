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
#ifndef _HELPERS_H_
#define _HELPERS_H_

#include "mbot.h"
#include "sync.h"
#include "cron.h"
#include "smanager.h"
#pragma once

enum MBE
{
	MBE_NOTHING=0,
	MBE_DBEI,
	MBE_HCONTACT,
	MBE_LPARAM,
	MBE_WPARAM,
	MBE_SSTRING,
	MBE_HDBEVENT,
	MBE_EVENTID,
	MBE_CUSTOM,
	MBE_DBVARIANT,
	MBE_LPCSTR,
	MBE_HTTPDENV,
	MBE_SRESULT,
	MBE_ACK,
	MBE_RECVFILE,
	MBE_DIALOGCB
};

enum MBT{
	MBT_NOTHING=0,
	MBT_SEND,
	MBT_PRERECV,
	MBT_RECV,
	MBT_EXECUTE,
	MBT_COMMAND,
	MBT_CONSOLE_CMD,
	MBT_SSCRIPT,
	MBT_STARTUP,
	MBT_CCSCRIPT,
	MBT_AUTHRECV,
	MBT_INIT,
	MBT_DBEVENT,
	MBT_CSCHANGED,
	MBT_NEWMYSTATUS,
	MBT_AUTOLOAD,
	MBT_SHUTDOWN,
	MBT_TIMER,
	MBT_CALLBACK,
	MBT_WEBPAGE,
	MBT_FILEIN,
	MBT_DIALOG,
	MBT_IRC_IN,
	MBT_IRC_OUT,
	MBT_IRC_RAW
};

enum MBC{
	MBC_CUSTOM=0,
	MBC_ACK=1,
	MBC_SENDMSG=2
};

enum PHPR
{
	PHPR_CONTINUE,
	PHPR_SEND,
	PHPR_DROP,
	PHPR_HIDE,
	PHPR_BREAK,
	PHPR_STORE,
	PHPR_END,
	PHPR_FAILED,
	PHPR_UNKNOWN
};

enum MBCT
{
	MBCT_TEXT=0,
	MBCT_RADIO=1,
	MBCT_CHECK=2,
	MBCT_COMBO=3,
	MBCT_LIST=4
};

struct mb_event
{
	MBT		event; //0
	WPARAM	wParam; //4
	LPARAM	lParam; //8
	void*	p1; //12
	void*	p2; //16
	void*	p3; //20
	void*	p4; //24
	MBE		t1;
	MBE		t2;
	MBE		t3;
	MBE		t4;
	PPHP	php;
	long	lFlags;
};

struct mb_ack{
	char*	proto;
	long	seq;
	HANDLE  hContact;
	char*	script;
	long	timeout;
	bool	result;
};

struct sMsgSync : public sSync
{
	HANDLE hContact;
	HANDLE hProcess;
	unsigned long timestamp;
	char* old_body;
	char* new_body;
};

struct sCBSync : public sSync
{
	PPHP		php;
	char		pszFunction[24];
};

struct sACKSync : public sCBSync
{
	HANDLE		hProcess;
	HANDLE		hContact;
	long		lType;
	long		lParam;
	char		pszProtocol[24];
public:
	sACKSync(){
		memset(this,0,sizeof(sACKSync));
	}
};

struct sMFSync : public sCBSync
{
	void*		fpFunction;
	void*		pParam;
	char		pszPrebuild[24];
	long		hMenu;
};

struct sFileInfo{
	short numFiles;
	short curFile;
	unsigned long bytesTotal;
	unsigned long bytesDone;
	unsigned long curSize;
	unsigned long curDone;
	unsigned long curTime;
};

struct sHESync : public sCBSync
{
	char		pszSvcName[48];
	long		hFunction;
	long		hHook;
	void*		pCode;
};

struct sCLESync : public sCBSync
{
	void*	pParam;
};

const char*		help_cache_php_file(const char* path);
const char*		help_get_funct_name(long event_id);
MBT				help_get_event_type(long event_id);
HANDLE			help_find_by_uin(const char* proto,const char* uin,int numeric);
LRESULT	WINAPI	help_popup_wndproc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

const char* help_static_time();
long	help_autoload(const char* path);
int		help_loadscript(const char* script,long recache=0);
int		help_fileexists(const char* path);
int		help_fileexists(const char* path,long* fs);
int		help_filemtime(const char* path,unsigned long* out);
int		help_direxists(const char* path);
int		help_iswin2k();
int		help_enable_script(PPHP p,bool enable);

void	help_callmenu(sMFSync* mfs,WPARAM wParam,LPARAM lParam);
long	help_callsvc(sHESync* hfs,WPARAM wParam,LPARAM lParam);

void*   help_makefunct(void* param,void* fcn);

char*	help_getfilename(long open,char* filter,const char* fname);
long	help_getprocaddr(const char* module,const char* name);
char*	help_getfilenamemultiple(char* filter,char* out,long outlen,long* offset);

unsigned long __fastcall fcn_log2(long event_id);

PHPR	help_parseoutput(const char* output);

#endif //_HELPERS_H_