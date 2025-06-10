// xml.cpp
#include "pch.h"

#ifdef USE_EXTERNAL_Z
#include "z.h"
#endif

#include "smxml.h"

// bool SM_XMLEXPORTFORMAT::UseSpace = false;
// int SM_XMLEXPORTFORMAT::nId = 1;

SM_XMLEXPORTFORMAT SM_XMLElement::xfformat;

static std::map<SMIdType, CString> _idToName;
static std::map<CString, SMIdType> _nameToId;
static SMIdType _currentId = 0;

static void mapNameToId(CString &newName, SMIdType &id) 
{
	std::map<CString, SMIdType>::iterator iter = _nameToId.find(newName);
	if (iter != _nameToId.end()) {
		id = iter->second;
	}
	else {
		// its a new one
		++_currentId;
		_idToName[_currentId] = newName;
		_nameToId[newName] = _currentId;

		id = _currentId;
	}
}

static void clearMaps()
{
	_idToName.clear();
	_nameToId.clear();
	_currentId = 0;
}

static void initMaps()
{
	_idToName[0] = "";
	_nameToId[""] = 0;
	if (_currentId == 0) {
		++_currentId;
	}
}

#ifdef __WATCOMC__
#define _atoi64(x) atoll(x)
#endif


#ifndef __SYMBIAN32__
#ifndef LINUX
#pragma comment(lib,"wininet.lib")
#endif
#endif

//#pragma warning (disable:4244)
//#pragma warning (disable:4267)
//#pragma warning (disable:4800)
#pragma warning (disable:4996)

#ifdef LINUX
#define strcmpi(a,b) strcmp(a,b)
#endif

#ifdef WINCE
#define strcmpi(a,b) strcmp(a,b)
#endif


#ifdef _WIN32
#pragma comment(lib,"Crypt32.lib")
#endif


#ifndef SM_XML_MAX_INIT_CHILDREN
#define SM_XML_MAX_INIT_CHILDREN 5
#endif

#ifndef SM_XML_MAX_INIT_VARIABLES
#define SM_XML_MAX_INIT_VARIABLES 3
#endif

#ifndef SM_XML_MAX_INIT_CONTENTS
#define SM_XML_MAX_INIT_CONTENTS 1
#endif


// not used
#ifndef SM_XML_MAX_INIT_COMMENTS
#define SM_XML_MAX_INIT_COMMENTS 10
#endif

#ifndef SM_XML_MAX_INIT_CDATAS
#define SM_XML_MAX_INIT_CDATAS 10
#endif

#ifndef SM_XML_MAX_INIT_COMMENTS_HEADER
#define SM_XML_MAX_INIT_COMMENTS_HEADER 5
#endif



// Help functions    
#define MATCH_TRUE 1
#define MATCH_FALSE 0
#define MATCH_ABORT -1

#define NEGATE_CLASS
#define OPTIMIZE_JUST_STAR
#undef MATCH_TAR_PATTERN

// OPTI

/* Extra definitions

SM_XML_OPTIONAL_WIN32
SM_XML_OPTIONAL_IMPORTDB
SM_XML_OPTIONAL_IMPORTRKEY
*/

// MIME Code
// Code from Yonat
// http://ootips.org/yonat/4dev/
#ifndef MIME_CODES_H
#define MIME_CODES_H
/******************************************************************************
 * MimeCoder -  Abstract base class for MIME filters.
 ******************************************************************************/
template <class InIter, class OutIter>
class MimeCoder
{
public:
	 virtual OutIter Filter( OutIter out, InIter inBeg, InIter inEnd ) = 0;
    virtual OutIter Finish( OutIter out ) = 0;
};

/******************************************************************************
 * Base64
 ******************************************************************************/
template <class InIter, class OutIter>
class Base64Encoder : public MimeCoder<InIter, OutIter>
{
public:
    Base64Encoder() : its3Len(0), itsLinePos(0) {}
    virtual OutIter Filter( OutIter out, InIter inBeg, InIter inEnd );
	 virtual OutIter Finish( OutIter out );
private:
    int             itsLinePos;
	 unsigned char   itsCurr3[3];
    int             its3Len;
    void EncodeCurr3( OutIter& out );
};

template <class InIter, class OutIter>
class Base64Decoder : public MimeCoder<InIter, OutIter>
{
public:
    Base64Decoder() : its4Len(0), itsEnded(0) {}
    virtual OutIter Filter( OutIter out, InIter inBeg, InIter inEnd );
	 virtual OutIter Finish( OutIter out );
private:
    int             itsEnded;
    unsigned char   itsCurr4[4];
    int             its4Len;
	 int             itsErrNum;
    void DecodeCurr4( OutIter& out );
};

/******************************************************************************
 * Quoted-Printable
 ******************************************************************************/
template <class InIter, class OutIter>
class QpEncoder : public MimeCoder<InIter, OutIter>
{
public:
    QpEncoder() : itsLinePos(0), itsPrevCh('x') {}
    virtual OutIter Filter( OutIter out, InIter inBeg, InIter inEnd );
    virtual OutIter Finish( OutIter out );
private:
    int             itsLinePos;
	 unsigned char   itsPrevCh;
};

template <class InIter, class OutIter>
class QpDecoder : public MimeCoder<InIter, OutIter>
{
public:
	 QpDecoder() : itsHexLen(0) {}
	 virtual OutIter Filter( OutIter out, InIter inBeg, InIter inEnd );
    virtual OutIter Finish( OutIter out );
private:
    int             itsHexLen;
    unsigned char   itsHex[2];
};
#endif // MIME_CODES_H
#define TEST_MIME_CODES
#define ___
static const int cLineLen = 72;

/******************************************************************************
 * Base64Encoder
 ******************************************************************************/
static const char cBase64Codes[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char CR = 13;
static const char LF = 10;

template <class InIter, class OutIter>
OutIter Base64Encoder<InIter, OutIter>::Filter(
	 OutIter out,
    InIter inBeg,
    InIter inEnd )
{
    for(;;) {
        for(; itsLinePos < cLineLen; itsLinePos += 4) {
            for (; its3Len < 3; its3Len++) {
                if (inBeg == inEnd)
___ ___ ___ ___ ___ return out;
                itsCurr3[its3Len] = *inBeg;
                ++inBeg;
            }
            EncodeCurr3(out);
            its3Len = 0;
		  } // for loop until end of line
        *out++ = CR;
        *out++ = LF;
        itsLinePos = 0;
    } // for (;;)
//    return out;
}

template <class InIter, class OutIter>
OutIter Base64Encoder<InIter, OutIter>::Finish( OutIter out )
{
    if (its3Len)
        EncodeCurr3(out);
	 its3Len = 0;
    itsLinePos = 0;

    return out;
}

template <class InIter, class OutIter>
void Base64Encoder<InIter, OutIter>::EncodeCurr3( OutIter& out )
{
    if (its3Len < 3) itsCurr3[its3Len] = 0;

    *out++ = cBase64Codes[ itsCurr3[0] >> 2 ];
    *out++ = cBase64Codes[ ((itsCurr3[0] & 0x3)<< 4) |
                           ((itsCurr3[1] & 0xF0) >> 4) ];
	 if (its3Len == 1) *out++ = '=';
    else
        *out++ = cBase64Codes[ ((itsCurr3[1] & 0xF) << 2) |
                               ((itsCurr3[2] & 0xC0) >>6) ];
    if (its3Len < 3) *out++ = '=';
    else
        *out++ = cBase64Codes[ itsCurr3[2] & 0x3F ];
}

/******************************************************************************
 * Base64Decoder
 ******************************************************************************/
#define XX 127

static const unsigned char cIndex64[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,62, XX,XX,XX,63,
    52,53,54,55, 56,57,58,59, 60,61,XX,XX, XX,XX,XX,XX,
    XX, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,XX, XX,XX,XX,XX,
    XX,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	 XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
};

template <class InIter, class OutIter>
OutIter Base64Decoder<InIter, OutIter>::Filter(
    OutIter out,
    InIter inBeg, 
    InIter inEnd )
{
    unsigned char c;
    itsErrNum = 0;

	 for (;;) {
        while (its4Len < 4) {
            if (inBeg == inEnd)
___ ___ ___ ___ return out;
            c = *inBeg;
            if ((cIndex64[c] != XX) || (c == '='))
                itsCurr4[its4Len++] = c;
            else if ((c != CR) && (c != LF)) ++itsErrNum; // error
            ++inBeg;
        } // while (its4Len < 4)
        DecodeCurr4(out);
        its4Len = 0;
    } // for (;;)
    
//	 return out;
}

template <class InIter, class OutIter>
OutIter Base64Decoder<InIter, OutIter>::Finish( OutIter out )
{
    its4Len = 0;
    if (itsEnded) return out;
    else { // error
        itsEnded = 0;
        return out;
    }
}

template <class InIter, class OutIter>
void Base64Decoder<InIter, OutIter>::DecodeCurr4( OutIter& out )
{
    if (itsEnded) {
        ++itsErrNum;
        itsEnded = 0;
    }

    for (int i = 0; i < 2; i++)
        if (itsCurr4[i] == '=') {
            ++itsErrNum; // error
___ ___ ___ return;
        }
        else itsCurr4[i] = cIndex64[itsCurr4[i]];

    *out++ = (itsCurr4[0] << 2) | ((itsCurr4[1] & 0x30) >> 4);
    if (itsCurr4[2] == '=') {
        if (itsCurr4[3] == '=') itsEnded = 1;
        else ++itsErrNum;
    } else {
        itsCurr4[2] = cIndex64[itsCurr4[2]];
        *out++ = ((itsCurr4[1] & 0x0F) << 4) | ((itsCurr4[2] & 0x3C) >> 2);
        if (itsCurr4[3] == '=') itsEnded = 1;
        else *out++ = ((itsCurr4[2] & 0x03) << 6) | cIndex64[itsCurr4[3]];
    }
}

/******************************************************************************
 * QpEncoder
 ******************************************************************************/

static const char cBasisHex[] = "0123456789ABCDEF";

template <class InIter, class OutIter>
OutIter QpEncoder<InIter, OutIter>::Filter(
    OutIter out,
    InIter inBeg, 
    InIter inEnd )
{
   unsigned char c;
    
    for (; inBeg != inEnd; ++inBeg) {
		  c = *inBeg;

        // line-breaks
        if (c == '\n') {
            if (itsPrevCh == ' ' || itsPrevCh == '\t') {
                *out++ = '='; // soft & hard lines
                *out++ = c;
            }
            *out++ = c;
            itsLinePos = 0;
            itsPrevCh = c;
        }

        // non-printable
		  else if ( (c < 32 && c != '\t')
                  || (c == '=')
                  || (c >= 127)
                  // Following line is to avoid single periods alone on lines,
                  // which messes up some dumb SMTP implementations, sigh...
                  || (itsLinePos == 0 && c == '.') ) {
            *out++ = '=';
            *out++ = cBasisHex[c >> 4];
            *out++ = cBasisHex[c & 0xF];
            itsLinePos += 3;
            itsPrevCh = 'A'; // close enough
        }
        
        // printable characters
		  else {
            *out++ = itsPrevCh = c;
            ++itsLinePos;
        }

        if (itsLinePos > cLineLen) {
            *out++ = '=';
            *out++ = itsPrevCh = '\n';
            itsLinePos = 0;
        }
    } // for loop over all input
    
    return out;
}

template <class InIter, class OutIter>
OutIter QpEncoder<InIter, OutIter>::Finish( OutIter out )
{
    if (itsLinePos) {
        *out++ = '=';
        *out++ = '\n';
    }

    itsLinePos = 0;
    itsPrevCh = 'x';

    return out;
}

/******************************************************************************
 * QpDecoder
 ******************************************************************************/

static const unsigned char cIndexHex[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
     0, 1, 2, 3,  4, 5, 6, 7,  8, 9,XX,XX, XX,XX,XX,XX,
    XX,10,11,12, 13,14,15,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,10,11,12, 13,14,15,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	 XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
};

template <class InIter, class OutIter>
OutIter QpDecoder<InIter, OutIter>::Filter(
    OutIter out,
    InIter inBeg, 
	 InIter inEnd )
{
    unsigned char c, c1, c2;
    int errn = 0;

    for (; inBeg != inEnd; ++inBeg) {
        if (itsHexLen) {                        // middle of a Hex triplet
            if (*inBeg == '\n') itsHexLen = 0;      // soft line-break
            else {                                  // Hex code
                itsHex[itsHexLen-1] = *inBeg;
                if (itsHexLen++ == 2) {
                    if (XX == (c1 = cIndexHex[itsHex[0]])) ++errn;
                    if (XX == (c2 = cIndexHex[itsHex[1]])) ++errn;
                    c = (c1 << 4) | c2;
						  if (c != '\r') *out++ = c;
                    itsHexLen = 0;
                }
            }
        }
        else if (*inBeg == '=') itsHexLen = 1;  // beginning of a new Hex triplet
        else *out++ = *inBeg;                   // printable character
    }
    
    return out;
}

template <class InIter, class OutIter>
OutIter QpDecoder<InIter, OutIter>::Finish( OutIter out )
{
    if (itsHexLen) { // error
        itsHexLen = 0;
___ ___ return out;
    }
    return out;
}
#define SM_XML_OPTIONAL_MIME

#ifdef SM_XML_USE_NAMESPACE
namespace SM_XMLPP
	{
#endif


// BASE 64 Class
// Code from Jerry Jiang
// http://www.codeproject.com/KB/cpp/RC4-BASE64.aspx
const unsigned char B64_offset[256] =
{
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
	64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
	64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};
const char base64_map[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
class CBase64
{
public:
	CBase64(){}

	char *Encrypt(const char * srcp, int len, char * dstp)
	{
		register int i = 0;
		char *dst = dstp;

		for (i = 0; i < len - 2; i += 3)
		{
			*dstp++ = *(base64_map + ((*(srcp+i)>>2)&0x3f));
			*dstp++ = *(base64_map + ((*(srcp+i)<< 4)&0x30 | (
                                *(srcp+i+1)>>4)&0x0f ));
			*dstp++ = *(base64_map + ((*(srcp+i+1)<<2)&0x3C | (
                                *(srcp+i+2)>>6)&0x03));
			*dstp++ = *(base64_map + (*(srcp+i+2)&0x3f));
		}
		srcp += i;
		len -= i;

		if(len & 0x02 ) /* (i==2) 2 bytes left,pad one byte of '=' */
		{      
			*dstp++ = *(base64_map + ((*srcp>>2)&0x3f));
			*dstp++ = *(base64_map + ((*srcp<< 4)&0x30 | (
                                *(srcp+1)>>4)&0x0f ));
			*dstp++ = *(base64_map + ((*(srcp+1)<<2)&0x3C) );
			*dstp++ = '=';
		}
		else if(len & 0x01 )  /* (i==1) 1 byte left,pad two bytes of '='  */
		{ 
			*dstp++ = *(base64_map + ((*srcp>>2)&0x3f));
			*dstp++ = *(base64_map + ((*srcp<< 4)&0x30));
			*dstp++ = '=';
			*dstp++ = '=';
		}

		*dstp = '\0';

		return dst;
	}

	void* Decrypt(const char * srcp, int len, char * dstp)
	{
		register int i = 0;
		void *dst = dstp;

		while(i < len)
		{
			*dstp++ = (B64_offset[*(srcp+i)] <<2 | 
                                B64_offset[*(srcp+i+1)] >>4);
			*dstp++ = (B64_offset[*(srcp+i+1)]<<4 | 
                                B64_offset[*(srcp+i+2)]>>2);
			*dstp++ = (B64_offset[*(srcp+i+2)]<<6 |
                                B64_offset[*(srcp+i+3)] );
			i += 4;
		}
		srcp += i;
		
		if(*(srcp-2) == '=')  /* remove 2 bytes of '='  padded while encoding */
		{	 
			*(dstp--) = '\0';
			*(dstp--) = '\0';
		}
		else if(*(srcp-1) == '=') /* remove 1 byte of '='  padded while encoding */
			*(dstp--) = '\0';

		*dstp = '\0';

		return dstp;
	};

	size_t B64_length(size_t len)
	{
		size_t  npad = len%3;
                // padded for multiple of 3 bytes
		size_t  size = (npad > 0)? (len +3-npad ) : len;
         return  (size*8)/6;
	}

	size_t Ascii_length(size_t len)
	{
		return  (len*6)/8;
	}

};


int SM_XML :: DoMatch(const char *text, char *p, bool IsCaseSensitive)
	{
	// probably the MOST DIFFICULT FUNCTION in TurboIRC
	// Thanks to BitchX for copying this function
	//int last;
	int matched;
	//int reverse;
	int pT = 0;
	int pP = 0;

	for(; p[pP] != '\0'; pP++, pT++)
		{
		if (text[pT] == '\0' && p[pP] != '*')
			return MATCH_ABORT;
		switch (p[pP])
			{
			//         case '\\': // Match with following char
			//                pP++;
			// NO BREAK HERE

			default:
				if (IsCaseSensitive)
					{
					if (text[pT] != p[pP])
						return MATCH_FALSE;
					else
						continue;
					}
				if (toupper(text[pT]) != toupper(p[pP]))
					//         if (DMtable[text[pT]] != DMtable[p[pP]])
					return MATCH_FALSE;
				continue;

			case '?':
				continue;

			case '*':
				if (p[pP] == '*')
					pP++;
				if (p[pP] == '\0')
					return MATCH_TRUE;
				while (text[pT])
					{
					matched = DoMatch(text + pT++, p + pP);
					if (matched != MATCH_FALSE)
						return matched;
					}
				return MATCH_ABORT;

			}
		}
#ifdef MATCH_TAR_PATTERN
	if (text[pT] == '/')
		return MATCH_TRUE;
#endif
	return (text[pT] == '\0');
	}



// This will be called from the other funcs
bool SM_XML :: VMatching(const char *text, char *p, bool IsCaseSensitive)
	{
#ifdef OPTIMIZE_JUST_STAR
	if (p[0] == '*' && p[1] == '\0')
		return MATCH_TRUE;
#endif
	return (DoMatch(text, p, IsCaseSensitive) == MATCH_TRUE);
	}




// SM_XML class

void SM_XML :: Version(SM_XML_VERSION_INFO* x)
	{
	x->VersionLow = (SM_XML_VERSION & 0xFF);
	x->VersionHigh = (SM_XML_VERSION >> 8);
	strcpy(x->RDate,SM_XML_VERSION_REVISION_DATE);
	}

SM_XML :: SM_XML()
	{
	Init();
#ifndef LINUX
	IsFileU = false;
#endif
	}

void SM_XML :: SetUnicode(bool x)
	{
#ifndef LINUX
	IsFileU = x;
#endif
	}

SM_XML :: SM_XML(const char* file,SM_XML_LOAD_MODE LoadMode,class SM_XMLTransform* eclass,class SM_XMLTransformData* edata)
	{
	Init();
	Load(file,LoadMode,eclass,edata);
	}

#ifndef LINUX
SM_XML :: SM_XML(const wchar_t* file,SM_XML_LOAD_MODE LoadMode,class SM_XMLTransform* eclass,class SM_XMLTransformData* edata)
	{
	Init();
	Load((char*)file,SM_XML_LOAD_MODE_LOCAL_FILE_U,eclass,edata);
	}
#endif


size_t SM_XML :: LoadText(const char* txt)
	{
	return Load(txt,SM_XML_LOAD_MODE_MEMORY_BUFFER,0,0);
	}

size_t SM_XML :: LoadText(const wchar_t* txt)
	{
#ifdef _WIN32
	size_t udsize = wcslen(txt);
	size_t dsize = udsize*4 + 1000; // just for safety
	Z<char> utfbuff(dsize);

	// Conver the array
	WideCharToMultiByte(CP_UTF8,0,txt,-1,utfbuff,(int)dsize,0,0);
	return LoadText(utfbuff);
#else
	return 0;
#endif
	}

void SM_XML :: Init()
	{
	SOnClose = 0;

	hdr = 0;
	root = 0;
	f = 0;

	initMaps();
	}

void SM_XML :: Clear()
	{
	if (SOnClose)
		Save();

	if (root)
		{
		root->RemoveAllElements();
		delete root;
		}
	root = 0;
	// hdr
	if (hdr)
		delete hdr;
	hdr = 0;
	// item
	if (f)
		delete[] f;
	f = 0;

	}



void SM_XML :: Lock(bool)
	{

	}



SM_XMLElement* SM_XML :: GetRootElement()
	{
	return root;
	}
void SM_XML :: SetRootElement(SM_XMLElement* newroot)
	{
	delete root;
	root = 0;
	root = newroot;
	return;
	}

SM_XMLElement* SM_XML :: RemoveRootElementAndKeep()
	{
	SM_XMLElement* x = root;
	root = new SM_XMLElement(0,"<root/>");
	return x;
	}


int SM_XML :: RemoveTemporalElements()
	{
	if (!root)
		return 0;
	int iN = 0;
	iN += root->RemoveTemporalElements(true);
	iN += root->RemoveTemporalVariables(true);
	return iN;
	}




SM_XMLHeader* SM_XML :: GetHeader()
	{
	return hdr;
	}

void SM_XML :: SetHeader(SM_XMLHeader* h)
	{
	if (hdr)
		delete hdr;
	hdr = 0;
	hdr = h;
	}


size_t SM_XML :: SM_XMLEncode(const char* src,char* trg)
	{
	if (!src)
		return 0;
	//*...
	size_t Y = strlen(src);

	size_t x = 0;
	for(size_t i = 0 ; i < Y ; i++)
		{
		if (src[i] == '&' && src[i + 1] != '#')
			{
			if (trg)
				strcat(trg + x,"&amp;");
			x += 5;
			continue;
			}
		if (src[i] == '>')
			{
			if (trg)
				strcat(trg + x,"&gt;");
			x += 4;
			continue;
			}
		if (src[i] == '<')
			{
			if (trg)
				strcat(trg + x,"&lt;");
			x += 4;
			continue;
			}
		if (src[i] == '\"')
			{
			if (trg)
				strcat(trg + x,"&quot;");
			x += 6;
			continue;
			}
		if (src[i] == '\'')
			{
			if (trg)
				strcat(trg + x,"&apos;");
			x += 6;
			continue;
			}

		if (trg)
			trg[x] = src[i];
		x++;
		}
	if (trg)
		trg[x] = 0;
	return x;
	}

#ifdef SM_XML_OPTIONAL_IMPORTRKEY

SM_XMLElement* SM_XML :: ImportRKey(IMPORTRKEYDATA* d)
	{
	HKEY pK = d->pK;
	int mode = d->StorageType;

	SM_XMLElement* x = new SM_XMLElement(0,"<root />");

	// Reads pK (Assumes it is open) and imports ALL children !

	// if mode == 0 , native backup
	// no name prefix, variable B,I,Y,E,Z,N,Q,S
	//

	// Reads Values of pK and writes it to myself
	for(int i = 0 ; ; i++)
		{
		Z<char> tmp1(300);
		DWORD ts = 20000;
		DWORD ty = 0;
		DWORD si = 0;
		RegEnumValueA(pK,i,tmp1,&ts,0,&ty,0,&si);
		Z<char> tmp2(si + 10);

		ts = 20000;
		if (RegEnumValueA(pK,i,tmp1,&ts,0,&ty,(LPBYTE)tmp2.operator char *() + 2,&si) != ERROR_SUCCESS)
			break; // end of values

		// write
		if (ty == REG_BINARY)
			tmp2[0] = 'B';
		if (ty == REG_DWORD)
			tmp2[0] = 'I';
		if (ty == REG_DWORD_BIG_ENDIAN)
			tmp2[0] = 'Y';
		if (ty == REG_EXPAND_SZ)
			tmp2[0] = 'E';
		if (ty == REG_MULTI_SZ)
			tmp2[0] = 'Z';
		if (ty == REG_NONE)
			tmp2[0] = 'N';
		if (ty == REG_QWORD)
			tmp2[0] = 'Q';
		if (ty == REG_SZ)
			tmp2[0] = 'S';

		if (mode == 0)
			tmp2[1] = '_';

		SM_XMLVariable* v = new SM_XMLVariable(tmp1,tmp2);
#ifdef SM_XML_USE_STL
		x->AddVariable(*v);
#else
		x->AddVariable(v);
#endif
		}

	// Now enum children keys and do the same
	for(int i = 0 ; ; i++)
		{
		Z<char> tmp1(300);
		Z<char> tmp2(300);
		DWORD si = 300;

		if (RegEnumKeyExA(pK,i,tmp1,&si,0,0,0,0) != ERROR_SUCCESS)
			break; // end of values

		sprintf(tmp2,"<%s />",tmp1.operator char*());
		SM_XMLElement* child = new SM_XMLElement(x,tmp2);


		HKEY NewPK = 0;
		RegOpenKeyExA(pK,tmp1,0,KEY_ALL_ACCESS,&NewPK);
		if (NewPK)
			{
			IMPORTRKEYDATA d2 = {0};
			d2.pK = NewPK;
			d2.StorageType = mode;
			ImportRKey(&d2);
			x->AddElement(child);
			RegCloseKey(NewPK);
			}
		}

	return x;
	}
#endif


#ifdef SM_XML_OPTIONAL_IMPORTDB
#include <exdisp.h>
#include <urlmon.h>
#include <shlobj.h>
//#include <adoid.h>
#include <adoint.h>

#define ADOGUID(name, l) extern "C" const GUID name = \
	{l, 0, 0x10, 0x80,0,0,0xAA,0,0x6D,0x2E,0xA4}
ADOGUID(CLSID_CADOConnection,0x00000514);
ADOGUID(IID_IADOConnection,0x00000550);

SM_XMLElement* SM_XML :: ImportDB(IMPORTDBPARAMS* db)
	{
	// Imports an MDB database with ADO
	SM_XMLElement* r = 0;
	Z<char> str(1000);
	Z<wchar_t> wstr(1000);

	ADOConnection* aC = 0;
	HRESULT hr;
	BSTR b1 = 0,b2 = 0,b3 = 0;

	// Open Database
	CoCreateInstance(CLSID_CADOConnection,0,CLSCTX_ALL,IID_IADOConnection,(void**)&aC);
	if (!aC)
		goto E_0;

	if (db->provstr == 0)
		sprintf(str,"Provider=Microsoft.Jet.OLEDB.4.0;Data Source=%s",db->dbname);
	else
		sprintf(str,db->provstr,db->dbname);

	MultiByteToWideChar(CP_ACP,0,str,-1,wstr,1000);


	b1 = SysAllocString(L"");
	b2 = SysAllocString(L"");
	b3 = SysAllocString(wstr);
	hr = aC->Open(b3,b1,b2,0);
	SysFreeString(b1);
	SysFreeString(b2);
	SysFreeString(b3);

	if (hr)
		goto E_1;

	r = new SM_XMLElement(0,"<db />");

	// Loop for all tables
	for(int iTable = 0 ; iTable < db->nTables ; iTable++)
		{
		ADORecordset* aR = 0;
		VARIANT* RecordsAffected = 0;

		sprintf(str,"SELECT * FROM %s",db->Tables[iTable].name);
		MultiByteToWideChar(CP_ACP,0,str,-1,wstr,1000);
		b1 = SysAllocString(wstr);
		hr = aC->Execute(b1,RecordsAffected,0,&aR);
		SysFreeString(b1);

		if (hr)
			continue;

		// Add this table to r
		sprintf(str,"<%s />",db->Tables[iTable].name);
#ifdef SM_XML_USE_STL
		SM_XMLElement& rT = r->AddElement(str);
#else
		SM_XMLElement* rT = new SM_XMLElement(r,str);
		r->AddElement(rT);
#endif

		aR->MoveFirst();
		long TotalRecords = 0;
		aR->get_RecordCount(&TotalRecords);

		for(unsigned int y = 0 ; y < (unsigned)TotalRecords ; y++)
			{
			// We are looping the items now
			short IsEOF;
			aR->get_EOF(&IsEOF);
			if (IsEOF == VARIANT_TRUE)
				break;


			long RecordCount = 0;
			ADOFields* aFS = 0;
			aR->get_Fields(&aFS);
			aFS->get_Count(&RecordCount);

			// Add this item
			sprintf(str,"<%s />",db->Tables[iTable].itemname);
#ifdef SM_XML_USE_STL
			SM_XMLElement& rItem = rT.AddElement(str);
#else
			SM_XMLElement* rItem = new SM_XMLElement(rT,str);
			rT->AddElement(rItem);
#endif

			for(int iVariables = 0 ; iVariables < RecordCount ; iVariables++)
				{
				// We are loopting the variables now, get only what we actually need
				ADOField* aF = 0;
				VARIANT vt;
				vt.vt= VT_I4;
				vt.intVal = iVariables;

				aFS->get_Item(vt,&aF);
				if (!aF)
					continue;

				wchar_t* Name;
				aF->get_Name(&Name);

				VARIANT Value;
				aF->get_Value(&Value);

				if (Value.vt == VT_BSTR)
					WideCharToMultiByte(CP_UTF8, 0, Value.bstrVal, -1, str, 1000,0,0);
				else
					if (Value.vt == VT_I4)
						sprintf(str,"%u",Value.lVal);

				// Should we add this variable to the rItem ?
				int SL = wcslen(Name);
				Z<char> nam(SL*2 + 100);
				WideCharToMultiByte(CP_UTF8, 0, Name, -1, nam, SL*2 + 100,0,0);
				int IsToAdd = -1;
				for(int yAdds = 0; yAdds < db->Tables[iTable].nVariables ; yAdds++)
					{
					if (strcmpi(db->Tables[iTable].Variables[yAdds],nam) == 0)
						{
						IsToAdd = yAdds;
						break;
						}
					}
				if (IsToAdd != -1)
					{
					char* thename = nam;
					if (db->Tables[iTable].ReplaceVariables[IsToAdd])
						thename = db->Tables[iTable].ReplaceVariables[IsToAdd];
#ifdef SM_XML_USE_STL
					rItem.AddVariable(thename,str);
#else
					SM_XMLVariable* var = new SM_XMLVariable(thename,str);
					rItem->AddVariable(var);
#endif
					}


				if (Name)
					SysFreeString(Name);
				if (Value.vt == VT_BSTR)
					SysFreeString(Value.bstrVal);
				aF->Release();
				}

			aFS->Release();
			aR->MoveNext();
			}
		}

E_1:
	aC->Release();

E_0:
	return r;
	}
#endif


int SM_XMLHelper :: pow(int P,int z)
	{
	if (z == 0)
		return 1;
	int x = P;
	for(int i = 1 ; i < z ; i++)
		x*= P;
	return x;
	}

size_t SM_XML :: SM_XMLDecode(const char* src,char* trg)
	{
	size_t Y = strlen(src);
	if (!trg)
		return Y;

	size_t x = 0;
	for(size_t i = 0 ; i < Y ; )
		{
		char* fo = strchr((char*)src + i,'&');
		if (!fo)
			{
			// end of &s
			strcpy(trg + x,src + i);
			x = strlen(trg);
			break;
			}

		if (fo)
			{
			size_t untilfo = fo - (src + i);
			strncpy(trg + x,src + i,untilfo);
			i += untilfo;
			x += untilfo;
			}

		if (src[i] == '&' && src[i + 1] == '#' && tolower(src[i + 2]) == 'x')
			{
			i += 3;
			int dig[10] = {0};
			int y = 0;

			while ((src[i] >= 0x30 && src[i] <= 0x39) || (src[i] >= 'a' && src[i] <= 'f') || (src[i] >= 'A' && src[i] <= 'F'))
				{
				char C = src[i];
				if (C >= 0x30 && C <= 0x39)
					C -= 0x30;
				else
					if (C >= 0x41 && C <= 0x46)
						C -= 55;
					else
						if (C >= 0x61 && C <= 0x66)
							C -= 87;

				dig[y] = C;
				y++;
				i++;
				}

			unsigned long N = 0;
			for(int z = (y - 1) ; z >= 0 ; z--)
				{
				N += dig[z] * SM_XMLHelper :: pow(16,(y - 1) - z);
				}

			// Convert result to UTF-8
			char d1[100] = {0};
#ifdef _WIN32
			wchar_t d2[100] = {0};
			swprintf(d2,L"%c",(wchar_t)N);
			WideCharToMultiByte(CP_UTF8,0,d2,-1,d1,100,0,0);
#endif
			strcat(trg + x,d1);
			x += strlen(d1);
			i++;
			continue;
			}
		if (src[i] == '&' && src[i + 1] == '#')
			{
			i += 2;
			int dig[10] = {0};
			int y = 0;

			while (src[i] >= 0x30 && src[i] <= 0x39)
				{
				dig[y] = src[i] - 0x30;
				y++;
				i++;
				}

			unsigned long N = 0;
			for(int z = (y - 1) ; z >= 0 ; z--)
				{
				N += dig[z] * SM_XMLHelper :: pow(10,(y - 1) - z);
				}

			// Convert result to UTF-8
			char d1[100] = {0};
#ifdef _WIN32
			wchar_t d2[100] = {0};
			swprintf(d2,L"%c",(wchar_t)N);
			WideCharToMultiByte(CP_UTF8,0,d2,-1,d1,100,0,0);
#endif
			strcat(trg + x,d1);
			x += strlen(d1);
			i++;
			continue;
			}


		if (src[i] == '&')
			{
			if (strncmp(src + i + 1,"amp;",4) == 0)
				{
				i += 5;
				trg[x] = '&';
				x++;
				}
			else
				if (strncmp(src + i + 1,"quot;",5) == 0)
					{
					i += 6;
					trg[x] = '\"';
					x++;
					}
				else
					if (strncmp(src + i + 1,"apos;",5) == 0)
						{
						i += 6;
						trg[x] = '\'';
						x++;
						}
					else
						if (strncmp(src + i + 1,"lt;",3) == 0)
							{
							i += 4;
							trg[x] = '<';
							x++;
							}
						else
							if (strncmp(src + i + 1,"gt;",3) == 0)
								{
								i += 4;
								trg[x] = '>';
								x++;
								}
							else
								x++; // ignore invalid symbol
			continue;
			}

		trg[x] = src[i];
		i++;
		x++;
		}

	trg[x] = 0;
	return strlen(trg);
	}


size_t SM_XML :: SM_XMLGetValue(const char* section2,const char* attr2,char* put2,size_t maxlen)
	{


	size_t y1 = SM_XMLEncode(section2,0);
	size_t y2 = SM_XMLEncode(attr2,0);

	Z<char> section(y1 + 10);
	Z<char> attr(y2 + 10);

	SM_XMLEncode(section2,section);
	SM_XMLEncode(attr2,attr);

	if (y1 == 0) // root
		{
		int k = root->FindVariable(attr);
		if (k == -1)
			return 0;


		SM_XMLVariable* v = root->GetVariables()[k];

		size_t Sug = v->GetValue(0);
		Z<char> value(Sug + 10);
		v->GetValue(value);
		size_t Y = strlen(value);
		if (Y > maxlen)
			return Y;

		strcpy(put2,value);
		return Y;
		}



	// section is a\b\c\d...
	SM_XMLElement* r = root;
	char* a2 = section.operator char *();

	for( ; ; )
		{
		char* a1 = strchr(a2,'\\');
		if (a1)
			*a1 = 0;

		int y = r->FindElement(a2);
		if (y == -1)
			{
			if (a1)
				*a1 = '\\';
			return 0;
			}


		r = r->GetChildren()[y];

		if (!a1) // was last
			break;

		*a1 = '\\';
		a2 = a1 + 1;
		}

	// element with this variable is found !
	int k = r->FindVariable(attr);
	if (k == -1)
		return 0;


	SM_XMLVariable* v = r->GetVariables()[k];

	size_t Sug = v->GetValue(0);
	Z<char> value(Sug + 10);
	v->GetValue(value);
	size_t Y = strlen(value);
	if (Y > maxlen)
		return Y;

	strcpy(put2,value);
	return Y;
	}

void SM_XML :: SM_XMLSetValue(const char* section2,const char* attr,char* put)
	{

	// section is a\b\c\d...
	SM_XMLElement* r = root;
	SM_XMLElement* rr = root;
	Z<char> section(strlen(section2) + 10);
	strcpy(section,section2);
	char* a2 = section.operator char *();

	// Also set to delete values
	// if put is NULL, delete the specified attribute
	// if attr is NULL, delete the specified section (and all subsections!)

	if (!section || strlen(section) == 0)
		{
		int k = root->FindVariable(attr);
		if (k == -1)
			{
			root->AddVariable(attr,"");
			k = root->FindVariable(attr);
			}

		if (put == 0)
			{
			// Delete this attribute
			root->RemoveVariable(k);
			}
		else
			{

			root->GetVariables()[k]->SetValue(put);

			}
		return;
		}

	int y = 0;
	for( ; ; )
		{
		char* a1 = strchr(a2,'\\');
		if (a1)
			*a1 = 0;

		y = r->FindElement(a2);
		if (y == -1)
			{
			// Create this element
			r->AddElement(a2);
			y = r->FindElement(a2);
			}

		rr = r;

		r = rr->GetChildren()[y];

		if (!a1) // was last
			break;

		*a1 = '\\';
		a2 = a1 + 1;
		}

	// element with this variable is found/created!
	if (attr == 0)
		{
		// DELETE this element AND all sub-elements!!
		rr->RemoveElement(y);
		return;
		}


	int k = r->FindVariable(attr);
	if (k == -1)
		{
		r->AddVariable(attr,"");
		k = r->FindVariable(attr);
		}


	if (put == 0)
		{
		// Delete this attribute
		r->RemoveVariable(k);
		}
	else
		{

		r->GetVariables()[k]->SetValue(put);

		}
	}




char* SM_XMLHelper :: FindSM_XMLClose(char* s)
	{
	// For Each <!-- we must find a -->
	// For Each <?   we must find a ?>
	// For each <> , we must find a </>
	// For each <![CDATA[ we mst find a ]]>
	// For each < /> , its all ok :)

	int d = 0;
	char* a2 = s;
	bool IsComment = false;
	bool IsCData = false;

	for(;;)
		{
//		int axy = strlen(a2);

		char* a1 = strchr(a2,'<');
		if (!a1) // duh
			return 0;

		if (*(a1 + 1) == '/')
			{
			a2 = strchr(a1,'>');
			if (!a2) // duh
				return 0; 
			a2++;
			d--;
			if (!d)
				return a2;

			continue;
			}

		if ((*(a1 + 1) == '!' && strlen(a1) > 2 && *(a1 + 2) == '-' && *(a1 + 3) == '-') || *(a1 + 1) == '?')
			IsComment = true;

		if (*(a1 + 1) == '!' && strlen(a1) > 8 && strncmp(a1 + 1,"![CDATA[",8) == 0)
			IsCData = true;

		bool Nest = 0;
		for(;;)
			{
			// Bugfix
			if (IsCData && (*(a1) != ']' || *(a1 + 1) != ']' || *(a1 + 2) != '>'))
				{
				a1++;
				continue;
				}
			if (IsCData)
				{
				a1 += 2;
				break;
				}

			if (*a1 != '/' && *a1 != '>')
				{
#ifdef ALLOW_SINGLE_QUOTE_VARIABLES
				if (*a1 == '\"' || *a1 == '\'')
					Nest = !Nest;
#else
				if (*a1 == '\"')
					Nest = !Nest;
#endif

				a1++;
				continue;
				}
			if (*a1 == '/' && Nest)
				{
				a1++;
				continue;
				}
			if (*a1 == '>' && Nest)
				{
				a1++;
				continue;
				}

			// Also continue if / and no comment/no cdata
			if (*a1 == '/' && (IsComment || IsCData))
				{
				a1++;
				continue;
				}

			// Also continue if > and cdata with no ]]
			if (*a1 == '>' && IsCData && (*(a1 - 1) != ']' || *(a1 - 2) != ']'))
				{
				a1++;
				continue;
				}

			// Also continue if > and comment with no --
			if (*a1 == '>' && IsComment && (*(a1 - 1) != '-' || *(a1 - 2) != '-'))
				{
				a1++;
				continue;
				}


			break;
			}

		d++;


		if ((*a1 == '/' || IsComment) && !IsCData) // nice, it closes
			{
			IsComment = false;
			a2 = a1 + 1;
			d--;
			if (d == 0)
				return a2; // finish !
			continue;
			}

		if (*a1 == '>' && IsCData && *(a1 - 1) == ']' && *(a1 - 2) == ']')
			{
			IsCData = false;
			a2 = a1 + 1;

			d--;
			if (d == 0)
				return a2; // finish !
			continue;
			}

		a2 = a1 + 1;
		}
	}   

void SM_XMLHelper :: AddBlankVariable(SM_XMLElement* parent,char *a2,int Pos)
	{
	size_t Y = strlen(a2);
	if (Y == 0 || parent == 0)
		return;

	char* a1 = a2;
	while(*a1 == ' ' || *a1 == '\t' || *a1 == '\n' || *a1 == '\r')
		a1++;
	size_t Z = strlen(a1);
	if (Z == 0)
		return;
	size_t PZ = Z;

	while(a1[PZ - 1] == '\t' || a1[PZ - 1] == '\r' || a1[PZ - 1] == '\n' || a1[PZ - 1] == ' ')
		PZ--;

	if (PZ == 0)
		return;

	char CC = a1[PZ];
	a1[PZ] = 0;

	// Add this vrb

	SM_XMLContent* x = new SM_XMLContent(parent,Pos,a1,true);
	parent->AddContent(x,Pos);

	a1[PZ] = CC;
	}

SM_XMLElement* SM_XMLHelper :: ParseElementTree(SM_XMLHeader* hdr,SM_XMLElement* parent,char* tree,char** EndValue,SM_XML_PARSE_STATUS& iParseStatus)
	{
	char *a1,*a2,*a3,*a4,*a5;//,*a6;
	char c1,c2;//,c3,c4,c5,c6;

	SM_XMLElement* root = 0;

	bool IsRootCommentSecond = false;

	a2 = tree;
	for(;;)
		{
		// find
		a3 = strchr(a2,'<');
		if (!a3)
			{
			int Pos = parent ? parent->GetChildrenNum() : 0;
			SM_XMLHelper :: AddBlankVariable(parent,a2,Pos);
			break; // end/error
			}


		// Bugfix: See if a3 is cdata
		bool IsCData = false;
		if (strncmp(a3,"<![CDATA[",8) == 0)
			IsCData = true;
		// Bugfix: See if a3 is comment
		bool IsComment = false;
		if (strncmp(a3,"<!--",4) == 0)
			IsComment = true;

		// Between a3 and a2, add everything which isn't \r\n,space,tabs
		*a3 = 0;
		int PosV = parent ? parent->GetChildrenNum() : 0;
		SM_XMLHelper :: AddBlankVariable(parent,a2,PosV);
		*a3 = '<';

		if (IsCData == true)
			a4 = strstr(a3,"]]>");
		else
			if (IsComment == true)
				a4 = strstr(a3,"-->");
			else
				a4 = strchr(a3,'>');
		if (!a4)
			break; // end/error
		if (IsCData)
			a4 += 2; // move to '>'
		if (IsComment)
			a4 += 2; // move to '>'

		if ((*(a3 + 1) == '!' && strlen(a3 + 1) > 2 && *(a3 + 2) == '-' && *(a3 + 3) == '-' ) || *(a3 + 1) == '?') // comment/markup
			{
			c2 = *a4;
			*a4 = 0;
			if (parent)
				{
				//SM_XMLElement* c = new SM_XMLElement(parent,a3 + 1,1);
				//parent->AddElement(c);
				int Pos = parent->GetChildrenNum();
				Z<char> com(strlen(a3) + 100);
				strncpy(com,a3 + 4,strlen(a3 + 4) - 2);

				SM_XMLComment* c = new SM_XMLComment(parent,Pos,com);
				parent->AddComment(c,Pos);

				}
			else // It is a root comment
				{
				int Pos = IsRootCommentSecond;
				Z<char> com(strlen(a3) + 100);
				if (strlen(a3 + 4) > 1)
					strncpy(com,a3 + 4,strlen(a3 + 4) - 2);

				SM_XMLComment* c = new SM_XMLComment(0,Pos,com);
				hdr->AddComment(c,Pos);

				}
			*a4 = c2;
			a2 = a4 + 1;
			continue;
			}

		if ((*(a3 + 1) == '!' && strlen(a3 + 1) > 8 && strncmp(a3 + 1,"![CDATA[",8) == 0)) // cdata
			{
			c2 = *a4;
			*a4 = 0;

			int Pos = parent->GetChildrenNum();
			Z<char> com(strlen(a3) + 100);
			strncpy(com,a3 + 9,strlen(a3 + 9) - 2);

			SM_XMLCData* c = new SM_XMLCData(parent,Pos,com);
			parent->AddCData(c,Pos);


			*a4 = c2;
			a2 = a4 + 1;
			continue;
			}


		if (*(a3 + 1) == '/') // bye bye from this element
			{
			if (parent && root && parent->FindElement(root) == -1)
				{

				parent->AddElement(root);

				}
			a2 = a4 + 1;
			continue;
			}


		IsRootCommentSecond = true;
		// It is an opening element
		// If < /> , all ok, Add to current and continue
		// If < > , then find relative at end, and recursive

		if (*(a4 - 1) == '/')
			{
			// Yes it is this element alone
			c2 = *a4;
			*a4 = 0;
			if (parent)
				{

				SM_XMLElement* c = new SM_XMLElement(parent,a3 + 1,0);
				parent->AddElement(c);
				if (!root)
					root = c;

				}
			else
				{

				SM_XMLElement* c = new SM_XMLElement(0,a3 + 1);
				if (!root)
					root = c;

				}

			*a4 = c2;
			a2 = a4 + 1;
			continue;
			}



		// Now it is an < > entry
		// Find this one at end, strchr <
		a5 = SM_XMLHelper :: FindSM_XMLClose(a3);
		if (!a5)
			{
			// ERROR in the FILE
			iParseStatus = SM_XML_PARSE_ERROR;
			return root;
			}

		a5--; // fixes a bug when next element is rightly after closing
		while(*a5 != '<')
			a5--;
		a1 = a5;
		c1 = *a1;
		*a1 = 0;

		// Create element a3
		c2 = *(a4 + 1);
		*(a4 + 1) = 0;
		root = new SM_XMLElement(parent,a3,0);
		*(a4 + 1) = c2;
		char* eV = 0;
		SM_XMLHelper :: ParseElementTree(hdr,root,a4 + 1,&eV,iParseStatus);



		char* sa2 = a2;
		*a1 = c1;
		a2 = a1;
		if (eV)
			a2 = eV;
		if (a2 == sa2)
			break; // whops ? error! . Break to avoid fatal loop
		continue;
		}

	return root;
	}

int SM_XMLElement :: RemoveAllElements()
	{

	for(int i = childrennum - 1 ; i >= 0 ; i--)
		{
		if (children[i] == 0)
			{
			// Unload 
			DeleteUnloadedElementFile(i);
			}

		bool DoDelete = true;
// 		for(unsigned int xi = 0 ; xi < NumBorrowedElements ; xi++)
// 			{
// 			if (BorrowedElements[xi].Active == 0)
// 				continue;
// 			if (BorrowedElements[xi].x == children[i])
// 				{
// 				BorrowedElements[xi].Active = 0;
// 				DoDelete = false;
// 				break;
// 				}
// 			}

		if (DoDelete)
			delete children[i];
		children[i] = 0;
		}
	childrennum = 0;

	return 0;
	}

// void SM_XMLElement :: SetElementParam(unsigned __int64 p)
// 	{
// 	_param = p;
// 	}
// 
// unsigned __int64 SM_XMLElement :: GetElementParam()
// 	{
// 	return _param;
// 	}


int SM_XMLElement :: DeleteUnloadedElementFile(int i)
	{
	// Find Unique STR
	size_t si = GetElementUniqueString(0);
	Z<char> us(si);
	GetElementUniqueString(us);
	if (us[strlen(us) - 1] == '-')
		us[strlen(us) - 1] = 0;
	// Add this element
	if (strlen(us))
		sprintf(us + strlen(us),"-%u",i);
	else
		sprintf(us + strlen(us),"%u",i);
	// Extension
	strcat(us,".xmltmp");

#ifdef LINUX
	return remove(us);
#else // Win32
#ifdef WINCE // Only Wide
	Z<wchar_t> usw(si + 1000);
	MultiByteToWideChar(CP_ACP,0,us,-1,usw,si + 1000);
	return DeleteFileW(usw);
#else
	return DeleteFileA(us);
#endif
#endif
	}


bool SM_XMLElement :: ReplaceElement(unsigned int i,SM_XMLElement* ne,SM_XMLElement** prev)
	{
	if (childrennum <= i)
		return false;
	SM_XMLElement* xu = 0;
	RemoveElementAndKeep(i,&xu);
	if (!xu)
		return false;
	if (prev)
		*prev = xu;
	else
		delete xu;
	InsertElement(i,ne);
	return true;
	}


int SM_XMLElement :: GetElementIndex(SM_XMLElement* e)
	{

	for(unsigned int i = 0 ; i < childrennum ; i++)
		{
		if (children[i] == e)
			return i;
		}
	return -1;

	}

int SM_XMLElement :: GetDeepLevel()
	{
	if (!parent)
		return 0;
	return parent->GetDeepLevel() + 1;
	}

int SM_XMLElement :: RemoveElement(SM_XMLElement* e)
	{
	int X = -1;

	for(unsigned int i = 0 ; i < childrennum ; i++)
		{
		if (children[i] == e)
			{
			X = RemoveElement(i);
			break;
			}
		}
	return X;
	}

int SM_XMLElement :: RemoveElement(unsigned int i)
	{

	if (i >= childrennum)
		return childrennum;

	if (children[i] == 0)
		{
		// Unloaded already, just delete the file
		DeleteUnloadedElementFile(i);
		}

	bool DoDelete = true;
	// Check if this item is borrowed
// 	for(unsigned int xi = 0 ; xi < NumBorrowedElements ; xi++)
// 		{
// 		if (BorrowedElements[xi].Active == 0)
// 			continue;
// 		if (BorrowedElements[xi].x == children[i])
// 			{
// 			BorrowedElements[xi].Active = 0;
// 			DoDelete = false;
// 			break;
// 			}
// 		}


	if (DoDelete)
		delete children[i];

	children[i] = 0;

	for(unsigned int k = i ; k < childrennum ; k++)
		children[k] = children[k + 1];

	children[childrennum - 1] = 0;
	return --childrennum;

	}


int SM_XMLElement :: RemoveElementAndKeep(unsigned int i,SM_XMLElement** el)
	{
	if (el) 
		*el = 0;   

	if (i >= childrennum)
		return childrennum;


	if (children[i] == 0) // unloaded
		ReloadElement(i);


	//delete children[i];
	if (el) 
		*el = children[i];   
	children[i] = 0;

	for(unsigned int k = i ; k < childrennum ; k++)
		children[k] = children[k + 1];

	children[childrennum - 1] = 0;
	return --childrennum;
	}



int SM_XMLElement :: UnloadElement(unsigned int i)
	{
	SM_XMLElement* e = children[i];
	if (!e)
		return 1; // already unloaded

	e->ReloadAllElements();

	// Find Unique STR
	size_t si = GetElementUniqueString(0);
	Z<char> us(si);
	GetElementUniqueString(us);
	if (us[strlen(us) - 1] == '-')
		us[strlen(us) - 1] = 0;
	// Add this element
	if (strlen(us))
		sprintf(us + strlen(us),"-%u",i);
	else
		sprintf(us + strlen(us),"%u",i);
	// Extension
	strcat(us,".xmltmp");



	FILE* fp = fopen(us,"rb");
	if (fp)
		{
		// file exists !
		fclose(fp);
		return 0;
		}

	fp = fopen(us,"wb");
	if (!fp)
		{
		// Failed !
		return 0;
		}

	e->Export(fp,1,SM_XML_SAVE_MODE_ZERO);
	fclose(fp);

	// Delete this element, but do not remove it.
	delete children[i];
	children[i] = 0;

	return 1;
	}

int SM_XMLElement :: ReloadElement(unsigned int i)
	{
	if (children[i])
		return 1; // Item is already here

	// Find Unique STR
	size_t si = GetElementUniqueString(0);
	Z<char> us(si);
	GetElementUniqueString(us);
	if (us[strlen(us) - 1] == '-')
		us[strlen(us) - 1] = 0;
	// Add this element
	if (strlen(us))
		sprintf(us + strlen(us),"-%u",i);
	else
		sprintf(us + strlen(us),"%u",i);
	// Extension
	strcat(us,".xmltmp");

	FILE* fp = fopen(us,"rb");
	if (!fp)
		{
		// file failed !
		return 0;
		}
	fclose(fp);
	SM_XML fx(us);
	int K = fx.ParseStatus();
	if (K == 2) // Fatal error
		return 0; 

	SM_XMLElement* r = fx.RemoveRootElementAndKeep();

	// Reload element
	children[i] = r;
	r->SetParent(this);

#ifdef LINUX
	remove(us);
#else // Win32
#ifdef WINCE // Only Wide
	Z<wchar_t> usw(si + 1000);
	MultiByteToWideChar(CP_ACP,0,us,-1,usw,si + 1000);
	DeleteFileW(usw);
#else
	DeleteFileA(us);
#endif
#endif
	return 1;
	}

int SM_XMLElement :: ReloadAllElements()
	{
	for(unsigned int i = 0 ; i < childrennum ; i++)
		{
		if (children[i] == 0)
			ReloadElement(i);
		}
	return 0;
	}


SM_XMLElement* SM_XMLElement :: MoveElement(unsigned int i,unsigned int y)
	{

	if (i >= childrennum || y >= childrennum)
		return 0;

	SM_XMLElement* x = children[i];

	children[i] = 0;
	for(unsigned int k = i ; k < childrennum ; k++)
		children[k] = children[k + 1];

	childrennum--;
	return InsertElement(y,x);

	}


SM_XMLElement* SM_XMLElement :: InsertElement(unsigned int y,SM_XMLElement* x)
	{
	// leave from 0 to y
	// move from y + 1 to childrennum + 1
	// save
	// childrennum++;
	if (y >= childrennum)
		return AddElement(x);

	SpaceForElement(1);

	memmove((void*)(children + y + 1),(void*)(children + y),(childrennum - y)*sizeof(SM_XMLElement*));
	children[y] = x;
	x->SetParent(this);
	childrennum++;
	return x;
	}


int SM_XMLElement :: BorrowElement(SM_XMLElement*x,unsigned int y)
	{
	// Same as Insert or Add, but no SetParent

	// put 'x' in the list of 'borrowed elements'
// 	if (BorrowedElements.is() <= NumBorrowedElements)
// 		BorrowedElements.AddResize(5);
// 	SM_XMLBORROWELEMENT xb = {0};
// 	xb.Active = 1;
// 	xb.x = x;
// 	BorrowedElements[NumBorrowedElements++] = xb;
// 
// 	SpaceForElement(1);
// 	if (y >= childrennum)
// 		{
// 		children[childrennum++] = x;
// 		return childrennum;
// 		}
// 
// 	memmove((void*)(children + y + 1),(void*)(children + y),(childrennum - y)*sizeof(SM_XMLElement*));
// 	children[y] = x;
// 	childrennum++;
// 
// 	return y;
		return 0;
	}

int SM_XMLElement :: ReleaseBorrowedElements()
	{
// 	int R = 0;
// 	for(unsigned int y = 0 ; y < NumBorrowedElements ; y++)
// 		{
// 		SM_XMLBORROWELEMENT& xb = BorrowedElements[y];
// 		if (xb.Active == 0)
// 			continue;
// 		for(int i = (childrennum - 1) ; i >= 0 ; i--)
// 			{
// 			if (children[i] == xb.x)
// 				{
// 				RemoveElement(i);
// 				xb.Active = 0;
// 				R++;
// 				}
// 			}
// 		}
// 	NumBorrowedElements = 0;
// 	return R;
		return 0;
	}


int SM_XMLElement :: UpdateElement(SM_XMLElement* e,bool UpdateVariableValues)
	{
	/*

	Formula

	Checks variables, if not existing, copies
	If existing, does nothing.
	Search is case-sensitive.

	Checks elements, if not existing, copies
	if existing, calls UpdateElement() for each element

	*/

	// Test the variables
	Z<char> vn(1000);
	unsigned int i;
	for(i = 0 ; i < e->GetVariableNum() ; i++)
		{

		SM_XMLVariable* v = e->GetVariables()[i];

		if (v->GetName(0) > 1000)
			vn.Resize(v->GetName(0) + 1000);
		v->GetName(vn);

		SM_XMLVariable* tv = FindVariableZ(vn,0);
		if (tv == 0)
			{
			// Create

			AddVariable(v);

			}
		else
			{
			if (UpdateVariableValues)
				{
				if (v->GetValue(0) > 1000)
					vn.Resize(v->GetValue(0) + 1000);
				v->GetValue(vn);
				tv->SetValue(vn);
				}
			}
		}

	// Test the elements
	for(i = 0 ; i < e->GetChildrenNum() ; i++)
		{

		SM_XMLElement* c = e->GetChildren()[i];

		if (c->GetElementName(0) > 1000)
			vn.Resize(c->GetElementName(0) + 1000);
		c->GetElementName(vn);

		SM_XMLElement* tc = FindElementZ(vn,0);
		if (tc == 0)
			{
			// Copy

			AddElement(c->Duplicate());

			}
		else
			{
			tc->UpdateElement(c,UpdateVariableValues);
			}
		}




	return 0;
	}

int SM_XMLElement :: RemoveTemporalVariables(bool Deep)
	{
	int iNum = 0;

	for(int i = variablesnum - 1 ; i >= 0 ; i--)

		{

		if (variables[i]->IsTemporal())

			{
			RemoveVariable(i);
			iNum++;
			}
		}
	if (Deep)
		{

		for(unsigned int i = 0 ; i < childrennum ; i++)

			{

			iNum += children[i]->RemoveTemporalVariables();

			}
		}
	return iNum;
	}

int SM_XMLElement :: RemoveTemporalElements(bool Deep)
	{
	int iNum = 0;

	for(int i = childrennum - 1 ; i >= 0 ; i--)

		{

		if (children[i]->IsTemporal())

			{
			RemoveElement(i);
			iNum++;
			}
		}
	if (Deep)
		{

		for(unsigned int i = 0 ; i < childrennum ; i++)

			{

			iNum += children[i]->RemoveTemporalElements();

			}	
		}
	return iNum;
	}

int SM_XMLElement :: RemoveAllVariables()
	{

	for(int i = variablesnum - 1 ; i >= 0 ; i--)
		{
		delete variables[i];
		variables[i] = 0;
		}
	variablesnum = 0;

	return 0;
	}


int SM_XMLElement :: RemoveVariable(SM_XMLVariable* e)
	{
	int X = -1;

	for(unsigned int i = 0 ; i < variablesnum ; i++)
		{
		if (variables[i] == e)
			{
			X = RemoveVariable(i);
			break;
			}
		}
	return X;

	}

int SM_XMLElement :: RemoveVariable(unsigned int i)
	{

	if (i >= variablesnum)
		return variablesnum;

	delete variables[i];
	variables[i] = 0;

	for(unsigned int k = i ; k < variablesnum ; k++)
		variables[k] = variables[k + 1];

	variables[variablesnum - 1] = 0;
	return --variablesnum;

	}


int SM_XMLElement :: RemoveVariableAndKeep(unsigned int i,SM_XMLVariable** vr)
	{
	if (vr) 
		{
		*vr = 0;   
		}
	if (i >= variablesnum)
		return variablesnum;

	//delete variables[i];
	if (vr) 
		{
		*vr = variables[i];   
		}
	variables[i] = 0;

	for(unsigned int k = i ; k < variablesnum ; k++)
		variables[k] = variables[k + 1];

	variables[variablesnum - 1] = 0;
	return --variablesnum;
	}


void SM_XML :: Export(FILE* fp,SM_XML_SAVE_MODE SaveMode,SM_XML_TARGET_MODE TargetMode,SM_XMLHeader *hdr,class SM_XMLTransform* eclass,class SM_XMLTransformData* edata)
	{
	// Export all elements

	root->Export(fp,1,SaveMode,TargetMode,hdr,eclass,edata);
	}

void SM_XML :: SetExportFormatting(SM_XMLEXPORTFORMAT* xf)
	{

	root->SetExportFormatting(xf);
	}

SM_XMLElement* SM_XMLElement :: GetElementInSection(const char* section2)
	{
	// From section, get the element we need
	SM_XMLElement* r = this;
	if (strcmp(section2,"") == 0)
		return this;

	Z<char> section(strlen(section2) + 1);
	strcpy(section,section2);

	char* a2 = section.operator char *();

	for( ; ; )
		{
		char* a1 = strchr(a2,'\\');
		if (a1)
			*a1 = 0;

		int y = r->FindElement(a2);
		if (y == -1)
			{
			if (a1)
				*a1 = '\\';
			return 0;
			}


		r = r->GetChildren()[y];

		if (!a1) // was last
			break;

		*a1 = '\\';
		a2 = a1 + 1;
		}


	return r;
	}


void SM_XMLElement :: Write16String(FILE* fp,const char* s)
	{
#ifdef _WIN32
	size_t sl = strlen(s)*2 + 100;
	Z<wchar_t> ws(sl);
	MultiByteToWideChar(CP_UTF8,0,s,-1,ws,(int)sl);
	fwrite(ws.operator wchar_t*(),1,wcslen(ws)*2,fp);
#endif
	}

void SM_XMLElement :: printc(FILE* fp,SM_XMLElement* root,int deep,int ShowAll,SM_XML_SAVE_MODE SaveMode,SM_XML_TARGET_MODE TargetMode)
	{
	if (!root)
		return;


	root->ReloadAllElements();


	char* sp = (char*)fp;
	if (TargetMode == 1)
		sp += strlen(sp);
	unsigned int spi = 0;
#ifdef _WIN32
	HKEY pKey = (HKEY)fp;
	HKEY pKey2 = 0;
#endif


	/*
	Targetmodes

	0 	- Export to a FILE*
	1 	- Export to memory
	2  - Export to a registry key (Win32)
	3  - Export to a FILE* , utf-16

	*/

	char DelimiterChar[100] = {0};
	if (root->xfformat.UseSpace)
		{
		for(int i = 0 ; i < root->xfformat.nId ; i++)
			strcat(DelimiterChar," ");
		}
	else
		{
		for(int i = 0 ; i < root->xfformat.nId ; i++)
			strcat(DelimiterChar,"\t");
		}
	//* Use it later


	size_t Sug = root->GetElementName(0,SaveMode);
	Z<char> b(Sug + deep + 100);
	for(int i = 0 ; i < deep ; i++)
		//strcat(b,"\t");
		strcat(b,DelimiterChar);

	strcat(b,"<");
	root->GetElementName(b.operator char*() + strlen(b),SaveMode);
	if (TargetMode == 1)
		{
		spi = sprintf(sp,"%s",b.operator char*());
		sp += spi;
		}
	else
		if (TargetMode == 2)
			{
#ifdef _WIN32
#ifndef WINCE
			SM_XMLElement* par = root->GetParent();
			int bP = 0;
			if (par)
				{
				bP = par->FindElement(root);
				}
			sprintf(b,"E%u",bP);
			root->GetElementName(b.operator char *() + strlen(b),SaveMode);
			DWORD dw = 0;
			RegCreateKeyExA(pKey,b,0,0,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,0,&pKey2,&dw);
#endif
#endif
			}
		else
			if (TargetMode == 3)
				Write16String(fp,b.operator char*());
			else
				fprintf(fp,"%s",b.operator char*());

	int iY = root->GetVariableNum();
	int iC = root->GetChildrenNum();

	// print variables if they exist
	//   SM_XMLVariable* SaveAstVariable = 0;
	if (iY)
		{
		for(int i = 0 ; i < iY ; i++)
		 {

		 SM_XMLVariable* v = root->GetVariables()[i];

		 size_t s1 = v->GetName(0,SaveMode);
		 size_t s2 = v->GetValue(0,SaveMode);

		 Z<char> Name(s1 + 10);
		 Z<char> Value(s2 + 10);

		 v->GetName(Name,SaveMode);
		 v->GetValue(Value,SaveMode);

		 /*		 if (strcmp(Name,"*") == 0)
		 SaveAstVariable = v;
		 else*/
			 {
			 if (TargetMode == 1)
				 {
				 spi = sprintf(sp," %s=",Name.operator char*());
				 sp += spi;
				 spi = sprintf(sp,"\"%s\"",Value.operator char*());
				 sp += spi;
				 }
			 else
				 if (TargetMode == 2)
					 {
#ifdef _WIN32
#ifndef WINCE
					 // create a value
					 Z<char> VName(strlen(Name) + 10);
					 sprintf(VName,"V%s",Name.operator char*());
					 RegSetValueExA(pKey2,VName,0,REG_SZ,(const BYTE*)Value.operator char *(),(int)(strlen(Value) + 1));
#endif
#endif
					 }
				 else
					 if (TargetMode == 3)
						 {
						 Z<char> xy(strlen(Name)*2 + 100 + strlen(Value)*2);
						 sprintf(xy," %s=\"%s\"",Name.operator char*(),Value.operator char*());
						 Write16String(fp,xy);
						 }
					 else // TM == 0
						 {
						 fprintf(fp," %s=",Name.operator char*());
						 fprintf(fp,"\"%s\"",Value.operator char*());
						 }
			 }
		 }
		}

	// cdatas, comments, contents may be between children
	int TotalCDatas = root->GetCDatasNum();
	int NextCData = 0;

	int TotalComments = root->GetCommentsNum();
	int NextComment = 0;

	int TotalContents = root->GetContentsNum();
	int NextContent = 0;

	// children ?
	// close now if no children/contents/comments
	if ((!iC || ShowAll == 0) && /*SaveAstVariable == 0 &&*/ TotalContents == 0 && TotalComments == 0 && TotalCDatas == 0)
		{
		if (TargetMode == 1)
			{
			spi = sprintf(sp,"/>\r\n");
			sp += spi;
			}
		else
			if (TargetMode == 2)
				; // Nothing :)
			else
				if (TargetMode == 3)
					fwrite(L"/>\r\n",1,4,fp);
				else // 0
					fprintf(fp,"/>\r\n");
		return;
		}
	if (TargetMode == 1)
		{
		spi = sprintf(sp,">\r\n",b.operator char*());
		sp += spi;
		}
	else
		{
		// Write \r\n only if ElementBreak
		if (root->xfformat.ElementsNoBreak == false || TotalContents != 1 || TotalComments || TotalCDatas || iC)
			{
			if (TargetMode == 2)
				; // Nothing :)
			else
				if (TargetMode == 3)
					fwrite(L">\r\n",1,3,fp);
				else
					fprintf(fp,">\r\n",b.operator char*());
			}
		else
			{
			if (TargetMode == 2)
				; // Nothing :)
			else
				if (TargetMode == 3)
					fwrite(L">",1,1,fp);
				else
					fprintf(fp,">",b.operator char*());
			}
		}


	if (ShowAll)
		{
		for(int i = 0 ; i < iC ; i++)
		 {
		 if (TotalComments && (NextComment < TotalComments))
			 {

			 while ((NextComment < TotalComments) && root->GetComments()[NextComment]->GetEP() <= i)

				 {
				 // print that comment now

				 const char* t = root->GetComments()[NextComment]->operator const char *();

				 Z<char> b(strlen(t) + deep + 200);
				 for(int i = 0 ; i < (deep + 1) ; i++)
					 //				  strcat(b,"\t");
					 strcat(b,DelimiterChar);
				 strcat(b,"<!--");
				 strcat(b,t);
				 strcat(b,"-->\r\n");

				 if (TargetMode == 1)
					 {
					 spi = sprintf(sp,"%s",b.operator char*());
					 sp += spi;
					 }
				 else
					 if (TargetMode == 2)
						 {
#ifdef _WIN32
#ifndef WINCE
						 // Create a comment
						 Z<char> VName(20);
						 sprintf(VName,"C%u",NextComment);
						 RegSetValueExA(pKey2,VName,0,REG_SZ,(const BYTE*)t,(int)(strlen(t) + 1));
#endif
#endif
						 }
					 else
						 if (TargetMode == 3)
							 Write16String(fp,b.operator char*());
						 else // 0
							 fprintf(fp,"%s",b.operator char*());

				 NextComment++;
				 }
			 }

		 if (TotalContents && (NextContent < TotalContents))
			 {

			 while ((NextContent < TotalContents) && root->GetContents()[NextContent]->GetEP() <= i)

				 {
				 // print that content now
				 //char* t = root->GetContents()[NextContent]->operator char *();

				 size_t vx = root->GetContents()[NextContent]->GetValue(0);


				 Z<char> b(vx + deep + 200);
				 if (root->xfformat.ElementsNoBreak == false)
					 {
					 for(int i = 0 ; i < (deep + 1) ; i++)
						 //strcat(b,"\t");
						 strcat(b,DelimiterChar);
					 //strcat(b,t);
					 }

				 root->GetContents()[NextContent]->GetValue(b.operator char*() + strlen(b),SaveMode);

				 if (root->xfformat.ElementsNoBreak == false)
					 strcat(b,"\r\n");

				 if (TargetMode == 1)
					 {
					 spi = sprintf(sp,"%s",b.operator char*());
					 sp += spi;
					 }
				 else
					 if (TargetMode == 2)
						 {
#ifdef _WIN32
#ifndef WINCE
						 // Create a content
						 Z<char> VName(20);
						 sprintf(VName,"D%u",NextContent);

						 root->GetContents()[NextContent]->GetValue(b,SaveMode);

						 RegSetValueExA(pKey2,VName,0,REG_SZ,(const BYTE*)b.operator char *(),(int)(strlen(b) + 1));
#endif
#endif
						 }
					 else
						 if (TargetMode == 3)
							 Write16String(fp,b.operator char*());
						 else // 0
							 fprintf(fp,"%s",b.operator char*());

				 NextContent++;
				 }
			 if (TotalCDatas && (NextCData < TotalCDatas))
				 {

				 while ((NextCData < TotalCDatas) && root->GetCDatas()[NextCData]->GetEP() <= i)

					 {
					 // print that CData now

					 const char* t = root->GetCDatas()[NextCData]->operator const char *();

					 Z<char> b(strlen(t) + deep + 200);
					 for(int i = 0 ; i < (deep + 1) ; i++)
						 //				  strcat(b,"\t");
						 strcat(b,DelimiterChar);
					 strcat(b,"<![CDATA[");
					 strcat(b,t);
					 strcat(b,"]]>\r\n");

					 if (TargetMode == 1)
						 {
						 spi = sprintf(sp,"%s",b.operator char*());
						 sp += spi;
						 }
					 else
						 if (TargetMode == 2)
							 {
#ifdef _WIN32
#ifndef WINCE
							 // Create a cdata
							 Z<char> VName(20);
							 sprintf(VName,"D%u",NextCData);
							 RegSetValueExA(pKey2,VName,0,REG_SZ,(const BYTE*)t,(int)(strlen(t) + 1));
#endif
#endif
							 }
						 else
							 if (TargetMode == 3)
								 Write16String(fp,b.operator char*());
							 else // 0
								 fprintf(fp,"%s",b.operator char*());

					 NextCData++;
					 }
				 }
			 }


#ifdef _WIN32
		 if (TargetMode == 2)

			 printc((FILE*)pKey2,root->GetChildren()[i],deep + 1,ShowAll,SaveMode,TargetMode);

		 else
#endif		 	
			 {

			 printc(fp,root->GetChildren()[i],deep + 1,ShowAll,SaveMode,TargetMode);

			 if (TargetMode == 1)
				 sp = (char*)fp + strlen((char*)fp);
			 }
		 }
		}

	// Check if there are still comments
	if (TotalComments && (NextComment < TotalComments))
		{
		while (NextComment < TotalComments)
		 {
		 // print that comment now

		 const char* t = root->GetComments()[NextComment]->operator const char *();

		 Z<char> b(strlen(t) + deep + 200);
		 for(int i = 0 ; i < (deep + 1) ; i++)
			 //strcat(b,"\t");
			 strcat(b,DelimiterChar);
		 strcat(b,"<!--");
		 strcat(b,t);
		 strcat(b,"-->\r\n");

		 if (TargetMode == 1)
			 {
			 spi = sprintf(sp,"%s",b.operator char*());
			 sp += spi;
			 }
		 else
			 if (TargetMode == 2)
				 {
#ifdef _WIN32
#ifndef WINCE
				 // Create a comment
				 Z<char> VName(20);
				 sprintf(VName,"C%u",NextComment);
				 RegSetValueExA(pKey2,VName,0,REG_SZ,(const BYTE*)t,(int)(strlen(t) + 1));
#endif
#endif
				 }
			 else
				 if (TargetMode == 3)
					 Write16String(fp,b.operator char*());
				 else // 0
					 fprintf(fp,"%s",b.operator char*());

		 NextComment++;
		 }
		}

	// Check if there are still cdatas
	if (TotalCDatas && (NextCData < TotalCDatas))
		{
		while (NextCData < TotalCDatas)
		 {
		 // print that CData now

		 const char* t = root->GetCDatas()[NextCData]->operator const char *();

		 size_t ix = strlen(t);
		 Z<char> b(strlen(t) + deep + 200);
		 for(int i = 0 ; i < (deep + 1) ; i++)
			 //strcat(b,"\t");
			 strcat(b,DelimiterChar);
		 strcat(b,"<![CDATA[");
		 strcat(b,t);
		 strcat(b,"]]>\r\n");

		 if (TargetMode == 1)
			 {
			 spi = sprintf(sp,"%s",b.operator char*());
			 sp += spi;
			 }
		 else
			 if (TargetMode == 2)
				 {
#ifdef _WIN32
#ifndef WINCE
				 // Create a cdata
				 Z<char> VName(20);
				 sprintf(VName,"D%u",NextCData);
				 RegSetValueExA(pKey2,VName,0,REG_SZ,(const BYTE*)t,(int)(strlen(t) + 1));
#endif
#endif
				 }
			 else
				 if (TargetMode == 3)
					 Write16String(fp,b.operator char*());
				 else // 0
					 fprintf(fp,"%s",b.operator char*());

		 NextCData++;
		 }
		}


	// Check if there are still Contents
	if (TotalContents && (NextContent < TotalContents))
		{
		while (NextContent < TotalContents)
		 {
		 // print that content now
		 //char* t = root->GetContents()[NextContent]->operator char *();

		 size_t vx = root->GetContents()[NextContent]->GetValue(0);


		 Z<char> b(vx + deep + 200);
		 if (root->xfformat.ElementsNoBreak == false)
			 {
			 for(int i = 0 ; i < (deep + 1) ; i++)
				 //strcat(b,"\t");
				 strcat(b,DelimiterChar);
			 }
		 //strcat(b,t);

		 root->GetContents()[NextContent]->GetValue(b.operator char*() + strlen(b),SaveMode);

		 if (root->xfformat.ElementsNoBreak == false)
			 strcat(b,"\r\n");

		 if (TargetMode == 1)
			 {
			 spi = sprintf(sp,"%s",b.operator char*());
			 sp += spi;
			 }
		 else
			 if (TargetMode == 2)
				 {
#ifdef _WIN32
#ifndef WINCE
				 // Create a content
				 Z<char> VName(20);
				 sprintf(VName,"D%u",NextContent);

				 root->GetContents()[NextContent]->GetValue(b,SaveMode);

				 RegSetValueExA(pKey2,VName,0,REG_SZ,(const BYTE*)b.operator char *(),(int)(strlen(b) + 1));
#endif
#endif
				 }
			 else
				 if (TargetMode == 3)
					 Write16String(fp,b.operator char*());
				 else // 0
					 fprintf(fp,"%s",b.operator char*());

		 NextContent++;
		 }
		}

	// ending
	strcpy(b,"");
	if (root->xfformat.ElementsNoBreak == false || iC || TotalCDatas || TotalComments || TotalContents != 1)
		{
		for(int i = 0 ; i < deep ; i++)
			//strcat(b,"\t");
			strcat(b,DelimiterChar);
		}
	strcat(b,"</");
	root->GetElementName(b.operator char*() + strlen(b));
	strcat(b,">\r\n");

	if (TargetMode == 1)
		{
		spi = sprintf(sp,"%s",b.operator char*());
		sp += spi;
		}
	else
		if (TargetMode == 2)
			{
			// Nothing
#ifdef _WIN32
			RegCloseKey(pKey2);
#endif
			}
		else
			if (TargetMode == 3)
				Write16String(fp,b.operator char*());
			else // 0
				fprintf(fp,"%s",b.operator char*());
	}

void SM_XMLElement :: SetExportFormatting(SM_XMLEXPORTFORMAT* xf)
	{
	if (xf)
		memcpy(&xfformat,xf,sizeof(SM_XMLEXPORTFORMAT));
	if (xfformat.nId > 50)
		xfformat.nId = 50;
	for(unsigned int i = 0 ; i < GetChildrenNum() ; i++)

		GetChildren()[i]->SetExportFormatting(xf);

	}

void SM_XMLElement :: Export(FILE* fp,int ShowAll,SM_XML_SAVE_MODE SaveMode,SM_XML_TARGET_MODE TargetMode,SM_XMLHeader* hdr,class SM_XMLTransform* eclass,class SM_XMLTransformData* edata)
	{
	// Export this element

	ReloadAllElements();


	if (eclass == 0)
		{
		if (hdr)
			hdr->Export(fp,0,TargetMode,eclass,edata);
		printc(fp,this,0,ShowAll,SaveMode,TargetMode);
		if (hdr)
			hdr->Export(fp,1,TargetMode,eclass,edata);
		}
	else
		{
		//* save to another fp, then encrypt with eclass to this fp
		size_t S = MemoryUsage();
		Z<char> ram(S);
		SM_XML_TARGET_MODE NewTargetMode = SM_XML_TARGET_MODE_MEMORY;
		if (hdr)
			hdr->Export((FILE*)ram.operator char *(),0,NewTargetMode,eclass,edata);
		printc((FILE*)ram.operator char *(),this,0,ShowAll,SaveMode,NewTargetMode);
		if (hdr)
			hdr->Export((FILE*)ram.operator char *(),1,NewTargetMode,eclass,edata);


		Z<char> yy(S + 100);

		// convert
		//eclass->Prepare(edata);
		S = strlen(ram);
		size_t nS = eclass->Encrypt(ram.operator char *(),S,0,yy.operator char *(),S + 100,0);

		// Write
		if (TargetMode == 0)
			fwrite(yy.operator char* (),1,nS,fp);
		else
			if (TargetMode == 1)
				memcpy((char*)fp,yy.operator char *(),nS);


		/*      char* tf = ".\\a.tmp";
		FILE* fpn = fopen(tf,"wb");
		if (!fpn)
		return;

		// save
		if (hdr)
		hdr->Export(fp,0,TargetMode,eclass,edata);
		printc(fpn,this,0,ShowAll,SaveMode,TargetMode);
		if (hdr)
		hdr->Export(fp,1,TargetMode,eclass,edata);

		int S = ftell(fpn);
		fclose(fpn);

		// read a.tmp again
		Z<char>* y = SM_XML :: ReadToZ(tf);
		SM_XML :: Kill(tf);

		Z<char> yy(S + 100);

		// convert
		eclass->Prepare(edata);
		int nS = eclass->Encrypt((*y).operator char *(),S,0,yy.operator char *(),S + 100,0);

		fwrite(yy.operator char* (),1,nS,fp);
		delete y;
		*/
		}
	}



int _USERENTRY  SM_XMLElementfcmp(const void * a, const void * b)
	{
	SM_XMLElement* x1 = *(SM_XMLElement**)a;
	SM_XMLElement* x2 = *(SM_XMLElement**)b;

	// compare names
	size_t z1 = x1->GetElementName(0);
	size_t z2 = x2->GetElementName(0);

	Z<char> s1(z1 + 10);
	Z<char> s2(z2 + 10);
	x1->GetElementName(s1);
	x2->GetElementName(s2);

	return strcmpi(s1,s2);
	}
int _USERENTRY  SM_XMLVariablefcmp(const void * a, const void * b)
	{
	SM_XMLVariable* x1 = *(SM_XMLVariable**)a;
	SM_XMLVariable* x2 = *(SM_XMLVariable**)b;

	// compare names
	size_t z1 = x1->GetName(0);
	size_t z2 = x2->GetName(0);

	Z<char> s1(z1 + 10);
	Z<char> s2(z2 + 10);
	x1->GetName(s1);
	x2->GetName(s2);

	return strcmpi(s1,s2);
	}



void SM_XMLElement :: SortElements(int (_USERENTRY *fcmp)(const void *, const void *))
	{
	// to all elements
	SM_XMLElement** x =  GetChildren();
	int y = GetChildrenNum();
	if (!fcmp)
		qsort(x,y,sizeof(SM_XMLElement*),SM_XMLElementfcmp);
	else
		qsort(x,y,sizeof(SM_XMLElement*),fcmp);
	}

void SM_XMLElement :: SortVariables(int (_USERENTRY *fcmp)(const void *, const void *))
	{
	// to all Variables
	SM_XMLVariable** x =  GetVariables();
	int y = GetVariableNum();
	if (!fcmp)
		qsort(x,y,sizeof(SM_XMLVariable*),SM_XMLVariablefcmp);
	else
		qsort(x,y,sizeof(SM_XMLVariable*),fcmp);
	}


// Memory usage funcs
size_t SM_XML :: MemoryUsage()
	{

	return GetRootElement()->MemoryUsage() + GetHeader()->MemoryUsage();

	}

void SM_XML :: CompressMemory()
	{

	GetRootElement()->CompressMemory();
	GetHeader()->CompressMemory();

	}

bool SM_XML :: IntegrityTest()
	{

	if (!GetHeader() || !GetRootElement())
		return false;
	return (GetHeader()->IntegrityTest() && root && GetRootElement()->IntegrityTest());

	}

int SM_XML :: Compare(SM_XML*x)
	{
	// 2 SM_XML = equals if headers & root elements compare ok

	int a1 = (GetRootElement()->Compare(x->GetRootElement()));
	int a2 = (GetHeader()->Compare(x->GetHeader()));

	return !(a1 == 0 && a2 == 0);
	}

size_t SM_XMLHeader :: MemoryUsage()
	{
	size_t m = 0;

	// Our size
	m += sizeof(*this);

	// Comments

	for(unsigned int i = 0 ; i < commentsnum ; i++)
		{
		m += GetComments()[i]->MemoryUsage();
		}
	// number of comment pointers
	m += TotalCommentPointersAvailable*4;



	// Text

	if (hdr)
		m += strlen(hdr);


	return m;
	}

void SM_XMLHeader :: CompressMemory()
	{
	// Remove wasted space by comments

	int P = commentsnum;
	if (P == 0)
		P = 1;
	SM_XMLComment** oldp = new SM_XMLComment*[P];
	if (commentsnum)
		memcpy(oldp,comments,commentsnum*sizeof(SM_XMLComment*));

	TotalCommentPointersAvailable = P;
	delete[] comments;
	comments = oldp;

	}

bool SM_XMLHeader :: IntegrityTest()
	{

	if (!hdr)
		return false;
#ifdef _WIN32
#ifndef WINCE
	if (IsBadStringPtrA(hdr,-1))
		return false;

	// Comments pointer
	if (IsBadReadPtr(comments,sizeof(SM_XMLComment*)*commentsnum))
		return false;
#endif


	// Check comment
	for(unsigned int i = 0 ; i < commentsnum ; i++)
		{
#ifdef _WIN32
#ifndef WINCE
		if (IsBadReadPtr(GetComments()[i],sizeof(SM_XMLComment*)))
			return false;
#endif
#endif
		if (!GetComments()[i]->IntegrityTest())
			return false;
		}
	return true;
#endif
	}



int SM_XMLHeader :: Compare(SM_XMLHeader* x)
	{
	// 2 Headers compare ok <=> Same text, same # comments, comments compare ok
	if (strcmp(hdr,x->hdr) != 0)
		return 1; // fail header

	unsigned int Y = GetCommentsNum();
	if (Y != x->GetCommentsNum())
		return 1;// differnet comment num

	for(unsigned int i = 0 ; i < Y ; i++)
		{
		if (GetComments()[i]->Compare(x->GetComments()[i]) == 1)
			return 1; // different comment
		}
	return 0; // OK!
	}


size_t SM_XMLComment :: MemoryUsage()
	{
	size_t m = 0;

	// Our size
	m += sizeof(*this);

	// Comment size

	if (c)
		m += strlen(c);


	return m;
	}

void SM_XMLComment :: CompressMemory()
	{
	}

bool SM_XMLComment :: IntegrityTest()
	{

	// check parent,c

	if (!c)
		return false;
#ifdef _WIN32
#ifndef WINCE
	if (IsBadStringPtrA(c,-1))
		return false;
#endif
#endif

	if (parent)
		{
		// Check pointer
#ifdef _WIN32
#ifndef WINCE
		if (IsBadReadPtr(parent,sizeof(SM_XMLElement*)))
			return false;
#endif
#endif
		}

	return true;

	}


int SM_XMLComment :: Compare(SM_XMLComment* x)
	{
	// Compare OK <=> Same Text
	if (strcmp(c,x->c) != 0)
		return 1;


	return 0;
	}


SM_XMLComment* SM_XMLComment :: Duplicate()
	{
	// returns a copy of myself

	return new SM_XMLComment(parent,ep,c);

	}


size_t SM_XMLContent :: MemoryUsage()
	{
	size_t m = 0;

	// Our size
	m += sizeof(*this);

	// Comment size

	if (c)
		m += strlen(c);

	return m;
	}

void SM_XMLContent :: CompressMemory()
	{
	}

bool SM_XMLContent :: IntegrityTest()
	{

	// check parent,c

	if (!c)
		return false;
#ifdef _WIN32
#ifndef WINCE
	if (IsBadStringPtrA(c,-1))
		return false;
#endif
#endif

	if (parent)
		{
#ifdef _WIN32
#ifndef WINCE
		if (IsBadReadPtr(parent,sizeof(SM_XMLElement*)))
			return false;
#endif
#endif
		}

	return true;

	}


int SM_XMLContent :: Compare(SM_XMLContent* x)

	{
	// Contents OK <=> Same text

	if (strcmp(c,x->c) != 0)
		return 1;

	return 0;
	}

SM_XMLContent* SM_XMLContent :: Duplicate()
	{
	// returns a copy of myself

	size_t s2 = GetValue(0);
	Z<char> x2(s2 + 100);
	GetValue(x2);

	return new SM_XMLContent(parent,_ep,x2);
	}


size_t SM_XMLCData :: MemoryUsage()
	{
	size_t m = 0;

	// Our size
	m += sizeof(*this);

	// CData size

	if (c)
		m += strlen(c);


	return m;
	}

void SM_XMLCData :: CompressMemory()
	{
	}

bool SM_XMLCData :: IntegrityTest()
	{

	// check parent,c

	if (!c)
		return false;
#ifdef _WIN32
#ifndef WINCE
	if (IsBadStringPtrA(c,-1))
		return false;
#endif
#endif

	if (parent)
		{
		// Check pointer
#ifdef _WIN32
#ifndef WINCE
		if (IsBadReadPtr(parent,sizeof(SM_XMLElement*)))
			return false;
#endif
#endif
		}

	return true;

	}


int SM_XMLCData :: Compare(SM_XMLCData* x)

	{
	// Compare OK <=> Same Text

	if (strcmp(c,x->c) != 0)
		return 1;

	return 0;
	}


SM_XMLCData* SM_XMLCData :: Duplicate()
	{
	// returns a copy of myself

	return new SM_XMLCData(parent,ep,c);

	}





size_t SM_XMLVariable :: MemoryUsage()
	{
	size_t m = 0;

	// Our size
	m += sizeof(*this);

	// Variable size
	m += GetName(0);
	m += GetValue(0);

	return m;
	}

void SM_XMLVariable :: CompressMemory()
	{
	}

bool SM_XMLVariable :: IntegrityTest()
	{

	// check vv,vn,owner
// 	if (!_vn || !_vv)
// 		return false;
#ifdef _WIN32
#ifndef WINCE
// 	if (IsBadStringPtrA(_vn,-1))
// 		return false;
// 	if (IsBadStringPtrA(_vv,-1))
// 		return false;
#endif
#endif
	if (owner)
		{
#ifdef _WIN32
#ifndef WINCE
		if (IsBadReadPtr(owner,sizeof(SM_XMLElement*)))
			return false;
#endif
#endif
		}
	return true;

	}


int SM_XMLVariable :: Compare(SM_XMLVariable* x)
	{

		if (_vnId != x->_vnId) {
			return 1;
		}

#ifdef USE_OLD_VV
	// Contents OK <=> Same value & name
// 	if (strcmp(_vn,x->_vn) != 0)
// 		return 1;
	size_t l1 = strlen(_vv);
	size_t l2 = strlen(x->_vv);
	if (l1 != l2)
		return 0;
	if (strncmp(_vv,x->_vv,l1) != 0)
		return 1;
	return 0;
#else
		size_t l1 = strlen(_idToName[_vvId]);
		size_t l2 = strlen(_idToName[x->_vvId]);
		if (l1 != l2)
			return 0;
		if (strncmp(_idToName[_vvId],_idToName[x->_vvId],l1) != 0)
			return 1;
		return 0;
#endif

	}


size_t SM_XMLElement :: MemoryUsage()
	{
	size_t m = 0;
	// returns # of bytes used by this element's data

	// Our size
	m += sizeof(*this);

	// Variables of this
	unsigned int i;

	for(i = 0 ; i < variablesnum ; i++)
		{
		m += GetVariables()[i]->MemoryUsage();
		}

	// Contents of this
	for(i = 0 ; i < contentsnum ; i++)
		{
		m += GetContents()[i]->MemoryUsage();
		}

	// Elements of this
	for(i = 0 ; i < childrennum ; i++)
		{
		if (GetChildren()[i])
			m += GetChildren()[i]->MemoryUsage();
		}

	// number of children pointers
	m += TotalChildPointersAvailable*4;

	// number of variable pointers
	m += TotalVariablePointersAvailable*4;

	// number of content pointers
	m += TotalContentPointersAvailable*4;


	// Element name
	m += GetElementName(0);
	return m;
	}

void SM_XMLElement :: CompressMemory()
	{

		{
		// Remove wasted space by variables
		int PV = variablesnum;
		if (PV == 0)
			PV = 1;

		SM_XMLVariable** oldpv = new SM_XMLVariable*[PV];
		if (variablesnum)
			memcpy(oldpv,variables,variablesnum*sizeof(SM_XMLVariable*));

		TotalVariablePointersAvailable = PV;
		delete[] variables;
		variables = oldpv;
		}

		{
		// Remove wasted space by children
		int PE = childrennum;
		if (PE == 0)
			PE = 1;

		SM_XMLElement** oldpv = new SM_XMLElement*[PE];
		if (childrennum)
			memcpy(oldpv,children,childrennum*sizeof(SM_XMLElement*));

		TotalChildPointersAvailable = PE;
		delete[] children;
		children = oldpv;
		}

		unsigned int i;

		// Do the same for all Contents
		for(i = 0 ; i < contentsnum ; i++)
			{
			_contents[i]->CompressMemory();
			}

		// Do the same for all Variables
		for(i = 0 ; i < variablesnum ; i++)
			{
			variables[i]->CompressMemory();
			}

		// Do the same for all child elements
		for(i = 0 ; i < childrennum ; i++)
			{
			if (children[i])
				children[i]->CompressMemory();
			}

	}

bool SM_XMLElement :: IntegrityTest()
	{

	// The main meat IntegrityTest

	/*

	Check

	name
	parent
	childen
	contents
	variables
	comments

	char* el; // element name
	SM_XMLElement* parent; // one
	SM_XMLElement** children; // many
	SM_XMLVariable** variables; // many
	SM_XMLComment** comments; // many
	SM_XMLContent** contents; // many;

	*/

#ifdef _WIN32
#ifndef WINCE
	// parent pointer
	if (parent && IsBadReadPtr(parent,sizeof(SM_XMLElement*)))
		return false;
// 	if (IsBadStringPtrA(_el,-1))
// 		return false;
#endif
#endif


#ifdef _WIN32
#ifndef WINCE
	// Comments pointer
// 	if (IsBadReadPtr(comments,sizeof(SM_XMLComment*)*commentsnum))
// 		return false;
#endif
#endif

#ifdef _WIN32
#ifndef WINCE
	// Contents pointer
	if (IsBadReadPtr(_contents,sizeof(SM_XMLContent*)*contentsnum))
		return false;
#endif
#endif

	unsigned int i;
	// Check content
	for(i = 0 ; i < contentsnum ; i++)
		{
#ifdef _WIN32
#ifndef WINCE
		if (IsBadReadPtr(GetContents()[i],sizeof(SM_XMLContent*)))
			return false;
#endif
#endif
		if (!GetContents()[i]->IntegrityTest())
			return false;
		}


#ifdef _WIN32
#ifndef WINCE
	// Variables pointer
	if (IsBadReadPtr(variables,sizeof(SM_XMLVariable*)*variablesnum))
		return false;
#endif
#endif

	// Check comment
	for(i = 0 ; i < variablesnum ; i++)
		{
#ifdef _WIN32
#ifndef WINCE
		if (IsBadReadPtr(GetVariables()[i],sizeof(SM_XMLVariable*)))
			return false;
#endif
#endif
		if (!GetVariables()[i]->IntegrityTest())
			return false;
		}

#ifdef _WIN32
#ifndef WINCE
	// Children pointer
	if (IsBadReadPtr(children,sizeof(SM_XMLElement*)*childrennum))
		return false;
#endif
#endif

	// Check children
	for(i = 0 ; i < childrennum ; i++)
		{
#ifdef _WIN32
#ifndef WINCE
		if (IsBadReadPtr(GetChildren()[i],sizeof(SM_XMLElement*)))
			return false;
#endif
#endif
		if (!GetChildren()[i]->IntegrityTest())
			return false;
		}



	return true;

	}



int SM_XMLElement :: Compare(SM_XMLElement* x)
	{
	/*
	SM_XMLElements match if

	Have same element name

	Have same # of variables,and they match
	Have same # of comments, and they match
	Have same # of contents, and they match
	Have same # of children, and they match
	*/

	if (_elId != x->_elId) 
		return 1;

	// Test element name
// 	if (strcmp(_el,x->_el) != 0)
// 		return 1;

	// Test Variables
	unsigned int nV = GetVariableNum();
	if (nV != x->GetVariableNum())
		return 1;

	unsigned int i;
	for(i = 0 ; i < nV ; i++)
		{
		if (GetVariables()[i]->Compare(x->GetVariables()[i]) != 0)
			return 1;
		}

	// Test Comments
	unsigned int nC = GetCommentsNum();
	if (nC != x->GetCommentsNum())
		return 1;
	for(i = 0 ; i < nC ; i++)
		{
		if (GetComments()[i]->Compare(x->GetComments()[i]) != 0)
			return 1;
		}

	// Test CDatas
	unsigned int nD = GetCDatasNum();
	if (nD != x->GetCDatasNum())
		return 1;
	for(i = 0 ; i < nD ; i++)
		{
		if (GetCDatas()[i]->Compare(x->GetCDatas()[i]) != 0)
			return 1;
		}

	// Test Contents
	unsigned int nT = GetContentsNum();
	if (nT != x->GetContentsNum())
		return 1;
	for(i = 0 ; i < nT ; i++)
		{
		if (GetContents()[i]->Compare(x->GetContents()[i]) != 0)
			return 1;
		}

	// Test Children Elements
	unsigned int nE = GetChildrenNum();
	if (nE != x->GetChildrenNum())
		return 1;
	for(i = 0 ; i < nE ; i++)
		{
		if (!GetChildren()[i] || !x->GetChildren()[i])
			continue;
		if (GetChildren()[i]->Compare(x->GetChildren()[i]) != 0)
			return 1;
		}

	return 0; // MATCH!
	}


void SM_XMLElement :: Copy()
	{
	// Copies this element to clipboard as a text
#ifdef _WIN32
	size_t M = MemoryUsage();
	Z<char> d(M);
	Export((FILE*)d.operator char *(),1,SM_XML_SAVE_MODE_DEFAULT,SM_XML_TARGET_MODE_MEMORY);
	size_t S = strlen(d);

	OpenClipboard(0);
	EmptyClipboard();

	HGLOBAL hG =
		GlobalAlloc(GMEM_MOVEABLE, S + 10);
	void *pp = GlobalLock(hG);
	//lstrcpyA((char *)pp, d.operator char *());
	strcpy((char*)pp,d.operator char *());
	GlobalUnlock(hG);
	SetClipboardData(CF_TEXT, hG);
	CloseClipboard();
#endif
	}

SM_XMLElement* SM_XML :: Paste(char* txt)
	{
	if (txt)
		{
		SM_XML* xm = new SM_XML();
		xm->Load(txt,SM_XML_LOAD_MODE_MEMORY_BUFFER,0,0);
		int K = xm->ParseStatus();
		if (K == 2) // Fatal error
			{
			delete xm;
			return 0;
			}


		if (xm->GetRootElement() == 0)
			{
			delete xm;
			return 0;
			}


		SM_XMLElement* r = xm->GetRootElement()->Duplicate(0);

		delete xm;
		return r;
		}

#ifdef _WIN32
	OpenClipboard(0);

	HGLOBAL hG =
		GetClipboardData(CF_TEXT);
	if (!hG)
		{
		CloseClipboard();
		return 0;
		}


	void *pp = GlobalLock(hG);
	size_t S = strlen((char*)pp);
	Z<char> d(S + 100);
	strcpy(d,(char*)pp);

	GlobalUnlock(hG);
	CloseClipboard();

	// d has the data, size S
	SM_XML* xm = new SM_XML();
	xm->Load(d,SM_XML_LOAD_MODE_MEMORY_BUFFER,0,0);
	int K = xm->ParseStatus();
	if (K == 2) // Fatal error
		{
		delete xm;
		return 0;
		}

	SM_XMLElement* r = xm->GetRootElement()->Duplicate(0);

	delete xm;
	return r;
#else
	return 0;
#endif
	}

SM_XMLElement* SM_XMLElement :: Duplicate(SM_XMLElement* par)
	{
	// Creates a new SM_XML element, excact copy of myself
	/*
	Formula
	dup all variables for this element
	dup all contents  for this element
	dup all comments  for this element
	dup all cdatas    for this element
	dup all elements  in a loop

	*/


	ReloadAllElements();

	size_t z1 = GetElementName(0);
	Z<char> en(z1 + 10);
	GetElementName(en);

	SM_XMLElement* nX = new SM_XMLElement(par,en);

	// Add All Variables
	int y = GetVariableNum();
	int i;
	for(i = 0 ; i < y ; i++)
		{
		nX->AddVariable(GetVariables()[i]->Duplicate());
		}

	// Add All Contents
	y = GetContentsNum();
	for(i = 0 ; i < y ; i++)
		{
		nX->AddContent(GetContents()[i]->Duplicate(),GetContents()[i]->GetEP());
		}

	// Add All Comments
	y = GetCommentsNum();
	for(i = 0 ; i < y ; i++)
		{
		nX->AddComment(GetComments()[i]->Duplicate(),GetComments()[i]->GetEP());
		}

	// Add All Cdatas
	y = GetCDatasNum();
	for(i = 0 ; i < y ; i++)
		{
		nX->AddCData(GetCDatas()[i]->Duplicate(),GetCDatas()[i]->GetEP());
		}

	// Recurse to add all child elements
	int c = GetChildrenNum();
	for(i = 0 ; i < c ; i++)
		{
		nX->AddElement(GetChildren()[i]->Duplicate(nX));
		}

	return nX;

	}

void SM_XML :: SaveOnClose(bool S)
	{
	SOnClose = S;
	}   

int SM_XML :: Save(const char* file,SM_XML_SAVE_MODE SaveMode,SM_XML_TARGET_MODE TargetMode,class SM_XMLTransform* eclass,class SM_XMLTransformData* edata)
	{
	if (TargetMode == 1)
		{
		if (!file)
			return 0;

		// TargetMode == 1, save to memory buffer

		Export((FILE*)file,SaveMode,SM_XML_TARGET_MODE_MEMORY,hdr,eclass,edata);

		return 1;
		}
	if (TargetMode == 2)
		{
		return 0; // We can't save to registry from SM_XML :: Save.
		}



	if (!file)

		file = f;

	if (!file)
		return 0;

	// write this file
	// Header, and all elements
	FILE* fp = 0;
#ifndef LINUX
	if (IsFileU)
		fp = _wfopen((wchar_t*)file,L"wb");
	else
#endif
		fp = fopen(file,"wb");

	if (!fp)
		return 0;

	if (TargetMode == 3)
		{
		// Write BOM
		fwrite("\xFF\xFE",1,2,fp); 

		// Hdr utf-16

		if (hdr)
			hdr->SetEncoding("UTF-16");

		}
	if (TargetMode == 0)
		{

		if (hdr)
			hdr->SetEncoding("UTF-8");

		}


	// Show

	Export(fp,SaveMode,TargetMode,hdr,eclass,edata);


	fclose(fp);
	return 1;
	}

void SM_XMLElement :: SetElementName(const char* x,const wchar_t* wx)
	{
	SM_XMLU wh(wx);
	if (!x)
		x = wh.bc();


	char *el = 0;
	size_t Sug = SM_XML :: SM_XMLEncode(x,0);
	el = new char[Sug +10];
	memset(el,0,Sug + 10);
	SM_XML :: SM_XMLEncode(x,el);

	CString newName = el;
	delete [] el;

	mapNameToId(newName, _elId);

// 	if (_el)
// 		delete[] _el;
// 	_el = 0;
// 	size_t Sug = SM_XML :: SM_XMLEncode(x,0);
// 	_el = new char[Sug +10];
// 	memset(_el,0,Sug + 10);
// 	SM_XML :: SM_XMLEncode(x,_el);
	}

size_t SM_XMLElement :: GetElementName(char* x,int NoDecode)
	{

	CString elemName = _idToName[_elId];

	if (!x)
		{
		if (NoDecode)
			return strlen(elemName);
		else
			return SM_XML :: SM_XMLDecode(elemName,0);
		}

	if (NoDecode)
		strcpy(x,elemName);
	else
		SM_XML :: SM_XMLDecode(elemName,x);
	return strlen(x);

	}

size_t SM_XMLElement :: GetElementFullName(char* x,int NoDecode)
	{
	Z<char> fel(5000); // full element name store here
	if (parent == 0) // this is the root element
		{
		return 0;
		}
	else
		{
		parent->GetElementFullName(fel,NoDecode);
		if (strlen(fel))
			strcat(fel,"\\");

		CString elemName = _idToName[_elId];
		strcat(fel,elemName);

		}

	if (!x)
		{
		if (NoDecode)
			return strlen(fel);
		else
			return SM_XML :: SM_XMLDecode(fel,0);
		}

	if (NoDecode)
		strcpy(x,fel);
	else
		SM_XML :: SM_XMLDecode(fel,x);
	return strlen(x);
	}

size_t SM_XMLElement :: GetElementUniqueString(char* x)
	{
	int d = GetDeep();
	if (!x)
		return (d*4) + 10;

	if (parent)
		parent->GetElementUniqueString(x);

	// strcat to x our position
	if (parent)
		{
		int iid = parent->FindElement(this);
		sprintf(x + strlen(x),"%i-",iid);
		}
	return strlen(x);
	}

int SM_XMLElement :: GetType()
	{
	//return _type;
	return 0;
	}

int SM_XMLElement :: FindElement(SM_XMLElement* x)
	{

	for(unsigned int i = 0 ; i < childrennum ; i++)
		{
		if (children[i] == x)
			return i;
		}

	return -1;
	}

int SM_XMLElement :: FindElement(const char* n)
	{

	for(unsigned int i = 0 ; i < childrennum ; i++)

		{

		if (!children[i])
			continue;
		SM_XMLElement* cc = children[i];

		size_t Sug = cc->GetElementName(0);
		Z<char> Name(Sug + 10);
		cc->GetElementName(Name);
		if (strcmp(Name,n) == 0)
			return i;
		}
	return -1;
	}

SM_XMLElement* SM_XMLElement:: FindElementZ(SM_XMLElement* x)
	{

	for(unsigned int i = 0 ; i < childrennum ; i++)
		{
		if (children[i] == x)
			return children[i];
		}
	return 0;

	}

SM_XMLElement* SM_XMLElement:: FindElementZ(const char* n,bool ForceCreate,char* el,bool Temp)
	{

	for(unsigned int i = 0 ; i < childrennum ; i++)

		{

		if (!children[i])
			continue;
		SM_XMLElement* cc = children[i];

		size_t Sug = cc->GetElementName(0);
		Z<char> Name(Sug + 10);
		cc->GetElementName(Name);
		if (strcmp(Name,n) == 0)
			return cc;
		}
	if (ForceCreate == 0)
		return 0;

	// Create New Element and add
	// Force to create a new element

	SM_XMLElement* vv = new SM_XMLElement(this,el ? el : n,0,Temp);
	AddElement(vv);
	return FindElementZ(vv);

	}

int SM_XMLElement :: FindVariable(SM_XMLVariable* x)
	{

	for(unsigned int i = 0 ; i < variablesnum ; i++)
		{
		if (variables[i] == x)
			return i;
		}

	return -1;
	}

SM_XMLVariable* SM_XMLElement :: FindVariableZ(SM_XMLVariable* x)
	{

	for(unsigned int i = 0 ; i < variablesnum ; i++)
		{
		if (variables[i] == x)
			return variables[i];
		}

	return 0;
	}

int SM_XMLElement :: FindVariable(const char*  x)
	{

	for(unsigned int i = 0 ; i < variablesnum ; i++)

		{

		SM_XMLVariable* V = variables[i];


		size_t Sug = V->GetName(0);
		Z<char> Name(Sug + 10);
		V->GetName(Name);
		if (strcmp(Name,x) == 0)
			return i;
		}
	return -1;
	}

SM_XMLVariable* SM_XMLElement :: FindVariableZ(const char*  x,bool ForceCreate,char* defnew,bool Temp)
	{

	for(unsigned int i = 0 ; i < variablesnum ; i++)
		{
		SM_XMLVariable* V = variables[i];

		size_t Sug = V->GetName(0);
		Z<char> Name(Sug + 10);
		V->GetName(Name);
		if (strcmp(Name,x) == 0)
			return V;
		}
	if (ForceCreate == 0)
		return 0;

	// Force to create a new variable

	SM_XMLVariable* vv = new SM_XMLVariable(x,defnew,0,Temp);
	AddVariable(vv);
	return FindVariableZ(x,0);

	}


int SM_XML :: PhantomElement(FILE* fp,SM_XMLElement* r,unsigned __int64 StartP,unsigned __int64 EndP)
	{
/*	Z<char> t(10000);
	Z<char> t2(10000);
	Z<char> eln(10000);
	if (!r)
		r = GetRootElement();

	// Save Starting and Ending Positions
	sprintf(t,"%I64u",StartP);
	r->AddVariable("f",t);
	sprintf(t,"%I64u",EndP);
	r->AddVariable("t",t);

	// Put to start 
	fseek(fp,StartP,SEEK_SET);

	for(;;)
		{
		// Read some
		t._clear();
		fread(t,1,10000,fp);

		char* a1 = strchr(t,'<');
		if (!a1)
			break; // error

		unsigned __int64 NewElementStart = (a1 - t) + StartP;

		// It is an element, get its name
		t2._clear();
		for(;;)
			{
			if (*a1 == ' ' || *a1 == '\t' || *a1 == '\n' || *a1 == '\r')
				break;
			strncat(t2,a1,1);
			a1++;
			}
		strcpy(eln,t2);

		// Find the end of that element, 
		char* a2 = strchr(a1,'>');
		if (!a2)
			{
			// We must read more data as long as we find the '>'

			}

		if (*(a2 - 1) != '/')
			{
			// This element we have found has children, so we must find </eln>


			}
		else
			{
			// This element we have found ends here, no children 
			unsigned __int64 NewElementEnd = (a2 - t) + StartP;
			SM_XMLElement* ne = new SM_XMLElement(r,eln);
			r->AddElement(ne);
			sprintf(t,"%I64u",NewElementStart);
			ne->AddVariable("f",t);
			sprintf(t,"%I64u",NewElementEnd);
			ne->AddVariable("t",t);

			// Seek 
			fseek(fp,NewElementEnd + 1,SEEK_SET);
			}
		}
*/
	return 0;
	}

#ifdef LINUX
int SM_XML :: 	PhantomLoad(const char* file)
#else
int SM_XML :: 	PhantomLoad(const char* file,bool IsUnicode,bool UseMap)
#endif
	{
/*
#ifdef LINUX
	FILE* fp = fopen(file,"rb");
#else
	FILE* fp = IsUnicode ? _wfopen((const wchar_t*)file,L"rb") : fopen(file,"rb");
#endif
	if (!fp)
		return 0;

	unsigned __int64 fsz = 0;
	fseek(fp,0,SEEK_END);
	fsz = ftell(fp);
	fseek(fp,0,SEEK_SET);

	Z<char> tmp(1000);

	// Bypass Header
	fread(tmp,1,1000,fp);
	if (strncmp(tmp,"<?xml",5) == 0)
		{
		char* a1 = strchr(tmp,'>');
		if (!a1) // duhs ?
			{
			fclose(fp);
			return 0;
			}
		a1++;
		fseek(fp,a1 - tmp,SEEK_SET);
		}
	else
		fseek(fp,0,SEEK_SET); // headerless
	unsigned __int64 fst = ftell(fp);

	PhantomElement(fp,0,fst,fsz);
	fclose(fp);
*/
	return 1;
	}


int SM_XML :: PartialLoad(const char* file,const char* map)
	{
	if (!map)
		return 0;
	/*
	FILE* fp = fopen(file,"rb");
	if (!fp)
	return 0;

	// Read 1MB portions of the file
	Z<char> bx(1048580);
	char* b = bx;
	int CurrentDeep = 0;

	unsigned long PositionStarting = -1;
	unsigned long PositionEnding = -1;

	for(;;)
	{
	int Y = fread(b,1,1048576,fp);
	if (Y == 0)
	break;

	bool IsQuote = false;
	bool IsHdr = false;
	bool IsClosingWithSlash = false;

	// Find < and /> or >
	for(int i = 0 ; i < Y ; i++)
	{
	if (b[i] == '<' && IsQuote == false)
	{
	if (b[i + 1] == '?')
	IsHdr = true;
	else
	if (b[i + 1] == '/')
	{
	IsClosingWithSlash = true;
	continue;
	}
	else
	{
	CurrentDeep++;
	if (CurrentDeep == d->deep && PositionStarting  == -1)
	PositionStarting = i;
	IsHdr = false;
	}
	}

	if (b[i] == '\"')
	IsQuote = !IsQuote;

	if (b[i] == '>' && b[i - 1] == '/')
	{
	CurrentDeep--;
	if (CurrentDeep < d->deep && PositionEnding == -1)
	{
	PositionEnding = i;
	break; // found positions !
	}
	}

	if (b[i] == '>' && IsClosingWithSlash)
	{
	CurrentDeep--;
	if (CurrentDeep < d->deep && PositionEnding == -1)
	{
	PositionEnding = i;
	break;
	}
	IsClosingWithSlash = false;
	}

	if (b[i] == '>' && IsHdr)
	{
	IsHdr = false;
	}

	if (CurrentDeep < 0)
	CurrentDeep = 0;

	}
	}

	// PositionStarting,PositionEnding are found (else error)
	if (PositionStarting == -1 || PositionEnding == -1)
	return 0; // Load failed


	// Time to parse "map"
	// Which is something like "el1\\el2\\el3" ...
	// Named elements to search

	*/








	// fclose(fp);
	return 0;// 
	}

SM_XMLElement * SM_XML :: PartialElement(const char* file,const char* map)
	{
	SM_XML x;

	if (x.PartialLoad(file,map) == 1)
		return x.GetRootElement()->Duplicate();

	return 0;
	}


int SM_XML :: Load(const char* file,SM_XML_LOAD_MODE LoadMode,SM_XMLTransform* eclass,class SM_XMLTransformData* edata)
	{
	Clear();
	Z<char>* y = 0;
	iParseStatus = SM_XML_PARSE_OK;
#ifndef LINUX
	IsFileU = false;
#endif

	if (LoadMode == SM_XML_LOAD_MODE_LOCAL_FILE) // local xml file
		{

		f = new char[strlen(file) + 1];
		strcpy(f,file);

		// parse this file
#ifndef LINUX
		IsFileU = false;
#endif
		y = ReadToZ(file,eclass,edata);
		if (!y)
			{
			// It is an empty SM_XML file.
			// Create the initial data/header


			hdr = new SM_XMLHeader("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>");
			root = new SM_XMLElement(0,"root",0);


			return 1;
			}
		}
#ifndef LINUX
	else
		if (LoadMode == SM_XML_LOAD_MODE_LOCAL_FILE_U) // local xml file unicode
			{
			wchar_t* wf = (wchar_t*)file;

			f = new char[(wcslen(wf)*2) + 2];
			memset(f,0,wcslen(wf)*2 + 2);
			IsFileU = true;
			memcpy(f,file,wcslen(wf)*2);


			// parse this file
			y = ReadToZ(file,eclass,edata,1);
			if (!y)
				{
				// It is an empty SM_XML file.
				// Create the initial data/header


				hdr = new SM_XMLHeader("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>");
				root = new SM_XMLElement(0,"root",0);

				return 1;
				}
			}
#endif
		else
			if (LoadMode == SM_XML_LOAD_MODE_MEMORY_BUFFER) // memory buffer
				{

				f = 0;

				if (!file || strlen(file) == 0)
					{
					// It is an empty SM_XML file.
					// Create the initial data/header


					hdr = new SM_XMLHeader("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>");
					root = new SM_XMLElement(0,"root",0);


					return 1;
					}
				else
					{
					size_t S = strlen(file) + 100;
					y = new Z<char>(S);
					strcpy(y->operator char *(),file);
					}
				}
			else
				if (LoadMode == SM_XML_LOAD_MODE_URL) // url
					{

					f = 0;

#ifdef _WIN32
#ifndef __SYMBIAN32__
#ifdef SM_XML_OPTIONAL_WIN32
					HINTERNET hI = 0,hRead = 0;
					hI = InternetOpen(_T("SM_XML Library"),INTERNET_OPEN_TYPE_PRECONFIG,0,0,0);
					Z<char> fx(1000);
					GetTempFileNameA(".","xml",0,fx);
					if (hI)
						{
						hRead = InternetOpenUrlA(hI,file,0,0,0,0);
						if (hRead)
							{
							// Get this file
							Z<char> Buff(1010);

							int err = 0;
							unsigned int TotalTransferred = 0;

							for(;;)
								{
								DWORD n;

								BOOL F = InternetReadFile(hRead,Buff,1000,&n);
								if (F == false)
									{
									err = 2;
									break;
									}
								if (n == 0)
									{
									// End of file !
									err = 0;
									break;
									}
								TotalTransferred += n;

								HANDLE hF = CreateFileA(fx,GENERIC_WRITE,0,0,OPEN_ALWAYS,0,0);
								SetFilePointer(hF,0,0,FILE_END);
								DWORD Actual = 0;
								WriteFile(hF,Buff,n,&Actual,0);
								FlushFileBuffers(hF);
								CloseHandle(hF);
								}
							if (err == 0)
								{
								// read that file now
#ifdef SM_XML_USE_STL
								f = fx;
#else
								f = new char[strlen(fx) + 1];
								strcpy(f,fx);
#endif
								// parse this file
								y = ReadToZ(fx,eclass,edata);
								}
							InternetCloseHandle(hRead);
							}
						InternetCloseHandle(hI);
						remove(fx);
						if (!y)
							{
							//It is an empty SM_XML file.
							// Create the initial data/header
#ifdef SM_XML_USE_STL
							hdr.GetHeader() = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>";
#else
							hdr = new SM_XMLHeader("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>");
							root = new SM_XMLElement(0,"root",0);
#endif
							return 1;
							}
						}
#endif
#endif
#endif
					}

				// Read file in y and create all SM_XML data
				char* d = (*y).operator char*();

				// read hdr
				char* a2 = 0;
				char c1;
				char* a1 = strstr(d,"?>");
				if (!a1)
					{

					if (f)
						delete[] f;
					f = 0;

					iParseStatus = SM_XML_PARSE_NO_HEADER;

					hdr = new SM_XMLHeader("\xEF\xBB\xBF<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>");

					//      root = new SM_XMLElement(0,"root",0);
					//      delete y;
					//      return 1;
					a1 = d;
					a2 = a1;
					}
				else
					{
					a1 += 2;
					c1 = *a1;
					*a1 = 0;

					hdr = new SM_XMLHeader(d);

					*a1 = c1;
					a2 = a1;
					}

				if (eclass)
					{
					// Delete f if was an encrypted opening

					if (f)
						delete[] f;
					f = 0;

					}

				/*
				Parse each < >
				Possible values

				<!-- -->   comment
				<>         element
				</>        end element
				<? ?>      markup

				*/


				a1 = strchr(a2,'<');


				if (a1)
					root = SM_XMLHelper :: ParseElementTree(hdr,0,a1,0,iParseStatus);
				else
					{
					if (f)
						delete[] f;
					f = 0;
					iParseStatus = SM_XML_PARSE_NO_HEADER;
					root = new SM_XMLElement(0,"<root>");
					}

				// Print all elements of this

				delete y;
				return 1;
	}

SM_XML_PARSE_STATUS SM_XML :: ParseStatus(int* v)
	{
	if (v)
		*v = iParseStatusPos;
	return iParseStatus;
	}   

SM_XML :: ~SM_XML()
	{
	Clear();
	clearMaps();
	}

SM_XML :: SM_XML(SM_XML& xml)
	{
	Clear();
	Init();
	operator =(xml);
	}

SM_XML& SM_XML :: operator =(SM_XML& xml)
	{
	Clear();
	Init();

	hdr = xml.GetHeader()->Duplicate();
	root = xml.GetRootElement()->Duplicate();
	f = 0;

	iParseStatus = SM_XML_PARSE_OK;
	SOnClose = false;
	return *this;
	}


// SM_XMLElement class
void SM_XMLElement :: Reparse(const char* elm2,int Type)
	{
	RemoveAllVariables();
	RemoveAllElements();
	RemoveAllComments();

	Z<char> elm(strlen(elm2) + 1);
	strcpy(elm,elm2);

	if (Type == 1)
		{


		CString newName = elm;

		mapNameToId(newName, _elId);

// 		_el = new char[strlen(elm) + 1];
// 		strcpy(_el,elm);

		return;
		}

	char* xel = 0;
	xel = new char[strlen(elm) + 10];
	memset(xel,0,strlen(elm) + 10);

	int x = 0;
	int i = 0;
	for( ; ; i++)
		{
		if (elm[i] == '<')
			continue;
#ifdef ALLOW_SINGLE_QUOTE_VARIABLES
		if (elm[i] == ' ' || elm[i] == '/' ||  elm[i] == '\t' || elm[i] == '>' || elm[i] == '\"' || elm[i] == '\'' || elm[i] == 0)
			break;
#else
		if (elm[i] == ' ' || elm[i] == '/' ||  elm[i] == '\t' || elm[i] == '>' || elm[i] == '\"' || elm[i] == 0)
			break;
#endif
		xel[x++] = elm[i];
		}

	size_t Sug = SM_XML :: SM_XMLEncode(xel,0);
	char* ael = new char[Sug + 10];
	memset(ael,0,Sug +10);
	SM_XML :: SM_XMLEncode(xel,ael);

	delete[] xel;

	//_el = ael;

	CString newName = ael;

	delete [] ael;

	mapNameToId(newName, _elId);


	// must be variable ?
	char* a1 = (char*)elm.operator char *() + i;
	for(;;)
		{
		// seek vars
		while(*a1 == ' ' || *a1 == '\t' || *a1 == '\r' || *a1 == '\n')
			a1++;

		if (*a1 == '>' || *a1 == '/')
			return;

		char* a2 = strchr(a1,'=');
		if (!a2)
			return;

		*a2 = 0;
		char* vvn = a1;
		a1 = a2 + 1;
		// 0x132 fix for white space after =
#ifdef ALLOW_SINGLE_QUOTE_VARIABLES
		while(*a1 != '\"' && *a1 != '\'')
#else
		while(*a1 != '\"')
#endif
			{
			if (*a1 == 0) // oups
				return;
			a1++;
			}

		char VF = '\"';
#ifdef ALLOW_SINGLE_QUOTE_VARIABLES
		if (*a1 == '\"')
			a1++;
		if (*a1 == '\'')
			{
			a1++;
			VF = '\'';
			}
		char* a3 = strchr(a1,VF);
#else
		if (*a1 == '\"')
			a1++;
		char* a3 = strchr(a1,'\"');
#endif
		if (!a3)
			return;
		*a3 = 0;


		SM_XMLVariable* v = new SM_XMLVariable(vvn,a1,true);

		*a2 = '=';
		*a3 = VF;
		AddVariable(v);
		a1 = a3 + 1;
		}
	}

int SM_XMLElement :: GetDeep()
	{
	int d = 0;
	SM_XMLElement* t = this;
	while(t->GetParent() != 0)
		{
		t = t->GetParent();
		d++;
		}
	return d;
	}

SM_XMLElement :: SM_XMLElement(SM_XMLElement* par,const char* elm,int theType,bool Temp,const unsigned short* welm)
//SM_XMLElement :: SM_XMLElement(SM_XMLElement* par /*= 0*/,const char* elm /*= 0*/,int theType /*= 0*/,bool Temp /*= false*/,const unsigned short* welm /*= 0*/)
	{
		_elId = 0;

	SM_XMLU wh(welm);
	if (!elm)
		elm = wh.bc();

	// parent
	parent = par;

	// Temp
	//Temporal = Temp;

	// type
	//_type = Type;


	// children
// 	children = new SM_XMLElement*[SM_XML_MAX_INIT_CHILDREN];
// 	memset(children,0,sizeof(SM_XMLElement*)*SM_XML_MAX_INIT_CHILDREN);
// 	TotalChildPointersAvailable = SM_XML_MAX_INIT_CHILDREN;
// 	childrennum = 0;

	children = NULL;
	TotalChildPointersAvailable = 0;
	childrennum = 0;


	// variables
// 	variables = new SM_XMLVariable*[SM_XML_MAX_INIT_VARIABLES];
// 	memset(variables,0,sizeof(SM_XMLVariable*)*SM_XML_MAX_INIT_VARIABLES);
// 	TotalVariablePointersAvailable = SM_XML_MAX_INIT_VARIABLES;
// 	variablesnum = 0;

	variables = NULL;
	TotalVariablePointersAvailable = 0;
	variablesnum = 0;

	// contents
// 	_contents = new SM_XMLContent*[SM_XML_MAX_INIT_CONTENTS];
// 	memset(_contents,0,sizeof(SM_XMLContent*)*SM_XML_MAX_INIT_CONTENTS);
// 	TotalContentPointersAvailable = SM_XML_MAX_INIT_CONTENTS;
// 	contentsnum = 0;

	_contents = NULL;
	TotalContentPointersAvailable = 0;
	contentsnum = 0;

	// Borrowed Elements
//	NumBorrowedElements = 0;


	// Set default format
	xfformat.nId = 1;
	xfformat.UseSpace = false;
	xfformat.ElementsNoBreak = true;

	// param 0
	//_param = 0;

	if (elm)
		Reparse(elm,theType);
	else
		Reparse("<root />",theType);
	}


void SM_XMLElement :: SetTemporal(bool x)
	{
	//Temporal = x;
	}

bool SM_XMLElement :: IsTemporal()
	{
	return false;
	}

void SM_XMLElement :: Clear()
	{
	RemoveAllVariables();
	RemoveAllElements();
	RemoveAllComments();
	RemoveAllContents();
	RemoveAllCDatas();
	// element

// 	if (_el)
// 		delete[] _el;
// 	_el = 0;

	}

SM_XMLElement :: ~SM_XMLElement()
	{
	Clear();


	if (variables)
		delete[] variables;
	variables = 0;
	variablesnum = 0;

	if (children)
		delete[] children;
	children = 0;
	childrennum = 0;

	if (_contents)
		delete[] _contents;
	_contents = 0;
	contentsnum = 0;


	}



SM_XMLElement* SM_XMLElement :: GetParent()
	{
	return parent;
	}

void SM_XMLElement :: SetParent(SM_XMLElement* Parent)
	{
	parent = Parent;
	}


SM_XMLElement** SM_XMLElement :: GetChildren()
	{
	return children;
	}



SM_XMLElement* SM_XMLElement :: operator [](int i)
	{
	return GetChildren()[i];
	}


unsigned int SM_XMLElement :: GetChildrenNum()
	{

	return childrennum;

	}


SM_XMLVariable** SM_XMLElement :: GetVariables()

	{
	return variables;
	}

unsigned int SM_XMLElement :: GetVariableNum()
	{

	return variablesnum;

	}


int SM_XMLElement :: ReserveSpaceForElements(unsigned int i)
	{
	return SpaceForElement(i);
	}

int SM_XMLElement :: SpaceForElement(unsigned int i)
	{
	if ((TotalChildPointersAvailable - childrennum) >= i)
		return (TotalChildPointersAvailable - childrennum);

	// make more space
	Z<SM_XMLElement*> oldp(childrennum);
	memcpy(oldp,children,childrennum*sizeof(SM_XMLElement*));

	TotalChildPointersAvailable += SM_XML_MAX_INIT_CHILDREN;
	if ((TotalChildPointersAvailable - childrennum) < i)
		TotalChildPointersAvailable = childrennum + i + 10;

	delete[] children;
	children = new SM_XMLElement*[TotalChildPointersAvailable];
	memcpy(children,oldp,childrennum*sizeof(SM_XMLElement*));
	return (TotalChildPointersAvailable - childrennum);
	}


int SM_XMLElement :: SpaceForVariable(unsigned int i)
	{
	if ((TotalVariablePointersAvailable - variablesnum) >= i)
		return (TotalVariablePointersAvailable - variablesnum);

	Z<SM_XMLVariable*> oldp(variablesnum);
	memcpy(oldp,variables,variablesnum*sizeof(SM_XMLVariable*));

	TotalVariablePointersAvailable += SM_XML_MAX_INIT_VARIABLES;

	delete[] variables;
	variables = new SM_XMLVariable*[TotalVariablePointersAvailable];
	memcpy(variables,oldp,variablesnum*sizeof(SM_XMLVariable*));
	return (TotalVariablePointersAvailable - variablesnum);
	}

int SM_XMLElement :: SpaceForComment(unsigned int i)
	{
	return 0;
	}
int SM_XMLHeader :: SpaceForComment(unsigned int i)
	{
	if ((TotalCommentPointersAvailable - commentsnum) >= i)
		return (TotalCommentPointersAvailable - commentsnum);

	Z<SM_XMLComment*> oldp(commentsnum);
	memcpy(oldp,comments,commentsnum*sizeof(SM_XMLComment*));

	TotalCommentPointersAvailable += SM_XML_MAX_INIT_COMMENTS;

	delete[] comments;
	comments = new SM_XMLComment*[TotalCommentPointersAvailable];
	memcpy(comments,oldp,commentsnum*sizeof(SM_XMLComment*));
	return 0;
	}

int SM_XMLElement :: SpaceForCData(unsigned int i)
	{
	return 0;
	}


int SM_XMLElement :: SpaceForContent(unsigned int i)
	{
	if ((TotalContentPointersAvailable - contentsnum) >= i)
		return (TotalContentPointersAvailable - contentsnum);

	Z<char*> oldp(contentsnum);
	memcpy(oldp,_contents,contentsnum*sizeof(char*));

	TotalContentPointersAvailable += SM_XML_MAX_INIT_CONTENTS;

	delete[] _contents;
	_contents = new SM_XMLContent*[TotalContentPointersAvailable];
	memcpy(_contents,oldp,contentsnum*sizeof(SM_XMLContent*));
	return (TotalContentPointersAvailable - contentsnum);
	}


SM_XMLElement* SM_XMLElement :: AddElement(SM_XMLElement* child)
	{
	SpaceForElement(1);
	children[childrennum++] = child;
	child->SetParent(this);
	return child;
	}
SM_XMLElement* SM_XMLElement :: AddElement(const char* t,const wchar_t* wt)
	{
	SM_XMLElement* x = new SM_XMLElement(this,t,0,0,wt);
	return AddElement(x);
	}


int SM_XMLElement :: AddVariable(SM_XMLVariable* v)
	{
	SpaceForVariable(1);
	variables[variablesnum++] = v;
	v->SetOwnerElement(this);
	return variablesnum;
	}
int SM_XMLElement :: AddVariable(const char* vn,const char* vv,const wchar_t* w1,const wchar_t* w2)
	{
	SM_XMLVariable* x = new SM_XMLVariable(vn,vv,0,0,w1,w2);
	return AddVariable(x);
	}


#ifdef SM_XML_OPTIONAL_MIME

int SM_XMLElement :: AddBinaryVariable(const char* vn,const char* vv,int S)
	{
	SM_XMLVariable* x = new SM_XMLVariable(vn,"");
	Z<char> tmp(S + 1);
	memcpy(tmp,vv,S);
	x->SetBinaryValue(tmp,S);
	return AddVariable(x);
	}
#endif


int SM_XMLElement :: AddComment(SM_XMLComment* v,int InsertBeforeElement)
	{
	return 0;
	}
int SM_XMLElement :: AddComment(const char*t,int InsertBeforeElement,const wchar_t* wt)
	{
	SM_XMLComment* x = new SM_XMLComment(this,InsertBeforeElement,t,wt);
	return AddComment(x,InsertBeforeElement);
	}
unsigned int SM_XMLElement :: GetCommentsNum()
	{
	return 0;
	}

SM_XMLComment** SM_XMLElement :: GetComments()
	{
	return NULL;
	}


int SM_XMLElement :: RemoveAllComments()
	{
	return 0;
	}


int SM_XMLElement :: RemoveComment(unsigned int i)
	{

	return 0;
	}

int SM_XMLElement :: AddCData(SM_XMLCData* v,int InsertBeforeElement)
	{
	return 0;
	}
int SM_XMLElement :: AddCData(const char*t,int InsertBeforeElement,const wchar_t* wt)
	{
	SM_XMLCData* x = new SM_XMLCData(this,InsertBeforeElement,t,wt);
	return AddCData(x,InsertBeforeElement);
	}
unsigned int SM_XMLElement :: GetCDatasNum()
	{
	return 0;
	}
SM_XMLCData** SM_XMLElement :: GetCDatas()
	{
	return NULL;
	}
int SM_XMLElement :: RemoveAllCDatas()
	{
	return 0;
	}
int SM_XMLElement :: RemoveCData(unsigned int i)
	{
	return 0;
	}


int SM_XMLElement :: AddContent(SM_XMLContent* v,int InsertBeforeElement)
	{
	SpaceForContent(1);
	_contents[contentsnum++] = v;
	v->SetParent(this,InsertBeforeElement);
	return contentsnum;
	}

int SM_XMLElement :: AddContent(const char* t,int InsertBeforeElement,int BinarySize,const unsigned short* wt)
	{
	SM_XMLContent* x = new SM_XMLContent(this,InsertBeforeElement,t,0,BinarySize,wt);
	return AddContent(x,InsertBeforeElement);
	}

int SM_XMLElement :: RemoveContent(unsigned int i)
	{
	if (i >= contentsnum)
		return contentsnum;

	delete _contents[i];
	_contents[i] = 0;

	for(unsigned int k = i ; k < contentsnum ; k++)
		_contents[k] = _contents[k + 1];

	_contents[contentsnum - 1] = 0;
	return --contentsnum;
	}

void SM_XMLElement :: RemoveAllContents()
	{
	for(int i = contentsnum - 1 ; i >= 0 ; i--)
		{
		delete _contents[i];
		_contents[i] = 0;
		}
	contentsnum = 0;
	}

SM_XMLContent** SM_XMLElement :: GetContents()
	{
	return _contents;
	}

unsigned int SM_XMLElement :: GetContentsNum()
	{
	return contentsnum;
	}



unsigned int SM_XMLElement :: GetAllChildren(SM_XMLElement** x,unsigned int deep)
	{
	int C = 0;

	for(unsigned int i = 0 ; i < childrennum && deep != 0 ; i++)
		{
		if (!children[i])
			continue;
		SM_XMLElement* ch = children[i];

		C += ch->GetAllChildren(x + C,deep == 0xFFFFFFFF ? deep : (deep - 1));
		x[C++] = ch;
		}

	return C;
	}

unsigned int SM_XMLElement :: GetAllChildrenNum(unsigned int deep)
	{
	int C = 0;

	for(unsigned int i = 0 ; i < childrennum && deep != 0 ; i++)
		{
		if (!children[i])
			continue;
		C += children[i]->GetAllChildrenNum(deep == 0xFFFFFFFF ? deep : (deep - 1));
		}
	C += childrennum;

	return C;
	}


// SM_XMLComment class
SM_XMLComment :: SM_XMLComment(SM_XMLElement* p,int ElementPosition,const char* ht,const wchar_t* wht)
	{
	SM_XMLU wh(wht);
	if (!ht)
		ht = wh.bc();

	parent = p;
	ep = ElementPosition;

	if (!ht)
		ht = " ";
	c = new char[strlen(ht) + 1];
	strcpy(c,ht);

	}

SM_XMLComment :: SM_XMLComment(const SM_XMLComment& h)
	{
	operator =(h);
	}

SM_XMLComment& SM_XMLComment :: operator =(const SM_XMLComment& h)
	{

	if (c)
		delete[] c;
	c = 0;


	parent = h.parent;
	ep = h.GetEP();

	const char* ht = h.operator const char*();
	c = new char[strlen(ht) + 1];
	strcpy(c,ht);

	return *this;
	}


SM_XMLComment :: ~SM_XMLComment()
	{

	if (c)
		delete[] c;
	c = 0;

	}

SM_XMLComment :: operator const char*() const
	{

	return c;

	}


void SM_XMLComment :: SetComment(const char* ht)
	{

	if (c)
		delete[] c;
	c = new char[strlen(ht) + 1];
	strcpy(c,ht);

	}

void SM_XMLComment :: SetParent(SM_XMLElement* p,int epp)
	{
	parent = p;
	ep = epp;
	}

int SM_XMLComment :: GetEP() const
	{
	return ep;
	}


// SM_XMLCData class
SM_XMLCData :: SM_XMLCData(SM_XMLElement* p,int ElementPosition,const char* ht,const wchar_t* wht)
	{
	SM_XMLU wh(wht);
	if (!ht)
		ht = wh.bc();

	parent = p;
	ep = ElementPosition;

	c = new char[strlen(ht) + 1];
	strcpy(c,ht);

	}

SM_XMLCData :: SM_XMLCData(const SM_XMLCData& h)
	{
	operator =(h);
	}

SM_XMLCData& SM_XMLCData :: operator =(const SM_XMLCData& h)
	{

	if (c)
		delete[] c;
	c = 0;

	parent = h.parent;
	ep = h.GetEP();

	const char* ht = h.operator const char*();
	c = new char[strlen(ht) + 1];
	strcpy(c,ht);

	return *this;
	}


SM_XMLCData :: ~SM_XMLCData()
	{

	if (c)
		delete[] c;
	c = 0;

	}

SM_XMLCData :: operator const char*() const
	{

	return c;

	}


void SM_XMLCData :: SetCData(const char* ht)
	{

	if (c)
		delete[] c;
	size_t nht = strlen(ht);
	c = new char[nht + 1];
	strcpy(c,ht);

	}

void SM_XMLCData :: SetParent(SM_XMLElement* p,int epp)
	{
	parent = p;
	ep = epp;
	}

int SM_XMLCData :: GetEP() const 
	{
	return ep;
	}


// SM_XMLContent class
SM_XMLContent :: SM_XMLContent(SM_XMLElement* p,int ElementPosition,const char* ht,int NoDecode,int BinarySize,const wchar_t* wt)
	{
	SM_XMLU wh(wt);
	if (!ht)
		ht = wh.bc();

	parent = p;
	_ep = ElementPosition;

	c = 0;

	if (ht)
		SetValue(ht,NoDecode,BinarySize);
	}

SM_XMLContent :: SM_XMLContent(const SM_XMLContent& h)
	{
	operator =(h);
	}

SM_XMLContent& SM_XMLContent :: operator =(const SM_XMLContent& h)
	{

	if (c)
		delete[] c;
	c = 0;


	parent = h.parent;
	_ep = h.GetEP();

	size_t k = h.GetValue(0,true);
	Z<char> vv(k + 10);
	h.GetValue(vv,true);
	SetValue(vv,true);
	return *this;
	}


SM_XMLContent :: ~SM_XMLContent()
	{

	if (c)
		delete[] c;
	c = 0;

	}

bool SM_XMLContent :: GetBinaryValue(char**o,int* len)
	{
#ifdef SM_XML_OPTIONAL_MIME
	if (!o || !len)
		return false;

	const char* d = 0;
	int ll = 0;

	d = c;

	if (!d)
		return false;
	ll = (int)strlen(d);

	char* a = new char[ll*5 + 1000];
	memset(a,0,ll*5 + 1000);

	char *oo;
	MimeCoder<char*,char*>* e;
	e = new Base64Decoder<char*,char*>;
	oo = e->Filter(a, (char*)d, (char*)d + ll);
	oo = e->Finish(oo);
	*oo = 0;                  // Put a zero to the end
	delete e;

	*o = a;
	int newlen = (int)(oo - a);
	*len = newlen;
	return true;
#else
	return false;
#endif
	}

size_t SM_XMLContent :: GetValue(char* x,int NoDecode) const
	{
	if (!x)
		{

		if (NoDecode)
			return strlen(c);
		else
			return SM_XML :: SM_XMLDecode(c,0);

		}


	if (NoDecode)
		strcpy(x,c);
	else
		SM_XML :: SM_XMLDecode(c,x);

	return strlen(x);
	}


void SM_XMLContent :: SetValue(const char* VV,int NoDecode,int BinarySize)
	{
#ifdef SM_XML_OPTIONAL_MIME
	if (BinarySize)
		{
		// Add binary data, do base64

	
		// Sets value using MIME
		char* a = new char[BinarySize*5 + 1000];
		memset(a,0,BinarySize*5 + 1000);

		char *oo;
		MimeCoder<char*,char*>* e;
		e = new Base64Encoder<char*,char*>;
		oo = e->Filter(a, (char*)VV, (char*)VV + BinarySize);
		oo = e->Finish(oo);
		*oo = 0;                  // Put a zero to the end
		delete e;

		SetValue(a,true,0);
		delete[] a;
		return;
		}
#endif


	if (c)
		delete[] c;
	size_t Sug = SM_XML :: SM_XMLEncode(VV,0);

	c = new char[Sug + 10];
	memset(c,0,Sug + 10);

	if (NoDecode)
		strcpy(c,VV);
	else
		SM_XML :: SM_XMLEncode(VV,c);


	}

void SM_XMLContent :: SetParent(SM_XMLElement* p,int epp)
	{
	parent = p;
	_ep = epp;
	}

int SM_XMLContent :: GetEP() const
	{
	return _ep;
	}



// SM_XMLHeader class
SM_XMLHeader :: SM_XMLHeader(const char* ht,const unsigned short* wht)
	{
	SM_XMLU wh(wht);
	if (!ht)
		ht = wh.bc();


	if (ht)
		{
		hdr = new char[strlen(ht) + 1];
		strcpy(hdr,ht);
		}
	else
		{
		hdr = new char[100];
		memset(hdr,0,100);
		}


	// comments

	comments = new SM_XMLComment*[SM_XML_MAX_INIT_COMMENTS_HEADER];
	memset(comments,0,sizeof(SM_XMLComment*)*SM_XML_MAX_INIT_COMMENTS_HEADER);
	TotalCommentPointersAvailable = SM_XML_MAX_INIT_COMMENTS_HEADER;
	commentsnum = 0;

	}

SM_XMLHeader :: SM_XMLHeader(SM_XMLHeader& h)
	{
	operator =(h);
	}

void SM_XMLHeader :: Clear()
	{

if (hdr)
		delete[] hdr;
	hdr = 0;


	RemoveAllComments();

	if (comments)
		delete[] comments;
	comments = 0;
	
	}

SM_XMLHeader& SM_XMLHeader :: operator =(SM_XMLHeader& h)
	{
	Clear();


	const char*ht = h.operator const char*();
	hdr = new char[strlen(ht) + 1];
	strcpy(hdr,ht);


	// comments

	comments = new SM_XMLComment*[SM_XML_MAX_INIT_COMMENTS_HEADER];
	memset(comments,0,sizeof(SM_XMLComment*)*SM_XML_MAX_INIT_COMMENTS_HEADER);
	TotalCommentPointersAvailable = SM_XML_MAX_INIT_COMMENTS_HEADER;
	commentsnum = 0;
	// Add comments from h
	int yC = h.GetCommentsNum();
	for(int i = 0 ; i < yC ; i++)
		{
		AddComment(h.GetComments()[i]->Duplicate(),h.GetComments()[i]->GetEP());
		}

	return *this;
	}


SM_XMLHeader :: ~SM_XMLHeader()
	{
	Clear();
	}


SM_XMLHeader :: operator const char*()
	{
	return hdr;
	}   


void SM_XMLHeader :: SetEncoding(const char* e)
	{
	// 
	Z<char> nt(1000);
	sprintf(nt,"<?xml version=\"1.0\" encoding=\"%s\" standalone=\"yes\" ?>",e);
	size_t sl = strlen(nt) + 1;

	delete[] hdr;
	hdr = new char[sl];
	memset(hdr,0,sl);
	strcpy(hdr,nt);

	}

SM_XMLHeader* SM_XMLHeader :: Duplicate()
	{

	SM_XMLHeader* hz = new SM_XMLHeader(hdr);
	// Add All Comments
	int y = GetCommentsNum();
	for(int i = 0 ; i < y ; i++)
		{
		hz->AddComment(GetComments()[i]->Duplicate(),GetComments()[i]->GetEP());
		}



	return hz;
	}


int SM_XMLHeader :: AddComment(SM_XMLComment* v,int pos)
	{
	SpaceForComment(1);
	comments[commentsnum++] = v;
	v->SetParent(0,pos);
	return commentsnum;
	}


unsigned int SM_XMLHeader :: GetCommentsNum()
	{

	return commentsnum;

	}


SM_XMLComment** SM_XMLHeader :: GetComments()
	{
	return comments;
	}



int SM_XMLHeader :: RemoveAllComments()
	{

	for(int i = commentsnum - 1 ; i >= 0 ; i--)
		{
		delete comments[i];
		comments[i] = 0;
		}
	commentsnum = 0;

	return 0;
	}


int SM_XMLHeader :: RemoveComment(unsigned int i)
	{

	if (i >= commentsnum)
		return commentsnum;

	delete comments[i];
	comments[i] = 0;

	for(unsigned int k = i ; k < commentsnum ; k++)
		comments[k] = comments[k + 1];

	comments[commentsnum - 1] = 0;
	return --commentsnum;

	}


void SM_XMLHeader :: Export(FILE* fp,int HeaderMode,SM_XML_TARGET_MODE TargetMode,class SM_XMLTransform* eclass,class SM_XMLTransformData* edata)
	{
	if (TargetMode == 1)
		{
		if (HeaderMode == 0)
			sprintf((char*)fp,"%s\r\n",hdr);

		for(unsigned int i = 0 ; i < commentsnum ; i++)
			{
			if (comments[i]->GetEP() == HeaderMode)
				sprintf((char*)fp,"<!--%s-->\r\n",comments[i]->operator const char *());
			}

		return;
		}

	if (TargetMode == 2)
		{
		return; //*

		}

	if (TargetMode == 0) // UTF-8
		{

		if (HeaderMode == 0)
			fprintf(fp,"%s\r\n",hdr);
		for(unsigned int i = 0 ; i < commentsnum ; i++)
			{
			if (comments[i]->GetEP() == HeaderMode)
				fprintf(fp,"<!--%s-->\r\n",comments[i]->operator const char *());
			}

		}
	if (TargetMode == 3) // UTF-16
		{
		if (HeaderMode == 0)
			{

			Z<char> txt(strlen(hdr)*2 + 100);
			sprintf(txt,"%s\r\n",hdr);

			SM_XMLElement :: Write16String(fp,txt);
			}


		for(unsigned int i = 0 ; i < commentsnum ; i++)
			{
			if (comments[i]->GetEP() == HeaderMode)
				{
				const char* c = comments[i]->operator const char *();
				Z<char> txt(strlen(c)*2 + 100);
				sprintf(txt,"<!--%s-->\r\n",c);
				SM_XMLElement :: Write16String(fp,txt);
				}
			}

		}
	}



// SM_XMLVariable class
void SM_XMLVariable :: SetName(const char* VN,int NoDecode,const wchar_t *WVN)
	{
	SM_XMLU wh(WVN);
	if (!VN)
		VN = wh.bc();

// 	if (_vn)
// 		delete[] _vn;
	size_t Sug = SM_XML :: SM_XMLEncode(VN,0);
	char *vn = new char[Sug + 10];
	memset(vn,0,Sug + 10);
	if (NoDecode)
		strcpy(vn,VN);
	else
		SM_XML :: SM_XMLEncode(VN,vn);

	// 0x132 fix for white space at the end of the variable
	while(vn[strlen(vn) - 1] == ' ')
		vn[strlen(vn) - 1] = 0;

	mapNameToId(CString(vn), _vnId);
	delete [] vn;

	}

void SM_XMLVariable :: SetValue(const char* VV,int NoDecode,const wchar_t* WVV)
	{
	SM_XMLU wh(WVV);
	if (!VV)
		VV = wh.bc();

#ifdef USE_OLD_VV
	if (_vv)
		delete[] _vv;
	size_t Sug = SM_XML :: SM_XMLEncode(VV,0);
	_vv = new char[Sug + 10];
	memset(_vv,0,Sug + 10);
	if (NoDecode)
		strcpy(_vv,VV);
	else
		SM_XML :: SM_XMLEncode(VV,_vv);

#else
	size_t Sug = SM_XML :: SM_XMLEncode(VV,0);
	char *vv = new char[Sug + 10];
	memset(vv,0,Sug + 10);
	if (NoDecode)
		strcpy(vv,VV);
	else
		SM_XML :: SM_XMLEncode(VV,vv);

	//_vv = vv;

	mapNameToId(CString(vv), _vvId);

	delete [] vv;

#endif
	}

SM_XMLVariable :: SM_XMLVariable(const char* VN,const char* VV,int NoDecode,bool Temp,const wchar_t* WVN,const wchar_t* WVV)
	{
#ifndef USE_OLD_VV
		_vvId = 0;
#endif
		_vnId = 0;

	SM_XMLU wh1(WVN);
	if (!VN)
		VN = wh1.bc();
	SM_XMLU wh2(WVV);
	if (!VV)
		VV = wh2.bc();


	//_vn = 0;

#ifdef USE_OLD_VV
	_vv = 0;
#endif

	owner = 0;
	//Temporal = Temp;
	SetName(VN,NoDecode);
	SetValue(VV,NoDecode);
	}

void SM_XMLVariable :: SetTemporal(bool x)
	{
	//Temporal = x;
	}

bool SM_XMLVariable :: IsTemporal()
	{
	//return Temporal;
	return false;
	}

SM_XMLElement* SM_XMLVariable :: SetOwnerElement(SM_XMLElement* o)
	{
	SM_XMLElement* oldowner = owner;
	owner = o;
	return oldowner;
	}

SM_XMLElement* SM_XMLVariable :: GetOwnerElement()
	{
	return owner;
	}

void SM_XMLVariable :: Clear()
	{

// 	if (_vn)
// 		delete[] _vn;
// 	_vn = 0;

#ifdef USE_OLD_VV
	if (_vv)
		delete[] _vv;
	_vv = 0;
#endif

	}

SM_XMLVariable :: ~SM_XMLVariable()
	{
	Clear();
	}

SM_XMLVariable :: SM_XMLVariable(const SM_XMLVariable& h)
	{
#ifndef USE_OLD_VV
		_vvId = 0;
#endif
		_vnId = 0;
	operator =(h);
	}

SM_XMLVariable& SM_XMLVariable :: operator =(const SM_XMLVariable& h)
	{
	Clear();

	owner = h.owner;
	//Temporal = h.Temporal;

	size_t n = h.GetName(0,true);
	Z<char> nn(n + 10);
	h.GetName(nn,true);
	SetName(nn,true);

	size_t k = h.GetValue(0,true);
	Z<char> vv(k + 10);
	h.GetValue(vv,true);
	SetValue(vv,true);
	return *this;
	}


size_t SM_XMLVariable :: GetName(char* x,int NoDecode) const
	{

	CString vn = _idToName[_vnId];
	if (!x)
		{
		if (NoDecode)
			return strlen(vn);
		else
			return SM_XML :: SM_XMLDecode(vn,0);
		}

	if (NoDecode)
		strcpy(x,vn);
	else
		SM_XML :: SM_XMLDecode(vn,x);
	return strlen(x);

	}

size_t SM_XMLVariable :: GetValue(char* x,int NoDecode) const
	{

#ifdef USE_OLD_VV
	if (!x)
		{
		if (NoDecode)
			return strlen(_vv);
		else
			return SM_XML :: SM_XMLDecode(_vv,0);
		}
	if (NoDecode)
		strcpy(x,_vv);
	else
		SM_XML :: SM_XMLDecode(_vv,x);
	return strlen(x);
#else
		if (!x)
		{
			if (NoDecode)
				return strlen(_idToName[_vvId]);
			else
				return SM_XML :: SM_XMLDecode(_idToName[_vvId],0);
		}
		if (NoDecode)
			strcpy(x,_idToName[_vvId]);
		else
			SM_XML :: SM_XMLDecode(_idToName[_vvId],x);
		return strlen(x);
#endif

	}

int SM_XMLVariable :: GetValueInt()
	{
	size_t p = GetValue(0);
	Z<char> d(p + 10);
	GetValue(d);
	return atoi(d);
	}
unsigned int SM_XMLVariable :: GetValueUInt()
	{
	size_t p = GetValue(0);
	Z<char> d(p + 10);
	GetValue(d);
	unsigned long x = 0;
	sscanf(d,"%u",&x);
	return x;
	}

__int64 SM_XMLVariable :: GetValueInt64()
	{
	size_t p = GetValue(0);
	Z<char> d(p + 10);
	GetValue(d);
	__int64 x = 0;
	sscanf(d,"%I64i",&x);
	return x;
	}

unsigned __int64 SM_XMLVariable :: GetValueUInt64()
	{
	size_t p = GetValue(0);
	Z<char> d(p + 10);
	GetValue(d);
	unsigned __int64 x = 0;
	sscanf(d,"%I64u",&x);
	return x;
	}

void SM_XMLVariable :: SetValueInt(int V)
	{
	char t[50] = {0};
	sprintf(t,"%i",V);
	SetValue(t);
	}
void SM_XMLVariable :: SetValueUInt(unsigned int V)
	{
	char t[50] = {0};
	sprintf(t,"%u",V);
	SetValue(t);
	}

void SM_XMLVariable :: SetValueInt64(__int64 V)
	{
	char t[50] = {0};
	sprintf(t,"%I64i",V);
	SetValue(t);
	}

void SM_XMLVariable :: SetValueUInt64(unsigned __int64 V)
	{
	char t[50] = {0};
	sprintf(t,"%I64u",V);
	SetValue(t);
	}

float SM_XMLVariable :: GetValueFloat()
	{
	size_t p = GetValue(0);
	Z<char> d(p + 10);
	GetValue(d);
	return (float)atof(d);
	}

void SM_XMLVariable :: SetFormattedValue(const char* fmt,...)
	{
	va_list args;
	va_start (args, fmt);
	Z<char> data(10000);
	vsprintf(data,fmt,args);
	SetValue(data);
	va_end (args);
	}




void SM_XMLVariable :: SetValueFloat(float V)
	{
	char t[50] = {0};
	sprintf(t,"%f",V);
	SetValue(t);
//	SetValueX(V,"%f");
	}

// template <typename T> void SM_XMLVariable :: SetValueX(T ty,const char* fmt)
// 	{
// 	char t[50] = {0};
// 	sprintf(t,fmt,ty);
// 	SetValue(t);
// 	}

template <typename T> T SM_XMLVariable :: GetValueX(const char* fmt)
	{
	char t[50] = {0};
	GetValue(t);
	T ty;
	sscanf(t,fmt,&ty);
	return ty;
	}


void SM_XMLVariable :: Copy()
	{
	}

SM_XMLVariable* SM_XMLVariable :: Duplicate()
	{
	// returns a copy of myself
	size_t s1 = GetName(0);
	size_t s2 = GetValue(0);
	Z<char> x1(s1 + 100);
	Z<char> x2(s2 + 100);
	GetName(x1);
	GetValue(x2);

	return new SM_XMLVariable(x1,x2,0);
	}


#ifdef SM_XML_OPTIONAL_MIME
size_t SM_XMLVariable :: SetBinaryValue(char* data,int len)
	{
	// Sets value using MIME
	char* a = new char[len*5 + 1000];
	memset(a,0,len*5 + 1000);

	char *oo;
	MimeCoder<char*,char*>* e;
	e = new Base64Encoder<char*,char*>;
	oo = e->Filter(a, data, data + len);
	oo = e->Finish(oo);
	*oo = 0;                  // Put a zero to the end
	delete e;

//	CBase64 b;
//	b.Encrypt(data,len,a);
	size_t I = strlen(a);
	for(size_t i = 0 ; I > 0 && i < (I - 1) ; i++)
		{
		if (a[i] == '\r' && a[i + 1] == '\n')
			{
			a[i] = '_';
			a[i + 1] = '_';
			}
		}
	SetValue(a,false);

	delete[] a;
	return I;
	}

size_t SM_XMLVariable :: GetBinaryValue(char* data)
	{
	size_t aL = GetValue(0);
	if (!aL)
		return 0;

	Z<char> bdata(aL + 100);
	GetValue(bdata);

	for(unsigned int i = 0  ; i < (strlen(bdata) - 1) ; i++)
		{
		if (bdata[i] == '_' && bdata[i + 1] == '_')
			{
			bdata[i] = '\r';
			bdata[i + 1] = '\n';
			}
		}


	Z<char> oout(aL + 1000);


	char *oo;
	MimeCoder<char*,char*>* e;
	e = new Base64Decoder<char*,char*>;
	oo = e->Filter(oout, bdata.operator char*(), bdata.operator char*() + aL);
	oo = e->Finish(oo);
	*oo = 0;                  // Put a zero to the end
	delete e;
	size_t S = oo - oout.operator char*();

//	CBase64 b;
//	char* dstp = (char*)b.Decrypt(bdata,aL,oout);
//	size_t S = dstp - oout;

	if (!data)
		{
		return S;
		}
	else
		{
		memcpy(data,oout,S);
		return S;
		}
	}

#endif

#ifdef _WIN32
#include <wincrypt.h>
bool SM_XMLEncryptDecryptData(bool Decrypt,const char*pwd,int pwdlen,const char* d,int sz,char** oo,int* oolen)
	{
	// AES-based encryption
	if (!d || !sz || !oo || !oolen)
		return 0;

	// Acquire context
	HCRYPTPROV hCryptProv = 0;
	if(!CryptAcquireContext(&hCryptProv,NULL,MS_ENH_RSA_AES_PROV,PROV_RSA_AES,0))
		return 0;

	// Generate hash
	HCRYPTHASH hCryptHash = 0;
	if (!CryptCreateHash(hCryptProv,CALG_SHA1,0,0,&hCryptHash))
		{
		CryptReleaseContext(hCryptProv,0);
		return 0;
		}

	// Add the key
	if (!CryptHashData(hCryptHash,(const BYTE*)pwd,pwdlen,0))
		{
		CryptDestroyHash(hCryptHash);
		CryptReleaseContext(hCryptProv,0);
		return 0;
		}

	// Generate the key
	HCRYPTKEY hCryptKey = 0;
	if (!CryptDeriveKey(hCryptProv,CALG_AES_256,hCryptHash,0,&hCryptKey))
		{
		CryptDestroyHash(hCryptHash);
		CryptReleaseContext(hCryptProv,0);
		return 0;
		}

	// Calculate needed data
	if (Decrypt)
		{
		DWORD dl = sz;
		char* out = new char[dl];
		memcpy(out,d,sz);
		if (!CryptDecrypt(hCryptKey,0,TRUE,0,(BYTE*)out,&dl))
			{
			delete[] out;
			CryptDestroyKey(hCryptKey);
			CryptDestroyHash(hCryptHash);
			CryptReleaseContext(hCryptProv,0);
			return 0;
			}

		CryptDestroyKey(hCryptKey);
		CryptDestroyHash(hCryptHash);
		CryptReleaseContext(hCryptProv,0);
		out[dl] = 0;
		*oo = out;
		*oolen = dl;
		return 1;
		}
	else
		{
		DWORD dl = sz;
		CryptEncrypt(hCryptKey,0,TRUE,0,0,&dl,sz);
	
		// dl must have now the buffer
		if (!dl)
			{
			CryptDestroyKey(hCryptKey);
			CryptDestroyHash(hCryptHash);
			CryptReleaseContext(hCryptProv,0);
			return 0;
			}
		char* out = new char[dl + 1];
		memcpy(out,d,sz);
		DWORD dl2 = sz;
		if (!CryptEncrypt(hCryptKey,0,TRUE,0,(BYTE*)out,&dl2,dl))
			{
			delete[] out;
			CryptDestroyKey(hCryptKey);
			CryptDestroyHash(hCryptHash);
			CryptReleaseContext(hCryptProv,0);
			return 0;
			}

		CryptDestroyKey(hCryptKey);
		CryptDestroyHash(hCryptHash);
		CryptReleaseContext(hCryptProv,0);
		*oo = out;
		*oolen = dl;
		return 1;
		}
	}
#endif


bool SM_XMLElement :: EncryptElement(unsigned int i,char* pwd)
	{
	if (i >= GetChildrenNum())
		return false;

	SM_XMLElement* j = children[i];
	SM_XMLElement* nj = j->Encrypt(pwd);

	if (!nj)
		return false;
	RemoveElement(i);

	InsertElement(0,nj);

	return true;
	}
bool SM_XMLElement :: DecryptElement(unsigned int i,char* pwd)
	{
	if (i >= GetChildrenNum())
		return false;

	SM_XMLElement* j = children[i];
	SM_XMLElement* nj = j->Decrypt(pwd);

	if (!nj)
		return false;
	RemoveElement(i);

	InsertElement(0,nj);

	return true;
	}

SM_XMLElement* SM_XMLElement :: Encrypt(const char* pwd)
	{
#ifndef _WIN32
	return false;
#else

	// Get this element as item
	// Encrypt into new element and return
	size_t M = MemoryUsage();
	Z<char> d(M);
	Export((FILE*)d.operator char *(),1,SM_XML_SAVE_MODE_DEFAULT,SM_XML_TARGET_MODE_MEMORY);
	size_t S = strlen(d);

	char* encrdata = 0;
	int encrln = 0;
	SM_XMLEncryptDecryptData(0,pwd,(int)strlen(pwd),d.operator char*(),(int)S,&encrdata,&encrln);
	if (!encrdata)
		return 0;

	SM_XMLElement* ne = new SM_XMLElement(0,"<el />",0,0);

	
	ne->SetElementName(_idToName[_elId]);


	ne->AddContent(encrdata,0,encrln);
	delete[] encrdata;
	return ne;
#endif
	}
SM_XMLElement* SM_XMLElement :: Decrypt(const char* pwd)
	{
#ifndef _WIN32
	return false;
#else
	// Get this element's binary data
	char* bd = 0;
	int bdl = 0;
	if (GetContentsNum() != 1)
		return 0; // duh


	SM_XMLContent* c = GetContents()[0];

	c->GetBinaryValue(&bd,&bdl);
	if (!bd)
		return 0;

	char* decrdata = 0;
	int decrln = 0;
	SM_XMLEncryptDecryptData(1,pwd,(int)strlen(pwd),bd,bdl,&decrdata,&decrln);
	if (!decrdata)
		{
		delete[] bd;
		return 0;
		}

	SM_XMLElement* ne = SM_XML::Paste(decrdata);
	delete[] decrdata;
	return ne;
#endif
	}


bool SM_XML :: TestMatch(const char* item1,const char* comp,const char* item2)
	{
	Z<char> iitem2(1000);
	if (item2[0] == '\"')
		{
		strcpy(iitem2,item2 + 1);
		if (strlen(iitem2))
			iitem2[strlen(iitem2) - 1] = 0;

		if (strcmp(comp,"==") == 0)
			return VMatching((char*)item1,(char*)iitem2);
		if (strcmp(comp,"!=") == 0)
			return !VMatching((char*)item1,(char*)iitem2);
		}
	else // integer
		{
		// Fix: Check if items are integers
		if (atoi(item1) == 0 && item1[0] != '0')
			return false;
		if (atoi(item2) == 0 && item2[0] != '0')
			return false;


		if (strcmp(comp,"==") == 0)
			return (atoi(item1) == atoi(item2));

		if (strcmp(comp,"!=") == 0)
			return (atoi(item1) != atoi(item2));

		if (strcmp(comp,">=") == 0)
			return (atoi(item1) >= atoi(item2));

		if (strcmp(comp,"<=") == 0)
			return (atoi(item1) <= atoi(item2));

		if (strcmp(comp,"<") == 0)
			return (atoi(item1) < atoi(item2));

		if (strcmp(comp,">") == 0)
			return (atoi(item1) > atoi(item2));
		}


	return true;
	}

int SM_XML :: SM_XMLQuery(const char* rootsection,const char* expression,SM_XMLElement** rv,unsigned int deep)
	{

	SM_XMLElement* r = root->GetElementInSection(rootsection);

	if (!r)
		return 0;
	return r->SM_XMLQuery(expression,rv,deep);
	}

int SM_XMLElement :: SM_XMLQuery(const char* expression2,SM_XMLElement** rv,unsigned int deep)
	{
	Z<char> expression(strlen(expression2) + 1);
	strcpy(expression,expression2);
	// Executes query based on expression of variables
	/*

	Valid expressions

	<item1> <comparator> <item2>[<expr> ...]

	<item1> is a variable name, or * if any variable can match or ? if it is to match
	the element name or ! if it is to match the full element name 
	or ~ to match the content name

	<item2> can be either integers, or strings, or strings with ? and *
	<comparator> can be
	==
	!=
	>=
	>
	<
	<=

	MUST HAVE SPACES
	EXAMPLES

	v == "xa*"
	a >= 5


	*/

	SM_XMLElement* r = this;

	// r is the base section
	// get all its elements
	int C = r->GetAllChildrenNum(deep);
	Z<SM_XMLElement*> allelements(C + 10);
	r->GetAllChildren(allelements,deep);
	Z<int> positives(C + 10);

	int i;

	for(i = 0 ; i < C ; i++)
		positives[i] = 1;

	char* a = expression.operator char *();
	for( ;; )
		{
		// Parse the expression

		// Get item 1
		char* a1 = strchr(a,' ');
		if (!a1)
			break;
		Z<char> item1(300);
		*a1 = 0;
		strcpy(item1,a);
		*a1 = ' ';
		a = a1 + 1;

		// Get comparator
		a1 = strchr(a,' ');
		if (!a1)
			break;
		Z<char> comp(100);
		*a1 = 0;
		strcpy(comp,a);
		*a1 = ' ';
		a = a1 + 1;

		// Get item 2
		if (*a == '\"')
			{
			a1 = strchr(a + 1,'\"');
			if (a1)
				a1++;
			}
		else
			a1 = strchr(a,' ');
		Z<char> item2(300);
		if (a1)
			*a1 = 0;
		strcpy(item2,a);
		if (a1)
			{
			*a1 = ' ';
			a = a1 + 1;
			}


		// Test this expression against all elements
		for(int y = 0 ; y < C ; y++)
			{
			Z<char> ItemToMatch(1000);

			if (allelements[y] == 0)
				continue;


			if (strcmp(item1,"?") == 0)
				{
				allelements[y]->GetElementName(ItemToMatch,0);
				if (SM_XML :: TestMatch(ItemToMatch,comp,item2) == 0)
					positives[y] = 0;
				}
			else
			if (strcmp(item1,"!") == 0)
				{
				allelements[y]->GetElementFullName(ItemToMatch,0);
				if (SM_XML :: TestMatch(ItemToMatch,comp,item2) == 0)
					positives[y] = 0;
				}
			else
			if (strncmp(item1,"~",1) == 0)
				{
				unsigned int iC = atoi(item1.operator char*() + 1);
				if (allelements[y]->GetContentsNum() > iC)

					allelements[y]->GetContents()[iC]->GetValue(ItemToMatch);


				if (SM_XML :: TestMatch(ItemToMatch,comp,item2) == 0)
					positives[y] = 0;
				}
			else
				{
				int V = allelements[y]->FindVariable(item1);
				if (V == -1)
					strcpy(ItemToMatch,"");
				else

					allelements[y]->GetVariables()[V]->GetValue(ItemToMatch,0);

				if (SM_XML :: TestMatch(ItemToMatch,comp,item2) == 0)
					positives[y] = 0;
				}
			}

		// Testing finished.
		if (!a1)
			break;


		}

	int N = 0;
	for(i = 0 ; i < C ; i++)
		{
		if (positives[i])
			{
			if (rv)
				rv[N] = allelements[i];
			N++;
			}
		}
	return N;
	}



// Global functions
Z<char>* SM_XML :: ReadToZ(const char* file,SM_XMLTransform* eclass,class SM_XMLTransformData* edata,bool IsU)
	{
	FILE* fp = 0;
#ifndef LINUX
	if (IsU)
		fp = _wfopen((wchar_t*)file,L"rb");
	else
#endif
		fp = fopen(file,"rb");
	if (!fp)
		return 0;

	fseek(fp,0,SEEK_END);
	int S = ftell(fp);
	fseek(fp,0,SEEK_SET);

	Z<char>* y = 0;
	if (eclass == 0)
		{
		y = new Z<char>(S + 32);
		fread((*y).operator char *(),1,S,fp);
		fclose(fp);
		}
	else
		{
		Z<char> yy(S + 32);
		y = new Z<char>(S + 32);
		fread(yy.operator char *(),1,S,fp);
		fclose(fp);

		//eclass->Prepare(edata);
		eclass->Decrypt(yy.operator char *(),S,0,(*y).operator char *(),S,0);
		}

	// Check if this SM_XML we loaded is a unicode SM_XML
	// In such case, we must convert to UTF-8
#ifdef _WIN32
	unsigned char* yy = (unsigned char*)(*y).operator char *();
	if (yy[0] == 0xFF && yy[1] == 0xFE)
		{
		// Whops, unicode SM_XML file loaded
		wchar_t* wd = (wchar_t*)(*y).operator char *();


		Z<char>* nyy = new Z<char>(S*4 + 32);
		WideCharToMultiByte(CP_UTF8,0,wd,-1,(*nyy),S*4 + 32,0,0);
		delete y;
		y = nyy;
		}
#endif

	return y;
	}


size_t SM_XMLGetString(const char* section,const char* Tattr,const char* defv,char*out,const size_t maxlen,const char* xml,SM_XML* af)
	{
	size_t Z = 0;
	if (!af)
		{
		SM_XML f(xml);
		Z = f.SM_XMLGetValue((char*)section,(char*)Tattr,out,maxlen);
		}
	else
		{
		Z = af->SM_XMLGetValue((char*)section,(char*)Tattr,out,maxlen);
		}

	if (Z)
		return Z;
	strcpy(out,defv);
	return strlen(defv);
	}

int SM_XMLSetString(const char* section,const char* Tattr,char*out,const char* xml,SM_XML* af)
	{
	if (!af)
		{
		SM_XML f(xml);
		f.SM_XMLSetValue((char*)section,(char*)Tattr,out);
		f.Save();
		}
	else
		{
		af->SM_XMLSetValue((char*)section,(char*)Tattr,out);
		}
	return 1;
	}

#ifndef __SYMBIAN32__
#ifdef _WIN32
int SM_XMLSetString(const char* section,const char* Tattr,wchar_t*out,const char* xml,SM_XML* af)
	{
	// Convert to UTF-8
	size_t S = wcslen(out);
	Z<char> ut(S*2 + 1000);
	WideCharToMultiByte(CP_UTF8,0,out,-1,ut,(int)(S*2 + 1000),0,0);
	return SM_XMLSetString(section,Tattr,ut,xml,af);
	}
#endif
#endif



int SM_XMLRenameElement(const char* section,const char* newname,const char* xml,SM_XML* af)
	{
	bool C = false;
	if (!af)
		{
		C = true;
		af = new SM_XML(xml);
		}


	SM_XMLElement* r = af->GetRootElement();
	SM_XMLElement* e = r->GetElementInSection(section);

	if (!e)
		return 0; // no items under this one

	e->SetElementName(newname);

	if (C)
		{
		delete af;
		af = 0;
		}
	return 1;
	}


unsigned int SM_XMLGetUInt(const char* item,const char* attr,const unsigned int defv,const char* xml,SM_XML* af)
	{
	Z<char> i(100);
	Z<char> id(100);
	sprintf(id,"%u",defv);
	SM_XMLGetString(item,attr,id,i,100,xml,af);
	unsigned int x = 0;
	sscanf(i,"%u",&x);
	return x;
	}

int SM_XMLGetInt(const char* item,const char* attr,const int defv,const char* xml,SM_XML* af)
	{
	Z<char> i(100);
	Z<char> id(100);
	sprintf(id,"%i",defv);
	SM_XMLGetString(item,attr,id,i,100,xml,af);
	return atoi(i);
	}

#ifdef _WIN32
__int64  SM_XMLGetInt64(const char* item,const char* attr,const __int64 defv,const char* xml,SM_XML* af)
	{
	Z<char> i(100);
	Z<char> id(100);
	sprintf(id,"%I64i",defv);
	SM_XMLGetString(item,attr,id,i,100,xml,af);
	return _atoi64(i);
	}
unsigned __int64  SM_XMLGetUInt64(const char* item,const char* attr,const unsigned __int64 defv,const char* xml,SM_XML* af)
	{
	Z<char> i(100);
	Z<char> id(100);
	sprintf(id,"%I64u",defv);
	SM_XMLGetString(item,attr,id,i,100,xml,af);
	unsigned __int64 x = 0;
	sscanf(i,"%I64u",&x);
	return x;
	}
#endif

float SM_XMLGetFloat(const char* item,const char* attr,const float defv,const char* xml,SM_XML* af)
	{
	Z<char> a1(30);
	sprintf(a1,"%f",defv);

	Z<char> a2(30);
	SM_XMLGetString(item,attr,a1,a2,30,xml,af);

	return (float)atof(a2);
	}

#ifdef SM_XML_OPTIONAL_MIME
size_t SM_XMLGetBinaryData(const char* item,const char* attr,const char* defv,char*out,const size_t maxlen,const char* xml,SM_XML* af)
	{
	Z<char> bdata(maxlen*5 + 5000);
	size_t len = SM_XMLGetString(item,attr,defv,bdata,maxlen*5 + 5000,xml,af);
	if (!len)
		return 0;

	for(unsigned int i = 0  ; i < (strlen(bdata) - 1) ; i++)
		{
		if (bdata[i] == '_' && bdata[i + 1] == '_')
			{
			bdata[i] = '\r';
			bdata[i + 1] = '\n';
			}
		}



	Z<char> oout(maxlen*5 + 5000);

	char *oo;
	MimeCoder<char*,char*>* e;
	e = new Base64Decoder<char*,char*>;
	oo = e->Filter(oout, bdata.operator char*(), bdata.operator char*() + len);
	oo = e->Finish(oo);
	*oo = 0;                  // Put a zero to the end
	delete e;


	size_t S = oo - oout.operator char*();
	if (S > maxlen)
		{
		memcpy(out,oout,maxlen);
		return maxlen;
		}
	else
		{
		memcpy(out,oout,S);
		return S;
		}
	}
#endif

int SM_XMLSetUInt(const char* section,const char* attr,unsigned int v,const char* xml,SM_XML* af)
	{
	char a[20] = {0};
	sprintf(a,"%u",v);
	return SM_XMLSetString(section,attr,a,xml,af);
	}
int SM_XMLSetInt(const char* section,const char* attr,int v,const char* xml,SM_XML* af)
	{
	char a[20] = {0};
	sprintf(a,"%i",v);
	return SM_XMLSetString(section,attr,a,xml,af);
	}

#ifdef _WIN32
int    SM_XMLSetUInt64(const char* section,const char* attr,unsigned __int64 v,const char* xml,SM_XML* af)
	{
	char a[40] = {0};
	sprintf(a,"%I64u",v);
	return SM_XMLSetString(section,attr,a,xml,af);
	}
int    SM_XMLSetInt64(const char* section,const char* attr,__int64 v,const char* xml,SM_XML* af)
	{
	char a[40] = {0};
	sprintf(a,"%I64i",v);
	return SM_XMLSetString(section,attr,a,xml,af);
	}
#endif

int    SM_XMLSetFloat(const char* section,const char* attr,float v,const char* xml,SM_XML* af)
	{
	char a[20] = {0};
	sprintf(a,"%f",v);
	return SM_XMLSetString(section,attr,a,xml,af);
	}

#ifdef SM_XML_OPTIONAL_MIME
int    SM_XMLSetBinaryData(const char* section,const char* attr,char* data,int len,const char* xml,SM_XML* af)
	{
	char* a = new char[len*5 + 1000];
	memset(a,0,len*5 + 1000);

	char *oo;
	MimeCoder<char*,char*>* e;
	e = new Base64Encoder<char*,char*>;
	oo = e->Filter(a, data, data + len);
	oo = e->Finish(oo);
	*oo = 0;                  // Put a zero to the end
	delete e;


	for(unsigned int i = 0 ; i < strlen(a) - 1 ; i++)
		{
		if (a[i] == '\r' && a[i + 1] == '\n')
			{
			a[i] = '_';
			a[i + 1] = '_';
			}
		}

	int I = SM_XMLSetString(section,attr,a,xml,af);
	delete[] a;
	return I;
	}
#endif

// vector based things
#ifndef __SYMBIAN32__

int SM_XMLGetAllVariables(const char* section,char** vnames,char** vvalues,const char*xml)
	{
	SM_XML f(xml);

	SM_XMLElement* r = f.GetRootElement();
	SM_XMLElement* e = r->GetElementInSection(section);
	if (!e)
		return 0; // no items under this one

	int Y = e->GetVariableNum();
	for(int i = 0 ; i < Y ; i++)
		{
		size_t yS = e->GetVariables()[i]->GetName(0);
		char* d = new char[yS + 10];
		memset(d,0,yS + 10);
		e->GetVariables()[i]->GetName(d);
		vnames[i] = d;

		yS = e->GetVariables()[i]->GetValue(0);
		char* d2 = new char[yS + 10];
		memset(d2,0,yS + 10);
		e->GetVariables()[i]->GetValue(d2);
		vvalues[i] = d2;
		}
	return Y;
	}

int SM_XMLGetAllItems(const char* section,char** vnames,const char*xml)
	{
	SM_XML f(xml);

	SM_XMLElement* r = f.GetRootElement();
	SM_XMLElement* e = r->GetElementInSection(section);
	if (!e)
		return 0; // no items under this one

	int Y = e->GetChildrenNum();
	for(int i = 0 ; i < Y ; i++)
		{
		size_t yS =  e->GetChildren()[i]->GetElementName(0);
		char* d = new char[yS + 10];
		memset(d,0,yS + 10);
		e->GetChildren()[i]->GetElementName(d);
		vnames[i] = d;
		}
	return Y;
	}


#endif


// Win32 Ansi/Unicode wrappers

// Win32 ANSI string
#ifdef _WIN32
#ifndef __SYMBIAN32__

int SM_XMLSetStringA(const char* item,const char* attr,const char*v,const char* xml)
	{
	size_t Y = strlen(v)*2 + 1000;
	Z<char> vv(Y);
	Z<wchar_t> ww(Y);

	MultiByteToWideChar(CP_ACP,0,v,(int)strlen(v),ww,(int)Y);
	WideCharToMultiByte(CP_UTF8,0,ww,(int)wcslen(ww),vv,(int)Y,0,0);
	return SM_XMLSetString(item,attr,vv,xml);
	}

int    SM_XMLSetStringW(const char* item,const char* attr,const wchar_t*v,const char* xml)
	{
	size_t Y = wcslen(v)*2 + 1000;
	Z<char> vv(Y);

	WideCharToMultiByte(CP_UTF8,0,v,(int)wcslen(v),vv,(int)Y,0,0);
	return SM_XMLSetString(item,attr,vv,xml);
	}

size_t SM_XMLGetStringA(const char* section,const char* Tattr,const char* defv,char*out,const size_t maxlen,const char* xml,SM_XML* aF,int CP)
	{
	size_t S = maxlen*2 + 1000;
	Z<char> a1(S);

	size_t S2 = strlen(defv) + 10;
	Z<char> a2(S2);
	strcpy(a2,defv);

	size_t Y = SM_XMLGetString(section,Tattr,defv,a1,S,xml,aF);
	if (strcmp(a2,a1) == 0)
		return Y; // default was returned

	// An uTf-8 string was returned
	Z<wchar_t> uv(S);
	MultiByteToWideChar(CP_UTF8,0,a1,-1,uv,(int)S);
	return WideCharToMultiByte(CP,0,uv,-1,out,(int)maxlen,0,0);
	}


int SM_XMLGetStringW(const char* section,const char* Tattr,const wchar_t* defv,wchar_t*out,const int maxlen,const char* xml,SM_XML* aF,int CP)
	{
	//
	return 0;
	}



#endif
#endif

#ifndef WINCE
#ifdef _WIN32
// Signature Functions

SM_XMLVariable* SM_XMLElement :: GetSignature(unsigned int i)
	{
	if (i == (unsigned int)-1)
		{
		// Self
		return FindVariableZ("__signature__");
		}

	if (GetChildrenNum() <= i)
		return 0;
	SM_XMLElement* e = GetChildren()[i];
	SM_XMLVariable* s = e->FindVariableZ("__signature__");
	return s;
	}

bool SM_XMLElement :: RemoveSignature(unsigned int i)
	{
	if (i == (unsigned int)-1)
		{
		for(;;)
			{
			SM_XMLVariable* s = FindVariableZ("__signature__");
			if (!s)
				break;
			RemoveVariable(s);
			}
		return true;
		}
	if (GetChildrenNum() <= i)
		return false;
	SM_XMLElement* e = GetChildren()[i];
	for(;;)
		{
		SM_XMLVariable* s = e->FindVariableZ("__signature__");
		if (!s)
			break;
		e->RemoveVariable(s);
		}
	return true;
	}

bool SM_XMLElement :: SignElement(unsigned int ij,PCCERT_CONTEXT pCert)
	{
	SM_XMLElement* e = 0;

	if (ij == (unsigned int)-1)
		e = this;
	else
		{
		if (GetChildrenNum() <= ij)
			return false;
		e = GetChildren()[ij];
		}

	HRESULT hr = 0;
	bool Status = false;
	if (!pCert)
		return Status;

	// Check if this element already has an __signature__ variable
	// If yes, fail
	if (e->FindVariableZ("__signature__"))
		return Status;

	// Take element text 
	size_t mu = e->MemoryUsage();
	Z<char> mut(mu*2 + 1000);
	const BYTE* pbContent = (const BYTE*)mut.operator char*();
	e->Export((FILE*)pbContent,1,SM_XML_SAVE_MODE_DEFAULT,SM_XML_TARGET_MODE_MEMORY);
	CRYPT_DATA_BLOB blob = {strlen(mut),(BYTE*)pbContent};

	// Parameters
	#define MY_ENCODING_TYPE  (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING)
	CRYPT_SIGN_MESSAGE_PARA SignMessagePara = {0};
	SignMessagePara.cbSize = sizeof(CRYPT_SIGN_MESSAGE_PARA);
	SignMessagePara.HashAlgorithm.pszObjId = szOID_RSA_SHA1RSA;
	SignMessagePara.pSigningCert = pCert;
	SignMessagePara.dwMsgEncodingType = MY_ENCODING_TYPE;
	SignMessagePara.cMsgCert = 1;
	SignMessagePara.rgpMsgCert = &pCert;

	const BYTE *rgpbToBeSigned[1];
    DWORD rgcbToBeSigned[1];
    rgpbToBeSigned[0] = pbContent;
    rgcbToBeSigned[0] = strlen(mut);


	// Get bytes
	if(CryptSignMessage(
		&SignMessagePara,
		TRUE,
		1,
		rgpbToBeSigned,
		rgcbToBeSigned,
		NULL,
		&blob.cbData))
		{
		Z<char> enc(blob.cbData + 100);

		if( CryptSignMessage(
			&SignMessagePara,
			TRUE,
			1,
			rgpbToBeSigned,
			rgcbToBeSigned,
			(BYTE*)enc.operator char*(),
			&blob.cbData))
			{
			// Signing done, save
			e->AddBinaryVariable("__signature__",enc.operator char*(),blob.cbData);
			Status = true;
			}
		}
	return Status;
	}


bool SM_XMLElement :: VerifyDigitalSignature(unsigned int ij,PCCERT_CONTEXT* ppCert)
	{
	SM_XMLElement* e = 0;
	if (ij == (unsigned int)-1)
		e = this;
	else
		{
		if (GetChildrenNum() <= ij)
			return false;
		e = GetChildren()[ij];
		}

	HRESULT hr = 0;
	bool Status = false;

	SM_XMLVariable* s = e->FindVariableZ("__signature__");
	if (!s)
		return Status;


	size_t SignatureSize = s->GetBinaryValue(0);
	Z<char> Signature(SignatureSize + 100);
	s->GetBinaryValue(Signature);

	// Remove Signature
	SM_XMLVariable* ds = s->Duplicate();
	e->RemoveVariable(s);
	

	// Take element text 
	size_t mu = e->MemoryUsage();
	Z<char> mut(mu*2 + 1000);
	BYTE* pbContent = (BYTE*)mut.operator char*();
	e->Export((FILE*)pbContent,1,SM_XML_SAVE_MODE_DEFAULT,SM_XML_TARGET_MODE_MEMORY);
	DWORD cbContent = strlen((char*)pbContent);

	CRYPT_DATA_BLOB blob = {cbContent,pbContent};

	// Readd signature
	e->AddVariable(ds);
	s = e->FindVariableZ("__signature__");
	if (!s)
		return Status;


	// Try it
	CRYPT_VERIFY_MESSAGE_PARA VerifyParams = {0};
    VerifyParams.cbSize = sizeof(CRYPT_VERIFY_MESSAGE_PARA);
    VerifyParams.dwMsgAndCertEncodingType = MY_ENCODING_TYPE;
    VerifyParams.hCryptProv = 0;
    VerifyParams.pfnGetSignerCertificate = NULL;
    VerifyParams.pvGetArg = NULL;
	const BYTE* bu[1];
	bu[0] = pbContent;

/*
	DWORD pbbc = 0;
	if (CryptVerifyMessageSignature(&VerifyParams,0,(BYTE*)Signature.operator char*(),SignatureSize,0,&pbbc,0))
		{
		
		Status = true;
		}
	else
		{
		DWORD err = GetLastError();
		Status = false;
		}
*/
	if (CryptVerifyDetachedMessageSignature(&VerifyParams,0,(BYTE*)Signature.operator char*(),SignatureSize,1,bu,&cbContent,ppCert))
		{
		Status = true;
		}
	else
		{
		int err = GetLastError();
		Status = false;
		}
	return Status;
	}


SM_XMLElement* SM_XMLElement :: EncryptElement(unsigned int ij,PCCERT_CONTEXT* pCert,int nCert)
	{
	SM_XMLElement* e = 0;
	if (ij == (unsigned int)-1)
		e = this;
	else
		{
		if (GetChildrenNum() <= ij)
			return 0;
		e = GetChildren()[ij];
		}

	HRESULT hr = 0;
	if (!pCert || nCert <= 0)
		return 0;

	// Take element text 
	size_t mu = e->MemoryUsage();
	Z<char> mut(mu*2 + 1000);
	BYTE* pbContent = (BYTE*)mut.operator char*();
	e->Export((FILE*)pbContent,1,SM_XML_SAVE_MODE_DEFAULT,SM_XML_TARGET_MODE_MEMORY);
	DWORD cbContent = strlen((char*)pbContent);


	CRYPT_ENCRYPT_MESSAGE_PARA cp = {0};
	cp.cbSize = sizeof(cp);
	cp.dwMsgEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
	cp.ContentEncryptionAlgorithm.pszObjId = szOID_RSA_RC4;

	DWORD rs = 0;
	if (CryptEncryptMessage(&cp,nCert,pCert,pbContent,cbContent,0,&rs))
		{
		Z<char> enc(rs + 100);
		rs += 100;
		if (CryptEncryptMessage(&cp,nCert,pCert,pbContent,cbContent,(BYTE*)enc.operator char*(),&rs))
			{
			Z<char> fn(10000);
			e->GetElementName(fn);
			SM_XMLElement* x = new SM_XMLElement(0,"<e />");
			x->SetElementName(fn);
			x->AddBinaryVariable("v",enc.operator char*(),rs);
			return x;
			}
		}
	return 0; // Fail
	}


SM_XMLElement* SM_XMLElement :: DecryptElement(unsigned int ij,PCCERT_CONTEXT* ppCert)
	{
	SM_XMLElement* e = 0;
	if (ij == (unsigned int)-1)
		e = this;
	else
		{
		if (GetChildrenNum() <= ij)
			return 0;
		e = GetChildren()[ij];
		}

	HRESULT hr = 0;

	// Take encrypted message
	if (!e->FindVariableZ("v"))
		return 0;
	size_t cbContent = e->FindVariableZ("v")->GetBinaryValue(0);
	Z<char> mut(cbContent*2 + 1000);
	e->FindVariableZ("v")->GetBinaryValue(mut);
	BYTE* pbContent = (BYTE*)mut.operator char*();

	CRYPT_DECRYPT_MESSAGE_PARA cp = {0};
	cp.cbSize = sizeof(cp);
	cp.dwMsgAndCertEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
	HCERTSTORE hStore = CertOpenStore(
		CERT_STORE_PROV_SYSTEM_W,
		X509_ASN_ENCODING,
		NULL,
		CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG,
		L"MY"
		);
	cp.cCertStore = 1;
	cp.rghCertStore = &hStore;

	DWORD rs = 0;
	if (CryptDecryptMessage(&cp,pbContent,cbContent,0,&rs,0))
		{
		Z<char> dec(rs + 100);
		rs += 100;
		if (CryptDecryptMessage(&cp,pbContent,cbContent,(BYTE*)dec.operator char*(),&rs,ppCert))
			{
			if (hStore)
				CertCloseStore(hStore,0);
			hStore = 0;
			SM_XMLElement* x = SM_XML::Paste(dec);
			return x;
			}
		else
			{
			if (hStore)
				CertCloseStore(hStore,0);
			hStore = 0;
			int Le = GetLastError();
			return 0;
			}
		}
	if (hStore)
		CertCloseStore(hStore,0);
	hStore = 0;
	return 0;
	}

#endif // WIN323
#endif // WINCE


#ifdef SM_XML_USE_NAMESPACE
};
#endif




static const unsigned int BUFF_SIZE = 1048576;

// bool SM_XMLElement::findVariable(const char* variableName, CStringA& value)
// {
// 	SM_XMLVariable** pVariables = GetVariables();
// 
// 	if ( pVariables == NULL ) {
// 		return false; 
// 	}
// 
// 	int ourVariable = FindVariable((char*)variableName);
// 	if ( ourVariable < 0) {
// 		return false;
// 	}
// 
// 	char buff[BUFF_SIZE];
// 
// 	if ( pVariables[ourVariable]->GetValue(buff) <= 0 ) {
// 		return false;
// 	}
// 
// 	value = buff;
// 	return true;
// 
// 
// }
#if _MSC_VER >= 1400
bool SM_XMLElement::findVariable(const char* variableName, CStringW& value)
{
	SM_XMLVariable** pVariables = GetVariables();

	if ( pVariables == NULL ) {
		return false; 
	}

	int ourVariable = FindVariable((char*)variableName);
	if ( ourVariable < 0) {
		return false;
	}

	char buff[BUFF_SIZE];

	if ( pVariables[ourVariable]->GetValue(buff) <= 0 ) {
		return false;
	}

	value = buff;
	return true;


}

bool SM_XMLElement::findVariable(const char* variableName, CStringA& value)
{
	SM_XMLVariable** pVariables = GetVariables();

	if ( pVariables == NULL ) {
		return false; 
	}

	int ourVariable = FindVariable((char*)variableName);
	if ( ourVariable < 0) {
		return false;
	}

	char buff[BUFF_SIZE];

	if ( pVariables[ourVariable]->GetValue(buff) <= 0 ) {
		return false;
	}

	value = buff;
	return true;


}
#else

bool SM_XMLElement::findVariable(const char* variableName, CString& value)
{
	SM_XMLVariable** pVariables = GetVariables();

	if ( pVariables == NULL ) {
		return false; 
	}

	int ourVariable = FindVariable((char*)variableName);
	if ( ourVariable < 0) {
		return false;
	}

	char buff[BUFF_SIZE];

	if ( pVariables[ourVariable]->GetValue(buff) <= 0 ) {
		return false;
	}

	value = buff;
	return true;


}
#endif
bool SM_XMLElement::findVariable(const char* variableName,char* pValue, DWORD bufferSize /*= SM_XML_DEFAULT_BUFFERSIZE*/)
{
	if ( pValue == NULL ){
		return false;
	}
	CString strValue;
	if ( ! findVariable(variableName,strValue) ) {
		return false;
	}

	strncpy(pValue,strValue,min(strValue.GetLength()+1,SM_XML_DEFAULT_BUFFERSIZE-1));

	return true;
}

bool SM_XMLElement::findVariable(const char* variableName, int& value)
{
	CString strValue;
	if ( ! findVariable(variableName,strValue) ) {
		return false;
	}

	value = atoi(strValue);

	return true;
}
bool SM_XMLElement::findVariable(const char* variableName, __int64& value)
{
	CString strValue;
	if ( ! findVariable(variableName,strValue) ) {
		return false;
	}

	value = _atoi64(strValue);

	return true;
}
bool SM_XMLElement::findVariable(const char* variableName, long& value)
{
	CString strValue;
	if ( ! findVariable(variableName,strValue) ) {
		return false;
	}

	value = atol(strValue);

	return true;
}

bool SM_XMLElement::findVariable(const char* variableName, double& value)
{
	CString strValue;
	if ( ! findVariable(variableName,strValue) ) {
		return false;
	}

	value = atof(strValue);

	return true;
}

bool SM_XMLElement::findVariable(const char* variableName, bool& value)
{
	CString strValue;
	if ( ! findVariable(variableName,strValue) ) {
		return false;
	}

	value = atoi(strValue) != 0;

	return true;
}
bool SM_XMLElement::findVariable(const char* variableName, byte& value)
{
	CString strValue;
	if ( ! findVariable(variableName,strValue) ) {
		return false;
	}

	value = atoi(strValue);

	return true;
}

bool SM_XMLElement::findVariable(const char* variableName, COLORREF& value)
{
	CString strValue;
	if ( ! findVariable(variableName,strValue) ) {
		return false;
	}

	//value = atoi(strValue);

	// make it unsigned
	value = strtoul(strValue, NULL, 10 );


	return true;
}


#if _MSC_VER >= 1400
bool SM_XMLElement::addVariable(const char* variableName, const CStringW& value)
{
	BSTR unicodestr = value.AllocSysString();
	int strSize = SysStringLen(unicodestr)+1;
	char* ansistr = new char[strSize];
	::WideCharToMultiByte(CP_ACP, 
		0, 
		unicodestr, 
		-1, 
		ansistr, 
		strSize, 
		NULL, 
		NULL);
	::SysFreeString(unicodestr);
	ansistr[strSize-1] = '\0';
	bool rValue = AddVariable(variableName,ansistr) > 0;
	delete[] ansistr;
	return rValue;
}

bool SM_XMLElement::addVariable(const char* variableName, const CStringA& value)
{
	return AddVariable(variableName,value) > 0;
}
#else
bool SM_XMLElement::addVariable(const char* variableName, const CString& value)
{
	return AddVariable(variableName,value) > 0;
}
#endif

bool SM_XMLElement::addVariable(const char* variableName, int value)
{
	CString strValue;
	strValue.Format("%d",value);

	return addVariable(variableName,strValue) ;

}
bool SM_XMLElement::addVariable(const char* variableName, const char* value)
{
	if ( value == NULL ) {
		return false;
	}

	CString strValue = value;;

	return addVariable(variableName,strValue) ;

}
bool SM_XMLElement::addVariable(const char* variableName, __int64 value)
{
	CString strValue;
	strValue.Format("%d",value);

	return addVariable(variableName,strValue) ;

}
bool SM_XMLElement::addVariable(const char* variableName, long value)
{
	CString strValue;
	strValue.Format("%d",value);

	return addVariable(variableName,strValue) ;

}
bool SM_XMLElement::addVariable(const char* variableName, double value)
{
	CString strValue;
	strValue.Format("%2.15f",value);

	return addVariable(variableName,strValue) ;

}
bool SM_XMLElement::addVariable(const char* variableName, bool value)
{
	CString strValue;
	strValue.Format("%d",value ? 1 : 0);

	return addVariable(variableName,strValue) ;

}

bool SM_XMLElement::addVariable(const char* variableName, byte value)
{
	CString strValue;
	strValue.Format("%d",value);

	return addVariable(variableName,strValue) ;

}

bool SM_XMLElement::addVariable(const char* variableName, COLORREF value)
{
	CString strValue;

	strValue.Format("%u",value);

	//strValue.Format("%d %d %d",  GetRValue(value), GetGValue(value), GetBValue(value));

	return addVariable(variableName, strValue) ;

}

#if _MSC_VER >= 1400
bool SM_XMLElement::setVariable(const char* variableName, const CStringW& value)
{
	int theVariableIndex = FindVariable((char*)variableName);
	if ( theVariableIndex != -1 ) {
		RemoveVariable(theVariableIndex);
	}

	return addVariable(variableName,value);

}

bool SM_XMLElement::setVariable(const char* variableName, const CStringA& value)
{
	int theVariableIndex = FindVariable((char*)variableName);
	if ( theVariableIndex != -1 ) {
		RemoveVariable(theVariableIndex);
	}

	return addVariable(variableName,value);

}
#else
bool SM_XMLElement::setVariable(const char* variableName, const CString& value)
{
	int theVariableIndex = FindVariable((char*)variableName);
	if ( theVariableIndex != -1 ) {
		RemoveVariable(theVariableIndex);
	}

	return addVariable(variableName,value);

}
#endif

bool SM_XMLElement::setVariable(const char* variableName, const char* value)
{

	if ( value == NULL ) {
		return false;
	}

	CString strValue = value;

	return setVariable(variableName,strValue) ;

}
bool SM_XMLElement::setVariable(const char* variableName, int value)
{
	CString strValue;
	strValue.Format("%d",value);

	return setVariable(variableName,strValue) ;

}
bool SM_XMLElement::setVariable(const char* variableName, __int64 value)
{
	CString strValue;
	strValue.Format("%d",value);

	return setVariable(variableName,strValue) ;

}
bool SM_XMLElement::setVariable(const char* variableName, long value)
{
	CString strValue;
	strValue.Format("%d",value);

	return setVariable(variableName,strValue) ;

}
bool SM_XMLElement::setVariable(const char* variableName, double value)
{
	CString strValue;
	strValue.Format("%2.15f",value);

	return setVariable(variableName,strValue) ;

}
bool SM_XMLElement::setVariable(const char* variableName, bool value)
{
	CString strValue;
	strValue.Format("%d",value ? 1 : 0);

	return setVariable(variableName,strValue) ;

}

bool SM_XMLElement::setVariable(const char* variableName, byte value)
{
	CString strValue;
	strValue.Format("%d",value);

	return setVariable(variableName,strValue) ;

}

bool SM_XMLElement::setVariable(const char* variableName, COLORREF value)
{
	CString strValue;
	strValue.Format("%u",value);
	//strValue.Format("%d %d %d",  GetRValue(value), GetGValue(value), GetBValue(value));

	return setVariable(variableName,strValue) ;
}






///////////////////////////////////////////////////////////////////////////////
//
//	Description:	
//		Finds an element based on the element name and the contents of the
//		name variable in the element.
//		
//	Prototype:	
//		bool SM_XMLElement::findElementByName(SM_XMLElement* pElement,
//								   SM_XMLElement* pParentElement,
//								   const char* elementName, 
//								   const char* nameVariableValue)
//
//	Notes:
//		None.
// 
///////////////////////////////////////////////////////////////////////////////

bool SM_XMLElement::findElementByName(SM_XMLElement*& pElement,
								   const char* elementName, 
								   const char* nameVariableValue )
{
	SM_XMLElement* pChild;
	CString varName;
	int numChildren = GetChildrenNum();
	int i;
	for ( i = 0; i < numChildren; ++i ) {
		pChild = GetChildren()[i];
		if ( pChild->findVariable(VARIABLE_name,varName) && (varName == nameVariableValue) ) {
			pElement = pChild;
			return true;
		}
	}

	return false;

}


