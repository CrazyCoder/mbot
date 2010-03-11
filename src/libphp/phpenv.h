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
#pragma once

#ifndef _PHP_ENV_H_
#define _PHP_ENV_H_

extern "C"{
	#include <main/php.h>
	#include <main/SAPI.h>
	#include <main/php_main.h>
	#include <main/php_variables.h>
	#include <main/php_ini.h>
	#include <zend_ini.h>
}
#include <string>
#include <cUtils.h>

const static long PHPENV_FLAG_DISABLE_OUTPUT = 0x01;
const static long PHPENV_MODE_SCRIPT = 0x01;
const static long PHPENV_MODE_FILE = 0x02;
const static long PHPENV_MODE_ALLOWHDR = 0x02;
const static long PHPENV_FLAG_NUMERIC_RESULT = 0x04;

#ifdef _DEBUG
#define DBGS(s) OutputDebugString(s)
#else
#define DBGS(s)
#endif

typedef std::map<long, long> sTHRlst;

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
	////////////////////////////
public:
	sPHPENV(){memset(this,0,sizeof(sPHPENV));}
};

typedef struct sPHPExeParam
{
	const char* pszBody;
	const char* pszFile;
	const char* pszFunction;
	const char* pszPT;
	va_list     pArguments;
	cutFile*	pOut;
	const void* c_param;
	////////////
	short dummy;
	////////////
	char cFlags;
	char cResType;
	////////////
	union{
		struct{
			char* val;
			long  len;
		}str;
		long  lval;
		double dval;
	}res;
	unsigned long th_id;
}sEPHP;

struct exe_helper{
	sEPHP* php;
	int result;
};

int GO_PhpExecute(const char* script,std::string* out,cutFile* redir_out,long mode = PHPENV_MODE_SCRIPT,
				   const char* querystring = NULL,void* cparam = NULL,PHPENV_CB fpCb = NULL);
int GO_PhpExecute2Locked(sEPHP* ephp);
int GO_PhpExecute2(sEPHP* ephp);

int GO_PhpGlobalInit();
int GO_PhpGlobalDeInit();

#endif //_PHP_ENV_H_