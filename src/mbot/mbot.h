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
#ifndef _MBOT_H_
#define _MBOT_H_

#pragma once

#pragma warning(disable: 4996)
#pragma warning(disable: 4005)

#define WINVER	0x0500
#define _WIN32_WINNT 0x0500
#define _WIN32_IE 0x0501
#define _WIN32_WINDOWS 0x0500

#define MBOT "MBot"
#define MS_MBOT_TRIGGER MBOT "/Trigger"
#define MS_MBOT_CLISTEVENT MBOT "/CLEHandler"
#define MS_MBOT_SHOWCONSOLE MBOT "/ShowConsole"

#define UNHOOK(e) if(e){UnhookEvent(e);e=NULL;}

#ifndef _DEBUG
#define MBLOG(s)
#define MBLOGEX(s)
#else
#define MBLOG(s) fprintf(dbgout,s)
#define MBLOGEX(s) if(lErrorLog && dbgout)fprintf(dbgout,"[ERROR_LOG@%s]: %s\r\n",help_static_time(),s);fflush(dbgout)
#endif

#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>
#include <Richedit.h>
#include <stdio.h>
#include <math.h>
//#include <max_optimize.h>
#pragma optimize("gsy12",on)

void EnterCriticalSectionR(LPCRITICAL_SECTION cs,const char* file,long line);
void LeaveCriticalSectionR(LPCRITICAL_SECTION cs,const char* file,long line);

#define EnterCriticalSectionX(cs) EnterCriticalSection(cs)
#define LeaveCriticalSectionX(cs) LeaveCriticalSection(cs)

 //LeaveCriticalSectionR(cs,__FUNCTION__ "@" __FILE__,__LINE__);
 //EnterCriticalSectionR(cs,__FUNCTION__ "@" __FILE__,__LINE__);

#include <string>
#include <map>

using namespace std;

#include "resource.h"
#include "helpers.h"
#include "sync.h"
#include "cron.h"
#include "config.h"
/******************************************
 * To download libphp visit my website    *
 ******************************************/
#include <libphp.h>
#include <cUtils.h>
#include <cXmldoc.h>

extern "C" {
	#include <newpluginapi.h>
	#include <m_clist.h>
	#include <m_clui.h>
	#include <m_system.h>
	#include <m_popup.h>
	#include <m_skin.h>
	#include <m_protosvc.h>
	#include <m_protocols.h>
	#include <m_protomod.h>
	#include <m_database.h>
	#include <m_langpack.h>
	#include <m_system.h>
	#include <m_options.h>
	#include <m_utils.h>
	#include <m_contacts.h>
	#include <m_chat.h>
	#include <m_ircscript.h>

	#include <main/php.h>
	#include <main/SAPI.h>
	#include <main/php_ini.h>
	#include <tsrm/tsrm.h>
}

#include "m_script.h"

#define MBOT_FLAG_BODY_CHANGED 0x0010
#define MBOT_FLAG_HANDLERS_CHANGED 0x0020
#define MBOT_FLAG_CACHE 0x01
#define MBOT_FLAG_SHARE 0x02
#define MBOT_FLAG_INACTIVE 0x04
#define MBOT_FLAG_DELETE 0x08
#define MBOT_FLAG_ASYNC 0x10
#define MBOT_FLAG_WORKING 0x20
#define MBOT_FLAG_SHARE_NAME 0x40
#define MBOT_FLAG_STORED 0x80
#define MBOT_FLAG_NOOUTPUT 0x0100
#define MBOT_FLAG_CONTACTM 0x0200
#define MBOT_FLAG_WANTPREBUILD 0x0400
#define MBOT_FLAG_SHALLDIE 0x0800

typedef int (*PHPENV_CB)(long code,void* param1,void* param2,void* cparam);
struct sPHPENV
{
	void*			c_param;
	PHPENV_CB		fp_callback;
	long			m_flags;
	////////////////////////////
	cutFile*		r_out;
	const char*		r_value;
	////////////////////////////
	const char*		script_path;
	const char*		query_string;
	const char*		content_type;
	long			content_length;
	////////////////////////////
	unsigned char*  post_data;
	long			post_len;
	long			post_read;
public:
	sPHPENV(){
		memset(this,0,sizeof(sPHPENV));
	}
};


void my_memfree(void* ptr);
char* my_strdup(const char* str);
void* my_malloc(long amount);
/******************************************
 * events                                 *
 ******************************************/
int event_contact_new(WPARAM wParam, LPARAM lParam);
int event_contact_del(WPARAM wParam, LPARAM lParam);
int event_contact_changed(WPARAM wParam, LPARAM lParam);
int event_new_db_event(WPARAM wParam, LPARAM lParam);
int event_pre_db_event(WPARAM wParam, LPARAM lParam);
int event_msg_ack(WPARAM wParam, LPARAM lParam);
int event_modules_loaded(WPARAM wParam, LPARAM lParam);
int event_typing(WPARAM wParam, LPARAM lParam);
int event_new_mystatus(WPARAM wParam, LPARAM lParam);
int event_opt_initialise(WPARAM wParam, LPARAM lParam);
int event_icq_awayreq(WPARAM wParam,LPARAM lParam);
int event_contact_menu_prebuild(WPARAM wParam,LPARAM lParam);
int event_oktoexit(WPARAM wParam,LPARAM lParam);
/******************************************
 * IRC functions                          *
 ******************************************/
int MBIRCGuiIn(WPARAM wParam, LPARAM lParam);
int MBIRCGuiOut(WPARAM wParam, LPARAM lParam);
int MBIRCRawIn(WPARAM wParam, LPARAM lParam);
int MBIRCRawOut(WPARAM wParam, LPARAM lParam);
int MBIRCRegister(WPARAM wParam, LPARAM lParam);

/******************************************
 * svc functions                          *
 ******************************************/
int MBRecvMsg(WPARAM wParam, LPARAM lParam);
int MBRecvUrl(WPARAM wParam, LPARAM lParam);
int MBSendMsg(WPARAM wParam, LPARAM lParam);
int MBSendAwayMsg(WPARAM wParam, LPARAM lParam);
int MBRecvAuth(WPARAM wParam, LPARAM lParam);
int MBRecvFile(WPARAM wParam, LPARAM lParam);
int MBTyping(WPARAM wParam, LPARAM lParam);
int MBotShowConsole(WPARAM wParam, LPARAM lParam);
int MBCListEventHandle(WPARAM wParam, LPARAM lParam);
int	MBExternalTrigger(WPARAM wParam, LPARAM lParam);
/******************************************
 * generic handlers                       *
 ******************************************/
int MBGenericSettChanged(HANDLE hContact, DBCONTACTWRITESETTING* dbws, long ev_id);
int MBGenericDBEvent(HANDLE hDBEvent,long ev_id,const char* pszFcn, HANDLE hContact,const char* pszData,long timestamp,void* p3);
int MBGenericRecv(WPARAM wParam, LPARAM lParam,long ev_id,mb_event* ce = NULL);
int MBNoParamExecute(long ev_id);
int MBMultiParamExecute(long ev_id,mb_event* mbe,const char* params,...);
int MBMultiParam(PPHP php,const char* fcn,mb_event* mbe,const char* params,...);
int MBBroadcastACK(HANDLE hContact);
/******************************************
 * other functions                        *
 ******************************************/
PHPR MBCommandExecute(const char* msg,HANDLE hContact,string* ss);
int  MBUnHookEvents();
int  MBHookEvents();
void my_stdout(const char* str,long length);
void my_stderr(const char* str,long length);

int RegisterContacts();
int GenericThreadProc(void* p);

extern char	g_root[MAX_PATH+1];
extern char g_profile[MAX_PATH+1];

extern CSyncList g_slist;

extern long	lh_flags;
extern DWORD g_mbot_version;
extern PCHAR g_mbot_version_s;
extern FILE* dbgout;

extern HWND	hDialog;
extern HWND	hConsole;
extern HWND	hCommandBox;
extern HWND	hToolbar;
extern HINSTANCE hInst;
#endif //_MBOT_H_