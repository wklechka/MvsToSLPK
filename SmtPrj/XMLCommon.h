#pragma once
#include <string>
#include "SMXml.h"

void addContent(SM_XMLElement* elem, CString& item);
void addContent(SM_XMLElement* elem, std::string& item);
void addContent(SM_XMLElement* elem, bool item);
void addContent(SM_XMLElement* elem, int item);
void addContent(SM_XMLElement* elem, char* item);
void addContent(SM_XMLElement* elem, const char* item);
void addContent(SM_XMLElement* elem, double item);
void addContent(SM_XMLElement* elem, COLORREF color);
void addContent(SM_XMLElement* elem, double item, int precision);
