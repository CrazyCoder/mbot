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
#include "functions.h"
#include "config.h"
#include "m_script.h"
#include "helpers.h"
#include "window.h"

MBT help_get_event_type(long event_id)
{
	static const MBT ftable[] = 
		{MBT_PRERECV,MBT_SEND,MBT_PRERECV,MBT_AUTHRECV,
		MBT_CALLBACK,MBT_CALLBACK,MBT_CALLBACK,
		MBT_NEWMYSTATUS,MBT_COMMAND,MBT_STARTUP,MBT_SHUTDOWN,
		MBT_CALLBACK,MBT_CALLBACK,MBT_CALLBACK,
		MBT_NOTHING,MBT_FILEIN};

	static const long  ftsize = (sizeof(ftable)/sizeof(MBT)) - 1;
	
	event_id = fcn_log2(event_id);

	if(event_id < ftsize){
		return ftable[event_id];
	}else{
		return MBT_NOTHING;
	}
}

const char* help_get_funct_name(long event_id)
{
	static const char* ftable[] = 
		{"mbe_MsgIn","mbe_MsgOut","mbe_UrlIn","mbe_AuthIn",
		"mbe_AwayMsgOut","mbe_AwayMsgReq","mbe_CStatus",
		"mbe_MyStatus","mbe_Command","mbe_StartUp","mbe_ShutDown",
		"mbe_CTyping","mbe_AwayMsgICQ","mbe_Dummy",
		"mbe_External","mbe_FileIn",
		"irc_GuiIn","irc_GuiOut","irc_RawIn","irc_RawOut","mbe_config",NULL};

	static const long  ftsize = (sizeof(ftable) / sizeof(char*)) - 1;
	
	event_id = fcn_log2(event_id);

	if(event_id < ftsize){
		return ftable[event_id];
	}else{
		return NULL;
	}
}

void help_callmenu(sMFSync* mfs,WPARAM wParam,LPARAM lParam)
{
	mb_event mbe={MBT_CALLBACK,0,0};

	if(!mfs)return;
	mbe.php = mfs->php;

	sman_inc(mfs->php);
	if(mfs->php->szBuffered){
		LPHP_ExecuteScript(mfs->php->szBuffered,mfs->pszFunction,NULL,&mbe,
			(mfs->hMenu)?("lll"):("ll"),mfs->pParam,wParam,mfs->hMenu);
	}else{
		LPHP_ExecuteFile(mfs->php->szFilePath,mfs->pszFunction,NULL,&mbe,
			(mfs->hMenu)?("lll"):("ll"),mfs->pParam,wParam,mfs->hMenu);
	}
	sman_dec(mfs->php);
}

///////////////////////////////////////////////////////////
long help_callsvc(sHESync* hfs,WPARAM wParam,LPARAM lParam)
{
	mb_event mbe={MBT_CALLBACK,0,0};
	const char* output = NULL;
	long  result = 0;

	if(!hfs)return 0;
	mbe.php = hfs->php;

	sman_inc(hfs->php);
	if(hfs->php->szBuffered){
		LPHP_ExecuteScript(hfs->php->szBuffered,hfs->pszFunction,&output,&mbe,"ll",wParam,lParam);
	}else{
		LPHP_ExecuteFile(hfs->php->szFilePath,hfs->pszFunction,&output,&mbe,"ll",wParam,lParam);
	}
	sman_dec(hfs->php);

	if(!output){
		return 0;
	}else{
		result = strtol(output + 1,NULL,0);
		LPHP_Free((void*)output);
		return result;
	}
}
void* help_makefunct(void* param,void* fcn)
{
	typedef long (*HFFX2)(void* hfs,WPARAM wParam,LPARAM lParam);
	struct s_dummy{
		HFFX2 fcn;
	};

	unsigned char* buff = (unsigned char*)my_malloc(32);
	unsigned char* bb = buff;
	if(!buff){return NULL;}

	*bb++ = 0x55;//push ebp
	*bb++ = 0x8B;//mov ebp
	*bb++ = 0xEC;//esp

	*bb++ = 0xff;//push
	*bb++ = 0x75;
	*bb++ = 0x0C;//ebp + 12

	*bb++ = 0xff;//push
	*bb++ = 0x75;
	*bb++ = 0x08;//ebp + 8

	*bb++ = 0x68;//push
	*(unsigned long*)(bb) = *(unsigned long*)&param; bb+=4;//src
	*bb++ = 0xBB;//mov ebx,
	((s_dummy*)bb)->fcn = (HFFX2)fcn; bb+=4;
	*bb++ = 0xFF;
	*bb++ = 0xD3;
	*bb++ = 0x83;//add
	*bb++ = 0xC4;//esp
	*bb++ = 0x0C;//12d
	*bb++ = 0x5D;//pop ebp
	*bb++ = 0xC3;//ret
	return (void*)(buff);//buff;
};

__declspec(naked) unsigned long __fastcall fcn_log2(long event_id)
{
	_asm
	{
		;function
		mov edx, ecx;
		or  edx, 0
		jz LEND;
		xor eax, eax;
		jmp ENTRY;
MLOOP:
		inc eax;
ENTRY:
		shr edx,1
		jnc MLOOP;
LEND:
		ret
	}
}

PHPR help_parseoutput(const char* output)
{
	PHPR result = PHPR_UNKNOWN;
	const char* tmp;

	if(output){
		tmp = output + strlen(output) + 1;
		
		if(*tmp == '0' && *(tmp+1)=='\0'){
			result = PHPR_CONTINUE;
		}else if(*tmp == '1' && *(tmp+1)=='\0'){
			result = PHPR_BREAK;
		}else if(strcmp(tmp,"end")==0){
			result = PHPR_END;
		}else if(strcmp(tmp,"drop")==0){
			result = PHPR_DROP;
		}else if(strcmp(tmp,"send")==0){
			result = PHPR_SEND;
		}else if(strcmp(tmp,"hide")==0){
			result = PHPR_HIDE;
		}else if(strcmp(tmp,"store")==0){
			result = PHPR_STORE;
		}else if(strcmp(tmp,"failed")==0){
			result = PHPR_FAILED;
		}
		LPHP_Free((void*)output);
	}
	return result;
}

int help_loadscript(const char* script,long recache)
{
	mb_event mbe = {MBT_AUTOLOAD,0};
	mbe.p3 = (void*)script;
	mbe.t3 = MBE_LPCSTR;
	mbe.t2 = MBE_LPARAM;
	mbe.p2 = (void*)recache;
	if(sman_getbyfile(script))return FALSE;
	return (LPHP_ExecuteFile(script,"mbot_load",NULL,&mbe,NULL)==TRUE);
}

int help_enable_script(PPHP php,bool enable)
{
	extern CSyncList sm_list;
	sm_list.Lock();
	PHANDLER ph;
	
	for(int i=0;i<32;i++){
		ph = sman_handler_find(php,1 << i);
		if(ph){
			if(enable){
				sman_handler_enable(ph);
			}else{
				sman_handler_disable(ph);
			}
		}
	}
	cron_enable_param(php,enable);
	if(enable){
		php->lFlags &= ~(MBOT_FLAG_INACTIVE | MBOT_FLAG_DELETE);
	}else{
		php->lFlags |= MBOT_FLAG_INACTIVE;
	}
	sm_list.Unlock();
	return 0;
}

long help_autoload(const char* path)
{
	intptr_t fptr = NULL;
	_finddata_t fdt = {0};
	long  count = 0;
	long  left = 0;
	int   result = 0;
	char  fpath[MAX_PATH + 32];
	char* fname = NULL;
	

	

	strncpy(fpath,path,sizeof(fpath)-1);
	fname = fpath + (left = strlen(fpath));
	left = (sizeof(fpath)-1) - left;

	strcpy(fname,"*.php");

	fptr = _findfirst(fpath,&fdt);
	if(fptr!=-1)
	{
		do
		{
			strncpy(fname,fdt.name,left);

			if(help_loadscript(fpath,0))
			{
				count ++;
			}else{
				MBotConsoleAppendText("Error loading script: ",0);
				MBotConsoleAppendText(fpath);
				MBotConsoleAppendText("!\r\nThere might be some error information in the debug file!\r\n",0);
				if(DBGetContactSettingByte(NULL,MBOT,"SWOnError",0)){
					MBotShowConsole(0,0);
				}
			}

		}while(_findnext(fptr,&fdt)!=-1);
		_findclose(fptr);
	}

	MBHookEvents();
	return count;
}

long help_getprocaddr(const char* module,const char* name)
{
	HMODULE hModule = NULL;

	if(!(*module) || strcmp(module,"miranda")==0)
	{
		if(!strcmp(name,"CallService")){
			return ((long)pluginLink->CallService);
		}else if(!strcmp(name,"CallProtoService")){
			return ((long)CallProtoService);
		}else if(!strcmp(name,"CallContactService")){
			return ((long)CallContactService);
		}else if(!strcmp(name,"ServiceExists")){
			return ((long)pluginLink->ServiceExists);
		}else{
			return 0;
		}
	}
	else
	{
		hModule = (HMODULE)GetModuleHandle(module);
		return ((hModule)?((long)GetProcAddress(hModule,name)):(0));
	}
}

int help_fileexists(const char* path,long* fs)
{
	intptr_t fptr = NULL;
	_finddata_t fdt = {0};

	fptr = _findfirst(path,&fdt);
	if(fptr!=-1){
		_findclose(fptr);
		if((fdt.attrib & _A_SUBDIR)==0){
			*fs = fdt.size;
			return 1;
		}else{
			return 0;
		}
	}else{
		return 0;
	}
}

int help_fileexists(const char* path)
{
	intptr_t fptr = NULL;
	_finddata_t fdt = {0};

	fptr = _findfirst(path,&fdt);
	if(fptr!=-1){
		_findclose(fptr);
		return (fdt.attrib & _A_SUBDIR)==0;
	}else{
		return 0;
	}
}
int help_filemtime(const char* path,unsigned long* out)
{
	intptr_t fptr = NULL;
	_finddata_t fdt = {0};

	fptr = _findfirst(path,&fdt);
	if(fptr!=-1){
		_findclose(fptr);
		if((fdt.attrib & _A_SUBDIR)==0){
			*out = fdt.time_write;
			return 1;
		}else{
			return 0;
		}
	}else{
		return 0;
	}
}

int help_direxists(const char* path)
{
	intptr_t fptr = NULL;
	_finddata_t fdt = {0};

	fptr = _findfirst(path,&fdt);
	if(fptr!=-1){
		_findclose(fptr);
		return (fdt.attrib & _A_SUBDIR)!=0;
	}else{
		return 0;
	}
}

int help_iswin2k()
{
	static OSVERSIONINFO vi = {sizeof(vi),0};
	GetVersionEx(&vi);
	if((vi.dwPlatformId != VER_PLATFORM_WIN32_NT) && (vi.dwMajorVersion < 5)){
		return 0;
	}else{
		return 1;
	}
}

const char* help_static_time()
{
	static char buffer[256];
	static SYSTEMTIME st;

	GetLocalTime(&st);
	_snprintf(buffer,sizeof(buffer)-1,"%.2d:%.2d:%.2d-%.3d",st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);
	return buffer;
}

LRESULT	WINAPI help_popup_wndproc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_COMMAND:
		if(HIWORD(wParam) == STN_CLICKED)
		{
			sMFSync* mfs = NULL;
			mfs = (sMFSync*)CallService(MS_POPUP_GETPLUGINDATA,(WPARAM)hWnd,(LPARAM)mfs);
			if(mfs){
				help_callmenu(mfs,(WPARAM)PUGetContact(hWnd),0);
			}
			PUDeletePopUp(hWnd);
			return TRUE;
		}
		break;
		case UM_FREEPLUGINDATA:
		{
			sMFSync* mfs = NULL;
			mfs = (sMFSync*)CallService(MS_POPUP_GETPLUGINDATA,(WPARAM)hWnd,(LPARAM)mfs);
			my_memfree(mfs);
			return TRUE;
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

HANDLE help_find_by_uin(const char* proto,const char* uin,int numeric)
{
	HANDLE hCid = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	long  bKnown = FALSE;
	const char *szProto;
	const char *szUIN;
	const char* szPTR;
	wchar_t* wc;
	char  szBuff[256];
	DBCONTACTGETSETTING dbgcs={0};
	DBVARIANT dbv = {0};

	if(!hCid){
		return NULL;
	}

	do
	{
		szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hCid,0);
		if(!szProto || *szProto != *proto || strcmp(proto,szProto)!=0)continue;

		if(!bKnown){
			szUIN = (char*)CallProtoService(proto,PS_GETCAPS,PFLAG_UNIQUEIDSETTING,0);
			if(!szUIN)return NULL;
			bKnown = TRUE;
			dbgcs.szModule = proto;
			dbgcs.szSetting = szUIN;
			dbgcs.pValue = &dbv;

			dbv.cchVal = sizeof(szBuff) - 1;
			dbv.pszVal = (char*)szBuff;
			dbv.type = DBVT_ASCIIZ;
		}

		if(CallService(MS_DB_CONTACT_GETSETTINGSTATIC,(WPARAM)hCid,(LPARAM)&dbgcs)){
			continue;
		}

		if(dbv.type == DBVT_ASCIIZ || dbv.type == DBVT_UTF8 || dbv.type == DBVT_WCHAR)
		{
			if(dbv.type == DBVT_WCHAR){
				szPTR = uin;

				for(wc = dbv.pwszVal; *wc && *szPTR; wc++, szPTR++){
					if(*wc != *szPTR)break;
				}

				if(!*wc && !*szPTR){
					return hCid;
				}
			}else if(strcmp(uin, dbv.pszVal) == 0){
				return hCid;
			}

		}else if(dbv.type && dbv.type <= 4){
			if(numeric){
				if(dbv.dVal == (DWORD)uin){
					return hCid;
				}
			}else{
				if(dbv.dVal == strtoul(uin,NULL,0)){
					return hCid;
				}
			}
		}
	}while(hCid = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hCid,0));

	return NULL;
}

const char* help_cache_php_file(const char* path)
{
	cutMemf in;
	cutMemf out;
	char ss[MAX_PATH];

	char* c = NULL;
	char* tmp = 0;
	long  status = 0;//normal
	long  skipped = 0;
	long  level = 0;//body

	if(!in.load(mb2u(path),0))
	{
		snprintf(ss, sizeof(ss), "%s\\mbot\\scripts\\%s", g_root, path);
		if(in.load(mb2u(ss),0))goto OK;

		snprintf(ss, sizeof(ss), "%s\\mbot\\%s", g_root, path);
		if(in.load(mb2u(ss),0))goto OK;

		return NULL;
	}
OK:
	if(!out.create(in.size() + 1)){
		return NULL;
	}

	//in.SetEOF(in.size() + 16);
	in.setpos(in.size() - 1);
	in.putc(0);

	c = (char*)in.getdata();
	c = strstr(c,"<?");

	if(!c++){
		return NULL;
	}

	if(*(unsigned long*)c == 'php?'){
		c += 4;
	}else{
		c++;
	}

	while(c && *c)
	{
		if(status == 0)
		{
			if(*c == '?' && *(c+1)=='>')
			{
				c = strstr(c+2,"<?");
				if(!c++){break;}
				if(*(DWORD*)c == 'php?'){
					c += 4;
				}else{
					c++;
				}
			}
			else if(*c == '\"'){
				status = 3;
				out.putc(*c);
			}
			else if(*c == '\''){
				status = 4;
				out.putc(*c);
			}
			else if(*c == '{'){
				level++;
				out.putc(*c);
			}
			else if(*c == '}'){
				level--;
				out.putc(*c);
			}
			else if(*((WORD*)c) == '//'){
				status = 1;
			}
			else if(*((WORD*)c) == '*/'){
				status = 2;
			}
			else
			{
				out.putc(*c);
			}
		}else if(status == 1){//style comment
			if(*c == '\r' || *c == '\n'){
				status = 0;
				out.putc(*c);
				c++;
			}
		}else if(status == 2){/*style comment*/
			if(*((WORD*)c) == '/*'){
				status = 0;
				c++;
			}else if(*c == '\n'){
				out.putc(*c);
			}
		}else if(status == 3){/* " quoted string*/
			if(*c == '\"' && *(c - 1) != '\\'){
				status = 0;
			}
			out.putc(*c);
		}else if(status == 4){/* ' quoted string*/
			if(*c == '\'' && *(c - 1) != '\\'){
				status = 0;
			}
			out.putc(*c);
		}
		c++;
	}
	out.putc(0);
	return (const char*)out.leave();
}

char* help_getfilename(long open,char* filter,const char* fname)
{
	static char fnbuffer[MAX_PATH + 1] = {0};
	OPENFILENAME ofn = {0};

	ofn.lStructSize = sizeof(ofn);

	if(fname){
		strncpy(fnbuffer,fname,sizeof(fnbuffer)-1);
	}
	
	ofn.hwndOwner = NULL; 
	ofn.lpstrFile = fnbuffer; 
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrTitle = TEXT("Select File");
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrCustomFilter = NULL;
	ofn.lpstrFileTitle = NULL;
	ofn.lpstrDefExt = NULL;

	if(open){
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_EXPLORER;
	}else{
		ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT | OFN_EXPLORER;
	}

	if(open && GetOpenFileName(&ofn)==IDOK){
		return fnbuffer;
	}else if(!open && GetSaveFileName(&ofn)==IDOK){
		return fnbuffer;
	}else{
		return NULL;
	}
}

char* help_getfilenamemultiple(char* filter,char* out,long outlen,long* offset)
{
	OPENFILENAME ofn = {0};

	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = out; 
	ofn.nMaxFile = outlen;
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrTitle = TEXT("Select File(s)");

	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | 
		OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

	if(GetOpenFileName(&ofn)==IDOK){
		*offset = ofn.nFileOffset;
		return out;
	}else{
		return NULL;
	}
}