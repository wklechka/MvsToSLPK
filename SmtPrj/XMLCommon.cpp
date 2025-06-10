#include "pch.h"
#include "XMLCommon.h"

#include "smxml.h"


void addContent(SM_XMLElement* elem, CString& item)
{
	elem->AddContent(item.GetBuffer(0), 0);
	item.ReleaseBuffer();
}
void addContent(SM_XMLElement* elem, std::string& item)
{
	elem->AddContent(item.c_str(), 0);
}

void addContent(SM_XMLElement* elem, bool item)
{
	CString tmp;
	tmp.Format("%d", item);
	addContent(elem, tmp);
}

void addContent(SM_XMLElement* elem, int item)
{
	CString tmp;
	tmp.Format("%d", item);
	addContent(elem, tmp);
}

void addContent(SM_XMLElement* elem, char* item)
{
	CString tmp = item;
	addContent(elem, tmp);
}

void addContent(SM_XMLElement* elem, const char* item)
{
	CString tmp = item;
	addContent(elem, tmp);
}

void addContent(SM_XMLElement* elem, double item)
{
	CString tmp;
	tmp.Format("%f", item);
	addContent(elem, tmp);
}

void addContent(SM_XMLElement* elem, COLORREF color)
{
	CString tmp;
	tmp.Format("%u", color);
	addContent(elem, tmp);
}

void addContent(SM_XMLElement* elem, double item, int precision)
{
	CString format;
	format.Format("%%.%de", precision);

	CString tmp;
	tmp.Format(format, item);
	addContent(elem, tmp);
}
