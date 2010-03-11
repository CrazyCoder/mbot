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

/*name: mb_SysLoadModule
params: $name{S:dll path};
desc: Tries to load a dynamicly linked library (DLL), and returns its handle or 0;
example:
*/
ZEND_FUNCTION(mb_SysLoadModule)
{
	char* name;
	long nl=0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&name,&nl) == FAILURE)
	{
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	RETURN_LONG((long)LoadLibrary(name));
}

/*name: mb_SysUnLoadModule
params: $handle{L:dll handle};
desc: Releases a module loaded with %mb_SysLoadModule;
example:
*/
ZEND_FUNCTION(mb_SysUnLoadModule)
{
	HMODULE hm;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&hm) == FAILURE)
	{
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	RETURN_LONG(FreeLibrary(hm));
}


/*name: mb_SysCallService
params: $name{S:service name}|$wparam{M:WPARAM},$lparam{M:LPARAM};
desc: Calls a miranda service specified by $name, passing up to two parameters. mb_SysCallService accepts up to two parameters (0, 1 or 3) and returns the numeric result;
example:
*/
ZEND_FUNCTION(mb_SysCallService)
{
	zval* lparam=0;
	zval* wparam=0;
	char* name=0;
	void *lp=NULL,*wp=NULL;
	long  nl=0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|z!z!",&name,&nl,&wparam,&lparam) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!nl){
		PHPWSE("You can't call an empty service!");
	}

	if(lparam){
		lp = (lparam->type==IS_STRING)?((void*)lparam->value.str.val):((void*)lparam->value.lval);
	}
	if(wparam){
		wp = (wparam->type==IS_STRING)?((void*)wparam->value.str.val):((void*)wparam->value.lval);
	}

	try{
		nl = CallService(name,(WPARAM)wp,(LPARAM)lp);
	}catch(...){
		RETURN_FALSE;
	}
	RETURN_LONG(nl);
}

ZEND_FUNCTION(mb_SysCallProtoService)
{
	zval* lparam=0;
	zval* wparam=0;
	char* name=0;
	char* proto=0;
	long  nl=0;
	long  pl=0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|z!z!",&proto,&pl,&name,&nl,&wparam,&lparam) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!nl || !pl){
		PHPWSE("Specified $serivce or $protocol name is empty!");
	}

	void* lp = (lparam->type==IS_STRING)?((void*)lparam->value.str.val):((void*)lparam->value.lval);
	void* wp = (lparam->type==IS_STRING)?((void*)wparam->value.str.val):((void*)wparam->value.lval);

	try{
		nl = CallProtoService(proto,name,(WPARAM)wp,(LPARAM)lp);
	}catch(...){
		RETURN_FALSE;
	}
	RETURN_LONG(nl);
}


ZEND_FUNCTION(mb_SysQuit)
{
	PostQuitMessage(0);
}

ZEND_FUNCTION(mb_SysCallContactService)
{
	zval* lparam=0;
	zval* wparam=0;
	char* name=0;
	long  nl=0;
	long  cid=0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lsz!z!",&cid,&name,&nl,&wparam,&lparam) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!nl || !cid){
		PHPWSE("Specified $cid or $service name is empty!");
	}

	void* lp = (lparam->type==IS_STRING)?((void*)lparam->value.str.val):((void*)lparam->value.lval);
	void* wp = (lparam->type==IS_STRING)?((void*)wparam->value.str.val):((void*)wparam->value.lval);

	try{
		nl = CallContactService((HANDLE)cid,name,(WPARAM)wp,(LPARAM)lp);
	}catch(...){
		RETURN_FALSE;
	}
	RETURN_LONG(nl);
}

ZEND_FUNCTION(mb_SysGetString)
{
	long str;
	char* n=0;
	long  len = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|l",&str,&len) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!str){
		PHPWSE("Null pointer given!");
	}
	try{
		if(!len){
			len = strlen((const char*)str);
		}
		n = (char*)emalloc(len+1);
		if(!n){
			RETURN_FALSE;
		}
		memcpy(n,(void*)str,len);
		n[len]='\0';
	}catch(...){
		RETURN_FALSE;
	}
	RETURN_STRINGL(n,len,0);
}

ZEND_FUNCTION(mb_SysGetNumber)
{
	long  ptr;
	long  type = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|l",&ptr,&type) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!ptr){
		PHPWSE("Null pointer given!");
	}

	try
	{
		if(type == 0 || type == 4){
			RETURN_LONG(*((long*)ptr));
		}else if(type == 2){
			RETURN_LONG(*((short*)ptr));
		}else if(type == 1){
			RETURN_LONG(*((char*)ptr));
		}else{
			RETURN_FALSE;
		}
	}catch(...){
		RETURN_FALSE;
	}
}

ZEND_FUNCTION(mb_SysGetPointer)
{
	zval*  ptr;
	long  type = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z",&ptr) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!ptr){
		PHPWSE("Null pointer given!");
	}

	if(ptr->type == IS_STRING){
		RETURN_LONG(((long)ptr->value.str.val));
	}else{
		RETURN_LONG(((long)&ptr->value.lval));
	}
}

ZEND_FUNCTION(mb_SysPutString)
{
	long  ptr=0;
	long  len=0;
	zval* val=0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz|l",&ptr,&val,&len) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!ptr){
		PHPWSE("Null pointer given!");
	}else if(val->type != IS_STRING){
		PHPWSE("$str must be a valid string!");
	}else if(len > val->value.str.len){
		PHPWSE("$str is shorter than you think it is!");
	}

	try{
		if(!len){
			strcpy((char*)ptr,val->value.str.val);
		}else{
			memcpy((void*)ptr,val->value.str.val,len);
		}
		RETURN_TRUE;
	}catch(...){
		RETURN_FALSE;
	}
}

ZEND_FUNCTION(mb_SysPutNumber)
{
	long  ptr;
	long  num;
	long  type = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll|l",&ptr,&num,&type) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!ptr){
		PHPWSE("Null pointer given!");
	}

	try
	{
		if(type == 0 || type == 4){
			*((long*)ptr) = num;
		}else if(type == 2){
			*((short*)ptr) = *((short*)&num);
		}else if(type == 1){
			*((char*)ptr) = *((char*)&num);
		}else{
			RETURN_FALSE;
		}
		RETURN_LONG(num);
	}catch(...){
		RETURN_FALSE;
	}
}

ZEND_FUNCTION(mb_SysMemCpy)
{
	long  p1,p2,amount;
	zval* vp2 = NULL;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lzl",&p1,&vp2,&amount) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!p1 || !vp2){
		PHPWSE("Null pointer given!");
	}else if(amount < 0){
		PHPWSE("You can't copy negative number of bytes!");
	}

	if(vp2->type == IS_STRING){
		p2 = (long)vp2->value.str.val;
		if(!amount){amount = vp2->value.str.len;}
	}else if(vp2->type == IS_LONG){
		p2 = vp2->value.lval;
	}else{
		PHPWSE("Wrong parameter type!");
	}

	try{
		memcpy((void*)p1,(void*)p2,(amount)?(amount):(strlen((const char*)p2)+1));
		RETURN_TRUE;
	}
	catch(...)
	{
		RETURN_FALSE;
	}
}

ZEND_FUNCTION(mb_SysMalloc)
{
	long  amount=0;
	void* ptr;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&amount) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(amount <= 0){
		PHPWSE("You can't allocate nothing or even less than nothing!");
	}

	try{
		ptr = my_malloc(amount);
		if(ptr){memset(ptr,0,amount);}
		RETURN_LONG((long)ptr);
	}catch(...){
		RETURN_FALSE;
	}
}

ZEND_FUNCTION(mb_SysGlobalAlloc)
{
	long  amount=0;
	void* ptr;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&amount) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(amount <= 0){
		PHPWSE("You can't allocate nothing or even less than nothing!");
	}

	try{
		ptr = my_malloc(amount);
		if(ptr){memset(ptr,0,amount);}
		RETURN_LONG((long)ptr);
	}catch(...){
		RETURN_FALSE;
	}
}

ZEND_FUNCTION(mb_SysGlobalFree)
{
	long  ptr;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&ptr) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!ptr){
		PHPWSE("Null pointer given!");
	}
	try{
		my_memfree((void*)ptr);
		RETURN_TRUE;
	}
	catch(...)
	{
		RETURN_FALSE;
	}
}

ZEND_FUNCTION(mb_SysFree)
{
	long  ptr;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",&ptr) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!ptr){
		PHPWSE("Null pointer given!");
	}
	try{
		efree((void*)ptr);
		RETURN_TRUE;
	}
	catch(...)
	{
		RETURN_FALSE;
	}
}

ZEND_FUNCTION(mb_SysGetProcAddr)
{
	char* module=0;
	char* function=0;
	long  ml=0;
	long  fl=0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",&module,&ml,&function,&fl) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	RETURN_LONG(help_getprocaddr(module,function));
}
struct sSysThreadHelper{
	PPHP php;
	char* cb;
	lphp_vparam vp;
	HANDLE hEvent;
};


long WINAPI SysThread(sSysThreadHelper* p)
{
	lphp_vparam vp;
	mb_event mbe = {MBT_CALLBACK,0};
	char cb[32];

	mbe.php = p->php;
	strncpy(cb,p->cb,sizeof(cb)-1);

	//make a copy of vp
	vp = p->vp;

	if(vp.type == LPHP_STRING){
		vp.data = strdup((const char*)vp.data);
	}

	sman_incref(mbe.php);
	SetEvent(p->hEvent);

	MBMultiParam(mbe.php,cb,&mbe,"v",&vp);
	sman_decref(mbe.php);

	if(vp.type == LPHP_STRING){
		my_memfree(vp.data);
	}
	return 0;
}

ZEND_FUNCTION(mb_SysBeginThread)
{
	zval* cb = NULL;
	zval* param = NULL;
	HANDLE hThread;
	DWORD  dwID;
	sSysThreadHelper shp;
	mb_event*  mbe = (mb_event*)((sPHPENV*)SG(server_context))->c_param;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz",&cb,&param) == FAILURE)
	{
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!mbe->php){
		PHPWSE("You can't register a service function here!");
	}

	if(cb->type != IS_STRING || !zend_is_callable(cb,0,NULL)){
		PHPWSE("$cb is expected to be a valid callback function!");
	}

	if(param->type == IS_STRING)
	{
		shp.vp.type = LPHP_STRING;
		shp.vp.data = (void*)param->value.str.val;
		shp.vp.length = param->value.str.len;
	}else{
		shp.vp.type = LPHP_NUMBER;
		shp.vp.data = (void*)param->value.lval;
		shp.vp.length = 4;
	}

	shp.php = mbe->php;
	shp.cb = cb->value.str.val;
	shp.hEvent = CreateEvent(0,0,0,0);
	if(shp.hEvent == INVALID_HANDLE_VALUE){
		RETURN_FALSE;
	}

	hThread = CreateThread(NULL,NULL,(LPTHREAD_START_ROUTINE)SysThread,(void*)&shp,0,&dwID);
	if(hThread){
		CloseHandle(hThread);
		WaitForSingleObject(shp.hEvent,INFINITE);
		CloseHandle(shp.hEvent);
		RETURN_LONG(dwID);
	}else{
		CloseHandle(shp.hEvent);
		RETURN_FALSE;
	}
}

ZEND_FUNCTION(mb_SysCallProc)
{
	extern FILE* dbgout;
	void* p_ebp;
	void* p_esp;
	void* p_res = NULL;

	long  fcn=0;
	long  argc;
	zval* params[10]={0};
	void* vp[10]={0};

	argc = ZEND_NUM_ARGS();

	if(argc < 2){
		WRONG_PARAM_COUNT;
	}

	if(zend_parse_parameters(argc TSRMLS_CC, "l|z!z!z!z!z!z!z!z!z!z!",&fcn,
		&params[0],&params[1],&params[2],&params[3],&params[4],&params[5],&params[6],&params[7],&params[8],&params[9]) == FAILURE)
	{
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!fcn){
		PHPWSE("Null function pointer given!");
	}

	__asm{
		mov p_ebp, ebp;
		mov p_esp, esp;
	}

	argc --;

	//fprintf(dbgout,"0x%.8x(",fcn);
	for(int i=0;i<argc;i++)
	{
		if(!params[i]){
			vp[i] = NULL;
		}else if(params[i]->type == IS_STRING){
			vp[i] = (void*)params[i]->value.str.val;
		}else{
			vp[i] = (void*)params[i]->value.lval;
		}

		//fprintf(dbgout,"%d,",vp[i]);
	}
	//fprintf(dbgout,");\r\n");
	//fflush(dbgout);

	try{
		//put the params
		p_res = (void*)vp;
		__asm{
			mov ecx, argc
			mov ebx, ecx
			shl ebx, 2d
			sub ebx, 4d
			add ebx, p_res
PETLA:
			push DWORD PTR [ebx]
			sub ebx, 4d
			dec ecx
			jnz PETLA
			//call the function
			mov ebx, fcn
			xor eax, eax
			call ebx
			mov p_res, eax
			//cleanup
			mov ebp, p_ebp;
			mov esp, p_esp;
		}
	}catch(...){
		__asm{
			mov ebp, p_ebp;
			mov esp, p_esp;
		}
		PHP_WARN "Execution failed :!");

	RETURN_FALSE;
	}

	RETURN_LONG((long)p_res);
}

ZEND_FUNCTION(mb_SysCreateService)
{
	extern CSyncList g_svlist;

	zval* name = NULL;
	zval* cb = NULL;
	sHESync* hfs = NULL;
	mb_event*  mbe = (mb_event*)((sPHPENV*)SG(server_context))->c_param;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz",&name,&cb) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!mbe->php){
		PHPWSE("You can't register a service function here!");
	}

	if(name->type != IS_STRING || name->value.str.len < 5){
		PHPWSE("$name must be a valid string of 5-47 characters!");
	}

	if(!zend_is_callable(cb,0,NULL)){
		PHPWSE("$cb is expected to be a valid callback function!");
	}

	hfs = (sHESync*)my_malloc(sizeof(sHESync));
	if(!hfs){
		PHPWSE("Could not allocate memory!");
	}
	memset(hfs,0,sizeof(sHESync));

	hfs->pCode = help_makefunct((void*)hfs,(void*)help_callsvc);
	if(!hfs->pCode){
		my_memfree((void*)hfs);
		PHPWSE("Could not create the machine code!");
	}

	sman_incref(mbe->php);
	hfs->php = mbe->php;

	strncpy(hfs->pszFunction,cb->value.str.val,sizeof(hfs->pszFunction)-1);
	strncpy(hfs->pszSvcName,name->value.str.val,sizeof(hfs->pszSvcName)-1);
	hfs->hFunction = (long)CreateServiceFunction(hfs->pszSvcName,(MIRANDASERVICE)hfs->pCode);
	if(!hfs->hFunction){
		my_memfree((void*)hfs->pCode);
		my_memfree((void*)hfs);
		sman_decref(mbe->php);
		PHPWSE("Could not create service function!");
	}
	g_svlist.Add(hfs);

	RETURN_LONG(hfs->hFunction);
}

ZEND_FUNCTION(mb_SysHookEvent)
{
	extern CSyncList g_svlist;

	zval* name = NULL;
	zval* cb = NULL;
	sHESync* hfs = NULL;
	mb_event*  mbe = (mb_event*)((sPHPENV*)SG(server_context))->c_param;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz",&name,&cb) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!mbe->php){
		PHPWSE("You can't register a service function here!");
	}

	if(name->type != IS_STRING || name->value.str.len < 1){
		PHPWSE("$name must be a valid string of 1-47 characters!");
	}

	if(!zend_is_callable(cb,0,NULL)){
		PHPWSE("$cb is expected to be a valid callback function!");
	}

	hfs = (sHESync*)my_malloc(sizeof(sHESync));
	if(!hfs){
		PHPWSE("Could not allocate memory!");
	}
	memset(hfs,0,sizeof(sHESync));

	hfs->pCode = help_makefunct((void*)hfs,(void*)help_callsvc);
	if(!hfs->pCode){
		my_memfree((void*)hfs);
		PHPWSE("Could not create the machine code!");
	}

	sman_incref(mbe->php);
	hfs->php = mbe->php;

	strncpy(hfs->pszFunction,cb->value.str.val,sizeof(hfs->pszFunction)-1);
	hfs->hHook = (long)HookEvent(name->value.str.val,(MIRANDAHOOK)hfs->pCode);

	if(!hfs->hHook){
		my_memfree((void*)hfs->pCode);
		my_memfree((void*)hfs);
		sman_decref(mbe->php);
		PHPWSE("Could not create service function!");
	}
	g_svlist.Add(hfs);

	RETURN_LONG(hfs->hFunction);
}

ZEND_FUNCTION(mb_SysEnumProtocols)
{
	PROTOCOLDESCRIPTOR **proto;
	int protoCount;

	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&protoCount,(LPARAM)&proto);
	if(!protoCount || array_init(return_value)==FAILURE){
		RETURN_FALSE;
	}

	for(int i=0;i<protoCount;i++)
	{
		if(proto[i]->type != PROTOTYPE_PROTOCOL) continue;
		add_index_string(return_value,i,proto[i]->szName,1);
	}
	return;
}
int xx_enummodules(const char *szModuleName,DWORD ofsModuleName,LPARAM lParam)
{
	cutMemf* mf = (cutMemf*)lParam;
	if(*szModuleName){
		mf->write((void*)szModuleName, strlen(szModuleName) + 1);
	}
	return 0;
}
ZEND_FUNCTION(mb_SysEnumModules)
{
	cutMemf mf;
	long ml=0;
	char* mod=NULL;

	if(!mf.create(2048)){
		RETURN_FALSE;
	}

	if(CallService(MS_DB_MODULES_ENUM,(WPARAM)&mf,(LPARAM)xx_enummodules)!=0){
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
}

ZEND_FUNCTION(mb_SysGetProfileName)
{
	RETURN_STRING(g_profile,1);
}

ZEND_FUNCTION(mb_SysTranslate)
{
	char* str = NULL;
	long  sl = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&str,&sl) == FAILURE){
		PHP_FALSE_AND_ERROR;
	}

	str = (char*)CallService(MS_LANGPACK_TRANSLATESTRING,0,(LPARAM)str);
	RETURN_STRING((str)?(str):(""),1);
}

ZEND_FUNCTION(mb_SysGetMirandaDir)
{
	RETURN_STRING(g_root,1);
}