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
#ifndef _XML_DOC_H_
#define _XML_DOC_H_

#include <memory.h>

/*/////////////////////
//CXmlDoc::Structures
/////////////////////*/

#define xmlmin(a,b) (((a) < (b)) ? (a) : (b))

struct sXmlProperty
{
	const char* name;
	const char* value;
	sXmlProperty* next;
public:
	sXmlProperty(){
		name = value = NULL;
		next = NULL;
	}
	void Free()
	{
		if(name){
			free((void*)name);
		}
		if(value){
			free((void*)value);
		}
	}
	int  type(){
		return 0;
	}
};

struct sXmlBinProperty : public sXmlProperty
{
public:
	void Free(){
		value = NULL;
		sXmlProperty::Free();
	}
	int  type(){return 1;}
};

struct sXmlNode
{
	const char* name;
	const char* value;
	long		type;
	long		num_children;
	sXmlProperty* f_prop;
	sXmlProperty* l_prop;

	sXmlNode* parent;
	sXmlNode* f_child;/*first child*/
	sXmlNode* l_child;/*last child*/
	sXmlNode* next;
public:
	sXmlNode(){memset(this,0,sizeof(sXmlNode));}
	void Free()
	{
		sXmlNode* f = this->f_child;
		sXmlProperty* p = this->f_prop;
		while(f)
		{
			sXmlNode* tmp = f->next;
			f->Free();
			free((void*)f);
			f = tmp;
		}
		while(p)
		{
			sXmlProperty* tmp = p->next;
			p->Free();
			free((void*)p);
			p = tmp;
		}
		if(this->name)free((void*)this->name);
		if(this->value)free((void*)this->value);
		memset(this,0,sizeof(sXmlNode));
	}
};




class cXmlDoc
{
public:

	enum {NODE_NONE = 0,NODE_DOC_TYPE,NODE_SPECIAL,NODE_COMMAND,NODE_ROOT,NODE_PARENT,NODE_DATA,NODE_COMMENT};

	const static long PARSE_NOT_REQUIRE_DEF = 0x01;
	const static long PARSE_COMMENTS = 0x02;
	const static long PARSE_IGNORE_WHITESPACES = 0x04;
	const static long PARSE_MULTIPLE_ROOTS = 0x08;
	const static long PARSE_SPECIAL = 0x10;
	const static long PARSE_COMMANDS = 0x20;
	const static long PARSE_IGNORE_BAD_PROPERTIES = 0x40;

public:
	cXmlDoc();
	~cXmlDoc();
public:
	long ParseFile(const char* file,long flags);
	long ParseString(char* string, long flags);
	long SaveToFile(const char* file);
	void Free();
public:
	const char* GetProperty(sXmlNode* node,const char* name);
	sXmlNode* GetRootNode(){return &m_root;}
	sXmlNode* GetNode(const char* path,sXmlNode* parent = NULL);
	sXmlNode* AddNewNode(const char* name,const char* value,sXmlNode* parent = NULL);
	long SetValue(sXmlNode* node,const char* value);
protected:
	long  SaveNode(FILE* f,sXmlNode* node);
	char* ParseNode(char* start,sXmlNode* parent);
	long  ParseProperties(char* start,sXmlNode* node);
	long  JoinParent(sXmlNode* parent,sXmlNode* child);
	long  AddProperty(sXmlProperty* prop,sXmlNode* node);
	long  AddBinProperty(sXmlBinProperty* prop,sXmlNode* node);
	long  AddNewBinProperty(sXmlNode* node,const char* name,void* value);
	long  SetBinPropertyValue(sXmlNode* node,const char* name,void* value);
	void* GetBinPropertyValue(sXmlNode* node,const char* name,long* success);
protected:
	sXmlNode m_root;
	long m_flags;
	long m_parse_flags;
	long m_level;
};

#endif //_XML_DOC_H_