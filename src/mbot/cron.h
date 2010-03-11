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
#ifndef _CRON_H_
#define _CRON_H_

#include "sync.h"
#include "mbot.h"
#include "smanager.h"
#pragma once

#define CRON_SECOND 1
#define CRON_MINUTE 60*CRON_SECOND
#define CRON_HOUR 60*CRON_MINUTE
#define CRON_DAY 24*CRON_HOUR

#define CRON_WORKING 1
#define CRON_SHUTDOWN 2
#define CRON_IDLE 3

//define this one!
#define CRON_PARAM PPHP

union cron_field{
	__int64 s64;
	struct{
		unsigned long l32;
		unsigned long h32;
	}dw;
};

struct sCronEvent{
	__int64 min;
	unsigned long hour;
	unsigned long wday;
};

struct sCronSync : public sSync
{
	sCronEvent ce;
	char	fcn[24];
	char	name[24];
	long	lTime;
	long	lSpent;
	long	lLastSpent;
	volatile long lFlags;
	CRON_PARAM data;
public:
	sCronSync(){
		memset(this,0,sizeof(sCronSync));
	}
};

int cron_parse(const char* q,sCronEvent* out);
int cron_calcnext(sCronEvent* ce);
int cron_register(sCronSync* ce);
int cron_unregister(const char* name);
int cron_unregister_param(void* data);
int cron_enable_param(void* data,bool enable);
int cron_enable(const char* name,long enable);
int cron_modify(const char* name,const sCronEvent& ce);
int WINAPI cron_thread(void* dummy);
int cron_initialize();
int cron_shutdown();
int cron_startup();
int cron_release();
int cron_signal_change();
void cron_free(sCronSync* e);

#endif //_CRON_H_