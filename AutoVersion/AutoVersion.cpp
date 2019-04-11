// AutoVersion.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <fstream>
#include <iostream>
#include <sstream> 
#include <list>
#include <codecvt>
#include "strext.h"
#include "vercmp.h"

#include <compact_enc_det\compact_enc_det.h>
#include <experimental/filesystem>
using namespace msdk;
namespace fs = std::experimental::filesystem;


std::wstring ReplaceFileVersion(std::wstring Context, const std::wstring& version)
{
	//FILEVERSION 1,0,0,5
	Context = strext::replace<WCHAR>(Context, L"FILEVERSION", L"");
	CVerCmp OldVer(Context.c_str()),NewVer(version.c_str());

	//only increment build version
	if (OldVer.sMajorVer == NewVer.sMajorVer &&
		OldVer.sMinorVer == NewVer.sMinorVer &&
		OldVer.sRevisionVer == NewVer.sRevisionVer)
	{
		NewVer.sBuildVer = OldVer.sBuildVer;
		NewVer.sBuildVer++;
	}
	else //set build version to 1
		NewVer.sBuildVer = 1;

	std::wcout << L"modify to " << strext::format(L"%d,%d,%d,%d",
		NewVer.sMajorVer, NewVer.sMinorVer, NewVer.sRevisionVer, NewVer.sBuildVer) << std::endl;

	return strext::format(L" FILEVERSION %d,%d,%d,%d", 
		NewVer.sMajorVer, NewVer.sMinorVer, NewVer.sRevisionVer, NewVer.sBuildVer);
}

std::wstring ReplaceFileVersion2(std::wstring Context, const std::wstring& version)
{
	//VALUE "FileVersion", "1.0.0.5"
	std::vector<std::wstring> sLines = strext::split<WCHAR>(Context, L",");
	if ( sLines.size() != 2)
		return Context;
	
	Context = strext::remove<WCHAR>(sLines[1], '\"');
	CVerCmp OldVer(Context.c_str()), NewVer(version.c_str());

	if (OldVer.sMajorVer == NewVer.sMajorVer &&
		OldVer.sMinorVer == NewVer.sMinorVer &&
		OldVer.sRevisionVer == NewVer.sRevisionVer)
	{
		NewVer.sBuildVer = OldVer.sBuildVer;
		NewVer.sBuildVer++;
	}
	else //set build version to 1
		NewVer.sBuildVer = 1;

	std::wcout << L"modify to " << strext::format(L"%d.%d.%d.%d",
		NewVer.sMajorVer, NewVer.sMinorVer, NewVer.sRevisionVer, NewVer.sBuildVer) << std::endl;

	return strext::format(L"            VALUE \"FileVersion\", \"%d.%d.%d.%d\"",
		NewVer.sMajorVer, NewVer.sMinorVer, NewVer.sRevisionVer, NewVer.sBuildVer);
}

Encoding GetEncoding(LPCWSTR lpszFile)
{
	std::ifstream is(lpszFile, std::ifstream::binary);
	if (!is.is_open())
		return UNKNOWN_ENCODING;

	is.seekg(0, is.end);
	int length = is.tellg();
	is.seekg(0, is.beg);

	std::vector<char> buffer;
	buffer.resize(length);
	is.read(&buffer[0], length);

	bool is_reliable;
	int bytes_consumed;
	return CompactEncDet::DetectEncoding(
		&buffer[0], buffer.size(),
		NULL, NULL, NULL,
		UNKNOWN_ENCODING,
		UNKNOWN_LANGUAGE,
		CompactEncDet::WEB_CORPUS,
		false,
		&bytes_consumed,
		&is_reliable);
}


BOOL Write(LPCWSTR lpszFile, std::wstring wstr, Encoding enc)
{
	std::wofstream fout(lpszFile, std::ios::binary);
	const unsigned long MaxCode = 0x10ffff;
	const std::codecvt_mode Mode = (std::codecvt_mode)(std::generate_header | std::little_endian);
	std::locale utf16_locale(fout.getloc(), new std::codecvt_utf16<wchar_t, MaxCode, Mode>);
	fout.imbue(utf16_locale);
	fout << wstr;
	return TRUE;
}

std::wstring GetTimeVersion()
{
	TCHAR lpszTime[MAX_PATH] = { 0 };

	SYSTEMTIME sysTime = { 0 };
	GetLocalTime(&sysTime);
	_stprintf_s(lpszTime, MAX_PATH, _T("%04d.%02d.%02d.1"),
		sysTime.wYear,
		sysTime.wMonth,
		sysTime.wDay);

	return lpszTime;
}

std::wstring readFile(const wchar_t* filename, Encoding enc)
{
	std::wifstream wif(filename);
	switch (enc)
	{
	case UTF8:
		wif.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
		break;
	case UTF16LE:
		wif.imbue(std::locale(std::locale::empty(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>));
		break;
	case UTF16BE:
		wif.imbue(std::locale(std::locale::empty(), new std::codecvt_utf16<wchar_t>));
		break;
	default:
		return L"";
		break;
	}
	
	std::wstringstream wss;
	wss << wif.rdbuf();
	return wss.str();
}

int ModifyVersion(LPCWSTR lpszFile)
{
	std::wcout << L"modify " << lpszFile << std::endl;
	int nCount = 0;
	std::vector<char> buffer;
	Encoding enc = GetEncoding(lpszFile);
	switch (enc)
	{
	case UTF8:
	case UTF16LE:
	case UTF16BE:
		break;
	default:
	{
		std::wcout << L"Unsupported encoding format.\n Supports UTF8 UTF16LE UTF16BE  encoding format" << std::endl;
		std::wcout << L"" << lpszFile << std::endl;
		return 1;
	}
	}

	std::wstring str = readFile(lpszFile, enc);
	std::wstring Context;

	std::wstring sTimeVersion = GetTimeVersion();
	std::vector<std::wstring> sLines = strext::split<WCHAR>(str, L"\r\n");
	for (std::vector<std::wstring>::iterator it = sLines.begin()
		; it != sLines.end(); it++)
	{
		std::wstring temp = *it;
		if ((*it).find(L"FILEVERSION") != std::wstring::npos)
		{
			nCount++;
			temp = ReplaceFileVersion(*it, sTimeVersion);
		}

		if ((*it).find(L"FileVersion") != std::wstring::npos)
		{
			nCount++;
			temp = ReplaceFileVersion2(*it, sTimeVersion);
		}

		Context += temp;
		Context += L"\n";
	}

	if (nCount)
	{
		Write(lpszFile, Context, enc);
	}

	std::wcout << nCount << L" changes have been modified." << std::endl;
	return nCount;
}

int ModifyPath(LPCWSTR lpszPath)
{
	int nCount = 0;
	std::wstring path = lpszPath;
	std::string ext(".rc");
	for (auto& p : fs::directory_iterator(path))
	{
		if (p.path().extension() == ext)
		{
			std::wstring temp = p.path();
			nCount+=ModifyVersion(temp.c_str());
		}
	}

	return nCount;
}


int _tmain(int argc, _TCHAR* argv[])
{
	//std::locale::global(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));


	if ( argc > 1)
	{
		LPCWSTR lpszFile = argv[1];

		if ( !PathFileExists(lpszFile) )
		{
			std::wcout << L"file not exist:" << lpszFile << std::endl;
			return 1;
		}
		int nCount = 0;
		if (PathIsDirectory(lpszFile) )
		{
			nCount = ModifyPath(lpszFile);
		}
		else
		{
			nCount = ModifyVersion(lpszFile);
		}
		 
		
		return 0;
	}
	else
	{
		ModifyPath(fs::current_path().c_str());
		
		return 0;
		
	}
}

