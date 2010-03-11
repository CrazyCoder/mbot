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
#include "../smanager.h"
#include "../cron.h"

ZEND_FUNCTION(mb_SelfRegister)
{
	long sl=0,event=0,id=0,priority=0x7fffffff;
	char cache = 0;
	PPHP php;
	PHANDLER ph;
	mb_event*  mbe = (mb_event*)((sPHPENV*)SG(server_context))->c_param;

	if(mbe->event != MBT_AUTOLOAD || zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|bl",&event,&cache,&priority) == FAILURE){
		PHP_FALSE_AND_WARNS("Invalid parameters given or this function is not allowed now!");
	}

	if(DBGetContactSettingByte(NULL,MBOT,"NoCache",0)){
		cache = 0;
	}

	if(php = sman_getbyfile((const char*)mbe->p3))
	{
		if(mbe->p2 != NULL){
			sman_recache(php);
		}
	}else{
		php = sman_register((const char*)mbe->p3,cache);
		if(!php){
			RETURN_FALSE;
		}
	}

	//count bytes
	for(sl=0;sl<32;sl++)
	{
		if(event & (1 << sl))
		{
			ph = sman_handler_add(php,event & (1 << sl),priority & 0x7fffffff,(cache)?(MBOT_FLAG_CACHE):(0));
			if(ph){
				id++;
			}
		}
	}

	if(!id){
		sman_unregister(php);
	}else{
		lh_flags |= event;
		mbe->php = php;
	}
	RETURN_LONG(id);
}

ZEND_FUNCTION(mb_SelfSetInfo)
{
	char* desc = NULL;
	long  dl = 0;
	mb_event*  mbe = (mb_event*)((sPHPENV*)SG(server_context))->c_param;

	if(!mbe->php || zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&desc,&dl) == FAILURE){
		PHP_FALSE_AND_WARNS("Script is not registered yet, or wrong parameters given!");
	}

	if(mbe->php->szDescription){
		my_memfree(mbe->php->szDescription);
	}
	mbe->php->szDescription = my_strdup(desc);
	RETURN_LONG(mbe->php->szDescription != NULL);
}

ZEND_FUNCTION(mb_SelfEnable)
{
	long event;
	char enable = 0;
	PHANDLER ph;
	mb_event*  mbe = (mb_event*)((sPHPENV*)SG(server_context))->c_param;

	if(!mbe->php || zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lb",&event,&enable) == FAILURE){
		PHP_FALSE_AND_WARNS("Invalid parameters given or this function is not allowed now!");
	}

	ph = sman_handler_find(mbe->php,event);
	if(!ph){
		PHPWSE("Event isn't registered!");
	}else{
		if(enable){
			sman_handler_enable(ph);
		}else{
			sman_handler_disable(ph);
		}
		RETURN_TRUE;
	}
}

ZEND_FUNCTION(mb_SchReg)
{
	extern CSyncList g_cron;
	char *cron=NULL,*funct=NULL,*name=NULL;
	long cl=0,sl=0,fl=0,nl=0;
	char async=0;
	char cache=0;
	sCronSync* csn = 0;
	sCronSync cs;

	sPHPENV* ctx = (sPHPENV*)SG(server_context);
	mb_event* mbe = (mb_event*)(ctx->c_param);

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|sbb",&name,&nl,&cron,&cl,&funct,
		&fl,&cache,&async) == FAILURE || !cl || !fl || !nl)
	{
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	//parse cron
	if(!cron_parse(cron,&cs.ce)){
		PHP_FALSE_AND_WARNS("You have an error in scheduler syntax!");
	}
	strncpy(cs.name,name,sizeof(cs.name)-1);
	strncpy(cs.fcn,funct,sizeof(cs.fcn)-1);

	if(mbe->event == MBT_AUTOLOAD){
		mbe->php = sman_register((const char*)mbe->p3,(cache!=0));
		if(!mbe->php){RETURN_FALSE;}
	}else if(mbe->php == NULL){
		RETURN_FALSE;
	}else{
		sman_incref(mbe->php);
	}

	if(async){
		cs.lFlags |= MBOT_FLAG_ASYNC;
	}

	csn = (sCronSync*)my_malloc(sizeof(sCronSync));
	if(!csn){
		goto Error;
	}

	*csn = cs;
	csn->data = mbe->php;
	
	if(!cron_register(csn)){
		goto Error;
	}
	cron_calcnext(&csn->ce);

	RETURN_LONG(1);
	//error
Error:
	sman_decref(mbe->php);
	if(csn)my_memfree((void*)csn);
	RETURN_FALSE;
}

ZEND_FUNCTION(mb_SchUnreg)
{
	char* name = NULL;
	long  nl = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&name,&nl) == FAILURE || !nl){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	RETURN_LONG(cron_unregister(name));
}

ZEND_FUNCTION(mb_SchEnable)
{
	char* name = NULL;
	long  nl = 0;
	zend_bool enable = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sb",&name,&nl,&enable) == FAILURE || !nl){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	RETURN_LONG(cron_enable(name,enable));
}

ZEND_FUNCTION(mb_SchModify)
{
	char* name = NULL;
	char* csl = NULL;
	long  nl = 0;
	zend_bool enable = 0;
	sCronSync cs;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",&name,&nl,&csl,&nl) == FAILURE || !nl){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	RETURN_LONG(cron_modify(name,cs.ce));
}

ZEND_FUNCTION(mb_SchList)
{
	extern CSyncList g_cron;

	zval	*fname;
	char	*name;
	zval	*argv[4] = {0};
	zval	**args[4] = {0};
	zval	*rv = NULL;
	long    result = 1;
	sCronSync *e;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z",&fname) == FAILURE || fname->type != IS_STRING){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!zend_is_callable(fname, 0, &name))
	{
		php_error_docref1(NULL TSRMLS_CC, name, E_WARNING, "First argument is expected to be a valid callback");
		efree(name);
		RETURN_FALSE;
	}

	MAKE_STD_ZVAL(argv[0]);
	MAKE_STD_ZVAL(argv[1]);
	MAKE_STD_ZVAL(argv[2]);
	MAKE_STD_ZVAL(argv[3]);

	args[0] = &argv[0];
	args[1] = &argv[1];
	args[2] = &argv[2];
	args[3] = &argv[3];

	g_cron.Lock();
	e = (sCronSync*)g_cron.m_head;
	while(e)
	{
		if(!(e->lFlags & MBOT_FLAG_DELETE))
		{
			ZVAL_STRING(argv[0],(char*)e->name,0);

			if(e->lFlags & MBOT_FLAG_INACTIVE){
				ZVAL_LONG(argv[1],0);
			}else{
				ZVAL_LONG(argv[1],e->lTime);
			}
			ZVAL_LONG(argv[2],e->lLastSpent);
			ZVAL_LONG(argv[3],e->lSpent);

			if(call_user_function_ex(CG(function_table),NULL,fname,&rv,4,args,0, NULL TSRMLS_CC) != SUCCESS){
				result = 0;
				break;
			}
		}
		e = (sCronSync*)e->next;
	}
	g_cron.Unlock();

	efree(name);
	FREE_ZVAL(argv[0]);
	FREE_ZVAL(argv[1]);
	FREE_ZVAL(argv[2]);
	FREE_ZVAL(argv[3]);
	RETURN_LONG(result);
}

ZEND_FUNCTION(mb_SysShallDie)
{
	extern CRITICAL_SECTION sm_sync;

	char* script = NULL;
	long  sl = 0;
	PPHP php;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s",&script,&sl) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!sl){
		php = sman_getbyfile(script);
	}else{
		mb_event*  mbe = (mb_event*)((sPHPENV*)SG(server_context))->c_param;
		php = mbe->php;
	}

	if(!php){
		RETURN_FALSE;
	}else{
		sman_lock();
		sl = (php->lFlags & MBOT_FLAG_SHALLDIE | MBOT_FLAG_INACTIVE)==0;
		sman_unlock();
	}
	RETURN_LONG(sl);
}

ZEND_FUNCTION(mb_SysManageScript)
{
	extern CRITICAL_SECTION sm_sync;

	char* path=NULL;
	long  action,pl;
	long  events = 0x0FffFFff;
	PPHP php;
	PHANDLER ph;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl|l",&path,&pl,&action,&events) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	php = sman_getbyfile(path);

	if(!php && action<7)goto Error;
	switch(action)
	{
	case 0://disable
		for(int i=0;i<32;i++){
			if(events & (1 << i)){
				ph = sman_handler_find(php,1 << i);
				if(ph)sman_handler_disable(ph);
			}
		}
		events = TRUE;
		break;
	case 1://enable
		for(int i=0;i<32;i++){
			if(events & (1 << i)){
				ph = sman_handler_find(php,1 << i);
				if(ph)sman_handler_enable(ph);
			}
		}
		events = TRUE;
		break;
	case 2://cache/recache
		events = sman_recache(php);
		break;
	case 3://uncache
		events = sman_uncache(php);
		break;
	case 4://uninstall/unload
		events = sman_uninstall(php,events == 1);
		break;
	case 5://enable file
		events = help_enable_script(php,true);
		break;
	case 6://disable file
		events = help_enable_script(php,false);
		break;
	case 7://install script
		events = help_loadscript(path,0);
		break;
	case 8://is registered
		events = php != NULL;
		break;
	default:
		goto Error;
	}
	RETURN_TRUE;
Error:
	RETURN_FALSE;
}

ZEND_FUNCTION(mb_SysEnumHandlers)
{
	zval	*fname;
	char	*name;
	zval	*argv[4] = {0};
	zval	**args[4] = {0};
	zval	*rv = NULL;
	long    result = 1;
	long	count = 0;
	PHANDLER ph;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z",&fname) == FAILURE || fname->type != IS_STRING){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!zend_is_callable(fname, 0, &name))
	{
		php_error_docref1(NULL TSRMLS_CC, name, E_WARNING, "First argument is expected to be a valid callback");
		efree(name);
		RETURN_FALSE;
	}

	MAKE_STD_ZVAL(argv[0]);
	MAKE_STD_ZVAL(argv[1]);
	MAKE_STD_ZVAL(argv[2]);
	MAKE_STD_ZVAL(argv[3]);

	args[0] = &argv[0];
	args[1] = &argv[1];
	args[2] = &argv[2];
	args[3] = &argv[3];

	for(int i=0;i<32;i++)
	{
		ph = sman_handler_get(1 << i);

		while(ph)
		{
			ZVAL_LONG(argv[0],(long)ph);
			ZVAL_STRING(argv[1],(char*)ph->php->szFilePath,0);
			ZVAL_LONG(argv[2],(1 << i));
			ZVAL_LONG(argv[3],ph->lFlags);

			if(call_user_function_ex(CG(function_table),NULL,fname,&rv,4,args,0, NULL TSRMLS_CC) != SUCCESS){
				result = 0;
				goto End;
			}
			ph = ph->next;
		}
	}
End:
	efree(name);
	FREE_ZVAL(argv[0]);
	FREE_ZVAL(argv[1]);
	FREE_ZVAL(argv[2]);
	FREE_ZVAL(argv[3]);
	RETURN_LONG(result);
}