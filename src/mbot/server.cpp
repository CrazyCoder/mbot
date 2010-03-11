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
#include "window.h"
#include "functions.h"
#include <cBase64.h>

#ifndef _NOHTTPD_

#pragma comment (lib,"ws2_32")

typedef volatile unsigned long VDWORD;

struct sHTTPDEnv
{
	cutSockf* sf;
	cutMemf*  mf;
	cutMemf*  req;
	map<string, string> hdr;

	char*	post_data;
	long	post_length;

	enum	eMethod{GET, POST} method;
	char	out_started;
	short	error_code;
	long	var_offset;
public:
	sHTTPDEnv(){
		sf = NULL;
		mf = NULL;
		req = NULL;
		post_data = NULL;
		post_length = 0;
		method = GET;
		out_started = 0;
		error_code = 200;
		var_offset = 0;
	}
};

struct sFDS
{
	u_int   fd_count;
	SOCKET  fd_array[2];
};

struct sHTTPSync : public sSync
{
	SOCKET s;
	unsigned long to;
};

struct sMIME
{
	char* ext;
	char* type;
};

struct sVS
{
	char  auth_req;
	char* path;
	char* ip;
	char* user;
	char* pass;

	bool operator()(const char* s1, const char* s2) const
	{
		return strcmp(s1, s2) < 0;
	}
};

///////////////////////////
SOCKET		svr_ls = NULL;
HANDLE		svr_hTh = NULL;
HANDLE		svr_q_hTh = NULL;
HANDLE		svr_q_event = NULL;
VDWORD		svr_q_tc = 0;
VDWORD		svr_cur_clients = 0;
HANDLE		svr_w_hTh = NULL;
CSyncList	svr_q_queue;

map<string,string> svr_mimes;
map<string,sVS>    svr_vs;

//////////////////////////
DWORD		svr_max_clients = 15;
DWORD		svr_timeout = 60000;
FILE*		svr_log = NULL;
CSECTION	svr_log_cs;
short		svr_listen_port = 8081;
long		svr_auth_required = 0;
char		svr_allowed_ip[32] = "*";
char		svr_wwwroot[260];
long		svr_wwwroot_len = 0;
long		svr_maxrequest = 4*1024*1024;

extern cXmlDoc cSettings;

const static char* error503 = "HTTP/1.0 503 Too many connections\r\nConnection: close\r\nRetry-After: 30\r\nContent-Type: text/html\r\n\r\n<h4>503 Too many connections</h4>";

static const char* svr_months[]={"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec",""};
static const char* svr_days[]={0,"Mon","Tue","Wed","Thu","Fri","Sat","Sun",""};

unsigned int httpd_mime_hash(sMIME* v)
{
	unsigned int index = 0;
	char* c = v->ext;
	while(*c){index = 31 * index + *c++;}
	return (index & 0x7EffFFff) + 1;
}

int httpd_mime_cmp(sMIME* v1,sMIME* v2,void* param){
	return strcmp(v1->ext,v2->ext);
}

int httpd_mime_set(sMIME* v1,sMIME* v2,void* param){
	*v1 = *v2;
	return 1;
}

unsigned int httpd_vs_hash(sVS* v)
{
	unsigned int index = 0;
	char* c = v->path;
	while(*c){index = 31 * index + *c++;}
	return (index & 0x7EffFFff) + 1;
}
int httpd_vs_cmp(sVS* v1,sVS* v2,void* param){
	return strcmp(v1->path,v2->path);
}
int httpd_vs_set(sVS* v1,sVS* v2,void* param){
	if(v2 != NULL){
		*v1 = *v2;
	}
	return 1;
}

int httpd_logaccess(const char* request,long code,long length,DWORD ip)
{
	
	static const char* tmp = NULL;
	static SYSTEMTIME st;

	if(!svr_log)return 1;
	//218.4.232.179 - - [11/Dec/2004:14:43:45 +0100] "GET / HTTP/1.1" 403 319
	EnterCriticalSectionX(&svr_log_cs);
	GetSystemTime(&st);

	if(request){
		tmp = strchr(request,'\r');
		if(!tmp && !(tmp = strchr(request,'\n'))){
			tmp = request + strlen(request) + 1;
		}
	}
	if(st.wMonth > 12){st.wMonth = 1;}

	fprintf(svr_log,"%s - - [%.2u/%s/%.4u:%.2u:%.2u:%.2u GMT] \"",
		inet_ntoa(*(in_addr*)&ip),
		st.wDay,
		svr_months[st.wMonth],
		st.wYear,
		st.wHour,
		st.wMinute,
		st.wSecond);

	if(request && tmp > request){
		fwrite(request,1,(tmp - request - 1),svr_log);
	}

	fprintf(svr_log,"\" %u %u\r\n",code,length);
	fflush(svr_log);
	LeaveCriticalSectionX(&svr_log_cs);
	return 1;
}

char* httpd_mime_reg(const char* extension)
{
	char sPath[MAX_PATH];
	unsigned long dwSize = MAX_PATH;
	unsigned long dwType = REG_SZ;
	HKEY hItem;

	if(RegOpenKeyEx(HKEY_CLASSES_ROOT,extension,0,KEY_READ,&hItem)	== SUCCESS)
	{ 
		if(RegQueryValueEx(hItem,"Content Type",NULL,&dwType,(LPBYTE)sPath,&dwSize)==ERROR_SUCCESS)
		{
			svr_mimes[extension] = sPath;
			return (char*)svr_mimes[extension].data();

			/*/sMIME m = {my_strdup(extension + 1),my_strdup(sPath)};

			RegCloseKey(hItem);

			/*if(!(m.ext) || !(m.type))
			{
				if(m.ext)my_memfree(m.ext);
				if(m.type)my_memfree(m.type);
				return NULL;
			}

			if(svr_mimes.Insert(&m)){
				return m.type;
			}else{
				my_memfree(m.ext);
				my_memfree(m.type);
				return NULL;
			}*/
		}
		else{
			RegCloseKey(hItem);
			return NULL;
		}
	}else{
		return NULL;
	}
}

int httpd_mime_init()
{
	/*if(svr_mimes.InitArray(911,sizeof(sMIME),(SA_COMPARE)httpd_mime_cmp,
		(SA_HASH)httpd_mime_hash,(SA_COMPARE)httpd_mime_set)==FALSE)
	{
		return 0;
	}*/
	return 1;
}

int httpd_vs_init()
{
	sVS vs={0};
	sXmlNode* n;
	const char *tmp;

	/*if(svr_vs.InitArray(253,sizeof(sVS),(SA_COMPARE)httpd_vs_cmp,
		(SA_HASH)httpd_vs_hash,(SA_COMPARE)httpd_vs_set)==FALSE)
	{
		return 0;
	}*/

	n = cSettings.GetNode("mbot/httpd/dirs/dir");
	while(n)
	{
		vs.path = (char*)cSettings.GetProperty(n,"path");
		if(vs.path && *vs.path)
		{
			tmp = cSettings.GetProperty(n,"auth_req");
			vs.auth_req = tmp?(*tmp!='0'):0;
			if(vs.auth_req){
				tmp = cSettings.GetProperty(n,"user");
				vs.user = tmp?tmp:":::";
				tmp = cSettings.GetProperty(n,"pass");
				vs.pass = tmp?tmp:":::";
			}

			tmp = cSettings.GetProperty(n,"ip_mask");
			vs.ip = tmp?tmp:"*";
			svr_vs[vs.path] = vs;
		}
		n = n->next;
	}
	return 1;
}

char* httpd_getmime(char* ext)
{
	std::map<string,string>::iterator loc;
	sMIME m = {(ext+1),NULL};

	if(!ext){
		return "application/octet-stream";
	}
	
	loc = svr_mimes.find(ext);
	if(loc != svr_mimes.end()){
		return (char*)loc->second.data();
	}else{
		char* tmp = httpd_mime_reg(ext);
		return (tmp)?(tmp):("application/octet-stream");
	}
}

char* httpd_getextension(char* filename)
{
	long l = strlen(filename);
	char* c = NULL;

	strlwr((char*)filename);

	if(l<2){
		return NULL;
	}

	c = filename + l - 1;
	while(c != filename)
	{
		if(*c == '.'){
			goto Okay;
		}else{
			c--;
		}
	}
	return NULL;
Okay:
	return c;
}
void  httpd_make_spaces(char* str){
	while(*str){
		if(*str=='+')*str=' ';
		str++;
	}
}
long  httpd_rrecv(SOCKET sock,void *buffer,long length,long timeout = 3000)
{
	if(!length || !timeout){
		return 0;
	}
	int read = 0;

	for(;timeout>0;)
	{
		read = recv(sock,(char*)buffer,length,0);
		if(read > 0)
		{
			return read;
		}
		else if(read < 0 && (WSAGetLastError() == 10035))
		{
			Sleep(50);
			timeout -= 50;
		}
		else
		{
			return 0;
		}
	}
	return 0;
}

char *httpd_strnws(const char* str)
{
	while(*str){
		if(!isspace(*str)){
			return (char*)str;
		}else{
			str++;
		}
	}
	return (NULL);
}

long httpd_count(const char* str,char c)
{
	long count = 0;

	while(*str){
		if(*str == c){
			count ++;
		}
		str++;
	}
	return count;
}
long httpd_writestring(cutSockf* gf, const char* str)
{
	return gf->write((void*)str, strlen(str));
}

long httpd_writeformatted(cutFile* gf, const char* format, ...)
{
	if(!gf){
		return 0;
	}
	char temp[1024];

	va_list args;
	va_start(args,format);
	_vsnprintf(temp, sizeof(temp), format, args);
	va_end(args);

	return gf->writestring(temp);
}

inline long httpd_unify(register char* str)
{
	register long len = 0;
	while(*str){
		if(*str == '\\'){
			*str = '/';
		}
		str++;
		len++;
	}
	return len;
}

char* httpd_hdr_get(const char* table,const char* name)
{
	const char* n = table;
	const char* v;
	while(*n)
	{
		v = n + strlen(n) + 1;
		if(stricmp(name,n)==0){
			return (char*)v;
		}
		n = v + strlen(v) + 1;
	}
	return NULL;
}

long  httpd_queue(SOCKET sock)
{
	sHTTPSync* hs = (sHTTPSync*)my_malloc(sizeof(sHTTPSync));
	if(!hs){
		return 0;
	}else{
		hs->prev = hs->next = NULL;
		hs->s = sock;
		hs->to = GetTickCount() + svr_timeout;
		svr_q_queue.Add(hs);
		SetEvent(svr_q_event);
		return 1;
	}
}
void  httpd_sname(SOCKET sock,cutMemf* out,DWORD* ip)
{
	SOCKADDR_IN sin = {0};
	int ssize = sizeof(sin);
	char* tmp = NULL;
	char val[10];

	if(getsockname(sock,(sockaddr*)&sin,&ssize)!=-1)
	{
		if(out)
		{
			tmp = inet_ntoa(sin.sin_addr);
			if(!tmp)tmp = "0.0.0.0";

			out->write("REMOTE_ADDR",12);
			out->write(tmp,strlen(tmp)+1);

			out->write("REMOTE_PORT",12);
			_snprintf(val,sizeof(val)-1,"%u",ntohs(sin.sin_port));
			out->write(val,strlen(val)+1);
			out->putc(0);
		}
		if(ip){
			*ip = *(DWORD*)&sin.sin_addr;
		}
	}
}

char *httpd_fix(char* str,char* sub)
{
	if(!str)return NULL;
	char* max = (char*) str + strlen(str);

	str += strlen(sub);
	while(str < max)
	{
		if(!isspace(*str)){
			return (char*)str;
		}else{
			str++;
		}
	}
	return NULL;
}

long  httpd_init_hdr(cutMemf* out)
{
	if(!out->create(512)){
		return 0;
	}
	out->write("content-type\0text/html",23);
	out->write("connection\0close",17);
	out->write("pragma\0no-cache",16);
	out->putc(0);
	return 1;
}

void  httpd_send_headers(sHTTPDEnv* env)
{
	const char* v = NULL;
	map<string, string>::iterator it = env->hdr.begin();

	httpd_writeformatted(env->sf,"HTTP/1.0 %u %s\r\n", env->error_code,
		(env->error_code==200) ? "OK": "ERROR");

	for(;it != env->hdr.end(); it++)
	{
		httpd_writeformatted(env->sf, "%s: %s\r\n", it->first.data(), it->second.data());
	}
	env->sf->write("\r\n",2);
}

long  httpd_sendfile(const char* file, sHTTPDEnv* env, char* buffer)
{
	cutDiskf gf;
	cutSockf* sf = env->sf;
	DWORD size = 0;
	DWORD sent = 0;
	DWORD read = 0;
	tm* ft = NULL;
	tm  sft;

	if(!gf.open(file,"rb")){
		return 0;
	}

	size = gf.size();

	sf->writestring("HTTP/1.0 200 OK\r\n");
	httpd_writeformatted(sf,"Content-Type: %s\r\n",httpd_getmime((char*)strrchr(file,'.')));
	httpd_writeformatted(sf,"Content-Length: %u\r\n",size);
	httpd_writeformatted(sf,"X-Powered-By: MBot/%s\r\n",g_mbot_version_s);
	help_filemtime(file,&sent);

	ft = gmtime((const time_t*)&sent);
	if(ft){sft = *ft;}
	sft.tm_wday = min((unsigned int)sft.tm_wday,6);
	sft.tm_mon = min((unsigned int)sft.tm_mon,11);

	httpd_writeformatted(sf,"Last-Modified: %s, %.2u %s %.4u %.2u:%.2u:%.2u GMT\r\n\r\n",
		svr_days[sft.tm_wday+1],
		sft.tm_mday,
		svr_months[sft.tm_mon+1],
		sft.tm_year + 1900,
		sft.tm_hour,
		sft.tm_min,
		sft.tm_sec);

	sent = 0;

	while(sent < size)
	{
		read = gf.read(buffer,1024);
		if(!read)goto Error;
		if(sf->write(buffer,read)!=read)goto Error;
		sent += read;
	}
	gf.close();
	return 1;
Error:
	gf.close();
	return 0;
}

long  httpd_php_cb(long code, void* param1, void* param2, mb_event* mbe)
{
	sHTTPDEnv* env;

	if(!mbe){
		return 0;
	}
	env = (sHTTPDEnv*)mbe->p1;

	if(code == LPHP_CB_SETHDR)
	{
		char* hdr = (char*)param1;
		char* cp = NULL;
		char* sn;
		char* sv;
		char* cx;

		if(env->out_started || !hdr || !(cp = strchr(hdr,':'))){
			return 0;
		}else{
			*cp = 0;
			sn = hdr;
			sv = cp + 1;
			cp = strchr(sv,'\r');
			if(cp){
				*cp = '\0';
				cx = cp - 1;
				while(isspace(*cx) && cx!=sn){
					*(cx--)=0;
				}
			}
			while(isspace(*sv)){sv++;}
			while(isspace(*sn)){sn++;}
			env->hdr[sn] = sv;

			if(stricmp(sn,"location") == 0){
				env->error_code = 301;
			}
			return 1;
		}
		return 0;
	}else if(code == LPHP_CB_OUTSTARTED){//start output
		if(env->out_started == 0){
			httpd_send_headers(env);
			env->out_started = 1;
		}
		return 1;
	}else if(code == LPHP_CB_GETMETHOD){//get method
		return (int)((env->method == 1)?"POST":"GET");
	}else if(code == LPHP_CB_GETCOOKIE){//get cookie
		return (int)httpd_hdr_get((const char*)env->req->getdata(),"HTTP_COOKIE");
	}else if(code == 4){// n/a
		return (int)NULL;
	}else if(code == LPHP_CB_POST_LENGTH){//get post data length
		return (int)env->post_length;
	}else if(code == LPHP_CB_POST_DATA){//get post data pointer
		return (int)env->post_data;
	}else if(code == LPHP_CB_GETENV){
		param2 = (void*)httpd_hdr_get((const char*)env->req->getdata(),(const char*)param1);
		return (long)((param2)?(param2):getenv((const char*)param1));
	}else if(code == LPHP_CB_GETCT){
		return (long)httpd_hdr_get((const char*)env->req->getdata(),"CONTENT_TYPE");
	}else if(code == LPHP_CB_GETCL){
		char* tmp = httpd_hdr_get((const char*)env->req->getdata(),"CONTENT_LENGTH");
		return (tmp)?(strtoul(tmp,NULL,10)):(0);
	}else if(code == LPHP_CB_GETVARS){
		return ((long)env->req->getdata() + env->var_offset);
	}else{
		return NULL;
	}
}

long  httpd_parse_headers(cutMemf* mf, sHTTPDEnv* env)
{
	long pos = mf->tellpos();
	long rcv = 0;
	char buffer[2052];
	char* line = NULL;

	mf->setpos(0);
	while(mf->gets(buffer, sizeof(buffer) - 1))
	{
		rcv ++;
		if(rcv == 1){
			strlwr(buffer);
			if(strncmp(buffer,"get ",4) == 0){
				env->method = sHTTPDEnv::GET;
			}else if(strncmp(buffer,"post ",5) == 0){
				env->method = sHTTPDEnv::POST;
			}else{
				mf->setpos(pos);
				return 0;
			}
		}
		else
		{
			char* ne = strchr(buffer,':');
			char* ns = NULL;
			char* vs = ne + 1;
			
			if(!ne){
				continue;
			}
			if(!(ns = httpd_fix(buffer,""))){
				continue;
			}
			*ne = '\0';

			if(!(vs = httpd_fix(vs,""))){
				continue;
			}
			strupr(ns);
			for(ne=ns;*ne;ne++){
				if(*ne == ' ' || *ne == '-'){
					*ne = '_';
				}
			}

			if(strncmp(ns,"CONTENT_",8)){
				env->req->write("HTTP_",5);
			}
			env->req->write(ns, strlen(ns) + 1);
			env->req->write(vs, strlen(vs) + 1);
		}
	}

	mf->setpos(pos);
	return (rcv != 0);
}

long httpd_authorize(const char* b64pass,char* user=0,char* pass=0)
{
	cBase64 b64;
	char* str_tmp = NULL;
	char* str_tmp2 = NULL;
	char  str_buff[128];
	extern FILE* dbgout;

	if(!b64pass){
		/*@*///fprintf(dbgout,"%d - Base64 not given\r\n",__LINE__);
		return FALSE;
	}

	str_tmp = (char*)b64pass;
	if(!(str_tmp = strchr(str_tmp,' '))){
		/*@*///fprintf(dbgout,"%d - Could not find ' '\r\n",__LINE__);
		goto Error;
	}
	str_tmp++;
	if(!b64.Decode(str_tmp)){
		/*@*///fprintf(dbgout,"%d - Error decoding Base64\r\n",__LINE__);
		goto Error;
	}

	str_tmp = (char*)b64.GetBuffer();
	if(!str_tmp || httpd_count(str_tmp,':')!=1 || strlen(str_tmp)<3){
		/*@*///fprintf(dbgout,"%d - Could not find ':'\r\n",__LINE__);
		goto Error;
	}
	str_tmp2 = strchr(str_tmp,':');
	if(!str_tmp2){
		/*@*///fprintf(dbgout,"%d - Could not find ':'\r\n",__LINE__);
		goto Error;
	}

	*str_tmp2='\0';
	str_tmp2++;

	if(!user){
		/*@*///fprintf(dbgout,"%d - No username given\r\n",__LINE__);
		MBotGetPrefString("WWWUser",str_buff,sizeof(str_buff)-1,"::::");
		user = str_buff;
	}
	if(strcmp(str_tmp,user)!=0)goto Error;
	/*@*///fprintf(dbgout,"%d - strcmp(%s,%s)\r\n",__LINE__,str_tmp,user);

	if(!pass){
		/*@*///fprintf(dbgout,"%d - No password given\r\n",__LINE__);
		MBotGetPrefString("WWWPass",str_buff,sizeof(str_buff)-1,"::::");
		pass = str_buff;
	}

	/*@*///fprintf(dbgout,"%d - strcmp(%s,%s)\r\n",__LINE__,str_tmp2,pass);
	if(strcmp(str_tmp2,pass)!=0)goto Error;

	/*@*///fprintf(dbgout,"%d - Authorization SUCCESS\r\n",__LINE__);
	return TRUE;
Error:
	/*@*///fprintf(dbgout,"%d - Authorization failed\r\n",__LINE__);
	return FALSE;
}

//
long httpd_authorize_host(char* path,char* ip,char* b64pass)
{
	sVS vs = {0};
	char*	tmp = strrchr(path,'/');
	char*	ntmp;
	char	rw = '/';
	int		root = 0;
	int		result = 0;
	map<string,sVS>::iterator loc;

	if(!tmp)return FALSE;

	if(tmp != path){
		*tmp = '\0';
	}else{
		rw = *(tmp + 1);
		*(tmp + 1) = '\0';
		root = 1;
	}

	vs.path = path;
	while(tmp)
	{
		loc = svr_vs.find(vs.path);
		if(svr_vs.end() == loc)
		{
			if(path == tmp){
				goto Okay;
			}
			ntmp = strrchr(path,'/');
			*tmp = (tmp==path)?rw:'/';
			if(!ntmp){
				goto Okay;
			}
			tmp = ntmp;
			if(tmp == path){
				rw = *(tmp + 1);
				root = 1;
				*(tmp+1) = '\0';
			}else{
				rw = '/';
				*tmp = '\0';
			}
		}else{
			vs = loc->second;
			break;
		}
	}
	
	if(!vs.ip){
		goto Okay;
	}

	if( *vs.ip && false == ut_str_match(vs.ip, ip)){
		result = -1;
		goto Error;
	}

	if(vs.auth_req)
	{
		if(!httpd_authorize(b64pass,vs.user?vs.user:"::::",vs.pass?vs.pass:"::::")){
			goto Error;
		}
	}

	*(tmp+root) = (root)?rw:'/';
Okay:
	return TRUE;
Error:
	*(tmp+root) = (root)?rw:'/';
	return result;
}

long httpd_client_thread(SOCKET sock)
{
	static timeval tv = {30,30};

	long  rcv = 0;
	long  post = 0;
	long  post_len = 0;
	long  post_got = 0;
	long  ip = 0;
	long  ok = 0;
	long  fs = 0;
	char* request = NULL;
	char* str_tmp = NULL;
	char* str_tmp2 = NULL;
	char* cgi_params = NULL;
	char* query_str = NULL;
	char* req_page;
	char  buffer[1028];

	sHTTPDEnv httpd_env;
	cutSockf sf;
	cutMemf mf;
	cutMemf hdr;
	cutMemf req;
	sFDS fd;
	mb_event mbe = {MBT_WEBPAGE, 0};

	httpd_sname(sock,NULL,(DWORD*)&ip);

	if(!mf.create(4*1024) || !(req.create(1024))){
		//malloc error
		goto Error500;
	}
	////////////////////////////////////////
	sf.open(sock);
	////////////////////////////////////////
	*fd.fd_array = sock;
	 fd.fd_count = 1;
	////////////////////////////////////////
	httpd_env.sf = &sf;
	httpd_env.mf = &mf;
	httpd_env.req = &req;
	////////////////////////////////////////

	while(select(0,(fd_set*)&fd,0,0,&tv) >= 1 && mf.tellpos() < svr_maxrequest)
	{
		if(!(rcv = httpd_rrecv(sock,buffer,1024,3000))){
			//recv error
			httpd_logaccess((const char*)mf.getdata(),500,sf.size(),ip);
			goto End;
		}
		if(!mf.write(buffer,rcv)){
			goto Error500;
		}
		mf.putc(0);

		if(post == FALSE)
		{
			if((rcv = mf.size()) > 4 && (request = (char*)strstr((const char*)mf.getdata(),"\r\n\r\n")))
			{
				httpd_env.var_offset = req.written();
				if(httpd_parse_headers(&mf, &httpd_env) != 1){
					goto Error;
				}
				req.setpos(0);

				if(httpd_env.method == 1)
				{
					post = TRUE;
					str_tmp = httpd_hdr_get((const char*)req.getdata(),"CONTENT_LENGTH");
					post_len = (str_tmp)?(strtoul(str_tmp,NULL,10)):(0);
					post_got = rcv - (request - ((const char*)mf.getdata()) + 4);
					if(!post_len || post_got >= post_len){
						ok = TRUE;
						break;
					}
				}
				else
				{
					ok = TRUE;
					break;
				}
			}
		}
		else
		{
			post_got += rcv;
			if(post_got >= post_len){
				ok = TRUE;
				break;
			}
		}
	}
	////////////////////////////////////////
	if(!ok || mf.size() < 10 || !(request = (char*)mf.getdata())){
		goto Error;
	}
	////////////////////////////////////////
	if(request[4 + post] != '/')goto Error;
	
	{
		str_tmp = strchr(request + 5,' ');
		if(!str_tmp)goto Error;
		*str_tmp = '\0';
		query_str = strchr(request + 5,'?');
		*str_tmp = ' ';
	}

	if(post == TRUE)
	{
		cgi_params = strstr(request,"\r\n\r\n");
		if(!cgi_params)goto Error;
		httpd_env.post_data = cgi_params + 4;
		httpd_env.post_length = post_len;

		if(post_got > post_len){
			*(httpd_env.post_data + post_len) = '\0';
		}
		cgi_params = NULL;
	}

	if(httpd_env.error_code != 200)
	{
		httpd_writeformatted(&sf,"HTTP/1.0 %u ERROR\r\nConnection: close\r\nPragma: no-cache\r\nContent-Type: text/html\r\n\r\n<h4>%u ERROR</h4>",httpd_env.error_code,httpd_env.error_code);
		goto End;
	}
	////////////////////////////////////////
	if(query_str)
	{
		*query_str = '\0';
		req_page = request + 4 + post;
		//*cgi_params = '?';
	}
	else
	{
		str_tmp = strchr(request + 4 + post,' ');
		if(!str_tmp)goto Error;
		*str_tmp = '\0';
		req_page = request + 4 + post;
		//*str_tmp = ' ';
	}

	///////////////////////////
	if(svr_auth_required && !httpd_authorize(
		httpd_hdr_get((const char*)req.getdata(),"HTTP_AUTHORIZATION")
	))
	{
		goto Error401;
	}
	///////////////////////////
	strlwr(req_page);
	httpd_unify(req_page);
	rcv = strlen(req_page);

	str_tmp = httpd_hdr_get((const char*)req.getdata(),"HTTP_AUTHORIZATION");
	str_tmp2 = inet_ntoa(*(in_addr*)&ip);


	strncpy(buffer,req_page,sizeof(buffer)-1);
	ok = httpd_unify(buffer);
	if(ok && buffer[ok-1]!='/'){buffer[ok]='/'; buffer[ok+1]='\0';}

	if((ok = httpd_authorize_host(buffer,str_tmp2?str_tmp2:"0.0.0.0",str_tmp?str_tmp:"")) < 1){
		if(ok < 0){
			goto Error403;
		}else{
			goto Error401;
		}
	}

	///////////////////////////
	if(!(*req_page)){
		_snprintf(buffer,MAX_PATH,"%s/index.php",svr_wwwroot);
		req_page = "/index.php";
	}else if(req_page[rcv-1]=='/'){
		_snprintf(buffer,MAX_PATH,"%s%sindex.php",svr_wwwroot,req_page);
		req_page = buffer + strlen(svr_wwwroot);
	}else{
		_snprintf(buffer,MAX_PATH,"%s%s",svr_wwwroot,req_page);
	}
	httpd_unify(buffer);

	ok = help_fileexists(buffer,&fs);

	if(!ok){
		ok = help_direxists(buffer);
		if(!ok){
			goto Error404;
		}else{
			httpd_writeformatted(&sf,"HTTP/1.0 301 Moved Permanently\r\nConnection: close\r\nLocation: %s/\r\nPragma: no-cache\r\nContent-Type: text/html\r\n\r\n<h4>301 Document moved permanently!</h4>",req_page);
			goto End;
		}
	}else{
		//do the authorization for vhosts
		str_tmp = httpd_getextension(buffer);

		if(str_tmp && true == ut_str_match(".php*", str_tmp))
		{
			if(query_str){
				*query_str = '?';
				if((str_tmp2 = strchr(query_str+1,' ')) || (str_tmp2 = strchr(query_str,'\r'))){
					*str_tmp2 = '\0';
				}
			}

			mbe.t1 = MBE_HTTPDENV;
			mbe.p1 = (void*)&httpd_env;

			if(!httpd_init_hdr(&hdr)){
				goto Error500;
			}

			httpd_sname(sock,&req,NULL);
			req.write("SERVER_SOFTWARE\0MBot (c) Piotr Pawluczuk (www.piopawlu.net)",60);
			//SCRIPT_NAME
			req.write("SCRIPT_NAME",12);
			req.write(req_page,strlen(req_page)+1);
			//PHP_SELF
			req_page = strrchr(req_page,'/');
			if(!req_page){goto Error404;}
			req_page++;
			req.write("PHP_SELF",9);
			req.write(req_page,strlen(req_page)+1);
			//END OF VARIABLES
			req.putc(0);

			if(!LPHP_ExecutePage(buffer,(query_str)?(query_str+1):NULL,
				(const char**)&sf,(void*)&mbe,(LPHP_ENVCB)httpd_php_cb,1)){
				goto Error500;
			}

			if(httpd_env.out_started == 0){
				httpd_send_headers(&httpd_env);
			}
			httpd_logaccess((const char*)mf.getdata(),200,sf.size(),ip);
			goto End;
		}
		else
		{
			if(httpd_sendfile(buffer,&httpd_env,buffer)){
				httpd_logaccess((const char*)mf.getdata(),200,sf.size(),ip);
			}else{
				httpd_logaccess((const char*)mf.getdata(),404,sf.size(),ip);
			}
			goto End;
		}
	}

	///////////////////////////
Error:
	///////////////////////////
	httpd_logaccess((const char*)mf.getdata(),400,sf.size(),ip);
	httpd_writestring(&sf,"HTTP/1.0 400 Bad Request\r\nConnection: close\r\nPragma: no-cache\r\nContent-Type: text/html\r\n\r\n<h4>400 Bad Request</h4>");
	goto End;
	///////////////////////////
Error500:
	///////////////////////////
	httpd_logaccess((const char*)mf.getdata(),500,sf.size(),ip);
	if(httpd_env.out_started == 0)
	{
		httpd_writestring(&sf,"HTTP/1.0 500 Internal Server Error\r\nConnection: close\r\nPragma: no-cache\r\nContent-Type: text/html\r\n\r\n<h4>500 Internal Server Error</h4>");
	}
	goto End;
	///////////////////////////
Error401:
	///////////////////////////
	httpd_logaccess((const char*)mf.getdata(),401,sf.size(),ip);
	httpd_writestring(&sf,"HTTP/1.0 401 Authorization Required\r\nWWW-Authenticate: Basic realm=\"MSP Server HTTPD\"\r\nstatus: 401 Unauthorized\r\nConnection: close\r\nPragma: no-cache\r\nContent-Type: text/html\r\n\r\n<h4>401 Authorization Required</h4>");
	goto End;
	///////////////////////////
Error403:
	///////////////////////////
	httpd_logaccess((const char*)mf.getdata(),401,sf.size(),ip);
	httpd_writestring(&sf,"HTTP/1.0 403 Access DENIED\r\nWWW-Authenticate: Basic realm=\"MSP Server HTTPD\"\r\nstatus: 403 Access DENIED\r\nConnection: close\r\nPragma: no-cache\r\nContent-Type: text/html\r\n\r\n<h4>403 Access DENIED</h4>");
	goto End;
	///////////////////////////
Error404:
	///////////////////////////
	httpd_logaccess((const char*)mf.getdata(),404,sf.size(),ip);
	httpd_writestring(&sf,"HTTP/1.0 404 Not Found\r\nConnection: close\r\nPragma: no-cache\r\nContent-Type: text/html\r\n\r\n<h4>404 Not Found</h4>");
	///////////////////////////
End:
	///////////////////////////
	sf.close();
	mf.close();
	svr_cur_clients --;
	return 0;
}

long WINAPI httpd_launch_client(void* st)
{
	httpd_client_thread((SOCKET)st);
	svr_q_tc--;
	SetEvent(svr_q_event);
	return 0;
}
long WINAPI httpd_server_queue(void* param)
{
	static sHTTPSync* task;
	static HANDLE hThread;
	static DWORD  result;

	while(1)
	{
		if((result = WaitForSingleObject(svr_q_event,INFINITE)) == WAIT_OBJECT_0)
		{
			while(svr_q_tc < svr_max_clients)
			{
				svr_q_queue.Lock();
				task = (sHTTPSync*)svr_q_queue.m_head;

				if(!task)
				{
					svr_q_queue.Unlock();
					break;
				}
				else
				{
					svr_q_queue.DelLocked(task);
					svr_q_queue.Unlock();
				}

				if((DWORD)task->to < GetTickCount())
				{
					closesocket((SOCKET)task->s);
					my_memfree(task);
				}
				else
				{
					svr_q_tc++;
					hThread = CreateThread(NULL,NULL,(LPTHREAD_START_ROUTINE)httpd_launch_client,(LPVOID)task->s,NULL,&result);

					my_memfree(task);

					if(!hThread)
					{
						svr_q_tc--;
					}
					else
					{
						CloseHandle(hThread);
					}
				}//else
			}//while
		}//if
		else
		{
			return 0;
		}
	}//while(1)
	return 1;
}

long WINAPI httpd_server_thread(void* param)
{
	static WSADATA wsad = {0};
	static SOCKADDR_IN server = {0};
	static SOCKADDR_IN client = {0};
	static SOCKET as = INVALID_SOCKET;
	static int client_size = sizeof(client);

	server.sin_family = AF_INET;
	server.sin_port = htons(DBGetContactSettingWord(NULL,MBOT,"WWWPort",8081));
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	MBotGetPrefString("WWWIPMask",svr_allowed_ip,sizeof(svr_allowed_ip)-1,"*");
	if(!MBotGetPrefString("WWWRoot",svr_wwwroot,sizeof(svr_wwwroot)-1,""))
{
		_snprintf(svr_wwwroot,sizeof(svr_wwwroot)-1,"%s/mbot/www",g_root);
	}

	svr_wwwroot_len = strlen(svr_wwwroot);

	svr_auth_required = DBGetContactSettingByte(NULL,MBOT,"WWWAuthR",0);
	svr_max_clients = DBGetContactSettingWord(NULL,MBOT,"WWWMaxC",5);
	svr_timeout = DBGetContactSettingWord(NULL,MBOT,"WWWTimeout",60);

	if(svr_max_clients == 0 || svr_max_clients > 20){
		svr_max_clients = 5;
	}
	if(svr_timeout < 10 || svr_timeout > 120){
		svr_timeout = 30;
	}
	svr_timeout *= 1000;

	if(WSAStartup(MAKEWORD(2,2),&wsad) == SOCKET_ERROR)
	{
		MB_Popup("HTTPDeamon","Could not initialize sockets!");
		httpd_logaccess("ERROR: Could not initialize sockets!",GetLastError(),0,0);
		goto Error;
	}

	if(!httpd_mime_init()){
		MB_Popup("HTTPDeamon","Could not initialize MIME table!");
		httpd_logaccess("ERROR: Could not initialize MIME table!",GetLastError(),0,0);
		goto Error;
	}

	if(!httpd_vs_init()){
		MB_Popup("HTTPDeamon","Could not initialize VS table!");
		goto Error;
	}


	svr_ls = socket(AF_INET,SOCK_STREAM,0);

	if(svr_ls == INVALID_SOCKET)
	{
		MB_Popup("HTTPDeamon","Could not create socket!");
		httpd_logaccess("ERROR: Could not create socket!",GetLastError(),0,0);
		goto Error;
	}

	if(bind(svr_ls,(sockaddr*)&server,sizeof(server))==-1)
	{
		MB_Popup("HTTPDeamon","Could not bind the server socket!");
		httpd_logaccess("ERROR: Could not bind the server socket!",GetLastError(),0,0);
		closesocket(svr_ls);
		goto Error;
	}

	httpd_logaccess("SYSTEM: Server started!",0,0,0);

	while(1)
	{
		if(listen(svr_ls,5)==-1){
			closesocket(svr_ls);
			MB_Popup("HTTPDeamon","An error occured while listening!");
			httpd_logaccess("ERROR: An error occured while listening!",GetLastError(),0,0);
			goto Error;
		}

		if((as = accept(svr_ls,(sockaddr*)&client,(int*)&client_size))!=INVALID_SOCKET)
		{
			char* client_ip = inet_ntoa(client.sin_addr);

			if(false == ut_str_match(svr_allowed_ip, client_ip)){
				closesocket(as);
			}
			else
			{
				if(svr_q_queue.m_count < 100){
					httpd_queue(as);
				}else{
					httpd_logaccess("SYSTEM: Queue FULL!",svr_q_queue.m_count,0,0);
					closesocket(as);
				}
			}
		}
	}
Error:
	svr_mimes.clear();
	svr_vs.clear();

	if(svr_ls != INVALID_SOCKET){
		closesocket(svr_ls);
		svr_ls = INVALID_SOCKET;
	}
	CloseHandle(svr_hTh);
	return 0;
}

long httpd_startup()
{
	static unsigned long tmpid;
	char tmp[MAX_PATH];

	////////////
	tzset();
	////////////

	svr_q_event = CreateEvent(0,0,0,0);
	if(!svr_q_event){
		MB_Popup("HTTPDeamon","Could not create queue sync event!");
		httpd_logaccess("ERROR: Could not create queue sync event!",GetLastError(),0,0);
		return 0;
	}
	if(!(svr_q_hTh = CreateThread(NULL,NULL,(LPTHREAD_START_ROUTINE)httpd_server_queue,NULL,0,&tmpid)))
	{
		MB_Popup("HTTPDeamon","Could not create queue thread!");
		httpd_logaccess("ERROR: Could not create queue thread!",GetLastError(),0,0);
		CloseHandle(svr_q_event);
		svr_q_event = NULL;
		return 0;
	}

	if(DBGetContactSettingByte(NULL,MBOT,"WWWLog",1)==1){
		InitializeCriticalSectionAndSpinCount(&svr_log_cs,0x80000100);
		_snprintf(tmp,sizeof(tmp)-1,"%s\\mbot\\httpd.log",g_root);
		svr_log = fopen(tmp,"a+b");
		if(!svr_log){
			MB_Popup("HTTPDeamon","Could not create log file!");
			httpd_logaccess("ERROR: Could not create log file!",GetLastError(),0,0);
			DeleteCriticalSection(&svr_log_cs);
		}
	}

	if(!(svr_hTh = CreateThread(NULL,NULL,(LPTHREAD_START_ROUTINE)httpd_server_thread,NULL,0,&tmpid)))
	{
		MB_Popup("HTTPDeamon","Could not create server thread!");
		httpd_logaccess("ERROR: Could not create server thread!",GetLastError(),0,0);

		CloseHandle(svr_q_event);
		TerminateThread(svr_q_hTh,0);
		CloseHandle(svr_q_hTh);

		svr_q_event = NULL;
		svr_q_hTh = NULL;
		return 0;
	}
	return 1;
}

long httpd_shutdown()
{
	if(svr_hTh)
	{
		TerminateThread(svr_hTh,0);
		CloseHandle(svr_hTh);
		svr_hTh = NULL;
	}
	if(svr_q_hTh)
	{
		TerminateThread(svr_q_hTh,0);
		CloseHandle(svr_q_hTh);
		svr_q_hTh = NULL;
	}
	if(svr_q_event){
		CloseHandle(svr_q_event);
		svr_q_event = NULL;
	}
	if(svr_ls && svr_ls!=INVALID_SOCKET){
		closesocket(svr_ls);
		svr_ls = 0;
	}

	httpd_logaccess("SYSTEM: Server stopped!",0,0,0);
	if(svr_log){
		DeleteCriticalSection(&svr_log_cs);
		fclose(svr_log);
		svr_log = NULL;
	}

	svr_mimes.clear();
	svr_vs.clear();
	return 1;
}

#endif //_NOHTTPD_