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
#ifndef _cUTILS_H__
#define _cUTILS_H__

#include <winsock2.h>
#include <string>

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif


class cutFile
{
#ifdef getc
#undef getc
#undef putc
#endif
public:
	virtual int read(void* out,unsigned long n)=0;
	virtual int write(void* out,unsigned long n)=0;
	virtual int writestring(const char* str){return write((void*)str, strlen(str));}
	virtual int size()=0;
	virtual int tellpos()=0;
	virtual int setpos(unsigned long pos)=0;
	virtual int close()=0;
	virtual int getc()=0;
	virtual int putc(int c)=0;
};

class cutSockf : public cutFile
{
public:
	int open(SOCKET s){m_written = 0; m_sock = s; return 1;}
	int read(void* out,unsigned long n){return recv(m_sock, (char*)out, n, 0);}
	int write(void* out,unsigned long n){m_written += n; return send(m_sock, (const char*)out, n, 0);}
	int writestring(const char* str){return write((void*)str, strlen(str));}
	int size(){return m_written;}
	int tellpos(){return 0;}
	int setpos(unsigned long pos){return 0;};
	int close(){closesocket(m_sock); return 1;}
	int getc(){int c=0; read(&c,1); return c;}
	int putc(int c){return write(&c, 1);}
protected:
	SOCKET m_sock;
	int m_written;
};

class cutDiskf : public cutFile
{
public:
	enum FTYPE{FT_NONE, FT_DIRECTORY, FT_FILE};
public:
	cutDiskf();
	~cutDiskf();
public:
	int open(const char* f,const char* mode);
	//int open(const wchar_t* f, const wchar_t* mode);
	int read(void* out,unsigned long n);
	int write(void* in,unsigned long n);
	int size();
	int tellpos();
	int seek(int pos,int method);
	int setpos(unsigned long pos);
	int close();
	int getc();
	int putc(int c){return fputc(c, (FILE*)m_fp);}
	void* getfp(){return m_fp;}

	static FTYPE checkFile(const char* path);

	operator FILE* (){return (FILE*)m_fp;}
protected:
	void* m_fp;
};

class cutMemf : public cutFile
{
public:
	cutMemf();
	cutMemf(void* data,unsigned long length);
	~cutMemf();
public:
	int create(unsigned long len);
	int read(void* out,unsigned long n);
	int write(void* in,unsigned long n);
	int size(){return (m_end - m_data);}
	unsigned long written(){return m_maxptr - m_data;}
	int tellpos(){return (m_ptr - m_data);}
	int setpos(unsigned long pos);
	int getc(){return (m_ptr<m_end)?(*m_ptr++):0;}
	int gets(char* out, unsigned long n);
	int putc(int c){ return (m_ptr < m_end)?(*m_ptr++ = c):(0); }
	void* getdata(){return m_data;}
	void* leave(){void* tmp = m_data; m_data = m_ptr = m_end = m_maxptr = NULL; return tmp;}
	void assign(void* data, unsigned long size);
	int close();
	int load(const char* f,int off);
	int removeblock(unsigned long off, unsigned long length);
protected:
	int resize(unsigned long ns);
	inline unsigned long round(unsigned long a,unsigned long b){
		return (a + ((a%b)?(b-(a%b)):0));
	}
public:
	unsigned char* m_data;
	unsigned char* m_ptr;
	unsigned char* m_end;
	unsigned char* m_maxptr;
	unsigned long  m_flags;
};

#define mb2u(a) a
int  ut_split(char* str, char chr, char** out, int max = 10);
void ut_str_replace(const char* from, const char* to, std::string& src);
bool ut_str_match(const char* tmpl, const char* str);

#endif //_cUTILS_H__