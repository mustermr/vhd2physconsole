// vhd2physconsole.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "stdafx.h"
#include <iostream>
#include "vhd2physconsole.h"
#include "VhdToDisk.h"

CVhdToDisk* pVhd2disk = NULL;

typedef struct _DUMPTHRDSTRUCT
{
	WCHAR sVhdPath[MAX_PATH];
	WCHAR sDrive[64];
}DUMPTHRDSTRUCT;

DWORD WINAPI DumpThread(LPVOID lpVoid)
{
	DUMPTHRDSTRUCT* pDumpStruct = (DUMPTHRDSTRUCT*)lpVoid;

	if (pVhd2disk->DumpVhdToDisk(pDumpStruct->sVhdPath, pDumpStruct->sDrive))
		std::cout << "VHD dumped on drive successfully!" << std::endl;
	else
		std::cout << "Failed to dump the VHD on drive!" << std::endl;
	return 0;
}

HANDLE hDumpThread = NULL;
DUMPTHRDSTRUCT dmpstruct;

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
			std::cout << "\\\\.\\PhysicalDrive" << i << std::endl;
		}
	}
}

void Help()
{
	std::cout << "This is a console application that will apply a vhd disk to a physical disk. Disk must be offline." << std::endl << std::endl;
	std::cout << "Example usage: vhd2physconsole.exe \"path to disk.vhd\" \\\\.\\physicaldrive1" << std::endl << std::endl;
	std::cout << "The available disks on this system are below:" << std::endl;
	ListPhysicalDrives();
}

BOOL Dump()
{
	DWORD dwThrdID = 0;
	ZeroMemory(&dmpstruct, sizeof(DUMPTHRDSTRUCT));
	if (wcslen(dmpstruct.sVhdPath) < 3) return TRUE;
	hDumpThread = CreateThread(NULL, 0, DumpThread, &dmpstruct, 0, &dwThrdID);
	return true;
}


int main(int argc, CHAR* argv[])
{
	WCHAR sVhdPath[MAX_PATH] = { 0 };
	WCHAR sPhysicalDrive[64] = { 0 };
    
	std::cout << "You have entered " << argc << " arguments:" << std::endl;
    for (int i = 0; i < argc; ++i)
    {
        std::cout << argv[i] << "\n";
    }
	
	if (argc != 3)
	{
		Help();
		return 1;
	}

	char* vIn1 = argv[1];
	wchar_t* wParam1 = new wchar_t[strlen(vIn1) + 1];
	mbstowcs_s(NULL, wParam1, strlen(vIn1) + 1, vIn1, strlen(vIn1));
	std::string param1(argv[1]);

	char* vIn2 = argv[2];
	wchar_t* wParam2 = new wchar_t[strlen(vIn2) + 1];
	mbstowcs_s(NULL, wParam2, strlen(vIn2) + 1, vIn2, strlen(vIn2));
	std::string param2(argv[2]);

	wcscpy_s(sVhdPath, wParam1);
	wcscpy_s(sPhysicalDrive, wParam2);

	// Testing
	std::cout << "argv[1] is: " << argv[1] << std::endl;
	std::cout << "wParam1 is: " << wParam1 << std::endl;
	std::cout << "sVhdPath is: " << sVhdPath << std::endl;
	std::cout << "argv[2] is: " << argv[2] << std::endl;
	std::cout << "wParam2 is: " << wParam2 << std::endl;
	std::cout << "sPhysicalDrive is: " << sPhysicalDrive << std::endl;

	pVhd2disk = new CVhdToDisk(sVhdPath);
	Dump();

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
