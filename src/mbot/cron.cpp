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
#include "sync.h"
#include "cron.h"

CSyncList		g_cron;
HANDLE			g_cron_event = NULL;
HANDLE			g_cron_thread = NULL;
long			g_cron_tid = 0;
volatile long	g_cron_status = 0;
volatile long	g_cron_signal = 0;
CRITICAL_SECTION g_cron_cts;

const char* g_cron_days[]={"mon","tue","wed","thu","fri","sat","sun",NULL};

unsigned long cron_day2num(const char* s)
{
	unsigned long i=0;
	for(const char** g = g_cron_days;*g;g++,i++)
	{
		if((*(long*)s & 0x00ffffff) == *(long*)(*g)){
			return i;
		}
	}
	return (-1);
}

unsigned long cron_strtoul(const char* s,const char** out)
{
	char c = *s;
	unsigned long res;

	if(isalpha(c) && (res = cron_day2num(s))!=(unsigned long)(-1))
	{
		*out = s + 3;
		return res;
	}

	if(c < '0' || c > '9')return (-1);
	s++;

	if(*s < '0' || *s > '9'){
		*out = s;
		return (c - '0');
	}

	res = (c - '0') * 10;
	c = *s;
	s++;

	*out = s;
	if(*s >= '0' && *s <= '9'){
		return (-1);
	}
	return res + (c - '0');
}

const char* cron_sub_parse(const char* s,cron_field* out,unsigned long max)
{
	unsigned int tmp;
	unsigned int last;
	const char* stop;
	char ss = 0;

	out->s64 = 0;

	if(*s == '*'){
		s++;
		if(*s == ' ' || !(*s))
		{
			out->s64 = 0x7fffffffFFFFFFFF;
			return s;
		}
		else if(*s == '/')
		{
			s++;
			tmp = cron_strtoul(s,&stop);
			if((*stop!=' ' && *stop) || !tmp || tmp > max){
				return NULL;
			}

			for(unsigned int i=0;i<max;i++){
				if(!(i % tmp)){
					out->s64 |= (((__int64)1) << i);
				}
			}
			return stop;
		}
		else{
			return NULL;
		}
	}else{

		do
		{
			tmp = cron_strtoul(s,&stop);
			if(tmp > max){
				return NULL;
			}

			if(ss == '-')
			{
				if(last >= tmp){
					return NULL;
				}

				for(unsigned int i=last;i<=tmp;i++){
					out->s64 |= ((__int64)1 << i);
				}
				ss = 0;
				s = stop + 1;
			}
			else if(*stop == '-')
			{
				last = tmp;
				ss = *stop;
				s = stop + 1;
			}
			else
			{
				out->s64 |= ((__int64)1 << tmp);
				s = stop + 1;
			}
		}while(*stop == ',' || *stop == '-');

		return stop;
	}
	return NULL;
}

unsigned long cron_fbit64(int n,__int64 x)
{
	for(register int i=n;i<64;i++){
		if(x & ((__int64)1 << i)){
			return i;
		}
	}
	for(register int i=0;i<n;i++){
		if(x & ((__int64)1 << i)){
			return i;
		}
	}
	return 64;
}

unsigned long cron_fbit32(int n,long x)
{
	for(register int i=n;i<32;i++){
		if(x & (1 << i)){
			return i;
		}
	}
	for(register int i=0;i<n;i++){
		if(x & (1 << i)){
			return i;
		}
	}
	return 64;
}

int cron_parse(const char* q,sCronEvent* out)
{
	const static char max[3]={60,24,7};
	cron_field cs[3] = {0};

	int i = 0;
	//min hour wday
	for(const char* s = q;*s;){
		if(*s!=' '){
			if(!(s = cron_sub_parse(s,&cs[i],max[i]))){
				return 0;
			}else{
				i++;
			}
		}else{
			s++;
		}
	}
	if(i != 3){
		return 0;
	}

	out->min = cs[0].s64;
	out->hour = cs[1].dw.l32;
	out->wday = cs[2].dw.l32;
	return 1;
}

int cron_calcnext(sCronEvent* ce)
{
	int i = 0;
	struct tm lt;
	struct tm *tlt;
	time_t ct;

	ct = time(0);
	ct += (CRON_MINUTE - (ct % CRON_MINUTE));

	while(1){
		tlt = localtime(&ct);
		if(!tlt)return 0;
		lt = *tlt;
		//wday
		i = cron_fbit32(lt.tm_wday,ce->wday);
		if(i < lt.tm_wday){
			//round to full day
			ct -= (lt.tm_min * CRON_MINUTE) + (lt.tm_hour * CRON_HOUR);
			ct += (CRON_DAY) * ((7 - lt.tm_wday) + i);
			continue;
		}else if(i > lt.tm_wday){
			ct -= (lt.tm_min * CRON_MINUTE) + (lt.tm_hour * CRON_HOUR);
			ct += (CRON_DAY) * (i - lt.tm_wday);
			continue;
		}
		//hour
		i = cron_fbit32(lt.tm_hour,ce->hour);
		if(i < lt.tm_hour){
			//round to full hour
			ct -= (lt.tm_min * CRON_MINUTE);
			ct += (CRON_HOUR) * ((24 - lt.tm_hour) + i);
			continue;
		}else if(i > lt.tm_hour){
			ct -= (lt.tm_min * CRON_MINUTE);
			ct += (CRON_HOUR) * (i - lt.tm_hour);
			continue;
		}
		//minute
		i = cron_fbit64(lt.tm_min,ce->min);
		if(i < lt.tm_min){
			ct += (CRON_MINUTE) * ((60 - lt.tm_min) + i);
			continue;
		}else if(i > lt.tm_min){
			ct += (CRON_MINUTE) * (i - lt.tm_min);
			continue;
		}
		break;
	}
	return ct;
}

int cron_register(sCronSync* ce)
{
	sCronSync* e;
	g_cron.Lock();
	e = (sCronSync*)g_cron.m_head;
	while(e)
	{
		if(strcmp(e->name,ce->name)==0){
			g_cron.Unlock();
			return 0;
		}
		e = (sCronSync*)e->next;
	}
	ce->lSpent = ce->lSpent = 0;
	g_cron.AddLocked((sSync*)ce);
	g_cron.Unlock();
	return 1;
}

int cron_unregister(const char* name)
{
	g_cron.Lock();
	sCronSync* e = (sCronSync*)g_cron.m_head;

	while(e)
	{
		if(strcmp(e->name,name)==0){
			e->lFlags |= MBOT_FLAG_DELETE | MBOT_FLAG_INACTIVE;
			cron_signal_change();
			g_cron.Unlock();
			return 1;
		}
		e = (sCronSync*)e->next;
	}
	g_cron.Unlock();
	return 0;
}

int cron_enable_param(void* data,bool enable)
{
	g_cron.Lock();
	sCronSync* e = (sCronSync*)g_cron.m_head;

	while(e)
	{
		if((void*)e->data == data){
			if(enable == false){
				e->lFlags |= MBOT_FLAG_DELETE | MBOT_FLAG_INACTIVE;
			}else{
				e->lFlags &= ~(MBOT_FLAG_INACTIVE | MBOT_FLAG_DELETE);
			}
			cron_signal_change();
		}
		e = (sCronSync*)e->next;
	}
	g_cron.Unlock();
	return 0;
}

int cron_unregister_param(void* data)
{
	g_cron.Lock();
	sCronSync* e = (sCronSync*)g_cron.m_head;

	while(e)
	{
		if((void*)e->data == data){
			e->lFlags |= MBOT_FLAG_DELETE | MBOT_FLAG_INACTIVE;
			cron_signal_change();
		}
		e = (sCronSync*)e->next;
	}
	g_cron.Unlock();
	return 0;
}

int cron_enable(const char* name,long enable)
{
	g_cron.Lock();
	sCronSync* e = (sCronSync*)g_cron.m_head;

	while(e)
	{
		if(strcmp(e->name,name)==0){
			if(enable){
				e->lFlags &= ~(MBOT_FLAG_INACTIVE | MBOT_FLAG_DELETE);
			}else{
				e->lFlags |= MBOT_FLAG_INACTIVE;
			}
			cron_signal_change();
			g_cron.Unlock();
			return 1;
		}
		e = (sCronSync*)e->next;
	}
	g_cron.Unlock();
	return 0;
}

int cron_modify(const char* name,const sCronEvent& ce)
{
	g_cron.Lock();
	sCronSync* e = (sCronSync*)g_cron.m_head;

	while(e)
	{
		if(strcmp(e->name,name)==0)
		{
			e->ce = ce;
			cron_signal_change();
			g_cron.Unlock();
			return 1;
		}
		e = (sCronSync*)e->next;
	}
	g_cron.Unlock();
	return 0;
}

int WINAPI cron_execute(sCronSync* event)
{
	time_t st;
	time_t en;
	int result;
	mb_event mbe = {MBT_TIMER,0,0};
	mbe.php = event->data;

	st = clock();
	sman_inc(mbe.php);
	if(mbe.php->szBuffered)
	{
		result = LPHP_ExecuteScript(mbe.php->szBuffered,event->fcn,NULL,&mbe,NULL);
	}
	else
	{
		result = LPHP_ExecuteFile(mbe.php->szFilePath,event->fcn,NULL,&mbe,NULL);
	}
	sman_dec(mbe.php);
	en = clock();

	event->lLastSpent = en - st;
	event->lSpent += event->lLastSpent;
	event->lFlags &= ~(MBOT_FLAG_WORKING);

	result = cron_calcnext(&event->ce);
	if(!result){
		event->lTime = 0x7fFFffFF;
	}else{
		event->lTime = result;
	}
	return 0;
}

int WINAPI cron_thread(void* dummy)
{
	static sCronSync* e;
	static sCronSync* te;
	static HANDLE hThread;
	static long tid;
	static long ct;
	static time_t st;

	WaitForSingleObject(g_cron_event,INFINITE);

	if(g_cron_status != CRON_WORKING){
		return 0;
	}

	e = (sCronSync*)g_cron.m_head;
	while(e)
	{
		if(!(tid = cron_calcnext(&e->ce))){
			e->lTime = 0x7fFFffFF;
		}else{
			e->lTime = tid;
		}
		e = (sCronSync*)e->next;
	}

	while(g_cron_status == CRON_WORKING)
	{
		ct = time(0);
		//do stuff
		g_cron.Lock();
 		e = (sCronSync*)g_cron.m_head;
		while(e)
		{
			if(e->lTime <= ct && !(e->lFlags & (MBOT_FLAG_INACTIVE|MBOT_FLAG_DELETE)) && !(e->lFlags & MBOT_FLAG_WORKING))
			{//execute
				if(e->lFlags & MBOT_FLAG_ASYNC)
				{
					e->lFlags |= (MBOT_FLAG_WORKING);
					if(!(hThread = CreateThread(NULL,NULL,(LPTHREAD_START_ROUTINE)cron_execute,(LPVOID)e,NULL,(LPDWORD)&tid)))
					{
						e->lFlags &= ~(MBOT_FLAG_WORKING);
					}
					CloseHandle(hThread);
				}
				else
				{
					e->lFlags |= (MBOT_FLAG_WORKING);
					g_cron.Unlock();
					cron_execute(e);
					g_cron.Lock();
				}
			}
			e = (sCronSync*)e->next;
		}
		//now remove events which want to be removed...
		if(g_cron_signal)
		{
			InterlockedExchange(&g_cron_signal,0);
			e = (sCronSync*)g_cron.m_head;
			while(e)
			{
				if((e->lFlags & MBOT_FLAG_DELETE))
				{
					te = (sCronSync*)e->next;
					g_cron.DelLocked(e);
					cron_free(e);
					e = te;
				}else{
					e = (sCronSync*)e->next;
				}
			}
		}
		g_cron.Unlock();


		st = time(0);
		ct = (60 - (st % 60));
		WaitForSingleObject(g_cron_event,ct * 1000);
	}

	return 0;
}

int cron_shutdown()
{
	if(g_cron_thread)
	{
		InterlockedExchange(&g_cron_status,CRON_SHUTDOWN);
		SetEvent(g_cron_event);
		WaitForSingleObject(g_cron_thread,10000);

		CloseHandle(g_cron_event);
		CloseHandle(g_cron_thread);

		g_cron_event = NULL;
		g_cron_thread = NULL;
		return 1;
	}else{
		return 0;
	}
}

int cron_startup()
{
	sCronSync* e;

	if(!g_cron_thread){
		return 0;
	}

	g_cron.Lock();

	e = (sCronSync*)g_cron.m_head;
	while(e){
		cron_calcnext(&e->ce);
		e = (sCronSync*)e->next;
	}
	g_cron.Unlock();

	InterlockedExchange(&g_cron_status,CRON_WORKING);
	SetEvent(g_cron_event);
	return 1;
}

int cron_initialize()
{
	InitializeCriticalSectionAndSpinCount(&g_cron_cts,0x80000100);

	g_cron_event = CreateEvent(0,0,0,0);

	if(g_cron_event == INVALID_HANDLE_VALUE){
		DeleteCriticalSection(&g_cron_cts);
		return 0;
	}

	if(g_cron_thread = CreateThread(NULL,(512*1024),(LPTHREAD_START_ROUTINE)cron_thread,
		(LPVOID)NULL,NULL,(LPDWORD)&g_cron_tid))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int cron_signal_change()
{
	InterlockedExchange(&g_cron_signal,1);
	return 1;
}

void cron_free(sCronSync* e)
{
	my_memfree((void*)e);
}

int cron_release()
{
	sCronSync* e;
	sCronSync* tmp;
	g_cron.Lock();
	e = (sCronSync*)g_cron.m_head;
	while(e)
	{
		tmp = (sCronSync*)e->next;
		cron_free(e);
		e = tmp;
	}
	g_cron.m_head = g_cron.m_tail = NULL;
	g_cron.Unlock();
	return 1;
}