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
#include "mbot.h"
#include "m_script.h"
#include "functions.h"
#include "helpers.h"
#include "window.h"
#include "config.h"
#include "cron.h"

#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"../../../php-new/php-5.2.13/Release_TS/php5ts")

/******************************************
 * Imports/Exports                        *
 ******************************************/
PLUGINLINK*		pluginLink;
MBOT_FUNCTIONS	mbot_functions;
HINSTANCE		hInst = NULL;
/***************
 *  HANDLERSs  *
 ***************/
CSyncList	g_slist;	//ack    callbacks
CSyncList	g_mlist;	//menu   callbacks
CSyncList	g_svlist;	//service list;
CSyncList	g_msglist;	//msg body list;
long		lh_count=0;	//num of handlers
long		lh_flags=0; //reg handlers flags
long		lm_flags=0; //reg menus flags
long		lMsgSeq=1;	//msg sequence id
long		lFinished=0; //execution finished
MM_INTERFACE mmi = {0};
/***********
 *  PATHs  *
 ***********/
char	g_root[MAX_PATH+1]={0};
char	g_profile[MAX_PATH+1]={0};
/***********
 *  HWNDs  *
 ***********/
HWND	hDialog = NULL;
HWND	hConsole = NULL;
HWND	hCommandBox = NULL;
HWND	hToolbar = NULL;
CRITICAL_SECTION csConsole;
/***********
 *  hooks  *
 ***********/
HANDLE	hHookNE = NULL;
HANDLE	hHookNU = NULL;
HANDLE	hHookDU = NULL;
HANDLE	hHookTY = NULL;
HANDLE	hHookACK = NULL;
HANDLE	hHookSC = NULL;
HANDLE	hHookNMS = NULL;
HANDLE	hHookOPT = NULL;
HANDLE	hHookSHUT = NULL;
HANDLE	hHookICQ = NULL;
HANDLE	hHookPBCM = NULL;
HANDLE	hHookOKToExit = NULL;
HANDLE	hHookPreDB = NULL;
HANDLE	hHookNewDB = NULL;
/***********
 *  svces  *
 ***********/
HANDLE	hFunctPSR_MESSAGE = NULL;
HANDLE	hFunctPSR_URL = NULL;
HANDLE	hFunctPSR_AUTH = NULL;
HANDLE	hFunctPSS_AWAYMSG = NULL;
HANDLE	hFunctPSR_FILE = NULL;
HANDLE	hFunctPSS_MESSAGE = NULL;
HANDLE	hFunctGET_FUNCTIONS = NULL;
HANDLE	hFunctEXT_TRIGGER = NULL;
HANDLE	hFunctCLIST_EVENT = NULL;

HANDLE  hFunctIRCRawIn = NULL;
HANDLE  hFunctIRCRawOut = NULL;
HANDLE  hFunctIRCGuiIn = NULL;
HANDLE  hFunctIRCGuiOut = NULL;
/***********
 *  debug  *
 ***********/
FILE* dbgout = NULL;
/************
 *   MISC   *
 ************/
HMODULE	hRichModule = NULL;
HICON	hMBotIcon = NULL;
DWORD	g_mbot_version = PLUGIN_MAKE_VERSION(0,0,3,6);
PCHAR	g_mbot_version_s = "0.0.3.6";
char* pname = "MSP - Miranda Scripting Plugin";
char* pdesc = 	"Miranda Scripting Plugin (MSP), known as mBot, is a multi-purpose plugin providing\
 PHP scripting in Miranda-IM. It lets you write your own very customized scripts as well\
 as use the ones which have been written by others; MSP has many built in Miranda specific\
 functions like sending/receiving messages, file transferring, contacts management, etc;\
 of which all can be used by you. In addition to that there is a task scheduler (with cron\
 syntax) and a simple web server embedded in the application; All that makes MSP a very\
 powerful tool with many applications; MBot is basing on my libphp library which lets you\
 include PHP scripting in your applications with minimal effort! This program uses PHP,\
 freely available from http://www.php.net";

// {22E20F09-C246-4d4a-BF7B-7634F1E3999C}
#define MIID_MBOT { 0x22e20f09, 0xc246, 0x4d4a, { 0xbf, 0x7b, 0x76, 0x34, 0xf1, 0xe3, 0x99, 0x9c } }

PLUGININFOEX pluginInfo = {
  sizeof(PLUGININFOEX),
  pname,
  g_mbot_version,
  pdesc,
  "Piotr Pawluczuk & Serge Baranov",
  "piotrek@piopawlu.net",
  "© 2004-2006 Piotr Pawluczuk | © 2007-2010 Serge Baranov",
  "http://www.pawluczuk.info",
  UNICODE_AWARE,
  0,	//{22E20F09-C246-4d4a-BF7B-7634F1E3999C}
  MIID_MBOT
};

//definitions
#ifndef _NOHTTPD_
	long httpd_startup();
	long httpd_shutdown();
#endif

int WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	if(fdwReason == DLL_PROCESS_ATTACH)
	{
		hInst = hinstDLL;
	}
	else if(fdwReason == DLL_THREAD_DETACH && LPHP_Initialized())
	{
		try{
			ts_free_thread();
		}catch(...){
			return TRUE;
		}
	}
	return TRUE;
}

__declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
  if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 7, 0, 0))
    return NULL;

  return &pluginInfo;
}

static const MUUID interfaces[] = {MIID_MBOT, MIID_LAST};

__declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
  return interfaces;
}
/******************************************
 * LPHP must have functions               *
 ******************************************/
void my_memfree(void* ptr){
	try{
		if(ptr){
			free(ptr);
		}else{
			return;
		}
	}catch(...){
		return;
	}
}
void* my_malloc(long amount){
	return malloc(amount);
}
char* my_strdup(const char* str)
{
	char* tmp;
	unsigned long lt = strlen(str) + 1;
	tmp = (char*)malloc(lt);
	if(tmp){
		memcpy(tmp,str,lt);
	}
	return tmp;
}
void my_freeresult(MBOT_RESULT* mbr){
	if(mbr)
	{
		my_memfree(mbr->pszOutput);
		my_memfree(mbr->pszResult);
	}
}

void my_stdout(const char* str,long length)
{
	extern long	lConToFile;
	if(hConsole){
		MBotConsoleAppendText(str,0);
	}

	if(lConToFile){my_stderr(str,-length-1);}
}

void my_stderr(const char* str,long length)
{
	extern long lDebugOut;

	if(lDebugOut && hConsole && length>=0){
		MBotConsoleAppendText(str,0);
	}

	if(dbgout){
		if(length < 0){
			length = (-length) - 1;
		}
		fprintf(dbgout,"%s [",help_static_time());
		fwrite(str,1,length,dbgout);
		fputc(']',dbgout);
		fputc('\n',dbgout);
		fflush(dbgout);
	}
}

/////////////////////////////////////
void WINAPI MBBroadcastACKT(void* p)
{
	if(p)
	{
		mb_ack* ack = (mb_ack*)p;

		SleepEx(50,FALSE);
		ack->proto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)ack->hContact,0);

		ProtoBroadcastAck(
			ack->proto,
			ack->hContact,
			ACKTYPE_MESSAGE, (ack->result?ACKRESULT_SUCCESS:ACKRESULT_FAILED),
			(HANDLE)ack->seq, 0);

		my_memfree((void*)ack);
	}
}

int MBBroadcastACK(HANDLE hContact, bool result)
{
	mb_ack* ack = (mb_ack*)my_malloc(sizeof(mb_ack));
	
	if(!lMsgSeq){
		lMsgSeq = 1;
	}

	if(ack){
		ack->seq = (lMsgSeq & 0x0FffFFff);
		ack->hContact = hContact;
		ack->result = result;
		CallFunctionAsync(MBBroadcastACKT,ack);
		return lMsgSeq++;
	}else{
		return NULL;
	}
}

int	MB_ExecuteScript(const char* script,MBOT_RESULT* mbr,void* cpv,MBOT_TYPE cpt)
{
	mb_event mbe = {MBT_EXECUTE,0,0,0};
	char* output = NULL;
	char* tmp = NULL;

	mbe.p4 = cpv;
	mbe.t4 = (MBE)cpt;

	try{
		if(!script || LPHP_ExecuteDirect(script,(mbr)?((const char**)&output):(NULL),&mbe) == FALSE){
			MB_Popup(PHP_ERROR_TITLE,PHP_ERROR_EXECUTION);
			return 1;
		}
	}catch(...){
		return -1;
	}

	if(output){
		mbr->pszOutput = strdup(output);
		mbr->pszResult = strdup((char*)(output + strlen(output) + 1));
		LPHP_Free(output);
		output = NULL;
	}
	return 0;
}
int	MB_ExecuteFile(const char* script,const char* function,MBOT_RESULT* mbr,void* cpv,MBOT_TYPE cpt,const char* ptypes,...)
{
	mb_event mbe = {MBT_EXECUTE,0,0,0};
	const char* output = NULL;
	const char* tmp = NULL;
	int	result = 0;

	if(!script || (mbr && mbr->cbSize!=sizeof(MBOT_RESULT))){
		return 1;
	}

	mbe.p4 = cpv;
	mbe.t4 = (MBE)cpt;

	try
	{
		if(function){
			va_list args = NULL;
			va_start(args,ptypes);
			result = LPHP_ExecuteFileVA(script,function,&output,(void*)&mbe,(function)?(ptypes):(NULL),args);
		}else{
			result = LPHP_ExecuteFileVA(script,function,&output,(void*)&mbe,NULL,NULL);
		}
	}catch(...){
		MB_Popup(PHP_ERROR_TITLE,PHP_ERROR_EXECUTION);
		return -1;
	}

	if(output)
	{
		mbr->pszOutput = strdup(output);
		mbr->pszResult = strdup((char*)(output + strlen(output) + 1));
		LPHP_Free((void*)output);
		output = NULL;
	}
	return 0;
}
int MB_GetFunctions(WPARAM wParam, LPARAM lParam)
{
	return (int)&mbot_functions;
}

int	MBExternalTrigger(WPARAM wParam, LPARAM lParam)
{
	MBMultiParamExecute(MB_EVENT_EXTERNAL,NULL,"sl",(wParam!=0)?((void*)wParam):((void*)""),(void*)lParam);
	return 0;
}

int MBCListEventHandle(WPARAM wParam, LPARAM lParam)
{
	CLISTEVENT* cle = (CLISTEVENT*)lParam;
	if(cle && cle->lParam && cle->lParam == (LPARAM)cle->hDbEvent)
	{
		sCLESync* ev = (sCLESync*)cle->lParam;
		mb_event mbe = {MBT_CALLBACK,0,0};
		mbe.php = ev->php;

		sman_inc(ev->php);
		if(ev->php->szBuffered){
			LPHP_ExecuteScript(ev->php->szBuffered,ev->pszFunction,NULL,&mbe,"ll",cle->hContact,ev->pParam);
		}else{
			LPHP_ExecuteFile(ev->php->szFilePath,ev->pszFunction,NULL,&mbe,"ll",cle->hContact,ev->pParam);
		}
		sman_dec(ev->php);
		my_memfree(ev);
	}
	return 0;
}

/******************************************
 * Load/Unload                            *
 ******************************************/
int __declspec(dllexport) Load(PLUGINLINK *link)
{
	PROTOCOLDESCRIPTOR	pd = {sizeof(PROTOCOLDESCRIPTOR),0};
	CLISTMENUITEM		mi = {0};
	char	path[MAX_PATH + 2] = {0};
	char*	tmp;

	pd.szName = MBOT;
	pd.type = /*PROTOTYPE_FILTER*/ PROTOTYPE_IGNORE;
	pluginLink = link;
	
	if(!DBGetContactSettingByte(NULL,MBOT,"Enable",1)){
		hHookOPT = HookEvent(ME_OPT_INITIALISE,event_opt_initialise);
		return 0;
	}

  if(AllocConsole())
  {
    while(true)
    {
      Sleep(100);
      HWND hConWin = GetConsoleWindow();
      if(hConWin)
      {
        ShowWindow(hConWin, SW_HIDE);
        break;
      }
    }
  }

	///////////////////////////////////////////////
	mbot_functions.cbSize = sizeof(mbot_functions);
	//memory
	mbot_functions.fp_free = my_memfree;
	mbot_functions.fp_malloc = my_malloc;
	mbot_functions.fp_strdup = my_strdup;
	mbot_functions.fp_freeresult = my_freeresult;
	//variables
	mbot_functions.fpGetVar = LPHP_GetVar;
	mbot_functions.fpSetVar = LPHP_SetVar;
	mbot_functions.fpDelVar = LPHP_DelVar;
	mbot_functions.fpNewVar = LPHP_NewVar;
	//execution
	mbot_functions.fpExecuteFile = MB_ExecuteFile;
	mbot_functions.fpExecuteScript = MB_ExecuteScript;
	//register/unregister
	mbot_functions.fpRegister = (MBOT_RegisterFunction)LPHP_RegisterFunction;
	mbot_functions.fpUnregister = LPHP_UnregisterFunction;

	//profile_name
	if(CallService(MS_DB_GETPROFILENAME,(WPARAM)(MAX_PATH-1),(LPARAM)g_profile)){
		MessageBox(NULL,"Could not retrive profile name!",NULL,MB_ICONERROR);
		return 1;
	}

	//include_path
	if(!GetModuleFileName(NULL,path,MAX_PATH)){
		MessageBox(NULL,"Could not retrive application path!",NULL,MB_ICONERROR);
		return 1;
	}

	for(tmp=path;*tmp;tmp++){
		if(*tmp == '\\')*tmp = '/';
	}
	if(!(tmp = strrchr(path,'/'))){
		MessageBox(NULL,"Could not found application path!",NULL,MB_ICONERROR);
		return 1;
	}
	*tmp = 0;
	strcpy(g_root,path);
	strlwr(g_root);

	//create the window
	if(!(hRichModule = LoadLibrary("riched20.dll"))){
		MessageBox(NULL,"Could not init richedit control!",NULL,MB_ICONERROR);
		return 1;
	}

	if(!LPHP_PreInit(600,256,(LPHP_MALLOC)my_malloc,(LPHP_FREE)my_memfree)){
		MessageBox(0,"Could generate pre-initialize libphp!","MBot",MB_ICONERROR);
		return 1;
	}
	

	if(php_generate_ini()){
		MessageBox(0,"Could generate php.ini!","MBot",MB_ICONERROR);
		goto Error;
	}

	_snprintf(path,MAX_PATH-4,"%s/mbot/%s",g_root,g_profile);
	tmp = strrchr(path,'.');
	if(tmp){
		strcpy(tmp,"_dbg.txt");
	}
	dbgout = fopen(path,"a+t");

	if(LPHP_Init((LPHP_OUTPUT)my_stdout,(LPHP_OUTPUT)my_stderr,(void*)mv_module_entry)!=TRUE){
		MessageBox(0,"Could not initialize php environment!","MBot",MB_ICONERROR);
		goto Error;
	}

	//////////////////////////////////////
	hMBotIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON1));
	//////////////////////////////////////
	CallService(MS_PROTO_REGISTERMODULE,0,(LPARAM)&pd);
	//////////////////////////////////////
	sman_startup();
	_snprintf(path,MAX_PATH-4,"%s/mbot/scripts/autoload/",g_root);
	help_autoload(path);
	//////////////////////////////////////
	hFunctGET_FUNCTIONS = CreateServiceFunction(MS_MBOT_GET_FCN_TABLE,MB_GetFunctions);
	hFunctCLIST_EVENT = CreateServiceFunction(MS_MBOT_CLISTEVENT,MBCListEventHandle);
	hFunctEXT_TRIGGER = CreateServiceFunction(MS_MBOT_TRIGGER,MBExternalTrigger);
	hFunctPSS_MESSAGE = CreateServiceFunction(MBOT PSS_MESSAGE,MBSendMsg);
	//////////////////////////////////////
	hHookOPT = HookEvent(ME_OPT_INITIALISE,event_opt_initialise);
	hHookNU = HookEvent(ME_SYSTEM_MODULESLOADED,event_modules_loaded);
	hHookOKToExit = HookEvent(ME_SYSTEM_OKTOEXIT,event_oktoexit);
	hHookPreDB = HookEvent(ME_DB_EVENT_FILTER_ADD,event_pre_db_event);
	hHookNewDB = HookEvent(ME_DB_EVENT_ADDED,event_new_db_event);
	CreateServiceFunction(MS_MBOT_SHOWCONSOLE,MBotShowConsole);
	CreateServiceFunction(MS_MBOT_REGISTERIRC,MBIRCRegister);
	//////////////////////////////////////
	mi.cbSize=sizeof(mi);
	mi.position = - 0x7FFFFFFF;
	mi.flags = 0;
	mi.hIcon = hMBotIcon;
	mi.pszPopupName = "MBot";
	mi.pszName="Show MBot Console";
	mi.pszService= MS_MBOT_SHOWCONSOLE;
	CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);
	//////////////////////////////////////
	hDialog = CreateDialog(hInst,MAKEINTRESOURCE(IDD_MBOTCONSOLE),NULL,(DLGPROC)MBotDlgProc);
	//////////////////////////////////////
	if(!hDialog){
		MessageBox(0,"Could not create MBot Console!","MBot",MB_ICONERROR);
		goto Error;
	}

	if(DBGetContactSettingByte(NULL,MBOT,"SWOnStartup",0)){
		ShowWindow(hDialog,SW_SHOW);
	}
	return 0;
Error:
	LPHP_DeInit();
	
	if(hMBotIcon){
		DeleteObject((HGDIOBJ)hMBotIcon);
		hMBotIcon = NULL;
	}
	if(hConsole){
		DestroyWindow(hConsole);
		hConsole = NULL;
	}
	if(hRichModule){
		FreeModule(hRichModule);
		hRichModule = NULL;
	}
	return 1;
}

void sync_g_mlist(sMFSync* mi)
{
	my_memfree(mi->fpFunction);
	my_memfree(mi);
}

int __declspec(dllexport) Unload(void)
{
	MBLOGEX("Shutdown sequence started!");

	MBLOGEX("Shutting down crond...");
	cron_shutdown();

#ifndef _NOHTTPD_
	MBLOGEX("Shutting down HTTPD...");
	httpd_shutdown();
#endif

	MBLOGEX("Stopping libphp...");
	LPHP_DeInit();

	MBLOGEX("Destroying console window...");
	DestroyWindow(hDialog);
	hDialog = NULL;
	hConsole = NULL;

	MBLOGEX("Releasing g_mlist...");
	g_mlist.Release((SYNC_RELEASE)sync_g_mlist);
	MBLOGEX("Releasing g_slist...");
	g_slist.Release((SYNC_RELEASE)my_memfree);


	MBLOGEX("Releasing crond tasks...");
	cron_release();
	MBLOGEX("Releasing config tree...");
	cSettings.Free();
	if(hRichModule){
		MBLOGEX("Releasing riched20.dll...");
		FreeModule(hRichModule);
	}
	MBLOGEX("Destroying global functions...");
	DestroyServiceFunction(MBOT PSS_MESSAGE);
	DestroyServiceFunction(MS_MBOT_GET_FCN_TABLE);
	MBLOGEX("Shutting down script manager...");
	sman_shutdown();
	MBLOGEX("Releasing console critical section...");
	DeleteCriticalSection(&csConsole);

	MBLOGEX("Shutdown sequence finished!");

	if(dbgout){
		fclose(dbgout);
	}

	FreeConsole();

	return 0;
}
/******************************************
 * events                                 *
 ******************************************/
int event_contact_new(WPARAM wParam, LPARAM lParam)
{
	CallService(MS_PROTO_ADDTOCONTACT,(WPARAM)wParam,(LPARAM)(char*)MBOT);
	return 0;
}

int event_oktoexit(WPARAM wParam,LPARAM lParam)
{
	UNHOOK(hHookOKToExit);

	if(!lFinished)
	{
		MBUnHookEvents();
		if(lh_flags & MB_EVENT_SHUTDOWN){
			MBNoParamExecute(MB_EVENT_SHUTDOWN);
		}

		lFinished = 1;
	}
	return 0;
}

int event_contact_del(WPARAM wParam, LPARAM lParam)
{
	CallService(MS_PROTO_REMOVEFROMCONTACT,(WPARAM)wParam,(LPARAM)(char*)MBOT);
	return 0;
}

int MBUnHookEvents()
{
	if(lh_flags & MB_EVENT_MSG_IN){DestroyServiceFunction(MBOT PSR_MESSAGE);}
	if(lh_flags & MB_EVENT_URL_IN){DestroyServiceFunction(MBOT PSR_URL);}
	if(lh_flags & MB_EVENT_AUTH_IN){DestroyServiceFunction(MBOT PSR_AUTH);}
	if(lh_flags & MB_EVENT_AWAY_MSG_OUT){DestroyServiceFunction(MBOT PSS_AWAYMSG);}
	if(lh_flags & MB_EVENT_FILE_IN){DestroyServiceFunction(MBOT PSR_FILE);}
	UNHOOK(hHookNMS)
	UNHOOK(hHookSC)
	UNHOOK(hHookTY)
	UNHOOK(hHookSHUT)
	UNHOOK(hHookICQ)

	UNHOOK(hHookACK)
	UNHOOK(hHookNU)
	UNHOOK(hHookDU)
	UNHOOK(hHookPBCM)
	return 0;
}

int MBHookEvents()
{
	if((lh_flags & MB_EVENT_MSG_IN) && !(hFunctPSR_MESSAGE)){
		hFunctPSR_MESSAGE = CreateServiceFunction(MBOT PSR_MESSAGE,MBRecvMsg);
	}
	if((lh_flags & MB_EVENT_URL_IN) && !(hFunctPSR_URL)){
		hFunctPSR_URL = CreateServiceFunction(MBOT PSR_URL,MBRecvUrl);
	}
	if((lh_flags & MB_EVENT_AUTH_IN) && !(hFunctPSR_AUTH)){
		hFunctPSR_AUTH = CreateServiceFunction(MBOT PSR_AUTH,MBRecvAuth);
	}
	if((lh_flags & MB_EVENT_AWAY_MSG_OUT) && !(hFunctPSS_AWAYMSG)){
		hFunctPSS_AWAYMSG = CreateServiceFunction(MBOT PSS_AWAYMSG,MBSendAwayMsg);
	}
	if((lh_flags & MB_EVENT_FILE_IN) && !(hFunctPSR_FILE)){
		hFunctPSR_FILE = CreateServiceFunction(MBOT PSR_FILE,MBRecvFile);
	}
	if((lh_flags & MB_EVENT_USER_TYPING) && !(hHookTY)){
		hHookTY = HookEvent(ME_PROTO_CONTACTISTYPING,event_typing);
	}
	if((lh_flags & MB_EVENT_NEW_CSTATUS) && !(hHookSC)){
		hHookSC = HookEvent(ME_DB_CONTACT_SETTINGCHANGED,event_contact_changed);
	}
	if((lh_flags & MB_EVENT_NEW_MYSTATUS) && !(hHookNMS)){
		hHookNMS = HookEvent(ME_CLIST_STATUSMODECHANGE,event_new_mystatus);
	}
	if((lh_flags & MB_EVENT_AWAY_MSG_ICQ) && !(hHookICQ)){
		hHookICQ = HookEvent("ICQ/StatusMsgReq",event_icq_awayreq);
	}
	if(!hHookPBCM){
		hHookPBCM = HookEvent(ME_CLIST_PREBUILDCONTACTMENU,event_contact_menu_prebuild);
	}

	//hook irc stuff
	if(!hFunctIRCGuiIn)
	{
		hFunctIRCGuiIn = CreateServiceFunction(MS_MBOT_IRC_GUI_IN,MBIRCGuiIn);
		hFunctIRCGuiOut = CreateServiceFunction(MS_MBOT_IRC_GUI_OUT,MBIRCGuiOut);
		hFunctIRCRawIn = CreateServiceFunction(MS_MBOT_IRC_RAW_IN,MBIRCRawIn);
		hFunctIRCRawOut = CreateServiceFunction(MS_MBOT_IRC_RAW_OUT,MBIRCRawOut);
	}

	return 0;
}

int event_modules_loaded(WPARAM wParam, LPARAM lParam)
{
	RegisterContacts();
	UnhookEvent(hHookNU);//unhook mod-loaded
	hHookNU = HookEvent(ME_DB_CONTACT_ADDED,event_contact_new);
	hHookDU = HookEvent(ME_DB_CONTACT_DELETED,event_contact_del);
	hHookACK = HookEvent(ME_PROTO_ACK,event_msg_ack);

	MBotGetPrefString("CmdTag",pszCmdTag,2,"m>");
	MBotGetPrefString("ScriptTag",pszPhpTag,2,"?>");

	lCmdTagLen = strlen(pszCmdTag);
	lPhpTagLen = strlen(pszPhpTag);

	if(lh_flags & MB_EVENT_STARTUP){
		MBNoParamExecute(MB_EVENT_STARTUP);
	}
	MBHookEvents();

	if(!DBGetContactSettingByte(NULL,MBOT,"NoScheduler",0) && cron_initialize()){
		cron_startup();
	}

	CallService(MS_SYSTEM_GET_MMI,0,(LPARAM)&mmi);

	//launch server
#ifndef _NOHTTPD_
	if(DBGetContactSettingByte(NULL,MBOT,"WWWEnabled",1)){
		httpd_startup();
	}
#endif
	return 0;
}

int event_contact_menu_prebuild(WPARAM wParam,LPARAM lParam)
{
	if(lm_flags & MBOT_FLAG_WANTPREBUILD)
	{
		sMFSync* mfs = (sMFSync*)g_mlist.m_head;
		mb_event mbe = {MBT_CALLBACK,0,0};
		mbe.php = mfs->php;

		while(mfs)
		{
			if(*(mfs->pszPrebuild))
			{
				sman_inc(mfs->php);
				if(mfs->php->szBuffered){
					LPHP_ExecuteScript(mfs->php->szBuffered,mfs->pszPrebuild,NULL,&mbe,"ll",wParam,mfs->hMenu);
				}else{
					LPHP_ExecuteFile(mfs->php->szFilePath,mfs->pszPrebuild,NULL,&mbe,"ll",wParam,mfs->hMenu);
				}
				sman_dec(mfs->php);
			}
			mfs = (sMFSync*)mfs->next;
		}
	}
	return 0;
}

int event_contact_changed(WPARAM wParam, LPARAM lParam)
{
	DBCONTACTWRITESETTING* dbws = (DBCONTACTWRITESETTING*)lParam;
	if(dbws && dbws->value.type && !mbot_our_own(dbws->szSetting))
	{
		if(*dbws->szSetting == 'S' && strcmp(dbws->szSetting,MBOT_CFG_STATUS)==0)
		{
			MBGenericSettChanged((HANDLE)wParam,dbws,MB_EVENT_NEW_CSTATUS);
		}
	}
	return 0;
}

int event_icq_awayreq(WPARAM mode,LPARAM uin)
{
	HANDLE hContact = help_find_by_uin("ICQ",(const char*)uin,1);
	MBMultiParamExecute(MB_EVENT_AWAY_MSG_ICQ,NULL,"uuuu",(void*)hContact,
		(void*)(DBGetContactSettingByte(hContact,"CList","NotOnList",1)==0),(void*)mode,(void*)uin);
	return 0;
}

int handle_ack_awaymsg(ACKDATA* ack,sACKSync* cs)
{
	mb_event mbe={MBT_CALLBACK,0,0};
	mbe.php = cs->php;

	sman_inc(cs->php);
	if(cs->php->szBuffered){
		LPHP_ExecuteScript(cs->php->szBuffered,cs->pszFunction,NULL,&mbe,"lls",
			ack->hContact,ack->result == ACKRESULT_SUCCESS,ack->lParam);
	}else{
		LPHP_ExecuteFile(cs->php->szFilePath,cs->pszFunction,NULL,&mbe,"lls",
			ack->hContact,ack->result == ACKRESULT_SUCCESS,ack->lParam);
	}
	sman_dec(cs->php);
	return 1;
}

int handle_ack_message(ACKDATA* ack,sACKSync* cs)
{
	mb_event mbe={MBT_CALLBACK,0,0};
	mbe.php = cs->php;

	sman_inc(cs->php);
	if(cs->php->szBuffered){
		LPHP_ExecuteScript(cs->php->szBuffered,cs->pszFunction,NULL,&mbe,"lll",
			ack->hContact,ack->result == ACKRESULT_SUCCESS,cs->lParam);
	}else{
		LPHP_ExecuteFile(cs->php->szFilePath,cs->pszFunction,NULL,&mbe,"lll",
			ack->hContact,ack->result == ACKRESULT_SUCCESS,cs->lParam);
	}
	sman_dec(cs->php);
	return 1;
}

int handle_ack_file(ACKDATA* ack,sACKSync* cs)
{
	mb_event mbe={MBT_CALLBACK,0,0};
	mbe.php = cs->php;
	sman_inc(cs->php);
	//file_cb($fth,$ack,$param,$pack)

	if(ack->result == ACKRESULT_DATA){
		PROTOFILETRANSFERSTATUS *fts=(PROTOFILETRANSFERSTATUS*)ack->lParam;
		sFileInfo* fi = (sFileInfo*)cs->pszProtocol;
		bool first = fi->numFiles == 0;

		fi->numFiles = fts->totalFiles;
		fi->curFile = fts->currentFileNumber;
		fi->bytesTotal = fts->totalBytes;
		fi->bytesDone = fts->totalProgress;
		fi->curDone = fts->currentFileProgress;
		fi->curSize = fts->currentFileSize;
		fi->curTime = fts->currentFileTime;
		if(!first){
			return 0;
		}else{
			ack->result = 101;
		}
	};

	if(cs->php->szBuffered){
		LPHP_ExecuteScript(cs->php->szBuffered,cs->pszFunction,NULL,&mbe,"llll",
			cs->hProcess,ack->hContact,ack->result,cs->lParam);
	}else{
		LPHP_ExecuteFile(cs->php->szFilePath,cs->pszFunction,NULL,&mbe,"llll",
			cs->hProcess,ack->hContact,ack->result,cs->lParam);
	}
	sman_dec(cs->php);
	return (ack->result == ACKRESULT_SUCCESS || ack->result == ACKRESULT_FAILED);
}

int handle_ack_search(ACKDATA* ack,sACKSync* cs)
{
	mb_event mbe={MBT_CALLBACK,0,0};
	PROTOSEARCHRESULT* sr = (PROTOSEARCHRESULT*)ack->lParam;
	mbe.php = cs->php;

	sman_inc(cs->php);

	mbe.p3 = (void*)sr;
	mbe.t3 = MBE_SRESULT;
	mbe.p2 = (void*)ack;
	mbe.t2 = MBE_ACK;

	if(ack->result == ACKRESULT_DATA)
	{
		if(cs->php->szBuffered){
			LPHP_ExecuteScript(cs->php->szBuffered,cs->pszFunction,NULL,&mbe,"lssss",
				2,sr->nick,sr->firstName,sr->lastName,sr->email);
		}else{
			LPHP_ExecuteFile(cs->php->szFilePath,cs->pszFunction,NULL,&mbe,"lssss",
				2,sr->nick,sr->firstName,sr->lastName,sr->email);
		}

		sman_dec(cs->php);
		return 0;
	}else{
		if(cs->php->szBuffered){
			LPHP_ExecuteScript(cs->php->szBuffered,cs->pszFunction,NULL,&mbe,"lllll",
				(ack->result == ACKRESULT_SUCCESS),0,0,0,0);
		}else{
			LPHP_ExecuteFile(cs->php->szFilePath,cs->pszFunction,NULL,&mbe,"lllll",
				(ack->result == ACKRESULT_SUCCESS),0,0,0,0);
		}
		sman_dec(cs->php);
		return 1;
	}
}

int event_msg_ack(WPARAM wParam, LPARAM lParam)
{
	ACKDATA* ack = (ACKDATA*)lParam;
	sACKSync* s;
	sMsgSync* ms;
	int result = 0;

	if(!ack){
		return 0;
	}

	if(ack->type == ACKTYPE_AWAYMSG && ack->result == ACKRESULT_SENTREQUEST 
		&& (lh_flags & MB_EVENT_AWAY_MSG_REQ))
	{
		MBMultiParamExecute(MB_EVENT_AWAY_MSG_REQ,NULL,"uu",
			(void*)ack->hContact,(void*)(DBGetContactSettingByte(ack->hContact,"CList","NotOnList",1)==0));
	}
	else if(g_slist.m_count || g_msglist.m_count)
	{
		if(ack->result != ACKRESULT_SUCCESS && g_msglist.m_count)
		{
			g_msglist.Lock();
			ms = (sMsgSync*)g_msglist.m_head;
			while(ms)
			{
				if(ms->hContact == ack->hContact && ms->hProcess == ack->hProcess)
				{
					g_msglist.DelLocked(ms);
					my_memfree(ms->new_body);
					my_memfree(ms->old_body);
					my_memfree(ms);
					break;
				}
				ms = (sMsgSync*)ms->next;
			}
			g_msglist.Unlock();
		}

		g_slist.Lock();
		s = (sACKSync*)g_slist.m_head;
		while(s)
		{
			if(s->lType == ack->type && s->hProcess == ack->hProcess && 
				s->hContact == ack->hContact)
			{
				g_slist.Unlock();
				switch(ack->type)
				{
					case ACKTYPE_AWAYMSG: result = handle_ack_awaymsg(ack,s);break;
					case ACKTYPE_SEARCH: result = handle_ack_search(ack,s);break;
					case ACKTYPE_MESSAGE: result = handle_ack_message(ack,s);break;
					case ACKTYPE_FILE: result = handle_ack_file(ack,s);break;
					default:
						break;
				}

				if(result == 1){
					g_slist.Del(s);
					my_memfree(s);
				}

				return 0;
			}
			s = (sACKSync*)s->next;
		}
		g_slist.Unlock();
	}
	return 0;
}

int MBNoParamExecute(long ev_id)
{
	int result = 0;
	const char* output = NULL;
	mb_event mbe={MBT_SHUTDOWN,0,0,0};

	PHANDLER ph = sman_handler_get(ev_id);
	while(ph)
	{
		if(ph->lFlags & MBOT_FLAG_INACTIVE){
			goto Next;
		}

		mbe.php = ph->php;

		sman_inc(ph->php);
		if(ph->php->szBuffered){
			result = LPHP_ExecuteScript(ph->php->szBuffered,ph->szFcnName,&output,&mbe,NULL);
		}else{
			result = LPHP_ExecuteFile(ph->php->szFilePath,ph->szFcnName,&output,&mbe,NULL);
		}
		sman_dec(ph->php);

		if(result!=TRUE){
			MB_Popup(PHP_ERROR_TITLE,PHP_ERROR_EXECUTION);
			goto Next;
		}
		if(help_parseoutput(output)!=PHPR_CONTINUE){
			break;
		}
Next:
		ph = ph->next;
	}
	return 0;
}

int MBMultiParam(PPHP php,const char* fcn,mb_event* mbe,const char* params,...)
{
	int result = 0;
	const char* output = NULL;
	va_list args;

	va_start(args,params);

	sman_inc(php);
	if(php->szBuffered){
		result = LPHP_ExecuteScriptVA(php->szBuffered,fcn,(mbe->lFlags&MBOT_FLAG_NOOUTPUT)?(NULL):(&output),mbe,params,args);
	}else{
		result = LPHP_ExecuteFileVA(php->szFilePath,fcn,(mbe->lFlags&MBOT_FLAG_NOOUTPUT)?(NULL):(&output),mbe,params,args);
	}
	sman_dec(php);

	if(result!=TRUE){
		MB_Popup(PHP_ERROR_TITLE,PHP_ERROR_EXECUTION);
		return 0;
	}

	if(mbe->lFlags&MBOT_FLAG_NOOUTPUT){
		return 1;
	}else{
		result = (int)help_parseoutput(output);
		return result;
	}
}

int MBMultiParamExecute(long ev_id,mb_event* me,const char* params,...)
{
	int result = 0;
	const char* output = NULL;
	va_list args;

	mb_event mbe={MBT_NOTHING,0};

	va_start(args,params);

	if(!me){
		mbe.event = help_get_event_type(ev_id);
		me = &mbe;
	}

	PHANDLER ph = sman_handler_get(ev_id);
	while(ph)
	{
		if(ph->lFlags & MBOT_FLAG_INACTIVE){
			goto Next;
		}
		me->php = ph->php;

		sman_inc(ph->php);
		if(ph->php->szBuffered){
			result = LPHP_ExecuteScriptVA(ph->php->szBuffered,ph->szFcnName,&output,me,params,args);
		}else{
			result = LPHP_ExecuteFileVA(ph->php->szFilePath,ph->szFcnName,&output,me,params,args);
		}
		sman_dec(ph->php);

		if(result!=TRUE){
			MB_Popup(PHP_ERROR_TITLE,PHP_ERROR_EXECUTION);
			goto Next;
		}
		if((result = (int)help_parseoutput(output)) != (int)PHPR_CONTINUE){
			break;
		}
Next:
		ph = ph->next;
	}
	return result;
}

int event_new_mystatus(WPARAM wParam, LPARAM lParam)
{
	MBMultiParamExecute(MB_EVENT_NEW_MYSTATUS,NULL,"su",(void*)lParam,(void*)wParam);
	return 0;
}

int event_typing(WPARAM wParam, LPARAM lParam)
{
	MBMultiParamExecute(MB_EVENT_USER_TYPING,NULL,"ll",(void*)wParam,(void*)lParam);
	return 0;
}

int event_new_db_event(WPARAM wParam, LPARAM lParam)
{
	HANDLE hc = (HANDLE)wParam;
	DBEVENTINFO dbei={0};
	CLISTEVENT cle={0};
	sMsgSync* ms;

	if(!g_msglist.m_count)return 0;

	dbei.cbSize=sizeof(dbei);
	dbei.cbBlob=0;
	CallService(MS_DB_EVENT_GET,lParam,(LPARAM)&dbei);
	if(!(dbei.flags&DBEF_SENT) || dbei.eventType)return 0;

	g_msglist.Lock();
	ms = (sMsgSync*)g_msglist.m_head;
	while(ms)
	{
		if(ms->hContact == hc && ms->timestamp == dbei.timestamp)
		{
			g_msglist.DelLocked(ms);
			my_memfree(ms->new_body);
			my_memfree(ms);
			break;
		}
		ms = (sMsgSync*)ms->next;
	}
	g_msglist.Unlock();
	return 0;
}

int event_pre_db_event(WPARAM wParam, LPARAM lParam)
{
	HANDLE hc = (HANDLE)wParam;
	DBEVENTINFO *dbev = (DBEVENTINFO*)lParam;
	sMsgSync* ms;
	if(!g_msglist.m_count)return 0;
	else{
		g_msglist.Lock();
		ms = (sMsgSync*)g_msglist.m_head;
		while(ms)
		{
			if(ms->old_body && ms->hContact == hc && (*ms->old_body == *dbev->pBlob) && !strcmp(ms->old_body,(const char*)dbev->pBlob))
			{
				if(!ms->new_body){
					my_memfree(ms->old_body);
					g_msglist.DelLocked(ms);
					my_memfree(ms);
					g_msglist.Unlock();
					return 1;
				}else{
					dbev->pBlob = (unsigned char*)ms->new_body;
					dbev->cbBlob = strlen(ms->new_body) + 1;
					my_memfree(ms->old_body);
					ms->old_body = NULL;
					ms->timestamp = dbev->timestamp;
				}
				break;
			}
			ms = (sMsgSync*)ms->next;
		}
		g_msglist.Unlock();
	}
	return 0;
}

int event_opt_initialise(WPARAM wParam, LPARAM lParam)
{
	static unsigned int adv_options[] = {IDC_ADV1,IDC_ADVCMDTAG,IDC_ADVLABEL1,IDC_ADVLABEL2,IDC_ADVSCRIPTTAG};

    OPTIONSDIALOGPAGE odp = { 0 };

    odp.cbSize = sizeof(odp);
    odp.position = 2000;
    odp.hInstance = hInst;
    odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTIONS);
    odp.pszTitle = Translate("MBot");
    odp.pszGroup = Translate("Plugins");
    odp.pfnDlgProc = MBotDlgProcOption;
	odp.hIcon = hMBotIcon;
	odp.expertOnlyControls = adv_options;
	odp.nExpertOnlyControls = sizeof(adv_options) / sizeof(unsigned int);
    odp.flags = ODPF_BOLDGROUPS;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) & odp);

	return 0;
}

/******************************************
 * generic handlers                       *
 ******************************************/
int MBGenericSettChanged(HANDLE hContact, DBCONTACTWRITESETTING* dbws, long ev_id)
{
	int bNotOnList = 0;
	int first = 0;
	int result = 0;
	const char* output = NULL;
	lphp_vparam vp;
	DBVARIANT* dbv;

	if(!hContact){
		return 0;
	}

	mb_event mbe={MBT_CSCHANGED,(WPARAM)hContact,(LPARAM)dbws,0,0};
	PHANDLER ph = sman_handler_get(ev_id);

	while(ph)
	{
		if(ph->lFlags & MBOT_FLAG_INACTIVE){
			goto Next;
		}else{
			if(!(first++))
			{
				bNotOnList = DBGetContactSettingByte(hContact,"CList","NotOnList",0);
				dbv = (DBVARIANT*)&dbws->value;
				mbe.t3 = MBE_DBVARIANT;
				mbe.p3 = (void*)dbv;

				if(dbv->type == DBVT_BLOB){
					vp.type = LPHP_STRING;
					vp.data = dbv->pbVal;
					vp.length = dbv->cpbVal;
				}else if(dbv->type == DBVT_ASCIIZ){
					vp.type = LPHP_STRING;
					vp.data = dbv->pszVal;
					vp.length = strlen((const char*)dbv->pszVal);
				}else{
					vp.type = LPHP_NUMBER;
					vp.length = 4;
					if(dbv->type == DBVT_BYTE){
						vp.data = ((void*)((long)dbv->bVal));
					}else if(dbv->type == DBVT_WORD){
						vp.data = ((void*)((long)dbv->wVal));
					}else{
						vp.data = ((void*)((long)dbv->lVal));
					}
				}
			}
		}

		mbe.php = ph->php;

		sman_inc(ph->php);
		if(ph->php->szBuffered){
			result = LPHP_ExecuteScript(ph->php->szBuffered,ph->szFcnName,&output,&mbe,"llsslv",
				hContact,bNotOnList==0,dbws->szModule,dbws->szSetting,dbws->value.type,&vp);
		}else{
			result = LPHP_ExecuteFile(ph->php->szFilePath,ph->szFcnName,&output,&mbe,"llsslv",
				hContact,bNotOnList==0,dbws->szModule,dbws->szSetting,dbws->value.type,&vp);
		}
		sman_dec(ph->php);

		if(result!=TRUE){
			MB_Popup(PHP_ERROR_TITLE,PHP_ERROR_EXECUTION);
			goto Next;
		}

		if(help_parseoutput(output)!=PHPR_CONTINUE){
			break;
		}
Next:
		ph = ph->next;
	}
	return 0;
}

int MBGenericRecv(WPARAM wParam, LPARAM lParam,long ev_id,mb_event* ce)
{
	const char* output = NULL;
	int first = 0;
	int result = 0;
	int bNotOnList = 0;
	PHPR ret_val = PHPR_UNKNOWN;
	string ss;
	CCSDATA *pccsd = (CCSDATA *)lParam;
	PROTORECVEVENT *ppre = (PROTORECVEVENT*)pccsd->lParam;

	mb_event mbe = {MBT_PRERECV,wParam,lParam,0};

	PHANDLER ph = sman_handler_get(ev_id);
	while(ph)
	{
		if(ph->lFlags & MBOT_FLAG_INACTIVE){
			goto Next;
		}else{
			if(!(first++)){
				bNotOnList = DBGetContactSettingByte(pccsd->hContact,"CList","NotOnList",0);
				mbe.t1 = MBE_SSTRING;
				mbe.p1 = (void*)&ss;
				if(!ce){
					ce = &mbe;
				}
			}
		}

		ce->php = ph->php;

		sman_inc(ph->php);
		if(ph->php->szBuffered)
		{
			result = LPHP_ExecuteScript(ph->php->szBuffered,ph->szFcnName,&output,(void*)ce,"usuu",
				pccsd->hContact,ppre->szMessage,ppre->timestamp,(bNotOnList==0));
		}else{
			result = LPHP_ExecuteFile(ph->php->szFilePath,ph->szFcnName,&output,(void*)ce,"usuu",
				pccsd->hContact,ppre->szMessage,ppre->timestamp,(bNotOnList==0));
		}
		sman_dec(ph->php);

		if(result != TRUE)
		{
			MB_Popup(PHP_ERROR_TITLE,PHP_ERROR_EXECUTION);
			goto Next;
		}

		ret_val = help_parseoutput(output);

		if(ret_val == PHPR_DROP || ret_val == PHPR_HIDE){
			return 1;
		}else if(ret_val != PHPR_CONTINUE){
			break;
		}

		if((mbe.lFlags & MBOT_FLAG_BODY_CHANGED) && ss.size()){
			ppre->szMessage = (char*)ss.data();
		}
Next:
		ph = ph->next;
	}

	if(ret_val == PHPR_STORE)
	{
		ppre->flags |= PREF_CREATEREAD;
	}

	return CallService(MS_PROTO_CHAINRECV, wParam,(LPARAM)pccsd);
}
/******************************************
 * service functions                      *
 ******************************************/
int MBRecvMsg(WPARAM wParam, LPARAM lParam){
	return MBGenericRecv(wParam,lParam,MB_EVENT_MSG_IN);
}
int MBRecvUrl(WPARAM wParam, LPARAM lParam){
	return MBGenericRecv(wParam,lParam,MB_EVENT_URL_IN);
}
int MBRecvFile(WPARAM wParam, LPARAM lParam)
{
	CCSDATA *pccsd = (CCSDATA *)lParam;
	PROTORECVEVENT* pre = (PROTORECVEVENT*)pccsd->lParam;
	PHPR result = PHPR_UNKNOWN;
	char* desc;
	char* file;

	mb_event mbe={MBT_FILEIN,wParam,lParam,0};

	file = (char*)(pre->szMessage + sizeof(DWORD));
	desc = file + strlen(file)+1;

	result = (PHPR)MBMultiParamExecute(MB_EVENT_FILE_IN,&mbe,"lssl",pccsd->hContact,
		(void*)desc,(void*)file,(void*)pre->timestamp);

	if(result == PHPR_DROP || result == PHPR_HIDE){
		return 1;
	}else{
		return CallService(MS_PROTO_CHAINRECV,wParam,lParam);
	}
}
int MBRecvAuth(WPARAM wParam, LPARAM lParam){
	CCSDATA* css = (CCSDATA*)lParam;
	PROTORECVEVENT  pqr;
	PROTORECVEVENT* o_pre;
	int result = 0;
	mb_event mbe = {MBT_AUTHRECV,wParam,lParam,0};

	if(!css || !css->lParam){
		return 1;
	}
	o_pre = (PROTORECVEVENT*)css->lParam;
	pqr = *o_pre;
	css->lParam = (LPARAM)&pqr;

	pqr.szMessage = (char*)o_pre->szMessage + sizeof(DWORD) + sizeof(HANDLE);
	pqr.szMessage += strlen(pqr.szMessage) + 1;//first
	pqr.szMessage += strlen(pqr.szMessage) + 1;//last
	pqr.szMessage += strlen(pqr.szMessage) + 1;//email
	pqr.szMessage += strlen(pqr.szMessage) + 1;//reason

	mbe.t2 = MBE_EVENTID;
	mbe.p2 = (void*)MB_EVENT_AUTH_IN;
	mbe.t3 = MBE_CUSTOM;
	mbe.p3 = (void*)o_pre->szMessage;
	mbe.t4 = MBE_HCONTACT;
	mbe.p4 = (void*)css->hContact;
	result = MBGenericRecv(wParam,lParam,MB_EVENT_AUTH_IN,&mbe);
	css->lParam = (LPARAM)o_pre;
	return result;
}

PHPR MBCommandExecute(const char* msg, HANDLE hContact, string* ss)
{
	if(!(*msg)){
		return PHPR_UNKNOWN;
	}
	while(*msg && isspace(*msg)){msg++;}
	if(!(*msg)){
		MBotConsoleAppendText("Command is empty!\r\n",1);
		return PHPR_UNKNOWN;
	}

	const char* output = NULL;
	int result = 0;
	PHPR retval = PHPR_UNKNOWN;
	PHANDLER ph = sman_handler_get(MB_EVENT_COMMAND);
	mb_event mbe = {MBT_COMMAND,0,0,0};

	if(!hContact || !ss){
		mbe.event = MBT_CONSOLE_CMD;
	}else{
		mbe.t1 = MBE_SSTRING;
		mbe.p1 = (void*)ss;
	}


	while(ph)
	{
		if(ph->lFlags & MBOT_FLAG_INACTIVE){
			goto Next;
		}

		mbe.php = ph->php;

		sman_inc(ph->php);
		if(ph->php->szBuffered){
			result = LPHP_ExecuteScript(ph->php->szBuffered,ph->szFcnName,&output,&mbe,"su",msg,hContact);
		}else{
			result = LPHP_ExecuteFile(ph->php->szFilePath,ph->szFcnName,&output,&mbe,"su",msg,hContact);
		}
		sman_dec(ph->php);

		if(result != TRUE){
			MB_Popup(PHP_ERROR_TITLE,PHP_ERROR_EXECUTION);
			goto Next;
		}

		retval = help_parseoutput(output);

		if((mbe.lFlags & MBOT_FLAG_BODY_CHANGED) && ss->size()){
			msg = ss->data();
			mbe.lFlags &= ~(MBOT_FLAG_BODY_CHANGED);
		}

		if(retval != PHPR_CONTINUE && retval != PHPR_UNKNOWN){
			break;
		}
Next:
		ph = ph->next;
	}
	return retval;
}

int MBSendAwayMsg(WPARAM wParam, LPARAM lParam)
{
	CCSDATA *pccsd = (CCSDATA *)lParam; 
	const char* msg = (const char*)pccsd->lParam;
	const char* output = NULL;
	int result = 0;

	mb_event mbe = {MBT_SEND,lParam,wParam,0};
	PHPR retval = PHPR_UNKNOWN;
	string ss;

	mbe.t1 = MBE_SSTRING;
	mbe.p1 = (void*)&ss;

	PHANDLER ph = sman_handler_get(MB_EVENT_AWAY_MSG_REQ);
	while(ph)
	{
		if(ph->lFlags & MBOT_FLAG_INACTIVE){
			goto Next;
		}

		mbe.php = ph->php;

		sman_inc(ph->php);
		if(ph->php->szBuffered){
			result = LPHP_ExecuteScript(ph->php->szBuffered,ph->szFcnName,&output,&mbe,"us",
				pccsd->hContact,msg);
		}else{
			result = LPHP_ExecuteFile(ph->php->szFilePath,ph->szFcnName,&output,&mbe,"us",
				pccsd->hContact,msg);
		}
		sman_dec(ph->php);

		if(result != TRUE){
			MB_Popup(PHP_ERROR_TITLE,PHP_ERROR_EXECUTION);
			goto Next;
		}

		retval = help_parseoutput(output);

		if((mbe.lFlags & MBOT_FLAG_BODY_CHANGED)){
			msg = (const char*)ss.data();
			pccsd->lParam = (LPARAM)msg;
			mbe.lFlags &= ~(MBOT_FLAG_BODY_CHANGED);
		}

		if(retval == PHPR_SEND){
			break;
		}else if(retval == PHPR_DROP){
			return 1;
		}
Next:
		ph = ph->next;
	}
	return CallService( MS_PROTO_CHAINSEND, wParam, lParam);
}


int MBSendMsg(WPARAM wParam, LPARAM lParam)
{
	CCSDATA *pccsd = (CCSDATA *)lParam; 
	const char* msg = (const char*)pccsd->lParam;
	const char* output = NULL;
	char  num_buff[64];
	int result = 0;
	PHPR retval = PHPR_UNKNOWN;
	string ss;
	sMsgSync mgs;
	sMsgSync* pmgs = NULL;

	if(pccsd->wParam & 0x800000){
		pccsd->wParam &= ~(0x800000);
		return CallService( MS_PROTO_CHAINSEND, wParam, lParam);
	}

	mb_event mbe = {MBT_COMMAND,lParam,wParam,0};

	mbe.t1 = MBE_SSTRING;
	mbe.p1 = (void*)&ss;

	mgs.old_body = (char*)msg;
	mgs.hContact = pccsd->hContact;
	mgs.new_body = NULL;

	if((lh_flags & MB_EVENT_COMMAND) && strncmp(pszCmdTag,msg,lCmdTagLen)==0)
	{
		retval = MBCommandExecute(msg + lCmdTagLen,pccsd->hContact,&ss);
		if(retval != PHPR_SEND){
			return MBBroadcastACK(pccsd->hContact, retval != PHPR_FAILED);
		}
	}
	else if(strncmp(pszPhpTag,msg,lPhpTagLen)==0)
	{
		string body;
		sprintf(num_buff, "$cid=%u;\r\n", pccsd->hContact);
		body = num_buff;
		body.append(msg + lPhpTagLen);

		mbe.event = MBT_SSCRIPT;

		if(LPHP_ExecuteDirect((const char*)body.data(), &output, (void*)&mbe) == FALSE){
			MB_Popup(PHP_ERROR_TITLE,PHP_ERROR_EXECUTION);
			return MBBroadcastACK(pccsd->hContact, false);
		}else{
			return MBBroadcastACK(pccsd->hContact, true);
		}
	}
	////////
	//normal message, or command said to send the message :-)
	////////
	if(lh_flags & MB_EVENT_MSG_OUT)
	{
		mbe.event = MBT_SEND;

		PHANDLER ph = sman_handler_get(MB_EVENT_MSG_OUT);
		while(ph)
		{
			if(ph->lFlags & MBOT_FLAG_INACTIVE){
				goto Next;
			}

			mbe.php = ph->php;
			sman_inc(ph->php);
			if(ph->php->szBuffered){
				result = LPHP_ExecuteScript(ph->php->szBuffered,ph->szFcnName,&output,&mbe,"us",
					pccsd->hContact,pccsd->lParam);
			}else{
				result = LPHP_ExecuteFile(ph->php->szFilePath,ph->szFcnName,&output,&mbe,"us",
					pccsd->hContact,pccsd->lParam);
			}
			sman_dec(ph->php);

			if(result != TRUE){
				MB_Popup(PHP_ERROR_TITLE,PHP_ERROR_EXECUTION);
				goto Next;
			}

			retval = help_parseoutput(output);

			if(((retval == PHPR_DROP) && !pmgs) || (mbe.lFlags & MBOT_FLAG_BODY_CHANGED))
			{
				if(retval != PHPR_DROP){
					pccsd->lParam = (LPARAM)ss.data();
					mbe.lFlags &= ~(MBOT_FLAG_BODY_CHANGED);
				}

				if(!pmgs){
					pmgs = (sMsgSync*)my_malloc(sizeof(sMsgSync));
					if(!pmgs){
						MB_Popup("MSP Error!","Could not allocate memory!");
						return NULL;
					}
					*pmgs = mgs;
					g_msglist.Add(pmgs);
				}

				pmgs->new_body = (char*)msg;
			}

			if(retval == PHPR_SEND || retval == PHPR_STORE || retval == PHPR_DROP)
			{
				break;
			}
			else if(retval == PHPR_HIDE || retval == PHPR_FAILED)
			{
				CallService( MS_PROTO_CHAINSEND, wParam, lParam);
				MBBroadcastACK(pccsd->hContact, retval != PHPR_FAILED);
			}
Next:
			ph = ph->next;
		}
	}
	result = ((retval==PHPR_STORE) || (retval==PHPR_DROP)) ?
		(MBBroadcastACK(mgs.hContact, retval != PHPR_FAILED)) : //else
		(CallService( MS_PROTO_CHAINSEND, wParam, lParam));

	
	if(!result && pmgs)
	{
		g_msglist.Del(pmgs);
		my_memfree(pmgs);
	}
	else if(pmgs)
	{
		pmgs->hProcess = (HANDLE)result;
		pmgs->old_body = my_strdup(msg);
		if(!pmgs->old_body){
			g_msglist.Del(pmgs);
			my_memfree(pmgs);
			return NULL;
		}
		pmgs->new_body = (retval == PHPR_DROP)?(NULL):((char*)my_strdup(ss.data()));
	}
	return result;
}
/******************************************
 * queue and other stuff                  *
 ******************************************/
int RegisterContacts()
{
	HANDLE hContact = NULL;

	hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	while(hContact){
		CallService(MS_PROTO_ADDTOCONTACT,(WPARAM)hContact,(LPARAM)(char*)MBOT);
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
	}
	return 0;
}

int MBotShowConsole(WPARAM wParam, LPARAM lParam)
{
	extern long lConTopMost;

	if(hDialog){
		ShowWindow(hDialog,SW_SHOW);
		SetWindowPos(hDialog,lConTopMost?HWND_TOPMOST:0,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
		UpdateWindow(hDialog);
	}
	return 1;
}

/******************************************
 * IRC functions                          *
 ******************************************/
int MBIRCGuiIn(WPARAM wParam, LPARAM lParam)
{
	GCEVENT* gce = (GCEVENT*)lParam;
	WPARAM_GUI_IN* wpi = (WPARAM_GUI_IN*)wParam;
	mb_event mbe = {MBT_IRC_IN,0};

	if(gce->dwItemData & 0x80000000)return 0;

	mbe.lParam = lParam;
	mbe.wParam = wParam;
	mbe.p4 = (void*)lParam;
	mbe.t4 = MBE_CUSTOM;

	return (MBMultiParamExecute(IRC_EVENT_GUI_IN,&mbe,"slsSS",wpi->pszModule,gce->pDest->iType,
		gce->pDest->pszID,&gce->pszNick,&gce->pszText) == PHPR_DROP);
}
int MBIRCGuiOut(WPARAM wParam, LPARAM lParam)
{
	GCHOOK * gch = (GCHOOK*)lParam;
	mb_event mbe = {MBT_IRC_OUT,0};

	if(gch->dwData & 0x80000000)return 0;

	mbe.lParam = lParam;
	mbe.wParam = wParam;
	mbe.p4 = (void*)lParam;
	mbe.t4 = MBE_CUSTOM;

	return (MBMultiParamExecute(IRC_EVENT_GUI_OUT,&mbe,"sLSSS",wParam,&gch->pDest->iType,&gch->pDest->pszID,&gch->pszUID,&gch->pszText)==PHPR_DROP);
}

int MBIRCRawIn(WPARAM wParam, LPARAM lParam)
{
	mb_event mbe = {MBT_IRC_RAW,0};
	mbe.lParam = lParam;
	mbe.wParam = wParam;
	mbe.p4 = (void*)lParam;
	mbe.t4 = MBE_CUSTOM;
	mbe.t3 = MBE_CUSTOM;
	mbe.p3 = (void*)TRUE;
	return (MBMultiParamExecute(IRC_EVENT_RAW_IN,&mbe,"sS",wParam,lParam) == PHPR_DROP);
}
int MBIRCRawOut(WPARAM wParam, LPARAM lParam)
{
	mb_event mbe = {MBT_IRC_RAW,0};
	mbe.lParam = lParam;
	mbe.wParam = wParam;
	mbe.p4 = (void*)lParam;
	mbe.t4 = MBE_CUSTOM;
	mbe.t3 = MBE_CUSTOM;
	mbe.p3 = (void*)FALSE;
	return (MBMultiParamExecute(IRC_EVENT_RAW_OUT,&mbe,"sS",wParam,lParam) == PHPR_DROP);
}

int MBIRCRegister(WPARAM wParam, LPARAM lParam)
{
	struct STR{
		char* val;
		unsigned long  len;
	};

	char* array = 0;
	char* narr = 0;
	char* module = (char*)lParam;
	unsigned long  temp=0,temp2=0;

	STR tmp = {0};
	STR pptmp = {0};
	STR* ptmp;
	SVAR_TYPE svt = SV_NULL;

	if(LPHP_GetVar("/irc/modules",(void**)&ptmp,&svt))
	{
		temp = strlen(module);//@ 0000 4
		pptmp = *ptmp;
		ptmp = &pptmp;
		tmp.len = temp + 6 + ptmp->len;
		narr = array = (char*)my_malloc(tmp.len);//
		if(!array)return 1;
		memcpy(array,ptmp->val,ptmp->len);
		array += ptmp->len - 1;
		//count
		*array++ = '%';
		*array++ = 'S';
		*((long*)array) = temp;
		array += 4;
		memcpy(array,module,temp);
		array += temp;
		*array++ = 'X';
		tmp.val = narr;
		LPHP_SetVar("/irc/modules",(void*)&tmp,SV_ARRAY);
		my_memfree(narr);
	}else{ //A@0000S0000[...]X
		temp = strlen(module);
		tmp.len = temp + 6 + 2;
		narr = array = (char*)my_malloc(tmp.len);
		if(!array)return 1;
		tmp.val = array;
		*array++ = 'A';
		*array++ = '%';
		*array++ = 'S';
		*((long*)array) = temp;
		array += 4;
		memcpy(array,module,temp);
		array += temp;
		*array++ = 'X';
		LPHP_NewVar("/irc/modules",&tmp,SV_ARRAY,0);
		my_memfree(narr);
	}
	return 0;
}