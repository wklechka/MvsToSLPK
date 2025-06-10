#ifndef _SM_XML_H
#define _SM_XML_H

// this is a hacked up version
// uses at least 1/3 the memory
// comments, CDATA are not allowed
// This was done for Summit large projects

#include <map>
typedef unsigned int SMIdType;

// xml.h
// v.0x160
// revision 15 - 07 - 2010
#define SM_XML_VERSION 0x161
#define SM_XML_VERSION_REVISION_DATE "15-07-2010"

// #define ALLOW_SINGLE_QUOTE_VARIABLES
// Define the above tow allow var='x' instead of var="x"

#ifdef __unix
#define LINUX
#endif


#ifdef __BORLANDC__
#pragma warn -pck
#endif

#ifdef _MSC_VER
#define _USERENTRY __cdecl
#endif

#ifdef LINUX
#define _USERENTRY
#define __cdecl
#endif

#ifdef __WATCOMC__
#define _USERENTRY
#endif

#define CRYPT_OID_INFO_HAS_EXTRA_FIELDS
#define SM_XML_OPTIONAL_MIME


// ANSI includes
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#ifdef LINUX
#include <wchar.h>
#endif

#ifdef _WIN32
#ifndef __SYMBIAN32__

#include "pch.h"

//#include <windows.h>
#include <wincrypt.h>
#include <commctrl.h>
#include <wininet.h>
#include <tchar.h>

#if WINVER >= 0x601
#define CRYPT_OID_INFO_HAS_EXTRA_FIELDS
#include <cryptxml.h>
#endif

#endif
#endif

#ifdef __SYMBIAN32__
#define _USERENTRY
#define strcmpi strcmp
#include <unistd.h>
#endif

#ifndef _Z_H
#define _Z_H
// Z template class
template <class T>class Z
{
private:

	T* d;
	size_t ss;

public:

	Z(size_t s = 0)
	{
		if (s == 0)
			s = 1;
		d = new T[s];
		memset(d,0,s*sizeof(T));
		ss = s;
	}
	~Z()
	{
		delete[] d;
	}

	operator T*()
	{
		return d;
	}

	T* p()
	{
		return d;
	}

	size_t bs()
	{
		return ss*sizeof(T);
	}

	size_t is()
	{
		return ss;
	}

	void _clear()
	{
		memset(d,0,ss*sizeof(T));
	}

	void Resize(size_t news)
	{
		if (news == ss)
			return; // same size

		// Create buffer to store existing data
		T* newd = new T[news];
		size_t newbs = news*sizeof(T);
		memset((void*)newd,0, newbs);

		if (ss < news)
			// we created a larger data structure
			memcpy((void*)newd,d,ss*sizeof(T));
		else
			// we created a smaller data structure
			memcpy((void*)newd,d,news*sizeof(T));
		delete[] d;
		d = newd;
		ss = news;
	}

	void AddResize(size_t More)
	{
		Resize(ss + More);
	}

};
#endif // Z_H

#ifdef SM_XML_USE_NAMESPACE
namespace SM_XMLPP
{
#endif


	// SM_XMLU Class, converts utf input to wide char and vice versa
	class SM_XMLU
	{
	public:
		char* bs;
		wchar_t* ws;
		bool n;
		SM_XMLU(const char* x)
		{
			bs = 0;
			ws = 0;
			n = false;
			if (!x || !strlen(x))
			{
				bs = "";
				ws = L"";
			}
			else
			{
				n = true;
				size_t si = strlen(x)*2 + 1000;
				ws = new wchar_t[si];
				bs = new char[si];
				memset((void*)ws,0,si*sizeof(wchar_t));
				memset((void*)bs,0,si*sizeof(char));
#ifdef _WIN32
				lstrcpyA(bs,x);
				MultiByteToWideChar(CP_UTF8,0,x,-1,ws,(int)si);
#endif
			}
		}
		SM_XMLU(const wchar_t* x)
		{
			bs = 0;
			ws = 0;
			n = false;
			if (!x || !wcslen(x))
			{
				bs = "";
				ws = L"";
			}
			else
			{
				n = true;
				size_t si = wcslen(x)*2 + 1000;
				ws = new wchar_t[si];
				bs = new char[si];
				memset((void*)ws,0,si*sizeof(wchar_t));
				memset((void*)bs,0,si*sizeof(char));
#ifdef _WIN32
				lstrcpyW(ws,x);
				WideCharToMultiByte(CP_UTF8,0,x,-1,bs,(int)si,0,0);
#endif
			}
		}
		wchar_t* wc()
		{
			return ws;
		}
		char* bc()
		{
			return bs;
		}
		operator wchar_t*() 
		{
			return ws;
		}
		operator char*() 
		{
			return bs;
		}
		~SM_XMLU()
		{
			if (n)
			{
				if (ws)
					delete[] ws;
				if (bs)
					delete[] bs;
			}
		}
	};


	class SM_XMLHeader;
	class SM_XMLElement;
	class SM_XMLVariable;
	class SM_XMLComment;
	class SM_XMLContent;
	class SM_XMLCData;
	class SM_XML;

	typedef struct
	{
		int VersionHigh;
		int VersionLow;
		char RDate[20];
	} SM_XML_VERSION_INFO;


#ifdef _WIN32
	struct IMPORTDBTABLEDATA
	{
		char name[256];
		char itemname[100];
		int nVariables;
		char** Variables;
		char** ReplaceVariables;
	};

	struct IMPORTDBPARAMS
	{
		char* dbname;
		char* provstr;
		int nTables;
		IMPORTDBTABLEDATA* Tables;
	};
#endif

	struct SM_XMLEXPORTFORMAT
	{
		bool UseSpace;
		int nId;
		bool ElementsNoBreak;
	};

#ifdef _WIN32
	struct IMPORTRKEYDATA
	{
		HKEY pK;
		int StorageType; // 0 - Native
		// 1 - Registry key from native SM_XML
		// 2 - Registry key from registry SM_XML
	};
#endif



	// UNLOAD elements

// 	struct SM_XMLUNLOADELEMENT
// 	{
// 		int i;
// 		char* fn[300];
// 	};


	// Enumerations
	enum SM_XML_LOAD_MODE
	{
		SM_XML_LOAD_MODE_LOCAL_FILE = 0,
		SM_XML_LOAD_MODE_MEMORY_BUFFER = 1,
		SM_XML_LOAD_MODE_URL = 2,
		SM_XML_LOAD_MODE_LOCAL_FILE_U = 7,
	};

	enum SM_XML_PARSE_STATUS
	{
		SM_XML_PARSE_OK = 0,
		SM_XML_PARSE_NO_HEADER = 1,
		SM_XML_PARSE_ERROR = 2,
	};

	enum SM_XML_SAVE_MODE
	{
		SM_XML_SAVE_MODE_ZERO = 0,
		SM_XML_SAVE_MODE_DEFAULT = 1,
	};

	enum SM_XML_TARGET_MODE
	{
		SM_XML_TARGET_MODE_FILE = 0,
		SM_XML_TARGET_MODE_MEMORY = 1,
		SM_XML_TARGET_MODE_REGISTRYKEY = 2,
		SM_XML_TARGET_MODE_UTF16FILE = 3,
	};

	// Global functions

	class SM_XMLHeader
	{
	public:

		// constructors/destructor
		SM_XMLHeader(const char* ht = 0,const unsigned short* wht = 0);
		operator const char*();
		size_t MemoryUsage();
		void CompressMemory();
		bool IntegrityTest();

		int Compare(SM_XMLHeader*);

		void SetEncoding(const char*);
		SM_XMLHeader* Duplicate();


		SM_XMLHeader(SM_XMLHeader&);
		SM_XMLHeader& operator =(SM_XMLHeader&);
		~SM_XMLHeader();

		// SM_XMLComment
		SM_XMLComment** GetComments();
		unsigned int GetCommentsNum();
		int AddComment(SM_XMLComment*,int pos);
		int RemoveComment(unsigned int i);
		int RemoveAllComments();
		int SpaceForComment(unsigned int);

		void Export(FILE* fp,int HeaderMode,SM_XML_TARGET_MODE TargetMode = SM_XML_TARGET_MODE_FILE,class SM_XMLTransform* eclass = 0,class SM_XMLTransformData* edata = 0);


	private:

		void Clear();

		int TotalCommentPointersAvailable;
		char* hdr;
		unsigned int commentsnum;
		SM_XMLComment** comments;

	};



#ifdef LINUX
	typedef int (*fcmp) (const void *, const void *);
#endif


// 	struct SM_XMLBORROWELEMENT
// 	{
// 		bool Active;
// 		class SM_XMLElement* x;
// 	};


	class SM_XMLElement
	{
	public:

		// Beginning of DATEM functions added
		// 

#define SM_XML_DEFAULT_BUFFERSIZE 2048

#define  VARIABLE_name "Name"

		// variable value finding functions
		bool findVariable(const char* variableName, int& value);
		bool findVariable(const char* variableName, __int64& value);
		bool findVariable(const char* variableName, long& value);
		bool findVariable(const char* variableName, double& value);
		bool findVariable(const char* variableName, bool& value);
		bool findVariable(const char* variableName, byte& value);
		bool findVariable(const char* variableName, char* value, DWORD bufferSize = SM_XML_DEFAULT_BUFFERSIZE);
		bool findVariable(const char* variableName, COLORREF& value);
#if _MSC_VER >= 1400
		bool findVariable(const char* variableName, CStringA& value);
		bool findVariable(const char* variableName, CStringW& value);
#else
		bool findVariable(const char* variableName, CString& value);
#endif
		// variable value adding functions.  
		// Note: You can re-add the same variable name 
		// multiple times.  Use setVariable if you only want to have 
		// one instance of the variable.
		bool addVariable(const char* variableName, int value);
		bool addVariable(const char* variableName, __int64 value);
		bool addVariable(const char* variableName, long value);
		bool addVariable(const char* variableName, double value);
		bool addVariable(const char* variableName, bool value);
		bool addVariable(const char* variableName, byte value);
#if _MSC_VER >= 1400
		bool addVariable(const char* variableName, const CStringW& value);
		bool addVariable(const char* variableName, const CStringA& value);
#else
		bool addVariable(const char* variableName, const CString& value);
#endif
		bool addVariable(const char* variableName, const char* value);
		bool addVariable(const char* variableName, COLORREF value);

		// if the variable exists, change it's value, else
		// add the variable.
		bool setVariable(const char* variableName, int value);
		bool setVariable(const char* variableName, __int64 value);
		bool setVariable(const char* variableName, long value);
		bool setVariable(const char* variableName, double value);
		bool setVariable(const char* variableName, bool value);
		bool setVariable(const char* variableName, byte value);

#if _MSC_VER >= 1400
		bool setVariable(const char* variableName, const CStringW& value);
		bool setVariable(const char* variableName, const CStringA& value);
#else 
		bool setVariable(const char* variableName, const CString& value);
#endif

		bool setVariable(const char* variableName, const char* value);
		bool setVariable(const char* variableName, COLORREF value);

		// Use this function to find a child element by the element name and 
		// the contents of the Name variable ( see VARIABLE_name )
		bool findElementByName(SM_XMLElement*& pElement, 
			const char* elementName, 
			const char* nameVariableValue);

		// End of DATEM functions added



		// constructors/destructor
		SM_XMLElement(SM_XMLElement* par = 0,const char* el = 0,int Type = 0,bool Temp = false,const unsigned short* wel = 0);

		//SM_XMLElement& operator =(SM_XMLElement&);
		~SM_XMLElement();

		void Clear();

		// No STL Functions
		SM_XMLElement* operator[](int);
		SM_XMLElement* AddElement(SM_XMLElement*);
		SM_XMLElement* AddElement(const char*,const wchar_t* = 0);
		SM_XMLElement* InsertElement(unsigned int,SM_XMLElement*);



// 		void SetElementParam(unsigned __int64 p);
// 		unsigned __int64 GetElementParam();

		void Reparse(const char*el,int Type = 0);
		int GetDeep();


		int BorrowElement(SM_XMLElement*,unsigned int = (unsigned)-1);
		int ReleaseBorrowedElements();
		int RemoveElementAndKeep(unsigned int i,SM_XMLElement** el);


		bool ReplaceElement(unsigned int i,SM_XMLElement* ne,SM_XMLElement** prev = 0);
		int UpdateElement(SM_XMLElement*,bool UpdateVariableValues = false);
		int FindElement(SM_XMLElement*);
		int FindElement(const char* n);
		SM_XMLElement* FindElementZ(SM_XMLElement*);
		SM_XMLElement* FindElementZ(const char* n,bool ForceCreate = false,char* el = 0,bool Temp = false);
		int RemoveElement(unsigned int i);
		int GetElementIndex(SM_XMLElement*);
		int GetDeepLevel();

		bool EncryptElement(unsigned int i,char* pwd);
		bool DecryptElement(unsigned int i,char* pwd);

#ifdef _WIN32
#ifndef WINCE
		bool SignElement(unsigned int i,PCCERT_CONTEXT pCert);
		SM_XMLVariable* GetSignature(unsigned int i);
		bool RemoveSignature(unsigned int i);
		bool VerifyDigitalSignature(unsigned int i,PCCERT_CONTEXT* ppCert);
		SM_XMLElement* EncryptElement(unsigned int i,PCCERT_CONTEXT* pCert,int nCert);
		SM_XMLElement* DecryptElement(unsigned int i,PCCERT_CONTEXT* ppCert);
#endif
#endif

		int RemoveElement(SM_XMLElement*);
		int RemoveAllElements();
		int RemoveTemporalElements(bool Deep = false);
		int DeleteUnloadedElementFile(int i);


		int UnloadElement(unsigned int i);
		int ReloadElement(unsigned int i);
		int ReloadAllElements();

		SM_XMLElement* MoveElement(unsigned int i,unsigned int y);



#ifdef LINUX
		void SortElements(fcmp);
		void SortVariables(fcmp);
		friend int SM_XMLElementfcmp(const void *, const void *);
		friend int SM_XMLVariablefcmp(const void *, const void *);
#else
		void SortElements(int (_USERENTRY *fcmp)(const void *, const void *));
		void SortVariables(int (_USERENTRY *fcmp)(const void *, const void *));
		friend int _USERENTRY SM_XMLElementfcmp(const void *, const void *);
		friend int _USERENTRY SM_XMLVariablefcmp(const void *, const void *);
#endif

		SM_XMLElement* Duplicate(SM_XMLElement* = 0);
		SM_XMLElement* Encrypt(const char* pwd);
		SM_XMLElement* Decrypt(const char* pwd);
		void Copy();
		size_t MemoryUsage();
		void CompressMemory();
		bool IntegrityTest();

		int Compare(SM_XMLElement*);


		// SM_XMLComment

		SM_XMLComment** GetComments();
		int AddComment(SM_XMLComment*,int InsertBeforeElement);
		int AddComment(const char*,int,const wchar_t* = 0);

		unsigned int GetCommentsNum();
		int RemoveComment(unsigned int i);
		int RemoveAllComments();

		// SM_XMLCData

		SM_XMLCData** GetCDatas();
		int AddCData(SM_XMLCData*,int InsertBeforeElement);
		int AddCData(const char*,int,const wchar_t* wt = 0);

		unsigned int GetCDatasNum();
		int RemoveCData(unsigned int i);
		int RemoveAllCDatas();

		// Content Stuff

		SM_XMLContent** GetContents();
		int AddContent(SM_XMLContent* v,int InsertBeforeElement);
		int AddContent(const char*,int,int BinarySize = 0,const unsigned short* = 0);

		int RemoveContent(unsigned int i);
		void RemoveAllContents();
		unsigned int GetContentsNum();

		// Children Stuff

		SM_XMLElement** GetChildren();

		unsigned int GetChildrenNum();
		unsigned int GetAllChildren(SM_XMLElement**,unsigned int deep = 0xFFFFFFFF);
		unsigned int GetAllChildrenNum(unsigned int deep = 0xFFFFFFFF);


		// Variable Stuff

		int AddVariable(SM_XMLVariable*);
		int RemoveVariableAndKeep(unsigned int i,SM_XMLVariable** vr);
		SM_XMLVariable** GetVariables();
		int AddVariable(const char*,const char*,const wchar_t* = 0,const wchar_t* = 0);
#ifdef SM_XML_OPTIONAL_MIME
		int AddBinaryVariable(const char*,const char*,int);
#endif


		int FindVariable(SM_XMLVariable*);
		int FindVariable(const char*  x);
		SM_XMLVariable* FindVariableZ(SM_XMLVariable*);
		SM_XMLVariable* FindVariableZ(const char* x,bool ForceCreate = false,char* defnew = 0,bool Temp = false);
		int RemoveVariable(unsigned int i);
		int RemoveVariable(SM_XMLVariable*);
		int RemoveAllVariables();
		int RemoveTemporalVariables(bool Deep = false);
		unsigned int GetVariableNum();



		SM_XMLElement* GetElementInSection(const char*);
		int SM_XMLQuery(const char* expression,SM_XMLElement** rv,unsigned int deep = 0xFFFFFFFF);
		SM_XMLElement* GetParent();
		void Export(FILE* fp,int ShowAll,SM_XML_SAVE_MODE SaveMode,SM_XML_TARGET_MODE TargetMode = SM_XML_TARGET_MODE_FILE,SM_XMLHeader* hdr = 0,class SM_XMLTransform* eclass = 0,class SM_XMLTransformData* edata = 0);
		void SetExportFormatting(SM_XMLEXPORTFORMAT* xf);
		void SetElementName(const char*,const wchar_t* = 0);
		size_t GetElementName(char*,int NoDecode = 0);
		size_t GetElementFullName(char*,int NoDecode = 0);
		size_t GetElementUniqueString(char*);
		void SetTemporal(bool);
		bool IsTemporal();
		int   GetType();
		static void Write16String(FILE* fp,const char* s);
		int ReserveSpaceForElements(unsigned int);


	private:

		SM_XMLElement(const SM_XMLElement&);


		//unsigned __int64 _param;
		//int _type; // type, 0 element

		SM_XMLElement* parent; // one

		//char* _el; // element name
		SMIdType _elId;

		SM_XMLElement** children; // many
		SM_XMLVariable** variables; // many
		//SM_XMLComment** comments; // many
		SM_XMLContent** _contents; // many;
		//SM_XMLCData** cdatas;
		unsigned int childrennum;
		unsigned short variablesnum;
		//unsigned int commentsnum;
		unsigned short contentsnum;
		//unsigned int cdatasnum;
		int SpaceForElement(unsigned int);
		int SpaceForVariable(unsigned int);
		int SpaceForComment(unsigned int);
		int SpaceForContent(unsigned int);
		int SpaceForCData(unsigned int);
		int TotalChildPointersAvailable;
		short TotalVariablePointersAvailable;
		//int TotalCommentPointersAvailable;
		short TotalContentPointersAvailable;
		//int TotalCDataPointersAvailable;



		//bool Temporal;

// 		Z<SM_XMLBORROWELEMENT> BorrowedElements;
// 		unsigned int NumBorrowedElements;

		// only one format allowed now
		// set to static to save memory
		static SM_XMLEXPORTFORMAT xfformat;

		static void printc(FILE* fp,SM_XMLElement* root,int deep,int ShowAll,SM_XML_SAVE_MODE SaveMode,SM_XML_TARGET_MODE TargetMode);
		void SetParent(SM_XMLElement*);
	};

//#define USE_OLD_VV

	class SM_XMLVariable
	{
	public:

		SM_XMLVariable(const char* = 0,const char* = 0,int NoDecode = 0,bool Temp = false,const wchar_t* = 0,const wchar_t* = 0);
		~SM_XMLVariable();
		SM_XMLVariable(const SM_XMLVariable&);
		SM_XMLVariable& operator =(const SM_XMLVariable&);


		size_t MemoryUsage();
		void CompressMemory();
		bool IntegrityTest();

		int Compare(SM_XMLVariable*);


		SM_XMLElement* SetOwnerElement(SM_XMLElement*);
		size_t GetName(char*,int NoDecode = 0) const;
		size_t GetValue(char*,int NoDecode = 0) const;
		int GetValueInt();
		unsigned int GetValueUInt();
		__int64 GetValueInt64();
		unsigned __int64 GetValueUInt64();
		float GetValueFloat();
		void SetName(const char*,int NoDecode = 0,const wchar_t* = 0);
		void SetValue(const char*,int NoDecode = 0,const wchar_t* = 0);
		void SetValueUInt(unsigned int);
		void SetValueInt(int);
		void SetValueInt64(__int64);
		void SetValueUInt64(unsigned __int64);
		void SetValueFloat(float);
		void SetFormattedValue(const char* fmt,...);
		template <typename T> T GetFormattedValue(const char* fmt)
		{
			size_t p = GetValue(0);
			Z<char> d(p + 10);
			GetValue(d);
			T x = 0;
			sscanf(d,fmt,&x);
			return x;
		}
		//template <typename T> void SetValueX(T t,const char* fmt);
		template <typename T> T GetValueX(const char* fmt);
		SM_XMLVariable* Duplicate();
		void Copy();
		SM_XMLElement* GetOwnerElement();
		void SetTemporal(bool);
		bool IsTemporal();

#ifdef SM_XML_OPTIONAL_MIME
		size_t GetBinaryValue(char*);
		size_t SetBinaryValue(char*,int);
#endif

	private:

		void Clear();

		//char* _vn;
		SMIdType _vnId;

#ifdef USE_OLD_VV
		char* _vv;
#else
		SMIdType _vvId;
#endif
		

		SM_XMLElement* owner;
		//bool Temporal;
	};



	class SM_XMLComment
	{
	public:

		// constructors/destructor
		SM_XMLComment(SM_XMLElement* p = 0,int ElementPosition = -1,const char* ht = 0,const wchar_t* wt = 0);
		operator const char*() const;
		void SetComment(const char* ht);
		size_t MemoryUsage();
		void CompressMemory();
		bool IntegrityTest();

		int Compare(SM_XMLComment*);


		SM_XMLComment(const SM_XMLComment&);
		SM_XMLComment& operator =(const SM_XMLComment&);
		~SM_XMLComment();

		SM_XMLComment* Duplicate();
		void SetParent(SM_XMLElement* p,int ep);
		int GetEP() const;

	private:

		SM_XMLElement* parent;

		char* c;

		int ep; // Element Position (Before)
	};


	class SM_XMLContent
	{
	public:

		// constructors/destructor
		SM_XMLContent(SM_XMLElement* p = 0,int ElementPosition = -1,const char* ht = 0,int NoDecode = 0,int BinarySize = 0,const wchar_t* wt = 0);
		operator const char*();
		size_t GetValue(char*,int NoDecode = 0) const; 
		bool GetBinaryValue(char**o,int* len);
		void SetValue(const char*,int NoDecode = 0,int BinarySize = 0);
		size_t MemoryUsage();
		void CompressMemory();
		bool IntegrityTest();

		int Compare(SM_XMLContent*);


		SM_XMLContent(const SM_XMLContent&);
		SM_XMLContent& operator =(const SM_XMLContent&);
		~SM_XMLContent();

		SM_XMLContent* Duplicate();
		void SetParent(SM_XMLElement* p,int ep);
		int GetEP() const;

	private:

		SM_XMLElement* parent;

		char* c;

		short _ep; // Element Position (Before)
	};

	class SM_XMLCData
	{
	public:

		// constructors/destructor
		SM_XMLCData(SM_XMLElement* p = 0,int ElementPosition = -1,const char* ht = 0,const wchar_t* wt = 0);
		operator const char*() const;
		void SetCData(const char* ht);
		size_t MemoryUsage();
		void CompressMemory();
		bool IntegrityTest();

		int Compare(SM_XMLCData*);


		SM_XMLCData(const SM_XMLCData&);
		SM_XMLCData& operator =(const SM_XMLCData&);
		~SM_XMLCData();

		SM_XMLCData* Duplicate();
		void SetParent(SM_XMLElement* p,int ep);
		int GetEP() const;

	private:

		SM_XMLElement* parent;

		char* c;

		int ep; // Element Position (Before)
	};

	class SM_XML
	{
	public:

		// constructors/destructor

		SM_XML();
		SM_XML(const char* file,SM_XML_LOAD_MODE LoadMode = SM_XML_LOAD_MODE_LOCAL_FILE,class SM_XMLTransform* eclass = 0,class SM_XMLTransformData* edata = 0);
#ifndef LINUX
		SM_XML(const wchar_t* file,SM_XML_LOAD_MODE LoadMode = SM_XML_LOAD_MODE_LOCAL_FILE,class SM_XMLTransform* eclass = 0,class SM_XMLTransformData* edata = 0);
#endif
		void Version(SM_XML_VERSION_INFO*);
		size_t MemoryUsage();
		void CompressMemory();
		bool IntegrityTest();
		int Compare(SM_XML*);

		SM_XML(SM_XML& x);
		SM_XML& operator =(SM_XML&);
		~SM_XML();

		//      static void Kill(char* tf);
#ifdef LINUX
		int PhantomLoad(const char* file);
#else
		int PhantomLoad(const char* file,bool IsUnicode = false,bool UseMap = false);
#endif
		int PhantomElement(FILE*fp,class SM_XMLElement* r,unsigned __int64 StartP,unsigned __int64 EndP);

		static int DoMatch(const char *text, char *p, bool IsCaseSensitive = false);
		static bool VMatching(const char *text, char *p, bool IsCaseSensitive = false);
		static bool TestMatch(const char* item1,const char* comp,const char* item2);
		static Z<char>* ReadToZ(const char*,class SM_XMLTransform* eclass = 0,class SM_XMLTransformData* edata = 0,bool IsU = 0);
		static int Convert2HexCharsToNumber(int c1, int c2);
		static SM_XMLElement* Paste(char* txt = 0);

		// Beginning of DATEM functions added
		// 
		// These functions are implemented in DAT_SM_XML.cpp
		static bool isSM_XMLFile(const char* filename);

		//
		// end of DATEM functions 
		//////////////////////////////////////////////////////////////////////////


		SM_XML_PARSE_STATUS ParseStatus(int* = 0);
		void SetUnicode(bool x);
		void SaveOnClose(bool);
		int Load(const char* data,SM_XML_LOAD_MODE LoadMode = SM_XML_LOAD_MODE_LOCAL_FILE,class SM_XMLTransform* eclass = 0,class SM_XMLTransformData* edata = 0);
		size_t LoadText(const char*);
		size_t LoadText(const wchar_t*);
		static int PartialLoad(const char* file,const char* map);
		static SM_XMLElement * PartialElement(const char* file,const char* map);
		int Save(const char* file = 0,SM_XML_SAVE_MODE SaveMode = SM_XML_SAVE_MODE_DEFAULT,SM_XML_TARGET_MODE TargetMode = SM_XML_TARGET_MODE_FILE,class SM_XMLTransform* eclass = 0,class SM_XMLTransformData* edata = 0); // Default, do not encode already encoded
		void Export(FILE* fp,SM_XML_SAVE_MODE SaveMode,SM_XML_TARGET_MODE TargetMode = SM_XML_TARGET_MODE_FILE,SM_XMLHeader *hdr = 0,class SM_XMLTransform* eclass = 0,class SM_XMLTransformData* edata = 0);
		void SetExportFormatting(SM_XMLEXPORTFORMAT* xf);

		void Lock(bool);
		int RemoveTemporalElements();
#ifdef SM_XML_OPTIONAL_IMPORTDB
		static SM_XMLElement* ImportDB(IMPORTDBPARAMS* );
#endif
#ifdef SM_XML_OPTIONAL_IMPORTRKEY
		static SM_XMLElement* ImportRKey(IMPORTRKEYDATA*);
#endif


		void SetRootElement(SM_XMLElement*);
		SM_XMLElement* RemoveRootElementAndKeep();
		SM_XMLElement* GetRootElement();
		SM_XMLHeader* GetHeader();
		void SetHeader(SM_XMLHeader* h);


		static size_t SM_XMLEncode(const char* src,char* trg);
		static size_t SM_XMLDecode(const char* src,char* trg);
		size_t SM_XMLGetValue(const char* section,const char* attr,char* put,size_t maxlen);
		void SM_XMLSetValue(const char* section,const char* attr,char* put);

		// Query functions
		int SM_XMLQuery(const char* rootsection,const char* expression,SM_XMLElement** rv,unsigned int deep = 0xFFFFFFFF);

	private:

		void Init();
		void Clear();

		SM_XML_PARSE_STATUS iParseStatus; // 0 Valid , 1 Error but recovered, 2 fatal error
		int iParseStatusPos;
#ifndef LINUX
		bool IsFileU; // unicode file
#endif


		char* f;          // filename
		SM_XMLHeader* hdr;   // header (one)
		SM_XMLElement* root; // root element (one)


		bool SOnClose;
		// For Windows


	};


	// public functions
	size_t SM_XMLGetString(const char* section,const char* Tattr,const char* defv,char*out,const size_t maxlen,const char* xml,SM_XML* af = 0);
	int     SM_XMLGetInt(const char* item,const char* attr,const int defv,const char* xml,SM_XML* af = 0);
	unsigned int SM_XMLGetUInt(const char* item,const char* attr,const unsigned int defv,const char* xml,SM_XML* af = 0);
#ifdef _WIN32
	__int64 SM_XMLGetInt64(const char* item,const char* attr,const __int64 defv,const char* xml,SM_XML* af = 0);
	unsigned __int64 SM_XMLGetUInt64(const char* item,const char* attr,const unsigned __int64 defv,const char* xml,SM_XML* af = 0);
#endif
	float   SM_XMLGetFloat(const char* item,const char* attr,const float defv,const char* xml,SM_XML* af = 0);
	size_t     SM_XMLGetBinaryData(const char* item,const char* attr,const char* defv,char*out,const size_t maxlen,const char* xml,SM_XML* af = 0);

	int    SM_XMLSetString(const char* section,const char* Tattr,char*out,const char* xml,SM_XML* af = 0);
	int    SM_XMLSetInt(const char* section,const char* attr,int v,const char* xml,SM_XML* af = 0);
	int    SM_XMLSetUInt(const char* section,const char* attr,unsigned int v,const char* xml,SM_XML* af = 0);
#ifdef _WIN32
	int    SM_XMLSetString(const char* section,const char* Tattr,wchar_t*out,const char* xml,SM_XML* af = 0);
	int    SM_XMLSetInt64(const char* section,const char* attr,__int64 v,const char* xml,SM_XML* af = 0);
	int    SM_XMLSetUInt64(const char* section,const char* attr,unsigned __int64 v,const char* xml,SM_XML* af = 0);
#endif
	int    SM_XMLSetFloat(const char* section,const char* attr,float v,const char* xml,SM_XML* af = 0);
	int    SM_XMLSetBinaryData(const char* section,const char* attr,char* data,int len,const char* xml,SM_XML* af = 0);

	int SM_XMLRenameElement(const char* section,const char* newname,const char* xml,SM_XML* af = 0);

#ifndef __SYMBIAN32__

	int    SM_XMLGetAllVariables(const char* section,char** vnames,char** vvalues,const char*xml);
	int    SM_XMLGetAllItems(const char* section,char** vnames,const char*xml);

#endif



	// SM_XMLTransform class

	class SM_XMLTransformData
	{
	public:
		SM_XMLTransformData() {}
	};

	class SM_XMLTransform
	{
	public:

		SM_XMLTransform(SM_XMLTransformData*) { }
		virtual ~SM_XMLTransform() {}
		virtual size_t Encrypt(const char*src,size_t srclen,int srctype,char* dst,size_t dstlen,SM_XMLTransformData* data = 0) = 0;
		virtual size_t Decrypt(const char*src,size_t srclen,int srctype,char* dst,size_t dstlen,SM_XMLTransformData* data = 0) = 0;

	};

	class SM_XMLHelper
	{
	public:

		// static functions
		static char* FindSM_XMLClose(char* s);
		static SM_XMLElement* ParseElementTree(SM_XMLHeader* hdr,SM_XMLElement* parent,char* tree,char** EndValue,SM_XML_PARSE_STATUS& iParseStatus);
		static void AddBlankVariable(SM_XMLElement* parent,char *a2,int Pos);
		static int pow(int P,int z);



	};

#ifdef SM_XML_USE_NAMESPACE
};
#endif


#endif // _SM_XML_H


