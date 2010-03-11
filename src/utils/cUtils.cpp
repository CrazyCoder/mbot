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
#include "includes.h"
#include "cUtils.h"

cutMemf::cutMemf(void* data,unsigned long length)
{
	m_ptr = (unsigned char*)data;
	m_end = m_ptr + length;
	m_data = m_ptr;
	m_flags = 0;
	m_maxptr = m_ptr;
}

cutMemf::cutMemf()
{
	m_data = m_ptr = m_end = m_maxptr = 0;
	m_flags = 0;
}

cutMemf::~cutMemf()
{
	close();
}

int cutMemf::close()
{
	if(m_data && (m_flags & 0x01)){
		free(m_data);
	}
	m_data = NULL;
	m_end = NULL;
	m_ptr = NULL;
	m_maxptr = NULL;
	m_flags = 0;
	return 1;
}

int cutMemf::create(unsigned long len)
{
	close();
	m_data = (unsigned char*)malloc(len);
	if(!m_data)return 0;
	m_ptr = m_data;
	m_end = m_data + len;
	m_maxptr = m_data;
	m_flags = 0x01;
	return 1;
}

void cutMemf::assign(void* data, unsigned long size)
{
	close();
	m_data = (unsigned char*)data;
	m_ptr = m_data;
	m_end = m_data + size;
	m_maxptr = m_end - 1;
	m_flags = 0x01;
}

int cutMemf::load(const char* f,int off)
{
	cutDiskf df;
	long fs;

	close();
	if(!df.open(f,"rb")){
		goto Error;
	}
	fs = df.size();
	if(fs <= off){
		goto Error;
	}
	fs -= off;
	df.setpos(off);
	if(!create(fs + 1)){
		goto Error;
	}
	if(!df.read(m_data,fs)){
		goto Error;
	}
	df.close();
	m_maxptr = m_data + fs;
	m_data[fs] = 0;
	return 1;
Error:
	return 0;
}

int cutMemf::resize(unsigned long ns)
{
	//safe
	void* tmp = malloc(ns);
	if(!tmp){
		return 0;
	}
	memcpy(tmp,m_data,m_end - m_data);
	m_ptr = (unsigned char*)tmp + (m_ptr - m_data);
	m_maxptr = (unsigned char*)tmp + (m_maxptr - m_data);
	m_end = (unsigned char*)tmp + ns;
	free(m_data);
	m_data = (unsigned char*)tmp;
	return 1;
}

int cutMemf::removeblock(unsigned long off, unsigned long length)
{
	if (m_data + off + length > m_maxptr){
		return 0;
	}
	memcpy(m_data + off, m_data + off + length, (m_maxptr - m_data) - off - length);
	m_end -= length;
	return 1;
}

int cutMemf::read(void* out,unsigned long n)
{
	n = min(m_end - m_ptr,n);
	if(!n)return 0;
	memcpy(out,m_ptr,n);
	m_ptr += n;
	return n;
}

int cutMemf::gets(char* out, unsigned long n)
{
	int c = 0, l = 0;

	while((c = getc()) > 0x10 && n > 0){
		*out++ = c;
		n--;
		l++;
	}
	if(c == '\r'){
		c = getc();
	}
	if(c == '\n'){
		getc();
	}
	out[l] = 0;
	return l;
}

int cutMemf::write(void* in,unsigned long n)
{
	if(!m_flags){
		n = min(m_end - m_ptr,n);
		if(!n){
			return 0;
		}
	}else{
		if(m_ptr + n > m_end){
			if(!resize(round((m_end - m_data) + n,2048))){
				return 0;
			}
		}
	}
	memcpy(m_ptr,in,n);
	m_ptr += n;
	if(m_ptr > m_maxptr){
		m_maxptr = m_ptr;
	}
	return n;
}

int cutMemf::setpos(unsigned long pos)
{
	if(m_data + pos >= m_end)return 0;
	m_ptr = m_data + pos;
	return 1;
}


cutDiskf::cutDiskf()
{
	m_fp = NULL;
}
cutDiskf::~cutDiskf()
{
	this->close();
}

int cutDiskf::open(const char* f,const char* mode)
{
	close();
	return (m_fp = (void*)fopen(f,mode))!= NULL;
}

int cutDiskf::read(void* out,unsigned long n)
{
	return (m_fp)?(fread(out,1,n,(FILE*)m_fp)):0;
}
int cutDiskf::write(void* in,unsigned long n)
{
	return (m_fp)?(fwrite(in,1,n,(FILE*)m_fp)):0;
}
int cutDiskf::size()
{
	int tp = tellpos();
	int fs;
	fseek((FILE*)m_fp,0,SEEK_END);
	fs = tellpos();
	fseek((FILE*)m_fp,tp,SEEK_SET);
	return fs;
}
int cutDiskf::tellpos()
{
	return (m_fp)?(ftell((FILE*)m_fp)):0;
}
int cutDiskf::setpos(unsigned long pos)
{
	return (m_fp)?(!fseek((FILE*)m_fp,pos,SEEK_SET)):0;
}

int cutDiskf::seek(int pos,int method)
{
	return (m_fp)?(!fseek((FILE*)m_fp,pos,method)):0;
}
int cutDiskf::close()
{
	if(m_fp){
		return fclose((FILE*)m_fp);
	}else{
		return 0;
	}
}
int cutDiskf::getc()
{
	return (m_fp)?(fgetc((FILE*)m_fp)):0;
}

cutDiskf::FTYPE cutDiskf::checkFile(const char* path)
{
	_finddata_t fd = {0};
	intptr_t sh = _findfirst(path, &fd);

	if(sh == -1){
		return FT_NONE;
	}else{
		_findclose(sh);
		if(fd.attrib & _A_SUBDIR){
			return FT_DIRECTORY;
		}else{
			return FT_FILE;
		}
	}
}

int ut_split(char* str, char chr, char** out, int max)
{
	int n = 0;
	char *lchr = str;

	while(*str && max > 0)
	{
		if(*str == chr){
			*str = 0;
			out[n++] = lchr;
			lchr = str + 1;
		}
		str++;
	}
	return n;
}

void ut_str_replace(const char* from, const char* to, std::string& src)
{
	std::string::size_type loc = 0;

	if(!strcmp(from, to))return;

	while(std::string::npos != (loc = src.find(from, 0)))
	{
		src.replace(loc, strlen(from), to);
	}
}

bool ut_str_match(const char* tmpl, const char* str)
{
	const char* c1 = tmpl;
	const char* c2 = str;
	unsigned l1 = strlen(tmpl);
	unsigned l2 = strlen(str);	

	while(*c1 && *c2)
	{
		if(*c1 == '?'){
			c1++;
			c2++;
		}else if(*c1 == '*'){
			c1++;
			if(*c1 == '\0'){
				return true;
			}else if(*c1 == '?'){
				c2 = c2 + (strlen(c2) - strlen(c1));
			}else{
				c2 = strchr(c2,*c1);
				if(!c2){
					return false;
				}
			}
		}else{
			if(*c1 != *c2){
				return false;
			}
			c1++;
			c2++;
		}
	}

	if((!(*c1) || (*c1) == '*') && !(*c2)){
		return true;
	}else{
		return false;
	}
}