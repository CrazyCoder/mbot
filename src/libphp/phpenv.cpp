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
#include "libphp.h"
#include "internals.h"
#include "svar.h"
#include "phpenv.h"

#define UH(s) (unsigned char*)(s)
#define PHPWS php_error_docref(NULL TSRMLS_CC,E_WARNING,
#define PHPWE );return
#define PHPWSE(s) php_error_docref(NULL TSRMLS_CC,E_WARNING,s);return

void	php_phpenv_sapi_error(int type, const char *fmt, ...);
int		php_phpenv_ub_write(const char *str, unsigned int str_length TSRMLS_DC);
void	php_phpenv_log_message(char *message);
int		php_phpenv_startup(sapi_module_struct *sapi_module);
char*	php_phpenv_read_cookies(TSRMLS_D);
void	php_phpenv_send_header(sapi_header_struct *sapi_header, void *server_context TSRMLS_DC);
void	php_phpenv_flush(void *server_context);
int		php_phpenv_read_post(char *buffer, uint count_bytes TSRMLS_DC);
int		php_phpenv_deactivate(TSRMLS_D);
char* 	php_phpenv_getenv(char *name, size_t name_len TSRMLS_DC);
void	php_phpenv_register_variables(zval *track_vars_array TSRMLS_DC);
int		php_set_ini_entry(char *entry, char *value, int stage);
int		php_update_ini_file(TSRMLS_D);

/////////////////////////////////////
//GLOBALS
/////////////////////////////////////
extern  LPHP_FREE		 g_free;
extern  LPHP_MALLOC		 g_malloc;
extern  LPHP_OUTPUT		 g_std_out;
extern  LPHP_OUTPUT		 g_std_err;
extern  CRITICAL_SECTION g_csection;

extern  sVARmap g_vars;
extern  sFCNmap g_fcns;
sTHRlst g_php_threads;

unsigned long	g_stat_num_executions = 0;
unsigned long	g_stat_num_errors = 0;
unsigned long	g_stat_num_threads = 0;

unsigned long	g_tls_key = 0;

zend_function_entry* g_zend_functions = NULL;

/* declaration of functions to be exported */
ZEND_FUNCTION(mt_echo);
ZEND_FUNCTION(mt_clock);
ZEND_FUNCTION(mt_getvar);
ZEND_FUNCTION(mt_delvar);
ZEND_FUNCTION(mt_setvar);
ZEND_FUNCTION(mt_isvar);
ZEND_FUNCTION(mt_call);
PHP_MINIT_FUNCTION(mt_module_entry);

/* compiled function list so Zend knows what's in this module */
zend_function_entry mt_functions[] =
{
    PHP_FE(mt_echo,NULL)// UH("s")) //ok
	PHP_FE(mt_clock,NULL)// UH(""))//ok
	PHP_FE(mt_getvar,NULL)// UH("s"))
	PHP_FE(mt_delvar,NULL)// UH("s"))
	PHP_FE(mt_setvar,NULL)// UH("ssl"))
	PHP_FE(mt_isvar,NULL)
	PHP_FE(mt_call,NULL)// UH("s|ssss"))
	////////////////////////////////
    {NULL, NULL, NULL}
};

/* compiled module information */
static zend_module_entry mt_module_entry =
{
    STANDARD_MODULE_HEADER,
    "libphp (www.piopawlu.net)",
    mt_functions,
    PHP_MINIT(mt_module_entry),
	NULL,
	NULL,
	NULL,
	NULL,
    NO_VERSION_YET,
    STANDARD_MODULE_PROPERTIES
};

PHP_MINIT_FUNCTION(mt_module_entry)
{
	if(g_php_module && ((zend_module_entry*)g_php_module)->module_startup_func)
	{
		((zend_module_entry*)g_php_module)->module_startup_func(INIT_FUNC_ARGS_PASSTHRU);
	}
	return SUCCESS;
}

static sapi_module_struct phpenv_sapi_module = {
	"libphp 1.0.0.8",				/* name */
	"libPHP",						/* pretty name */

	php_phpenv_startup,				/* startup */
	php_module_shutdown_wrapper,	/* shutdown */

	NULL,							/* activate */
	php_phpenv_deactivate,			/* deactivate */

	php_phpenv_ub_write,			/* unbuffered write */
	php_phpenv_flush,				/* flush */
	NULL,							/* get uid */
	php_phpenv_getenv,				/* getenv */

	php_phpenv_sapi_error,			/* error handler */

	NULL,							/* header handler */
	NULL,							/* send headers handler */
	php_phpenv_send_header,			/* send header handler */

	php_phpenv_read_post,			/* read POST data */
	php_phpenv_read_cookies,		/* read Cookies */

	php_phpenv_register_variables,	/* register server variables */
	php_phpenv_log_message,			/* Log message */

	NULL,							/* Block interruptions */
	NULL,							/* Unblock interruptions */

	STANDARD_SAPI_MODULE_PROPERTIES
};

ZEND_FUNCTION(mt_echo)
{
    char* str;
	long  strl;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &str,&strl) == FAILURE){
		PHPWSE("mt_echo takes exactly one string parameter!");
	}

	g_std_out(str,strl);
    RETURN_LONG(1);
}

ZEND_FUNCTION(mt_clock)
{
	RETURN_LONG(clock());
}

ZEND_FUNCTION(mt_call)
{
	char *cmd=NULL, *par1=NULL, *par2=NULL, *par3=NULL, *par4=NULL, *tmp=NULL;
	long cmd_l=0, par1_l=0, par2_l=0, par3_l=0, par4_l=0;
	long np = 0;
	long rt = 0;
	void* rv = NULL;
	const lphp_funct* f;
	sFCNmap::const_iterator it;

	#define xtol(a) strtol(a,NULL,0)

	sPHPENV* ctx = (sPHPENV*)SG(server_context);

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|ssss",
		&cmd,&cmd_l,&par1,&par1_l,&par2,&par2_l,&par3,&par3_l,&par4,&par4_l) == FAILURE || !cmd_l || strlen(cmd)<1)
	{
		PHPWSE("Not enough parameters given!");
		return;
	}

	it = g_fcns.find(cmd);
	if(it == g_fcns.end() || !it->second.ptr){
		PHPWS "Function %s is not defined!", cmd PHPWE;
		return;
	}
	f = &it->second;

	rt = f->pinfo & 0x03;

	try
	{
		switch(f->lp)
		{
			case 0:
				{
					LPHP_GEN0 g0 = (LPHP_GEN0)f->ptr;
					rv = g0(ctx->c_param);
					break;
				}
			case 1:
				{
					LPHP_GEN1 g1 = (LPHP_GEN1)f->ptr;
					rv = g1(ctx->c_param,(f->pinfo & 0x0C)?((void*)par1):((void*)xtol(par1)));
					break;
				}
			case 2:
				{
					LPHP_GEN2 g2 = (LPHP_GEN2)f->ptr;
					rv = g2(ctx->c_param,(f->pinfo & 0x0C)?((void*)par1):((void*)xtol(par1)),
							(f->pinfo & 0x30)?((void*)par2):((void*)xtol(par2)));
					break;
				}
			case 3:
				{
					LPHP_GEN3 g3 = (LPHP_GEN3)f->ptr;
					rv = g3(ctx->c_param,(f->pinfo & 0x0C)?((void*)par1):((void*)xtol(par1)),
							(f->pinfo & 0x30)?((void*)par2):((void*)xtol(par2)),
							(f->pinfo & 0xC0)?((void*)par3):((void*)xtol(par3)));
					break;
				}
			case 4:
				{
					LPHP_GEN4 g4 = (LPHP_GEN4)f->ptr;
					rv = g4(ctx->c_param,(f->pinfo & 0x000C)?((void*)par1):((void*)atol(par1)),
							(f->pinfo & 0x0030)?((void*)par2):((void*)xtol(par2)),
							(f->pinfo & 0x00C0)?((void*)par3):((void*)xtol(par3)),
							(f->pinfo & 0x0300)?((void*)par4):((void*)xtol(par4)));
					break;
				}
			default:
				PHPWS "You can't call a function taking %u parameters!", f->lp PHPWE;
				return;
		}

		if(rt == 0){
			RETURN_LONG((long)rv);
		}else if(rt == 1){
			if(rv){
				tmp = estrdup((const char*)rv);
				g_free((void*)rv);
				RETURN_STRING(tmp,FALSE);
			}else{
				RETURN_FALSE;
			}
		}else{
			RETURN_LONG(1);
		}
	}
	catch(...)
	{
		PHPWS "An exception occured while executing the %s function!", cmd PHPWE;
		return;
	}
}


int return_var(zval* out,unsigned char* data)
{
	int kt;
	char* key;
	char* str;
	double* dbl;
	ulong nk=0;
	ulong* tmp;
	zval* val;
	int toff = 0;
	unsigned char* od = data;

	while(*data != 'X')
	{
		val = NULL;
		if(*data == '%'){
			kt = 0;
			data++;
		}else if(*data == '>'){
			kt = HASH_KEY_IS_STRING;
			tmp = (ulong*)(++data);
			key = (char*)data + 4;
			data += *tmp + 4;
		}else{
			kt = HASH_KEY_IS_LONG;
			tmp = (ulong*)(++data);
			nk = *tmp;
			data += 4;
		}

		val = NULL;
		MAKE_STD_ZVAL(val);

		switch(*data++)
		{
			case 'S':
				tmp = (ulong*)(data);
				data += 4;
				str = (char*)data;
				ZVAL_STRINGL(val,str,*tmp,1);
				data += *tmp;
				break;
			case 'L':
				tmp = (ulong*)(data);
				data += 4;
				ZVAL_LONG(val,*tmp);
				break;
			case 'D':
				dbl = (double*)(data);
				data += 8;
				ZVAL_DOUBLE(val,*dbl);
				break;
			case 'B':
				ZVAL_BOOL(val,*data++);
				break;
			case 'N':
				ZVAL_NULL(val);
				break;
			case 'A':
				if(array_init(val)==FAILURE || !(toff = return_var(val,data))){
					return 0;
				}else{
					data += toff;
				}
				break;
			default:
				return 0;
		}//switch
		if(kt == HASH_KEY_IS_STRING){
			add_assoc_zval(out,key,val);
		}else if(kt == HASH_KEY_IS_LONG){
			add_index_zval(out,nk,val);
		}else{
			add_next_index_zval(out,val);
		}
	}data++;

	return (data - od);
}

ZEND_FUNCTION(mt_getvar)
{
	cLock(g_csection);

	char*	vname = NULL;
	long	vnl = 0;
	const sVar*	sv = NULL;
	sVARmap::const_iterator it;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&vname,&vnl) == FAILURE){
		PHPWSE("You've not provided the numeber of parameters the function needs!");
		return;
	}

	it = g_vars.find(vname);

	if(it == g_vars.end()){
		PHPWS "Variable %s is not defined!", vname PHPWE;
	}

	sv = &it->second;

	if(sv->type == SV_LONG || sv->type == SV_WORD){
		RETVAL_LONG(sv->lval);
	}else if(sv->type == SV_LPOINTER){
		RETVAL_LONG(*((long*)sv->lval));
	}else if(sv->type == SV_DOUBLE){
		RETVAL_DOUBLE(sv->dval);
	}else if(sv->type == SV_STRING){
		RETVAL_STRING(sv->str.val,1);
	}else if(sv->type == SV_ARRAY){
		if(array_init(return_value)==FAILURE){
			PHPWSE("Could not initialize array!");
		}
		return_var(return_value,(unsigned char*)(sv->str.val + 1));
	}else if(sv->type == SV_NULL){
		RETVAL_NULL();
		return;
	}else{
		PHPWS "Unknown variable type %u!", sv->type PHPWE;
		RETURN_FALSE;
	}
}

ZEND_FUNCTION(mt_delvar)
{
	char*	vname = NULL;
	long	vnl = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&vname,&vnl) == FAILURE || !vnl){
		PHPWSE("No parameters given!");
	}

	if(LPHP_DelVar(vname)){
		RETURN_LONG(1);
	}else{
		PHPWS "Variable %s wasn't defined!", vname PHPWE;
	}
}

ZEND_FUNCTION(mt_isvar)
{
	char*	vname = NULL;
	long	vnl = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&vname,&vnl) == FAILURE || !vnl){
		PHPWSE("No parameters given!");
	}

	RETURN_LONG(LPHP_IsVar(vname));
}

int make_var(zval* v,cutMemf* mf,bool keep_idx)
{
	unsigned long t1;
	unsigned long t2;

	if(v->type == IS_STRING){
		t1 = v->value.str.len;
		mf->putc('S');
		mf->write(&t1,4);
		mf->write(v->value.str.val,t1);
	}else if(v->type == IS_LONG){
		mf->putc('L');
		mf->write(&v->value.lval,4);
	}else if(v->type == IS_DOUBLE){
		mf->putc('D');
		mf->write(&v->value.dval,8);
	}else if(v->type == IS_BOOL){
		mf->putc('B');
		mf->write(&v->value.lval,1);
	}else if(v->type == IS_NULL){
		mf->putc('N');
	}else if(v->type == IS_ARRAY){
		HashPosition pos;
		zval** aitem;
		zval*  aval;
		uint str_len;
		char *str;
		ulong num_key = 0;

		mf->putc('A');

		zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(v),&pos);
		while(zend_hash_get_current_data_ex(Z_ARRVAL_P(v),(void**)&aitem,&pos) == SUCCESS)
		{
			aval = *aitem;
			if(!aval)return 0;

			if(!keep_idx){
				mf->putc('%');
			}else{
				t2 = zend_hash_get_current_key_ex(Z_ARRVAL_P(v),&str,&str_len,&num_key,0,&pos);
				if(t2 == HASH_KEY_IS_STRING){
					mf->putc('>');
					str_len++;
					mf->write(&str_len,4);
					mf->write(str,str_len-1);
					mf->putc('\0');
				}else{
					mf->putc('@');
					mf->write(&num_key,4);
				}
			}

			make_var(aval,mf,keep_idx);
			zend_hash_move_forward_ex(Z_ARRVAL_P(v), &pos);
		}
		mf->putc('X');
	}else{
		return 1;
	}
	return 1;
}

ZEND_FUNCTION(mt_setvar)
{
	char* vname = NULL;
	long  vnl = 0;
	zval* val;
	void* mem;
	zend_bool create = 1;
	zend_bool keep_idx = 0;
	cutMemf mf;
	sVar sv;
	sVARmap::iterator it;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|bb",&vname,&vnl,&val,&create,&keep_idx) == FAILURE){
		PHPWSE("Not enough parameters given!");
		return;
	}

	if(!create && (it = g_vars.find(vname)) == g_vars.end()){
		RETURN_LONG(0);
	}

	if(val->type == IS_STRING){
		sVariable(&sv,SV_STRING,(char*)val->value.str.val,0,0);
	}else if(val->type == IS_DOUBLE){
		sVariable(&sv,val->value.dval,0);
	}else if(val->type == IS_LONG || val->type == IS_BOOL){
		sVariable(&sv,SV_LONG,(void*)(val->value.lval),0,0);
	}else if(val->type == IS_ARRAY){
		if(!mf.create(512) || !make_var(val,&mf,keep_idx!=0)){
			PHPWSE("Could not create variable!");
		}
		mf.putc(0);
		mem = svar_malloc(vnl = mf.size());
		if(!mem){
			mf.close();
			PHPWSE("Could not allocate memory!");
		}else{
			memcpy(mem,mf.getdata(),vnl);
			mf.close();
			sVariable(&sv,SV_ARRAY,mem,vnl,0);
		}
	}else if(val->type == IS_NULL){
		sVariable(&sv,SV_NULL,0,0,0);
	}else{
		PHPWSE("Unsupported variable type!");
	}

	if(!create){
		it->second = sv;
	}else{
		g_vars[vname] = sv;
		vnl = 1;
	}

	if(vnl){
		RETURN_LONG(1);
	}else{
		svar_freevar(&sv);
	}
	PHPWSE("Could not put the variable into the bucket!");
}
////////////////////////////////
//GO_PHPSCRIPT
////////////////////////////////

int GO_EvalString(sEPHP* ephp TSRMLS_DC)
{
	zval pv;
	zval fn;
	zval *rv = NULL;
	zval **args[16] = {NULL};
	zval *argv[16] = {0};
	zval *local_retval_ptr = NULL;
	zend_op_array *new_op_array;
	int retval = FAILURE;
	const char* pt = ephp->pszPT;
	long pc = (!pt)?(0):(strlen(pt));

	zend_op_array *original_active_op_array = EG(active_op_array);
	zend_function_state *original_function_state_ptr = EG(function_state_ptr);
	zend_uchar original_handle_op_arrays;
	zval **original_return_value_ptr_ptr = NULL;
	zend_op **original_opline_ptr = NULL;

	if(pc > 16){
		return FAILURE;
	}else if(pt){
		for(int i=0;i<pc;i++,pt++)
		{
			MAKE_STD_ZVAL(argv[i]);
			switch(*pt)
			{
				case 's':
					{
						char* tmp = va_arg(ephp->pArguments,char*);
						if(tmp){
							ZVAL_STRING(argv[i],tmp,0);
						}else{
							ZVAL_NULL(argv[i]);
						}
					}
					break;
				case 'S':
					{
						char** tmp = va_arg(ephp->pArguments,char**);
						if(tmp && *tmp){
							ZVAL_STRING(argv[i],*tmp,1);
						}else{
							ZVAL_NULL(argv[i]);
						}
					}
					break;
				case 'd':
				case 'l':
				case 'u':
				case 'c':
				case 'x':
					{
						long tmp = va_arg(ephp->pArguments,long);
						ZVAL_LONG(argv[i],tmp);
					}
					break;
				case 'U':
				case 'D':
				case 'X':
				case 'C':
				case 'L':
					{
						long* tmp = va_arg(ephp->pArguments,long*);
						ZVAL_LONG(argv[i],*tmp);
					}
					break;
				case 'f':
					{
						float tmp = va_arg(ephp->pArguments,float);
						ZVAL_DOUBLE(argv[i],tmp);
					}
					break;
				case 'q':
					{
						double tmp = va_arg(ephp->pArguments,double);
						ZVAL_DOUBLE(argv[i],tmp);
					}
					break;
				case 'b':
					{
						int tmp = va_arg(ephp->pArguments,int);
						ZVAL_BOOL(argv[i],tmp);
					}
					break;
				case 'm'://string array
					{
						char** tmp = va_arg(ephp->pArguments,char**);
						long n = 0;

						if(!tmp){
							ZVAL_LONG(argv[i],0);
						}else{
							if(array_init(argv[i])==FAILURE){
								goto CleanUp;
							}
							while(*tmp){
								add_index_string(argv[i],n++,*tmp,0);
								tmp++;
							}
						}
					}
					break;
				case 'v':
					{
						lphp_vparam* tmp = va_arg(ephp->pArguments,lphp_vparam*);
						if(!tmp){
							ZVAL_LONG(argv[i],0);
						}else{
							if(tmp->type == LPHP_STRING){
								ZVAL_STRINGL(argv[i],(char*)tmp->data,tmp->length,0);
							}else{
								ZVAL_LONG(argv[i],(long)tmp->data);
							}
						}
					}
					break;
				default:
					pc = i+1;
					goto CleanUp;
			}//switch(*pt)
			args[i] = &argv[i];
		}
	}

	if(ephp->pszFunction){
		fn.value.str.len = strlen(ephp->pszFunction);
		fn.value.str.val = (char*)ephp->pszFunction;
		fn.type = IS_STRING;
	}

	pv.value.str.len = strlen(ephp->pszBody);
	pv.value.str.val = (char*)ephp->pszBody;
	pv.type = IS_STRING;

	original_handle_op_arrays = CG(handle_op_arrays);
	CG(handle_op_arrays) = 0;
	try{
		new_op_array = compile_string(&pv,(ephp->pszFile)?((char*)ephp->pszFile):((char*)ephp->pszFunction) TSRMLS_CC);
	}catch(...){
		CG(handle_op_arrays) = original_handle_op_arrays;
		return FAILURE;
	}
	CG(handle_op_arrays) = original_handle_op_arrays;

	if (new_op_array)
	{
		original_return_value_ptr_ptr = EG(return_value_ptr_ptr);
		original_opline_ptr = EG(opline_ptr);

		EG(return_value_ptr_ptr) = &local_retval_ptr;
		EG(active_op_array) = new_op_array;

		try{
			zend_execute(new_op_array TSRMLS_CC);
		}catch(...){
			goto Skip;
		}

		if(ephp->pszFunction){
			retval = call_user_function_ex(CG(function_table),NULL,&fn,&rv,pc,(pc)?args:NULL,0,NULL TSRMLS_CC);
		}else{
			retval = SUCCESS;
		}

		if(rv){
			sPHPENV* ctx = (sPHPENV*)SG(server_context);
			if(rv->type == IS_LONG || rv->type == IS_BOOL){
				ctx->m_flags |= PHPENV_FLAG_NUMERIC_RESULT;
				ctx->r_value = (const char*)rv->value.lval;
			}else if(rv->type == IS_DOUBLE){
				ctx->m_flags |= PHPENV_FLAG_NUMERIC_RESULT;
				ctx->r_value = (const char*)((long)rv->value.dval);
			}else if(rv->type == IS_STRING){
				ctx->r_value = (const char*)g_malloc(rv->value.str.len+1);
				if(ctx->r_value){
					memcpy((void*)ctx->r_value,rv->value.str.val,rv->value.str.len);
					((char*)ctx->r_value)[rv->value.str.len]='\0';
				}
			}else{
				ctx->m_flags |= PHPENV_FLAG_NUMERIC_RESULT;
				ctx->r_value = NULL;
			}
			zval_ptr_dtor(&rv);
		}

		EG(return_value_ptr_ptr) = &local_retval_ptr;

		if(local_retval_ptr){
			zval_ptr_dtor(&local_retval_ptr);
		}
Skip:
		EG(no_extensions)=0;
		EG(opline_ptr) = original_opline_ptr;
		EG(active_op_array) = original_active_op_array;
		EG(function_state_ptr) = original_function_state_ptr;
		EG(return_value_ptr_ptr) = original_return_value_ptr_ptr;
		destroy_op_array(new_op_array TSRMLS_CC);
		efree(new_op_array);
	} else {
		retval = FAILURE;
	}
CleanUp:
	for(int i=0;i<pc;i++){
		argv[i]->type = IS_LONG;
		argv[i]->value.lval = 0;
		FREE_ZVAL(argv[i]);
	}
	return retval;
}

int GO_PhpExecute2(sEPHP* ephp)
{
	ephp->th_id = GetCurrentThreadId();
	if(!GO_PhpExecute2Locked(ephp)){
		g_stat_num_errors++;
		return FALSE;
	}else{
		return TRUE;
	}
	return 0;
}

/* frees all resources allocated for the current thread */

struct TSRMENTRY{
	void **storage;
	int count;
	unsigned long thread_id;
	TSRMENTRY *next;
};

TSRMENTRY* GetParentEntry(unsigned long tid)
{
	cLock(g_csection);

	TSRMENTRY* tmp = NULL;
	tmp = (TSRMENTRY*)TlsGetValue(g_tls_key);

	g_stat_num_executions++;

	while(tmp)
	{
		if(tmp->thread_id == tid){
			break;
		}
		tmp = tmp->next;
	}
	return tmp;
}

int exceptionhandler(LPEXCEPTION_POINTERS *e, LPEXCEPTION_POINTERS ep)
{
	*e=ep;
	return TRUE;
}

int GO_LockThread(unsigned long thID)
{
	cLock(g_csection);

	sTHRlst::iterator it;
	it = g_php_threads.find(thID);

	if(it != g_php_threads.end()){
		return FALSE;
	}
	g_php_threads[thID] = 1;
	return TRUE;
}

int GO_UnlockThread(unsigned long thID)
{
	extern int g_initialized;

	if(!g_initialized){
		//we do not want it, cause another thread is cleaning up...
		return TRUE;
	}

	cLock(g_csection);
	
	sTHRlst::iterator it;
	it = g_php_threads.find(thID);

	if(it == g_php_threads.end()){
		return FALSE;
	}

	g_php_threads.erase(it);
	return TRUE;
}

int GO_PhpExecute2Locked(sEPHP* ephp)
{
	long rv = 0xff00ff00;
	long result = 0;
	long started = 0;
	void ***tsrm_ls = NULL;
	sPHPENV php_ctx;
	LPEXCEPTION_POINTERS e;
	int parent_to = 0;
	int branch = 0;
	TSRMENTRY* parent = GetParentEntry(ephp->th_id);
	TSRMENTRY* tht;
	TSRMENTRY  op;
	/////////////////////////////
	//check if this is not a php call
	/////////////////////////////
	if(!GO_LockThread(ephp->th_id)){
		branch = 1;
		g_stat_num_threads++;
	}

	if(branch && parent){
		tsrm_ls = (void ***) ts_resource_ex(0,NULL);
		parent_to = EG(timeout_seconds);
		EG(timeout_seconds) = 0;

		memcpy(&op,parent,sizeof(op));
		memset(parent,0,sizeof(op));
		TlsSetValue(g_tls_key,NULL);
	}

	__try
	{
		tsrm_ls = (void ***) ts_resource_ex(0,NULL);
	}
	__except(exceptionhandler(&e, GetExceptionInformation())){
		if(branch && parent){
			memcpy(parent,&op,sizeof(op));
			TlsSetValue(g_tls_key,parent);
			tsrm_ls = (void ***) ts_resource_ex(0,NULL);
			EG(timeout_seconds) = parent_to;
		}else{
			GO_UnlockThread(ephp->th_id);
		}
		lphp_error(MT_LOG_EXCEPTION,e,"[%s],ts_resource_ex(0,NULL);\r\n",ephp->pszFile);
	}

	tht = GetParentEntry(ephp->th_id);
	//////////////////
	SG(server_context) = &php_ctx;

	//////////////////
	php_ctx.r_out = ephp->pOut;
	php_ctx.m_flags = (ephp->pOut)?(1):(0);
	php_ctx.c_param = (void*)ephp->c_param;
	php_ctx.script_path = ephp->pszFile;
	//////////////////
	if(!ephp->pszBody || tsrm_ls == NULL){
		goto End;
	}
	//////////////////
	php_update_ini_file(TSRMLS_C);
	SG(options) |= SAPI_OPTION_NO_CHDIR;
	//////////////////
	zend_first_try
	//////////////////
	{
		__try
		{
			SG(headers_sent) = 1;
			SG(request_info).no_headers = 1;

			php_request_startup(TSRMLS_C);started = 1;
			rv = GO_EvalString(ephp TSRMLS_CC);
			result = (rv == SUCCESS);
		}
		__except(exceptionhandler(&e, GetExceptionInformation())){
			lphp_error(MT_LOG_EXCEPTION,e,"[%s],php_execute_script();\r\n",ephp->pszFile);
		}
	}
	//////////////////
	zend_catch
	{
		result = 0xffffffff;
	}
	zend_end_try();
	//////////////////

	if(result != 1){
		lphp_error(MT_LOG_ERROR,NULL,"[%s],php_execute_script [%.8x][%.8x]\r\n",ephp->pszFile,rv,result);
	}else{
		if(ephp->cFlags & 0x01)
		{
			if(php_ctx.m_flags & PHPENV_FLAG_NUMERIC_RESULT)
			{
				ephp->cResType = 0x00;
				ephp->res.lval = (long)php_ctx.r_value;
			}
			else if(php_ctx.r_value)
			{
				ephp->cResType = 0x01;
				if(ephp->res.str.val && ephp->res.str.len){
					strncpy(ephp->res.str.val,(const char*)php_ctx.r_value,ephp->res.str.len);
				}else{
					ephp->res.str.val = _strdup((const char*)php_ctx.r_value);
					ephp->res.str.len = strlen((const char*)php_ctx.r_value);
					g_free((void*)php_ctx.r_value);
				}
			}
			else
			{
				ephp->cResType = 0x00;
				ephp->res.str.val = NULL;
				ephp->res.str.len = 0;
			}
		}
	}

	if(started == 1){
		__try
		{
			php_request_shutdown(NULL);
		}
		__except(exceptionhandler(&e, GetExceptionInformation())){
			lphp_error(MT_LOG_EXCEPTION,e,"[%s],php_request_shutdown(NULL);\r\n",ephp->pszFile);
		}
	}

	if(branch && parent)
	{
		__try{
			ts_free_thread();
		}__except(exceptionhandler(&e, GetExceptionInformation())){
			lphp_error(MT_LOG_EXCEPTION,e,"[%s],ts_free_thread();\r\n",ephp->pszFile);
		}
		if(parent){
			memcpy(parent,&op,sizeof(op));
			TlsSetValue(g_tls_key,parent);
			tsrm_ls = (void ***) ts_resource_ex(0,NULL);
			EG(timeout_seconds) = parent_to;
		}
	}else{
		GO_UnlockThread(ephp->th_id);
	}
End:
	return (result == 1);
}

int GO_PhpExecute(const char* script,std::string* out,cutFile* redir_out,long mode,
				   const char* querystring,void* cparam,PHPENV_CB fpCb)
{
	long rv = 0xff00ff00;
	long result = 0;
	long started = 0;
	void ***tsrm_ls = NULL;
	unsigned long thread_id = GetCurrentThreadId();
	sPHPENV php_ctx;
	zend_file_handle file_handle = {0};
	zval* ret_val = NULL;
	char full_path[260] = {0};
	/////////////////////////////
	//check if this is not a php call
	/////////////////////////////

	if(!GO_LockThread(thread_id)){
		lphp_error(MT_LOG_ERROR,NULL,"You must not call php from another php script!");
		return 0;
	}

	try
	{
		tsrm_ls = (void ***) ts_resource_ex(0, NULL);
	}
	catch(LPEXCEPTION_POINTERS e)
	{
		lphp_error(MT_LOG_EXCEPTION,e,"ts_resource_ex(0,NULL);");
		goto End;
	}


	//////////////////
	SG(server_context) = &php_ctx;
	//////////////////
	php_ctx.r_out = redir_out;
	php_ctx.m_flags = (redir_out)?(1):(0);
	php_ctx.c_param = cparam;
	php_ctx.fp_callback = fpCb;
	php_ctx.query_string = querystring;
	php_ctx.script_path = (mode == PHPENV_MODE_SCRIPT)?"":script;
	//////////////////
	if(!script || !strlen(script) || tsrm_ls == NULL){
		goto End;
	}
	//////////////////
	php_update_ini_file(TSRMLS_C);
	//////////////////
	if(mode & PHPENV_MODE_FILE)
	{
		if(!fpCb){
			goto End;
		}

		strncpy(full_path,script,sizeof(full_path));
	}
	//////////////////
	SG(options) |= SAPI_OPTION_NO_CHDIR;
	//////////////////
	zend_first_try
	//////////////////
	{
		try
		{
			if(mode & PHPENV_MODE_SCRIPT)
			{
				if(!(mode & PHPENV_MODE_ALLOWHDR))
				{
					SG(headers_sent) = 1;
					SG(request_info).no_headers = 1;
				}
				else
				{
					if(!fpCb){
						goto End;
					}
				}

				php_request_startup(TSRMLS_C);started = 1;
				rv = zend_eval_string((char*)script,NULL,"libphp (www.piopawlu.net)" TSRMLS_CC);
				result = (rv == SUCCESS);
			}
			else
			{
				file_handle.filename = full_path;
				file_handle.free_filename = 0;
				file_handle.type = ZEND_HANDLE_FILENAME;
				file_handle.opened_path = NULL;

				php_ctx.post_len = (long)fpCb(LPHP_CB_POST_LENGTH,0,0,cparam);
				php_ctx.post_data = (unsigned char*)fpCb(LPHP_CB_POST_DATA,0,0,cparam);
				php_ctx.content_length = (long)fpCb(LPHP_CB_GETCL,0,0,cparam);
				php_ctx.content_type = (const char*)fpCb(LPHP_CB_GETCT,0,0,cparam);

				SG(request_info).cookie_data = (char*)fpCb(LPHP_CB_GETCOOKIE,0,0,cparam);
				SG(request_info).request_method = (const char*)fpCb(LPHP_CB_GETMETHOD,0,0,cparam);
				SG(request_info).content_type = php_ctx.content_type;
				SG(request_info).content_length = php_ctx.post_len;
				SG(request_info).path_translated = full_path;
				SG(request_info).query_string = (querystring)?((char*)querystring):((char*)"");
				//////////
				php_request_startup(TSRMLS_C);started = 1;
				php_execute_simple_script(&file_handle, &ret_val TSRMLS_CC);
				result = 1;
			}

			if(out && php_ctx.r_value){
				*out = php_ctx.r_value;
			}
		}
		catch(LPEXCEPTION_POINTERS e)
		{
			lphp_error(MT_LOG_EXCEPTION,e,"php_execute_script();");
		}
	}
	//////////////////
	zend_catch
	//////////////////
	{
		result = 0xffffffff;
	}
	//////////////////
	zend_end_try();
	//////////////////

	if(result != 1){
		lphp_error(MT_LOG_ERROR,NULL,"php_execute_script [%.8x][%.8x]",rv,result);
	}

	if(started == 1)
	{
		try{
			php_request_shutdown(NULL);
		}
		catch(LPEXCEPTION_POINTERS e)
		{
			lphp_error(MT_LOG_EXCEPTION,e,"php_request_shutdown(NULL);");
		}
	}

End:
	GO_UnlockThread(thread_id);
	return (result == 1);
}

int GO_PhpGlobalInit()
{
	const char* php_ini = NULL;
	void*** key = NULL;
	TSRMENTRY* te;
	unsigned long php_tc = g_pref_ul("/cfg/php/tc",10,1);
	sVar sv;

	php_ini = g_pref("/cfg/php/ini");
	phpenv_sapi_module.php_ini_path_override = (php_ini)?(php_ini):"php.ini";

	if(!tsrm_startup(64,1,NULL,NULL)){
		return FALSE;
	}

	//g_tls_key
	key = (void ***) ts_resource_ex(0,NULL);
	if(key){
		for(int i=0;i<512;i++){
			te = (TSRMENTRY*)TlsGetValue(i);
			if(te && ((void***)te) == key){
				g_tls_key = i;
				break;
			}
		}
		if(!g_tls_key){
			tsrm_shutdown();
			return FALSE;
		}
	}else{
		tsrm_shutdown();
		return FALSE;
	}


	sapi_startup(&phpenv_sapi_module);

	if(phpenv_sapi_module.startup){
		phpenv_sapi_module.startup(&phpenv_sapi_module);
	}

	sVariable(&sv,SV_LPOINTER,&g_stat_num_executions, 0, 1);
	g_vars["/stat/php/num_execs"] = sv;

	sVariable(&sv,SV_LPOINTER,&g_stat_num_errors, 0, 1);
	g_vars["/stat/php/num_errors"] = sv;

	sVariable(&sv,SV_LPOINTER,&g_stat_num_threads, 0, 1);
	g_vars["/stat/php/num_threads"] = sv;

	return TRUE;
}
int GO_PhpGlobalDeInit()
{
	HANDLE hThread;

	//lock further execution and wait for all the threads remaining
	cLock(g_csection);
	sTHRlst::iterator it = g_php_threads.begin();

	while(it != g_php_threads.end())
	{
		cULock();
		hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, it->first);
		if(hThread != NULL){
			if(WaitForSingleObject(hThread, 2000) != WAIT_OBJECT_0){
				TerminateThread(hThread, -1);
			}
			CloseHandle(hThread);
		}
		cLLock();
		it++;
	}

	if(phpenv_sapi_module.shutdown)
	{
		try{
			phpenv_sapi_module.shutdown(&sapi_module);
		}catch(...){}
	}

	try{
		tsrm_shutdown();
	}catch(...){}

	mt_module_entry.functions = mt_functions;
	if(g_zend_functions){
		free(g_zend_functions);
		g_zend_functions = NULL;
	}
	return TRUE;
}

int php_phpenv_ub_write(const char *str, unsigned int str_length TSRMLS_DC)
{
	sPHPENV* ctx = (sPHPENV*)SG(server_context);

	SG(headers_sent) = 1;

	if(ctx->fp_callback){
		ctx->fp_callback(1,0,0,ctx->c_param);
	}

	if(ctx->r_out){
		ctx->r_out->write((void*)str,str_length);
		return SUCCESS;
	}

#ifdef _DEBUG
	g_std_out(str, strlen(str));
	return SUCCESS;
#else
	if((ctx->m_flags & PHPENV_FLAG_DISABLE_OUTPUT)){
		return SUCCESS;
	}else{
		g_std_out(str,strlen(str));
		return SUCCESS;
	}
#endif
}
void php_phpenv_sapi_error(int type, const char *fmt, ...)
{
	char error[1024];

	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(error,sizeof(error)-1,fmt,ap);
	php_phpenv_ub_write(error,strlen(error),0);
	va_end(ap);
}
void php_phpenv_log_message(char *message)
{
	g_std_err(message,strlen(message));
}

int php_set_ini_entry(char *entry, char *value, int stage)
{
	return (SUCCESS == zend_alter_ini_entry(entry, strlen(entry) + 1, value, strlen(value) + 1,PHP_INI_SYSTEM, stage));
}
int php_phpenv_startup(sapi_module_struct *sapi_module)
{
	if(g_php_module == NULL)
	{
		mt_module_entry.functions = mt_functions;
		if(php_module_startup(sapi_module,&mt_module_entry,1)==FAILURE){
			return FAILURE;
		}
		return SUCCESS;
	}
	else //new functions
	{
		long c1 = 0;
		long c2 = 0;
		long c = 0;
		long x = 0;
		zend_function_entry* cfe = mt_functions;
		//count our own functions
		while(cfe->fname){
			c1++;
			cfe++;
		}
		//count given functions
		cfe = ((zend_module_entry*)g_php_module)->functions;
		while(cfe->fname){
			c2++;
			cfe++;
		}
		c = c1 + c2;

		g_zend_functions = (zend_function_entry*)malloc((c + 1)*sizeof(zend_module_entry));
		if(!g_zend_functions){
			return FAILURE;
		}
		memset(g_zend_functions,0,(c + 1)*sizeof(zend_module_entry));
		//copy our own functions
		cfe = mt_functions;
		for(int i=0;i<c1;i++){
			g_zend_functions[i] = *cfe++;
		}
		//copy given functions
		cfe = ((zend_module_entry*)g_php_module)->functions;
		for(int i=0;i<c2;i++){
			g_zend_functions[i + c1] = *cfe++;
		}

		mt_module_entry.functions = g_zend_functions;
		try{
			if(php_module_startup(sapi_module,&mt_module_entry,1)==FAILURE){
				return FAILURE;
			}
		}catch(...){
			free(g_zend_functions);
			g_zend_functions = NULL;
			mt_module_entry.functions = mt_functions;
			return FAILURE;
		}

		return SUCCESS;
	}
}

char* php_phpenv_read_cookies(TSRMLS_D)
{
	sPHPENV* ctx = (sPHPENV*)SG(server_context);
	if(ctx->fp_callback)
	{
		return (char*)ctx->fp_callback(LPHP_CB_GETCOOKIE,0,0,ctx->c_param);
	}
	else{
		return NULL;
	}
	return 0;
}

void php_phpenv_flush(void *server_context)
{
	return;
}

void php_phpenv_register_variables(zval *track_vars_array TSRMLS_DC)
{
	sPHPENV* ctx = (sPHPENV*)SG(server_context);
	const char* rm = "GET";
	const char* rv = NULL;
	char temp[64];

	if(ctx->fp_callback)
	{
		rm = (const char*)ctx->fp_callback(LPHP_CB_GETMETHOD,0,0,ctx->c_param);

		php_register_variable("REQUEST_METHOD",(char*)rm,track_vars_array TSRMLS_CC);

		if(ctx->content_type){
			php_register_variable("CONTENT_TYPE",(char*)ctx->content_type,track_vars_array TSRMLS_CC);
		}

		_snprintf(temp,sizeof(temp)-1,"%u",ctx->content_length);
		php_register_variable("CONTENT_LENGTH",temp,track_vars_array TSRMLS_CC);

		if(ctx->script_path){
			php_register_variable("PATH_TRANSLATED",(char*)ctx->script_path,track_vars_array TSRMLS_CC);
		}
		if(ctx->query_string){
			php_register_variable("QUERY_STRING",(char*)ctx->query_string,track_vars_array TSRMLS_CC);
		}

		rm = (const char*)ctx->fp_callback(LPHP_CB_GETVARS,0,0,ctx->c_param);
		if(rm){
			while(*rm){
				rv = rm + strlen(rm) + 1;
				php_register_variable((char*)rm,(char*)rv,track_vars_array TSRMLS_CC);
				rm = rv + strlen(rv) + 1;
			}
		}
	}
	php_import_environment_variables(track_vars_array TSRMLS_CC);
}

int php_phpenv_read_post(char *buffer, uint count_bytes TSRMLS_DC)
{
	sPHPENV* ctx = (sPHPENV*)SG(server_context);
	unsigned long to_read = 0;
	unsigned long rd_left = 0;

	if(!ctx || !ctx->post_data){
		return 0;
	}

	rd_left = ctx->post_len - ctx->post_read;
	to_read = (rd_left >= count_bytes)?count_bytes:rd_left;
	if(to_read <= 0){
		return 0;
	}

	memcpy(buffer,(ctx->post_data + ctx->post_read),to_read);
	ctx->post_read += to_read;
	*(buffer + to_read) = '\0';
	return to_read;
	return 0;
}

void php_phpenv_send_header(sapi_header_struct *sapi_header, void *server_context TSRMLS_DC)
{
	if(!sapi_header)return;
	sPHPENV* ctx = (sPHPENV*)SG(server_context);
	if(ctx && ctx->fp_callback){
		ctx->fp_callback(LPHP_CB_SETHDR,(void*)sapi_header->header,(void*)sapi_header->replace,ctx->c_param);
	}
}
int php_phpenv_deactivate(TSRMLS_D){
	return SUCCESS;
}

char* php_phpenv_getenv(char *name, size_t name_len TSRMLS_DC)
{
	sPHPENV* ctx = (sPHPENV*)SG(server_context);

	if(ctx->fp_callback){
		return (char*)ctx->fp_callback(LPHP_CB_GETENV,(void*)name,NULL,ctx->c_param);
	}else{
		return getenv(name);
	}
	return NULL;
}

int	php_update_ini_file(TSRMLS_D)
{
	sPHPENV* ctx = (sPHPENV*)SG(server_context);

	if(!ctx->fp_callback && g_pref_ul("/cfg/php/noupd",10)){
		return 1;
	}

	char inc_path_full[512];
	char* tmp = (char*)g_pref("/cfg/php/root");
	const char* root_dir = (tmp)?(tmp):("");

	try
	{
		tmp = (char*)g_pref("/cfg/php/sessions");
		if(tmp && !php_set_ini_entry("session.save_path", (char*)tmp, PHP_INI_STAGE_ACTIVATE)){
			return 0;
		}

		tmp = (char*)g_pref("/cfg/php/includes");
		if(tmp && !php_set_ini_entry("include_path", (char*)tmp, PHP_INI_STAGE_ACTIVATE)){
			return 0;
		}

		if(ctx->fp_callback && ctx->script_path)
		{
			strncpy(inc_path_full+1,ctx->script_path,sizeof(inc_path_full)-2);
			*inc_path_full = 0;
			tmp = inc_path_full + strlen(ctx->script_path);
			while(*tmp)
			{
				if(*tmp == '\\' || *tmp == '/'){
					*tmp='\0';
					break;
				}else{
					tmp--;
					if(!(*tmp)){
						*tmp = (char)(0xff);
					}
				}
			}

			if(*tmp!=0xFF){
				if(!php_set_ini_entry("doc_root", (char*)(inc_path_full+1), PHP_INI_STAGE_ACTIVATE))return 0;
			}
		}
		else
		{
			tmp = (char*)g_pref("/cfg/php/doc_root");
			if(tmp && !php_set_ini_entry("doc_root", (char*)tmp, PHP_INI_STAGE_ACTIVATE)){
				return 0;
			}
		}

		tmp = (char*)g_pref("/cfg/php/uploads");
		if(tmp && !php_set_ini_entry("upload_tmp_dir", (char*)tmp, PHP_INI_STAGE_ACTIVATE)){
			return 0;
		}

		tmp = (char*)g_pref("/cfg/php/extensions");
		if(tmp && !php_set_ini_entry("extension_dir",(char*)tmp, PHP_INI_STAGE_ACTIVATE)){
			return 0;
		}
		return 1;
	}
	catch(...)
	{
		return 0;
	}
	return 1;
}