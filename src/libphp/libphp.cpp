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
#include "internals.h"
#include "phpenv.h"
#include "svar.h"

HINSTANCE		 hInstance = NULL;
LPHP_OUTPUT		 g_std_out = NULL;
LPHP_OUTPUT		 g_std_err = NULL;
CRITICAL_SECTION g_csection = {0};

LPHP_MALLOC g_malloc = NULL;
LPHP_FREE	g_free = NULL;
HANDLE		g_event = NULL;
HANDLE		g_initth = NULL;
HANDLE		g_heap = NULL;
int			g_initialized = FALSE;
int			g_preinitialized = FALSE;
void*		g_php_module = NULL;

sVARmap g_vars;
sFCNmap g_fcns;
sVARmap* pg_vars = &g_vars;
sFCNmap* pg_fcns = &g_fcns;


int my_compare(long* l1,long * l2,void* param){
	if(*l1 == *l2){
		return 0;
	}else if(*l1 > *l2){
		return 1;
	}else{
		return -1;
	}
}

int lphp_funct_compare(lphp_funct* f1,lphp_funct * f2,void* param){
	return strcmp(f1->name,f2->name);
}

unsigned int lphp_funct_hash(lphp_funct* v)
{
	int index = 0;
	char* c = v->name;

	if(!*c){
		return 0;
	}

	while(*c){
		 index = 31 * index + *c++;
	}

	return (index & 0x7EffFFff) + 1;
}

void my_std_out(const char* data,long length = 0)
{
	if(!length){
		length = strlen(data);
	}
	fwrite(data,1,length,stdout);
}

void lphp_error(MTLOG_ENUM log_class,void* _ep,const char* info,...)
{
	va_list args;
	char ss[2048];

	va_start(args,info);
	_vsnprintf(ss,sizeof(ss),info,args);
	va_end(args);
	g_std_err(ss,strlen(ss));
	return;
}
#ifndef _LPHP_STATIC_
int WINAPI DllMain(HINSTANCE hInst,unsigned long dwReason,VOID* data)
{
	if(dwReason == DLL_PROCESS_ATTACH)
	{
		g_std_out = my_std_out;
		g_std_err = my_std_out;
	}else if(dwReason == DLL_THREAD_DETACH && g_initialized){
		try{
			ts_free_thread();
		}catch(...){
			return TRUE;
		}
	}
	hInstance = hInst;
	return TRUE;
}
#endif //_LPHP_STATIC_

const char* g_pref(const char* name)
{
	return g_pref_def(name, NULL);
}

const char* g_pref_def(const char* name,const char* def)
{
	cLock(g_csection);

	sVARmap::const_iterator it = g_vars.find(name);
	if(it == g_vars.end()){
		return NULL;
	}else{
		if((it->second).type == SV_STRING && (it->second).locked){
			return (it->second).str.val;
		}else{
			return def;
		}
	}
}
unsigned long g_pref_ul(const char* name, long base,unsigned long def)
{
	cLock(g_csection);

	sVARmap::const_iterator it = g_vars.find(name);
	if(it == g_vars.end()){
		return NULL;
	}else{
		if((it->second).type == SV_STRING){
			return strtoul((it->second).str.val,NULL,base);
		}else{
			return (it->second).lval;
		}
	}
}

LPHPAPI int LPHP_PreInit(unsigned long vt_size,unsigned long ft_size,LPHP_MALLOC fp_malloc,LPHP_FREE fp_free)
{
	if(vt_size < 256){
		vt_size = 256;
	}else if(vt_size > 65530){
		vt_size = 65530;
	}

	g_std_out = my_std_out;
	g_std_err = my_std_out;

	if(ft_size < 256){
		ft_size = 256;
	}else if(ft_size > 65530){
		ft_size = 65530;
	}

	g_free = fp_free;
	g_malloc = fp_malloc;
	InitializeCriticalSection(&g_csection);
	return g_preinitialized = TRUE;
}

unsigned long WINAPI InitTh(void* dummy)
{
	printf("initializing...\n");
	g_initialized = GO_PhpGlobalInit();
	printf("retval: %u\n",g_initialized);
	SetEvent(g_event);
	printf("suspending\n");
	SuspendThread(GetCurrentThread());
	if(g_initialized){
		g_initialized = 0;
		printf("shutting down\n");
		GO_PhpGlobalDeInit();
	}
	printf("done!\n");
	return 0;
}

LPHPAPI int LPHP_Init(LPHP_OUTPUT fp_output,LPHP_OUTPUT fp_error,void* php_module)
{
	unsigned long tmp;
	if(g_initialized!=FALSE || !g_preinitialized){
		return 0;
	}

	g_php_module = php_module;

	g_event = CreateEvent(0,0,0,0);
	//add checking
	g_initth = CreateThread(0,64*1024,(LPTHREAD_START_ROUTINE)InitTh,NULL,0,&tmp);
	WaitForSingleObject(g_event,INFINITE);

	if(g_initialized)
	{
		if(fp_output)g_std_out = fp_output;
		if(fp_error)g_std_err = fp_error;
		g_initialized = TRUE;
		return TRUE;
	}
	else
	{
		TerminateThread(g_initth,NULL);
		CloseHandle(g_initth);
		g_php_module = NULL;
		return FALSE;
	}
}
LPHPAPI int LPHP_DeInit()
{
	if(g_initialized == FALSE && g_preinitialized == FALSE){
		return FALSE;
	}

	ResumeThread(g_initth);
	WaitForSingleObject(g_initth,INFINITE);
	CloseHandle(g_initth);
	CloseHandle(g_event);

	g_std_out = my_std_out;
	g_std_err = my_std_out;
	g_php_module = NULL;
	g_initialized = FALSE;
	g_preinitialized = FALSE;
	DeleteCriticalSection(&g_csection);
	return TRUE;
}

LPHPAPI int LPHP_Initialized()
{
	return g_initialized;
}

LPHPAPI int LPHP_Free(void* ptr)
{
	try{
		g_free(ptr);
		return TRUE;
	}catch(...){
		return FALSE;
	}
}

LPHPAPI int LPHP_RegisterFunction(const char* name,void* fptr,long lp,LPHP_TYPE rval,
								   LPHP_TYPE p1,LPHP_TYPE p2,LPHP_TYPE p3,LPHP_TYPE p4)
{
	cLock(g_csection);

	lphp_funct nf(name, fptr, lp, 
		(rval & 0x03) | ((p1 & 0x03) << 2) | ((p2 & 0x03) << 4) |
		((p3 & 0x03) << 6) | ((p4 & 0x03) << 8));

	if(g_fcns.find(name) != g_fcns.end()){
		return FALSE;
	}else{
		g_fcns[name] = nf;
		return TRUE;
	}
}
LPHPAPI	int LPHP_UnregisterFunction(const char* name)
{
	cLock(g_csection);

	sFCNmap::const_iterator it = g_fcns.find(name);
	if(it != g_fcns.end()){
		
	}
	return TRUE;
}

LPHPAPI int LPHP_ExecutePage(const char* path, const char* querystring, const char** output, void* cparam, LPHP_ENVCB cb, long flags)
{
	cutMemf mf;
	cutMemf* of = &mf;
	std::string ss;

	if(flags & 0x01){
		of = (cutMemf*)output;
	}else if(output && !mf.create(8*1024)){
		return 0;
	}

	if(GO_PhpExecute(path,(output)?(&ss):(NULL),(output)?(of):(NULL),PHPENV_MODE_FILE,querystring,cparam,(PHPENV_CB)cb))
	{
		if(output && !(flags & 0x01)){
			mf.putc('\0');
			mf.write((void*)ss.data(), ss.length());
			mf.putc(0);
			*output = (const char*)mf.leave();
		}
		return TRUE;
	}else{
		return FALSE;
	}
}

LPHPAPI int LPHP_ExecuteDirect(const char* body,const char** output,void* cparam)
{
	cutMemf mf;
	sEPHP ephp={0};
	long result = 0;
	char tmp[32];

	if(output && !mf.create(8*1024)){
		return 0;
	}

	ephp.pszBody = body;
	ephp.pszFile = "[direct source]";
	ephp.c_param = (void*)cparam;
	ephp.pOut = (output)?(&mf):(NULL);
	ephp.cFlags = (output)?(0x01):(0);

	result = GO_PhpExecute2(&ephp);

	if(result){
		if(output){
			mf.putc(0);
			if(ephp.cResType == 0x01){
				mf.writestring(ephp.res.str.val);
				free(ephp.res.str.val);
			}else{
				_snprintf(tmp, sizeof(tmp) - 1, "%d", ephp.res.lval);
				mf.writestring(tmp);
			}
			mf.putc(0);
			*output = (const char*)mf.leave();
		}
		return TRUE;
	}else{
		return FALSE;
	}
}

long LPHP_ExecuteThread(exe_helper* hh)
{
	return (hh->result = GO_PhpExecute2(hh->php));
}

LPHPAPI int LPHP_ExecuteFile(const char* path,const char* funct,const char** output,void* cparam,const char* ptypes,...)
{
	va_list args;
	int result;

	va_start(args,ptypes);
	result = LPHP_ExecuteFileVA(path,funct,output,cparam,ptypes,args);
	va_end(args);
	return result;
}

LPHPAPI int LPHP_ExecuteFileVA(const char* path,const char* funct,const char** output,
								void* cparam,const char* ptypes,va_list args)
{
	cutMemf mf;
	int result = 0;
	char code[MAX_PATH + 64];
	sEPHP ephp={0};

	if(output && !mf.create(1024)){
		return 0;
	}

	_snprintf(code, sizeof(code) - 1, "require_once('%s');\r\n", path);

	ephp.pszFile = path;
	ephp.pszFunction = funct;
	ephp.pszPT = ptypes;
	ephp.pszBody = code;
	ephp.c_param = (void*)cparam;
	ephp.pOut = (output)?(&mf):(NULL);
	ephp.cFlags = (output)?(0x01):(0);
	ephp.pArguments = args;

	result = GO_PhpExecute2(&ephp);

	if(result)
	{
		if(output)
		{
			mf.putc('\0');
			if(ephp.cResType == 0x01){
				mf.writestring(ephp.res.str.val);
				free(ephp.res.str.val);
			}else{
				_snprintf(code, sizeof(code) - 1,"%d",ephp.res.lval);
				mf.writestring(code);
			}
			mf.putc(0);
			*output = (const char*)mf.leave();
		}
		return TRUE;
	}else{
		if(mf.size()){
			g_std_err((const char*)mf.getdata(),mf.size());
		}
		return FALSE;
	}
}

LPHPAPI int LPHP_ExecuteScript(const char* body,const char* funct,const char** output,void* cparam,const char* ptypes,...)
{
	va_list args;
	int result;

	va_start(args,ptypes);
	result = LPHP_ExecuteScriptVA(body,funct,output,cparam,ptypes,args);
	va_end(args);
	return result;
}

LPHPAPI int LPHP_ExecuteScriptVA(const char* body,const char* funct,const char** output,void* cparam,
								  const char* ptypes,va_list args)
{
	cutMemf mf;
	int result = 0;
	sEPHP ephp={0};
	char tmp[32];

	if(output && !mf.create(1024)){
		return 0;
	}

	ephp.pszFile = (funct)?(funct):("[direct source]");
	ephp.pszFunction = funct;
	ephp.pszPT = ptypes;
	ephp.pszBody = body;
	ephp.pOut = (output)?(&mf):(NULL);
	ephp.cFlags = (output)?(0x01):(0);
	ephp.c_param = (void*)cparam;
	ephp.pArguments = args;

	result = GO_PhpExecute2(&ephp);

	if(result)
	{
		if(output)
		{
			mf.putc('\0');
			if(ephp.cResType == 0x01){
				mf.writestring(ephp.res.str.val);
				free(ephp.res.str.val);
			}else{
				
				_snprintf(tmp,sizeof(tmp)-1,"%d",ephp.res.lval);
				mf.writestring(tmp);
			}
			mf.putc(0);
			*output = (const char*)mf.leave();
		}
		return TRUE;
	}else{
		if(mf.size()){
			g_std_err((const char*)mf.getdata(),mf.size());
		}
		return FALSE;
	}
}

LPHPAPI int LPHP_GetVar(const char* name,void** value,SVAR_TYPE* cType)
{
	cLock(g_csection);

	if(!name || !value || !cType){
		return FALSE;
	}

	sVARmap::const_iterator it;
	sVar &sv = *((sVar*)NULL);

	if((it = g_vars.find(name)) == g_vars.end()){
		return FALSE;
	}

	sv = it->second;

	*cType = (SVAR_TYPE)sv.type;

	if(sv.type == SV_STRING){
		*value = (void*)sv.str.val;
	}else if(sv.type == SV_DOUBLE){
		*value = (void*)&sv.dval;
	}else if(sv.type == SV_LONG || sv.type == SV_NULL){
		*value = (void*)sv.lval;
	}else if(sv.type >= 11){
		*value = (void*)&sv.dval;
	}else{
		*value = NULL;
	}
	return TRUE;
}

LPHPAPI int LPHP_DelVar(const char* name)
{
	cLock(g_csection);

	sVARmap::iterator it = g_vars.find(name);
	if(it == g_vars.end() || it->second.locked){
		return FALSE;
	}
	return TRUE;
}

LPHPAPI int LPHP_IsVar(const char* name)
{
	cLock(g_csection);

	return (g_vars.find(name) != g_vars.end());
}

LPHPAPI int LPHP_SetVar(const char* name,void* value,SVAR_TYPE cType)
{
	cLock(g_csection);

	sVar sv;
	void* tmp;
	STR* ss = (STR*)value;
	sVARmap::iterator it = g_vars.find(name);

	if(it == g_vars.end()){
		return FALSE;
	}

	sv.type = cType;

	if(cType == SV_DOUBLE){
		sVariable(&sv,*(double*)value,2);
	}else if((int)cType >= 11){
		tmp = svar_malloc(ss->len);
		if(!tmp){
			return 0;
		}
		memcpy(tmp,ss->val,ss->len);
		sVariable(&sv,cType,tmp,ss->len,2);
	}else{
		sVariable(&sv,cType,value,0,2);
	}
	it->second = sv;

	return TRUE;
}
LPHPAPI int LPHP_NewVar(const char* name,void* value,SVAR_TYPE cType,char locked)
{
	cLock(g_csection);

	sVar sv;
	void* tmp;
	STR* ss = (STR*)value;

	if(cType == SV_DOUBLE){
		sVariable(&sv,*(double*)value,locked);
	}else if((int)cType >= 11){
		tmp = svar_malloc(ss->len);
		if(!tmp){
			return 0;
		}
		memcpy(tmp,ss->val,ss->len);
		sVariable(&sv,cType,tmp,ss->len,(char)locked);
	}else{
		sVariable(&sv,cType,value,0,(char)locked);
	}

	g_vars[name] = sv;
	return TRUE;
}