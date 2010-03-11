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
#include "cBase64.h"

#define BASE64_BM 0x000000FC

static const char* base64_etable;
static unsigned char base64_dtable[256];
static bool base64_dtable_ok = false;

cBase64::cBase64()
{
	if(base64_dtable_ok == false)
	{
		int i = 0;
		memset(base64_dtable,0xAAAAAAAA,sizeof(base64_dtable));
		for(const char* c = base64_etable; i<67; i++, c++){
			base64_dtable[*c] = i;
		}
		base64_etable = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=\0";
		base64_dtable_ok = true;
	}
}

cBase64::~cBase64()
{

}

inline bool cBase64::IsValidB64(register char c)
{
	return (base64_dtable[c] != 0xAA);
}

int cBase64::EncodeRAW(unsigned char* pszIn, int length)
{
	m_data.close();

	if(!m_data.create((length * 3) / 2)){
		return 0;
	}

	int tocopy = 0;
	int encoded = 0;

	while(encoded < length)
	{
		QUAD q = {0};
		tocopy = min(3, length - encoded);
		q.size = tocopy;

		memcpy(&q, pszIn + encoded, tocopy);

		EncodeQuad(q);

		if(!m_data.write(&q,4)){
			goto Error;
		}
		encoded += tocopy;
	}
	m_data.putc(0);
	return encoded;
Error:
	m_data.close();
	return 0;
}

int cBase64::Encode(const char* pszData)
{
	return this->EncodeRAW((unsigned char*)pszData, strlen(pszData));
}

int cBase64::Decode(const char* pszIn)
{
	int len = strlen(pszIn);
	int done = 0;
	unsigned long *quads = (unsigned long*)pszIn;

	if(!len || len % 4)return 0;

	for(const char* c=pszIn;*c;c++){
		if(!IsValidB64(*c)){
			return 0;
		}
	}

	m_data.close();
	if(!m_data.create(len / 3 + 5)){
		return 0;
	}

	while(done < len)
	{
		QUAD q ={0};
		q.size = 4;
		q.d.bDword = *quads++;

		DecodeQuad(q);
		if(!m_data.write(&q.d.bDword,q.size)){
			goto Error;
		}
		done += 4;
	}
	m_data.putc(0);
	return 1;
Error:
	m_data.close();
	return 0;
}

int cBase64::GetString(char* pszOut)
{
	strcpy(pszOut, (const char*)m_data.getdata());
	return m_data.size();
}

int cBase64::DecodedMessageSize()
{
	return m_data.size();
}

const char* cBase64::GetBuffer()
{
	return (const char*)m_data.getdata();
}

void cBase64::EncodeQuad(QUAD &quad)
{
	QUAD out = {0};
	unsigned char tmp_2 = 0;

	tmp_2 = ((quad.d.bData[0] & BASE64_BM) >> 2);
	out.d.bData[0] = base64_etable[tmp_2];

	tmp_2 = ((quad.d.bData[0] & 0x03) << 4) | ((quad.d.bData[1] & 0xF0) >> 4);
	out.d.bData[1] = base64_etable[tmp_2];

	tmp_2 = ((quad.d.bData[1] & 0x0F) << 2) | ((quad.d.bData[2] & 0xC0) >> 6);
	out.d.bData[2] = base64_etable[tmp_2];

	out.d.bData[3] = base64_etable[quad.d.bData[2] & 0x3F];

	if(quad.size == 1)
	{
		out.d.bData[2] = '=';
		out.d.bData[3] = '=';
	}
	else if(quad.size == 2)
		out.d.bData[3] = '=';

	out.size = 4;
	quad = out;
}

void cBase64::DecodeQuad(QUAD &quad)
{
	QUAD out = {0};
	unsigned char tmp_0 = 0;
	unsigned char tmp_1 = 0;

	quad.d.bData[0] = base64_dtable[quad.d.bData[0]];
	quad.d.bData[1] = base64_dtable[quad.d.bData[1]];
	quad.d.bData[2] = base64_dtable[quad.d.bData[2]];
	quad.d.bData[3] = base64_dtable[quad.d.bData[3]];

	if(quad.d.bData[2]==0xff && quad.d.bData[3]==0xff)
		out.size = 1;
	else if(quad.d.bData[3] == 0xff)
		out.size = 2;
	else
		out.size = 3;

	tmp_0 = (quad.d.bData[0]) << 2;
	tmp_1 = (quad.d.bData[1] & 0x30) >> 4;
	out.d.bData[0] = tmp_0 | tmp_1;

	if(out.size>1)
		out.d.bData[1] = ((quad.d.bData[1] & 0x0f) <<4) | ((quad.d.bData[2] & 0x3C) >> 2);

	if(out.size>2)
		out.d.bData[2] = ((quad.d.bData[2] & 0x03) << 6) | (quad.d.bData[3] & 0x3f);

	out.d.bData[out.size] = '\0';
	quad = out;
}
