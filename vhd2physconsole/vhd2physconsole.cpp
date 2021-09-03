// vhd2physconsole.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "stdafx.h"
#include <iostream>
#include "vhd2physconsole.h"
#include "VhdToDisk.h"
#include <locale>
#include <codecvt>
#include <string>

using namespace std;

wstring wStrParam1 = L"";
wstring wStrParam2 = L"";
wchar_t* wParam1;
wchar_t* wParam2;

wstring utf8ToUtf16(const string& utf8Str)
{
	wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
	return conv.from_bytes(utf8Str);
}

string utf16ToUtf8(const wstring& utf16Str)
{
	wstring_convert<codecvt_utf8_utf16<wchar_t>> conv;
	return conv.to_bytes(utf16Str);
}

typedef struct _DUMPTHRDSTRUCT
{
	WCHAR sVhdPath[MAX_PATH];
	WCHAR sDrive[64];
}DUMPTHRDSTRUCT;

HANDLE hDumpThread = NULL;
CVhdToDisk* pVhd2disk = NULL;
DUMPTHRDSTRUCT dmpstruct;

DWORD WINAPI DumpThread(LPVOID lpVoid)
{
	cout << "DumpThread() - Start" << endl;
	DUMPTHRDSTRUCT* pDumpStruct = (DUMPTHRDSTRUCT*)lpVoid;

	if (pVhd2disk->DumpVhdToDisk(pDumpStruct->sVhdPath, pDumpStruct->sDrive))
	{
		cout << "DumpThread() - Return 0 (True)" << endl;
		return 0;
	}
	else
	{
		cout << "DumpThread() - Return 1 (False)" << endl;
		return 1;
	}
	/*
	if (pVhd2disk->DumpVhdToDisk(pDumpStruct->sVhdPath, pDumpStruct->sDrive))
		cout << "VHD dumped on drive successfully!" << endl;
	else
		cout << "Failed to dump the VHD on drive!" << endl;
	return 0;
	*/
}

void ListPhysicalDrives()
{
	HANDLE hFile = NULL;
	WCHAR sPhysicalDrive[64] = { 0 };

	for (int i = 0; i < 99; i++)
	{
		wsprintf(sPhysicalDrive, L"\\\\.\\PhysicalDrive%d", i);
		hFile = CreateFile(sPhysicalDrive
			, GENERIC_WRITE
			, 0
			, NULL
			, OPEN_EXISTING
			, FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING
			, NULL);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hFile);
			cout << "\\\\.\\PhysicalDrive" << i << endl;
		}
	}
}

void Help()
{
	cout << "This is a console application that will apply a vhd disk to a physical disk. Disk must be offline." << endl << endl;
	cout << "Example usage: vhd2physconsole.exe \"path to disk.vhd\" \\\\.\\physicaldrive1" << endl << endl;
	cout << "The available disks on this system are below:" << endl;
	ListPhysicalDrives();
}

string Dump()
{
	cout << "Dump() - Started" << endl;
	DWORD dwThrdID = 0;
	ZeroMemory(&dmpstruct, sizeof(DUMPTHRDSTRUCT));
	//if (wcslen(dmpstruct.sVhdPath) < 3) return TRUE;
	wcscpy_s(dmpstruct.sVhdPath, wParam1);
	wcscpy_s(dmpstruct.sDrive, wParam2);
	cout << "dmpstruct.sVhdPath is: " + utf16ToUtf8(dmpstruct.sVhdPath) << endl;
	cout << "dmpstruct.sDrive is: " + utf16ToUtf8(dmpstruct.sDrive) << endl;
	
	// TODO this thread is failing somewhere without any captured/printed output
	hDumpThread = CreateThread(NULL, 0, DumpThread, &dmpstruct, 0, &dwThrdID);
	return "Dump() - Thread handle: " + *reinterpret_cast<string*>(hDumpThread) + " Finished";
}


int main(int argc, CHAR* argv[])
{
	WCHAR sVhdPath[MAX_PATH] = { 0 };
	WCHAR sPhysicalDrive[64] = { 0 };
    
	cout << "You have entered " << argc << " arguments:" << endl;
    for (int i = 0; i < argc; ++i)
    {
        cout << argv[i] << endl;
    }
	
	if (argc != 3)
	{
		Help();
		return 1;
	}

	char* vIn1 = argv[1];
	wParam1 = new wchar_t[strlen(vIn1) + 1];
	mbstowcs_s(NULL, wParam1, strlen(vIn1) + 1, vIn1, strlen(vIn1));
	wStrParam1 = wParam1;
	string param1(argv[1]);

	char* vIn2 = argv[2];
	wParam2 = new wchar_t[strlen(vIn2) + 1];
	mbstowcs_s(NULL, wParam2, strlen(vIn2) + 1, vIn2, strlen(vIn2));
	wStrParam2 = wParam2;
	string param2(argv[2]);

	wcscpy_s(sVhdPath, wParam1);
	wcscpy_s(sPhysicalDrive, wParam2);

	// Testing
	cout << "argv[1] is: " << argv[1] << endl;
	//cout << "wParam1 is: " << *wParam1 << endl;
	cout << "wStrParam1 is: " << utf16ToUtf8(wStrParam1) << endl;
	//cout << "sVhdPath is: " << *sVhdPath << endl;
	cout << "argv[2] is: " << argv[2] << endl;
	//cout << "wParam2 is: " << *wParam2 << endl;
	cout << "wStrParam2 is: " << utf16ToUtf8(wStrParam2) << endl;
	//cout << "sPhysicalDrive is: " << *sPhysicalDrive << endl;

	//pVhd2disk = new CVhdToDisk(sVhdPath);
	cout << Dump() << endl;

	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
