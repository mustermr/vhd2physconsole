#include "StdAfx.h"
#include "Trace.h"
#include "VhdToDisk.h"
#include <locale>
#include <codecvt>
#include "resource.h"
#include <iostream>

using namespace std;

wstring utf8ToUtf16_2(const string& utf8Str)
{
	wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
	return conv.from_bytes(utf8Str);
}

string utf16ToUtf8_2(const wstring& utf16Str)
{
	wstring_convert<codecvt_utf8_utf16<wchar_t>> conv;
	return conv.to_bytes(utf16Str);
}

CVhdToDisk::CVhdToDisk(void)
{
	cout << "CVhdToDisk::CVhdToDisk(void) - Start" << endl;
	m_hVhdFile = NULL;
	m_hPhysicalDrive = NULL;

	ZeroMemory(&m_Foot, sizeof(VHD_FOOTER));
	ZeroMemory(&m_Dyn, sizeof(VHD_DYNAMIC));
}

CVhdToDisk::CVhdToDisk(LPWSTR sPath)
{
	cout << "CVhdToDisk::CVhdToDisk() - Start" << endl;
	m_hVhdFile = NULL;
	m_hPhysicalDrive = NULL;

	ZeroMemory(&m_Foot, sizeof(VHD_FOOTER));
	ZeroMemory(&m_Dyn, sizeof(VHD_DYNAMIC));

	if(!OpenVhdFile(sPath))
		return;

	if(!ReadFooter())
		return;

	if(!ReadDynHeader())
		return;
}


CVhdToDisk::~CVhdToDisk(void)
{
	cout << "CVhdToDisk::~CVhdToDisk() - Start" << endl;
	if(m_hVhdFile)
		CloseVhdFile();

	if(m_hPhysicalDrive)
		ClosePhysicalDrive();
}

BOOL CVhdToDisk::OpenVhdFile(LPWSTR sPath)
{
	cout << "CVhdToDisk::OpenVhdFile() - Start" << endl;
	m_hVhdFile = CreateFile(sPath
		, GENERIC_READ
		, FILE_SHARE_READ
		, NULL
		, OPEN_EXISTING
		, FILE_ATTRIBUTE_NORMAL
		, NULL);

	if(m_hVhdFile == INVALID_HANDLE_VALUE) return FALSE;

	return TRUE;
}

BOOL CVhdToDisk::CloseVhdFile()
{
	cout << "CVhdToDisk::CloseVhdFile() - Start" << endl;
	BOOL bReturn = TRUE;

	if(m_hVhdFile != INVALID_HANDLE_VALUE)
		bReturn = CloseHandle(m_hVhdFile);

	m_hVhdFile = NULL;

	return bReturn;
}

BOOL CVhdToDisk::OpenPhysicalDrive(LPWSTR sDrive)
{
	cout << "CVhdToDisk::OpenPhysicalDrive() - Start" << endl;
	m_hPhysicalDrive = CreateFile(sDrive
		, GENERIC_WRITE
		, 0
		, NULL
		, OPEN_EXISTING
		, FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING
		, NULL);

	if(m_hPhysicalDrive == INVALID_HANDLE_VALUE) return FALSE;

	return TRUE;

}

BOOL CVhdToDisk::ClosePhysicalDrive()
{
	cout << "CVhdToDisk::ClosePhysicalDrive() - Start" << endl;
	BOOL bReturn = TRUE;

	if(m_hPhysicalDrive != INVALID_HANDLE_VALUE)
		bReturn = CloseHandle(m_hPhysicalDrive);

	m_hPhysicalDrive = NULL;

	return bReturn;
}

BOOL CVhdToDisk::ReadFooter()
{
	cout << "CVhdToDisk::ReadFooter() - Start" << endl;
	BOOL bReturn = FALSE;
	DWORD dwByteRead = 0;

	if(m_hVhdFile == INVALID_HANDLE_VALUE) return FALSE;

	bReturn = ReadFile(m_hVhdFile, &m_Foot, sizeof(VHD_FOOTER), &dwByteRead, 0);

	if(bReturn)
		bReturn = (sizeof(VHD_FOOTER) == dwByteRead);

	return bReturn;
}

BOOL CVhdToDisk::ReadDynHeader()
{
	cout << "CVhdToDisk::ReadDynHeader() - Start" << endl;
	BOOL bReturn = FALSE;
	DWORD dwByteRead = 0;
	LARGE_INTEGER filepointer;
	filepointer.QuadPart = 512;

	if(m_hVhdFile == INVALID_HANDLE_VALUE) return FALSE;

	SetFilePointerEx(m_hVhdFile, filepointer, NULL, FILE_BEGIN);

	bReturn = ReadFile(m_hVhdFile, &m_Dyn, sizeof(VHD_DYNAMIC), &dwByteRead, 0);

	if(bReturn)
		bReturn = (sizeof(VHD_DYNAMIC) == dwByteRead);

	return bReturn;
}

BOOL CVhdToDisk::ParseFirstSector(HWND hDlg)
{
	cout << "CVhdToDisk::ParseFirstSector() - Start" << endl;
	BOOL bReturn = FALSE;
	DWORD dwBootSector = 0x00000000;
	DWORD dwByteRead = 0;
	WCHAR sTemp[64] = {0};

	LARGE_INTEGER filepointer;
	UINT32 blockBitmapSectorCount = (_byteswap_ulong(m_Dyn.blockSize) / 512 / 8 + 511) / 512;
	UINT32 sectorsPerBlock = _byteswap_ulong(m_Dyn.blockSize) / 512;
	UINT32 bats = _byteswap_ulong(m_Dyn.maxTableEntries);

	filepointer.QuadPart = _byteswap_uint64(m_Dyn.tableOffset);
	UINT8* bitmap = new UINT8[blockBitmapSectorCount * 512];
	UINT32* bat = new UINT32[bats * 4];
	BYTE* pBuff = new BYTE[512 * sectorsPerBlock];
	if (!bitmap || !bat || !pBuff)
	{
		if (bitmap) delete[] bitmap;
		if (bat) delete[] bat;
		if (pBuff) delete[] pBuff;
		return bReturn;
	}


	if(!SetFilePointerEx(m_hVhdFile, filepointer, NULL, FILE_BEGIN))
	{
		//TRACE("Failed to SetFilePointer(0x%08X, %lld, 0, FILE_BEGIN)\n", m_hVhdFile, filepointer.QuadPart);
		if (bitmap) delete[] bitmap;
		if (bat) delete[] bat;
		if (pBuff) delete[] pBuff;
		return bReturn;
	}

	if(!ReadFile(m_hVhdFile, bat, bats * sizeof(*bat), &dwByteRead, 0))
	{
		//TRACE("Failed to ReadFile(0x%08X, 0x%08X, %d,...) with error 0x%08X\n", m_hVhdFile, bat, bats * 4, GetLastError());
		if (bitmap) delete[] bitmap;
		if (bat) delete[] bat;
		if (pBuff) delete[] pBuff;
		return bReturn;;
	}

	UINT32 b = 0;
	UINT64 bo = _byteswap_ulong(bat[b]) * 512LL;

	filepointer.QuadPart = bo;

	bReturn = SetFilePointerEx(m_hVhdFile, filepointer, NULL, FILE_BEGIN);
	if (!bReturn)
	{
		if (bitmap) delete[] bitmap;
		if (bat) delete[] bat;
		if (pBuff) delete[] pBuff;
		return bReturn;
	}

	bReturn = ReadFile(m_hVhdFile, bitmap, 512 * blockBitmapSectorCount, &dwByteRead, 0);
	if (!bReturn)
	{
		if (bitmap) delete[] bitmap;
		if (bat) delete[] bat;
		if (pBuff) delete[] pBuff;
		return bReturn;
	}

	filepointer.QuadPart = bo + 512 * LONGLONG(blockBitmapSectorCount);

	bReturn = SetFilePointerEx(m_hVhdFile, filepointer, NULL, FILE_BEGIN);
	if (!bReturn)
	{
		if (bitmap) delete[] bitmap;
		if (bat) delete[] bat;
		if (pBuff) delete[] pBuff;
		return bReturn;
	}

	bReturn = ReadFile(m_hVhdFile, pBuff, 512 * sectorsPerBlock, &dwByteRead, 0);
	if (!bReturn)
	{
		if (bitmap) delete[] bitmap;
		if (bat) delete[] bat;
		if (pBuff) delete[] pBuff;
		return bReturn;
	}

	dwBootSector = ((DWORD)pBuff[510]) << 8;
	dwBootSector += ((BYTE)pBuff[511]);

	int nItem = 0;

	ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_LIST_VOLUME));

	if(dwBootSector == 0x000055AA)
	{
		int nPartitions = 1;
		DWORD dwOffset = 0x1be;
		HWND hwdListCtrl = GetDlgItem(hDlg, IDC_LIST_VOLUME);
		if (!hwdListCtrl)
		{
			if (bitmap) delete[] bitmap;
			if (bat) delete[] bat;
			if (pBuff) delete[] pBuff;
			return bReturn;
		}
		while(dwOffset < 0x1fe)
		{
			if(pBuff[dwOffset + 4] != 0x00)
			{
				LVITEM item;
				item.mask = LVIF_TEXT;
				item.iItem = nItem;
				item.iSubItem = 0;
				if (pBuff[dwOffset] == 0x80)
				{
					item.pszText = LPWSTR(L"Y");
					//item.pszText = L"Y";
				}
				else
				{
					item.pszText = LPWSTR(L"N");
					//item.pszText = L"N";
				}

				ListView_InsertItem(hwdListCtrl, &item);

				// TYPE (FS)
				if(pBuff[dwOffset + 4] == 0x07)
					wsprintf(sTemp, L"NTFS");
				else
					wsprintf(sTemp, L"0x%02X", pBuff[dwOffset + 4]);
				
				item.iSubItem = 1;
				item.pszText = sTemp;
				item.cchTextMax = wcslen(sTemp) + 1;
				ListView_SetItem(hwdListCtrl, &item);

				wsprintf(sTemp, L"0x%02X%02X%02X%02X"
					, pBuff[dwOffset + 15]
				, pBuff[dwOffset + 14]
				, pBuff[dwOffset + 13]
				, pBuff[dwOffset + 12]);

				item.iSubItem = 2;
				item.pszText =  sTemp;
				item.cchTextMax = wcslen(sTemp) + 1;

				ListView_SetItem(hwdListCtrl, &item);

				// Cylinder-head-sector address of the first sector in the partition
				wsprintf(sTemp, L"0x%02X 0x%02X 0x%02X"
								, pBuff[dwOffset + 1]
								, pBuff[dwOffset + 2]
								, pBuff[dwOffset + 3]);

				item.mask = LVIF_TEXT;
				item.iItem = nItem;
				item.iSubItem = 3;
				item.pszText =  sTemp;
				item.cchTextMax = wcslen(sTemp) + 1;

				ListView_SetItem(hwdListCtrl, &item);


				wsprintf(sTemp, L"0x%02X 0x%02X 0x%02X"
								, pBuff[dwOffset + 5]
								, pBuff[dwOffset + 6]
								, pBuff[dwOffset + 7]);

				item.mask = LVIF_TEXT;
				item.iItem = nItem;
				item.iSubItem = 4;
				item.pszText =  sTemp;
				item.cchTextMax = wcslen(sTemp) + 1;
				ListView_SetItem(hwdListCtrl, &item);

				nItem++;
			}
			dwOffset+= 0x10;
		}
	}

	if(bitmap) delete[] bitmap;
	if(bat) delete[] bat;
	if(pBuff) delete[] pBuff;

	return bReturn;
}


BOOL CVhdToDisk::Dump()
{
	cout << "CVhdToDisk::Dump() - Start" << endl;
	BOOL bReturn = FALSE;
	DWORD dwByteRead = 0;
	UINT32 emptySectors = 0;
	UINT32 usedSectors = 0;
	UINT32 usedZeroes = 0;
	
	LARGE_INTEGER filepointer;
	UINT32 blockBitmapSectorCount = (_byteswap_ulong(m_Dyn.blockSize) / 512 / 8 + 511) / 512;
	UINT32 sectorsPerBlock = _byteswap_ulong(m_Dyn.blockSize) / 512;
	UINT32 bats = _byteswap_ulong(m_Dyn.maxTableEntries);
	
	filepointer.QuadPart = _byteswap_uint64(m_Dyn.tableOffset);

	UINT8* bitmap = new UINT8[blockBitmapSectorCount * 512];
	UINT32* bat = new UINT32[bats * 4];
	char* pBuff = new char[512 * sectorsPerBlock];
	if (!bitmap || !bat || !pBuff)
	{
		if (bitmap) delete[] bitmap;
		if (bat) delete[] bat;
		if (pBuff) delete[] pBuff;
		return bReturn;
	}
	
	if(!SetFilePointerEx(m_hVhdFile, filepointer, NULL, FILE_BEGIN))
	{
		//TRACE("Failed to SetFilePointer(0x%08X, %lld, 0, FILE_BEGIN)\n", m_hVhdFile, filepointer.QuadPart);
		if (bitmap) delete[] bitmap;
		if (bat) delete[] bat;
		if (pBuff) delete[] pBuff;
		return bReturn;
	}
	
	if(!ReadFile(m_hVhdFile, bat, bats * sizeof(*bat), &dwByteRead, 0))
	{
		//TRACE("Failed to ReadFile(0x%08X, 0x%08X, %d,...) with error 0x%08X\n", m_hVhdFile, bat, bats * 4, GetLastError());
		if (bitmap) delete[] bitmap;
		if (bat) delete[] bat;
		if (pBuff) delete[] pBuff;
		return bReturn;
	}

	std::cout << "Start dumping..." << std::endl;
		
	for(UINT32 b = 0; b < bats; b++)
	{
		if((b + 1) % 100 == 0)
		{
			WCHAR sText[256] = {0};
			std::cout << sText << "dumping blocks..." << b + 1 << bats << std::endl;

		}

		
		if(_byteswap_ulong(bat[b]) == 0xFFFFFFFF)
		{
			emptySectors += sectorsPerBlock;
			continue;
		}

		UINT64 bo = _byteswap_ulong(bat[b]) * 512LL;

		filepointer.QuadPart = bo;

		bReturn = SetFilePointerEx(m_hVhdFile, filepointer, NULL, FILE_BEGIN);
		if(!bReturn)
		{
			if (bitmap) delete[] bitmap;
			if (bat) delete[] bat;
			if (pBuff) delete[] pBuff;
			return bReturn;
		}

		bReturn = ReadFile(m_hVhdFile, bitmap, 512 * LONGLONG(blockBitmapSectorCount), &dwByteRead, 0);
		if(!bReturn)
		{
			if (bitmap) delete[] bitmap;
			if (bat) delete[] bat;
			if (pBuff) delete[] pBuff;
			return bReturn;
		}

		UINT64 opos = 0xffffffffffffffffLL;

		filepointer.QuadPart = bo + 512 * LONGLONG(blockBitmapSectorCount);

		bReturn = SetFilePointerEx(m_hVhdFile, filepointer, NULL, FILE_BEGIN);
		if(!bReturn)
		{
			if (bitmap) delete[] bitmap;
			if (bat) delete[] bat;
			if (pBuff) delete[] pBuff;
			return bReturn;
		}

		bReturn = ReadFile(m_hVhdFile, pBuff, 512 * sectorsPerBlock, &dwByteRead, 0);
		if(!bReturn)
		{
			if (bitmap) delete[] bitmap;
			if (bat) delete[] bat;
			if (pBuff) delete[] pBuff;
			return bReturn;
		}

		filepointer.QuadPart = (b * LONGLONG(sectorsPerBlock)) * 512LL;

		//TRACE("Writing at %lld\n", filepointer.QuadPart);

		SetFilePointerEx(m_hPhysicalDrive, filepointer, NULL, FILE_BEGIN);

		bReturn = WriteFile(m_hPhysicalDrive, pBuff, 512 * sectorsPerBlock, &dwByteRead, 0);
		if(!bReturn)
		{			
			if (bitmap) delete[] bitmap;
			if (bat) delete[] bat;
			if (pBuff) delete[] pBuff;
			return bReturn;
		}
	}

	return bReturn;
}

BOOL CVhdToDisk::DumpVhdToDisk(const LPWSTR sPath, const LPWSTR sDrive)
{
	cout << "CVhdToDisk::DumpVhdToDisk() - Start" << endl;
	BOOL bReturn = FALSE;

	cout << "CVhdToDisk::DumpVhdToDisk() - m_hVhdFile" << endl;
	if(m_hVhdFile == NULL)
	{
		cout << "CVhdToDisk::DumpVhdToDisk() - m_hVhdFile value is NULL" << endl;
		bReturn = OpenVhdFile(sPath);
		if(!bReturn)
		{
			cout << "CVhdToDisk::DumpVhdToDisk() - Failed to open file: " << utf16ToUtf8_2(sPath) << endl;
			//TRACE("Failed to open %s\n", sPath);
			goto exit;
		}
	}

	cout << "CVhdToDisk::DumpVhdToDisk() - OpenPhysicalDrive" << endl;
	bReturn = OpenPhysicalDrive(sDrive);
	if(!bReturn)
	{
		cout << "CVhdToDisk::DumpVhdToDisk() - Failed to open physical drive: " << utf16ToUtf8_2(sDrive) << endl;
		//TRACE("Failed to open physical drive: %S\n", sDrive);
		CloseVhdFile();
		goto exit;
	}
	
	cout << "CVhdToDisk::DumpVhdToDisk() - ReadFooter" << endl;
	bReturn = ReadFooter();
	if(!bReturn)
	{
		cout << "CVhdToDisk::DumpVhdToDisk() - Failed to read footer" << endl;
		//TRACE("Failed to read footer\n");
		goto clean;
	}

	cout << "CVhdToDisk::DumpVhdToDisk() - ReadDynHeader" << endl;
	bReturn = ReadDynHeader();
	if(!bReturn)
	{
		cout << "CVhdToDisk::DumpVhdToDisk() - Failed to read dynamic header" << endl;
		//TRACE("Failed to read dynamic header\n");
		goto clean;
	}

	cout << "CVhdToDisk::DumpVhdToDisk() - Dump" << endl;
	bReturn = Dump();
	if (!bReturn)
	{
		cout << "CVhdToDisk::DumpVhdToDisk() - Failed to Dump" << endl;
		//TRACE("Failed to Dump\n");
		goto clean;
	}

clean:

	CloseVhdFile();
	ClosePhysicalDrive();

exit:
	return bReturn;
}