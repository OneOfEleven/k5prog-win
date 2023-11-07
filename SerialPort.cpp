
#ifdef __BORLANDC__
	#include <string.h>
	#pragma hdrstop
	#include "SerialPort.h"
	#pragma package(smart_init)
#else
	#ifdef _MSC_VER
		#include "StdAfx.h"
	#endif
	#include <string.h>
	#include "SerialPort.h"
#endif

// ***********************************************************

#ifdef __BORLANDC__
	char * strcpy_s(char *d, int max, char *s)
	{
		return strcpy(d, s);
	}
#endif

// ********************************************

CSerialPort::CSerialPort()
{
	Init();
}

CSerialPort::~CSerialPort()
{
	Disconnect();
}

void CSerialPort::GetSerialPortList()
{  // enumerate the com-ports - non-WMI
	#define MAX_KEY_LENGTH 255
	#define MAX_VALUE_NAME 16383

	m_serialPortList.clear();

	HKEY hKey = NULL;
	LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_QUERY_VALUE, &hKey);
	if (res == ERROR_SUCCESS && hKey != NULL)
	{
//		TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
//		DWORD    cbName;                   // size of name string
		TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name
		DWORD    cchClassName = MAX_PATH;  // size of class string
		DWORD    cSubKeys=0;               // number of subkeys
		DWORD    cbMaxSubKey;              // longest subkey size
		DWORD    cchMaxClass;              // longest class string
		DWORD    cValues;					// number of values for key
		DWORD    cchMaxValue;          // longest value name
		DWORD    cbMaxValueData;       // longest value data
		DWORD    cbSecurityDescriptor; // size of security descriptor
		FILETIME ftLastWriteTime;      // last write time

		TCHAR achValue[MAX_VALUE_NAME];
		DWORD cchValue = MAX_VALUE_NAME;

		// Get the class name and the value count.
		res = RegQueryInfoKey(
				hKey,                    // key handle
				achClass,                // buffer for class name
				&cchClassName,           // size of class string
				NULL,                    // reserved
				&cSubKeys,               // number of subkeys
				&cbMaxSubKey,            // longest subkey size
				&cchMaxClass,            // longest class string
				&cValues,                // number of values for this key
				&cchMaxValue,            // longest value name
				&cbMaxValueData,         // longest value data
				&cbSecurityDescriptor,   // security descriptor
				&ftLastWriteTime);       // last write time

		if (res == ERROR_SUCCESS)
		{
			if (cValues > 0)
			{
				//printf( "\nNumber of values: %d\n", cValues);
				for (int i = 0, retCode = ERROR_SUCCESS; i < (int)cValues; i++)
				{
					cchValue = MAX_VALUE_NAME;
					achValue[0] = '\0';
					retCode = RegEnumValue(hKey, i, achValue, &cchValue, NULL, NULL, NULL, NULL);
					if (retCode == ERROR_SUCCESS)
					{
						DWORD type;
						char str[MAX_PATH];
						DWORD size = sizeof(str);

						retCode = RegQueryValueEx(hKey, achValue, NULL, &type, (BYTE *)str, &size);
						if (retCode == ERROR_SUCCESS)
						{
							T_SerialPortInfo serialPortInfo;
							strcpy_s(serialPortInfo.name, sizeof(serialPortInfo.name), str);
							strcpy_s(serialPortInfo.deviceID, sizeof(serialPortInfo.deviceID), achValue);
							m_serialPortList.push_back(serialPortInfo);
						}
					}
				}
			}
		}
	}

	if (hKey != NULL)
		RegCloseKey(hKey);
}

bool CSerialPort::isSerialPortPresent(char *name)
{  // enumerate the com-ports - non-WMI
	#define MAX_KEY_LENGTH 255
	#define MAX_VALUE_NAME 16383

	bool present = false;

	if (name == NULL)
		return false;

	HKEY hKey = NULL;
	LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_QUERY_VALUE, &hKey);
	if (res == ERROR_SUCCESS && hKey != NULL)
	{
//		TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
//		DWORD    cbName;                   // size of name string
		TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name
		DWORD    cchClassName = MAX_PATH;  // size of class string
		DWORD    cSubKeys=0;               // number of subkeys
		DWORD    cbMaxSubKey;              // longest subkey size
		DWORD    cchMaxClass;              // longest class string
		DWORD    cValues;					// number of values for key
		DWORD    cchMaxValue;          // longest value name
		DWORD    cbMaxValueData;       // longest value data
		DWORD    cbSecurityDescriptor; // size of security descriptor
		FILETIME ftLastWriteTime;      // last write time

		TCHAR achValue[MAX_VALUE_NAME];
		DWORD cchValue = MAX_VALUE_NAME;

		// Get the class name and the value count.
		res = RegQueryInfoKey(
				hKey,                    // key handle
				achClass,                // buffer for class name
				&cchClassName,           // size of class string
				NULL,                    // reserved
				&cSubKeys,               // number of subkeys
				&cbMaxSubKey,            // longest subkey size
				&cchMaxClass,            // longest class string
				&cValues,                // number of values for this key
				&cchMaxValue,            // longest value name
				&cbMaxValueData,         // longest value data
				&cbSecurityDescriptor,   // security descriptor
				&ftLastWriteTime);       // last write time

		if (res == ERROR_SUCCESS)
		{
			if (cValues > 0)
			{
				//printf( "\nNumber of values: %d\n", cValues);
				for (int i = 0, retCode = ERROR_SUCCESS; i < (int)cValues; i++)
				{
					cchValue = MAX_VALUE_NAME;
					achValue[0] = '\0';
					retCode = RegEnumValue(hKey, i, achValue, &cchValue, NULL, NULL, NULL, NULL);
					if (retCode == ERROR_SUCCESS)
					{
						DWORD type;
						char str[MAX_PATH];
						DWORD size = sizeof(str);

						retCode = RegQueryValueEx(hKey, achValue, NULL, &type, (BYTE *)str, &size);
						if (retCode == ERROR_SUCCESS)
						{
							if (strcmp(str, name) == 0)
							{
								present = true;
								break;
							}
						}
					}
				}
			}
		}
	}

	if (hKey != NULL)
		RegCloseKey(hKey);

	return present;
}

void CSerialPort::GetSerialPortList(std::vector<T_SerialPortInfo> &serialPortList)
{
	GetSerialPortList();

	serialPortList.clear();
	for (int i = 0; i < (int)m_serialPortList.size(); i++)
		serialPortList.push_back(m_serialPortList[i]);
}

void CSerialPort::Init()
{
	LastError = 0;
//	LastErrorStr[0] = 0;

	memset(&overlapped_Read, 0, sizeof(OVERLAPPED));
	memset(&overlapped_Write, 0, sizeof(OVERLAPPED));

	device_name[0] = 0;
	device_handle = INVALID_HANDLE_VALUE;

	GetSerialPortList();

	memset(&MyDCB, 0, sizeof(DCB));
	MyDCB.DCBlength = sizeof(DCB);
	MyDCB.BaudRate = 115200;
	MyDCB.fBinary = true;
	MyDCB.fParity = false;
	MyDCB.fOutxCtsFlow = false;
	MyDCB.fOutxDsrFlow = false;
	MyDCB.fDtrControl = DTR_CONTROL_DISABLE;	// DTR_CONTROL_DISABLE, DTR_CONTROL_ENABLE, DTR_CONTROL_HANDSHAKE
//	MyDCB.fDsrSensitivity:1;   // DSR sensitivity
//	MyDCB.fTXContinueOnXoff:1; // XOFF continues Tx
	MyDCB.fOutX = false;
	MyDCB.fInX = false;
//	MyDCB.fErrorChar: 1;       // enable error replacement
	MyDCB.fNull = false;
	MyDCB.fRtsControl = RTS_CONTROL_DISABLE;	// RTS_CONTROL_DISABLE, RTS_CONTROL_ENABLE, RTS_CONTROL_HANDSHAKE
	MyDCB.fAbortOnError = false;
//	MyDCB.fDummy2:17;          // reserved
//	MyDCB.XonLim;               // transmit XON thrhold
//	MyDCB.XoffLim;              // transmit XOFF thrhold
	MyDCB.ByteSize = 8;				// 8 bits per byte - when was it ever any different ??????
	MyDCB.Parity = NOPARITY;
	MyDCB.StopBits = ONESTOPBIT;
//	MyDCB.XonChar;              // Tx and Rx XON character
//	MyDCB.XoffChar;             // Tx and Rx XOFF character
//	MyDCB.ErrorChar;            // error replacement character
//	MyDCB.EofChar;              // end of input character
//	MyDCB.EvtChar;              // received event character

	memset(&MyTimeouts, 0, sizeof(COMMTIMEOUTS));
	MyTimeouts.ReadIntervalTimeout = MAXDWORD;
	MyTimeouts.ReadTotalTimeoutMultiplier = 0;
	MyTimeouts.ReadTotalTimeoutConstant = 0;
	MyTimeouts.WriteTotalTimeoutMultiplier = 0;
	MyTimeouts.WriteTotalTimeoutConstant = 0;

	ReceiveQueue  = 32768;
	TransmitQueue = 32768;

	MaxFails = 3;
}

bool CSerialPort::GetCommStat(DCB *dcb)
{
	if (device_handle == INVALID_HANDLE_VALUE)
		return false;

	if (!GetCommState(device_handle, dcb))
	{
		LastError = GetLastError();
//		GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));
		return false;
	}

	return true;
}

bool CSerialPort::Connected()
{
	if (device_handle == INVALID_HANDLE_VALUE)
		return false;



	// we need to be able to detect when a USB comport is no longer plugged in or working



	return true;
}

bool CSerialPort::Connect(char *NewDeviceName)
{
	DCB dcb;
	COMMTIMEOUTS timeouts;

	// just incase we are already connected
	Disconnect();

	if (NewDeviceName == NULL)
		return false;

	strcpy_s(device_name, sizeof(device_name), NewDeviceName);

	char dn[MAX_PATH];
	strcpy_s(dn, sizeof(dn), "\\\\.\\");
	strcpy_s(dn + strlen(dn), sizeof(dn) - strlen(dn), device_name);

	// Open device
	#ifdef SERIAL_OVERLAPPED
		device_handle = CreateFileA(dn, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
	#else
		device_handle = CreateFileA(dn, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	#endif
	if (device_handle == INVALID_HANDLE_VALUE)
	{
		LastError = GetLastError();
//		GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));
		return false;
	}

	// save the current port parameters
	if (!GetCommState(device_handle, &OriginalDCB))
	{
		LastError = GetLastError();
//		GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));

		CloseHandle(device_handle);
		device_handle = INVALID_HANDLE_VALUE;
		return false;
	}
	if (!GetCommTimeouts(device_handle, &OriginalTimeouts))
	{
		LastError = GetLastError();
//		GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));

		CloseHandle(device_handle);
		device_handle = INVALID_HANDLE_VALUE;
		return false;
	}

	// modify the ports parameters
	memcpy(&dcb, &OriginalDCB, sizeof(DCB));
	dcb.DCBlength = MyDCB.DCBlength;
	dcb.BaudRate = MyDCB.BaudRate;
	dcb.fBinary = MyDCB.fBinary;
	dcb.fParity = MyDCB.fParity;
//	dcb.fOutxCtsFlow = MyDCB.fOutxCtsFlow;
//	dcb.fOutxDsrFlow = MyDCB.fOutxDsrFlow;
	dcb.fOutxCtsFlow = 0;
	dcb.fOutxDsrFlow = 0;
	dcb.fDtrControl = MyDCB.fDtrControl;
	dcb.fOutX = MyDCB.fOutX;
	dcb.fInX = MyDCB.fInX;
	dcb.fNull = MyDCB.fNull;
	dcb.fRtsControl = MyDCB.fRtsControl;
	dcb.fAbortOnError = MyDCB.fAbortOnError;
	dcb.ByteSize = MyDCB.ByteSize;
	dcb.Parity = MyDCB.Parity;
	dcb.StopBits = MyDCB.StopBits;
	if (!SetCommState(device_handle, &dcb))
	{
		LastError = GetLastError();
//		GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));

		CloseHandle(device_handle);
		device_handle = INVALID_HANDLE_VALUE;
		return false;
	}

	memcpy(&timeouts, &OriginalTimeouts, sizeof(COMMTIMEOUTS));
	timeouts.ReadIntervalTimeout = MyTimeouts.ReadIntervalTimeout;
	timeouts.ReadTotalTimeoutMultiplier = MyTimeouts.ReadTotalTimeoutMultiplier;
	timeouts.ReadTotalTimeoutConstant = MyTimeouts.ReadTotalTimeoutConstant;
	timeouts.WriteTotalTimeoutMultiplier = MyTimeouts.WriteTotalTimeoutMultiplier;
	timeouts.WriteTotalTimeoutConstant = MyTimeouts.WriteTotalTimeoutConstant;
	if (!SetCommTimeouts(device_handle, &timeouts))
	{
		LastError = GetLastError();
//		GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));

		Disconnect();
		return false;
	}

	if (!SetupComm(device_handle, 1024 * ReceiveQueue, 1024 * TransmitQueue))
	{
		LastError = GetLastError();
//		GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));

		Disconnect();
		return false;
	}

	if (!SetupComm(device_handle, 1024 * ReceiveQueue, 1024 * TransmitQueue))
	{
		LastError = GetLastError();
//		GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));

		Disconnect();
		return false;
	}

	#ifdef SERIAL_OVERLAPPED
		overlapped_Read.hEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);
		overlapped_Write.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (overlapped_Read.hEvent == NULL || overlapped_Write.hEvent == NULL)
		{
			LastError = GetLastError();
//			GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));

			Disconnect();
			return false;
		}
	#endif

	{
		const DWORD flags = EV_BREAK | EV_CTS | EV_DSR | EV_ERR | EV_RING | EV_RLSD | EV_RXCHAR | EV_RXFLAG | EV_TXEMPTY;
		if (!SetCommMask(device_handle, flags))
		{
			LastError = GetLastError();
//			GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));

			Disconnect();
			return false;
		}
	}

	return (device_handle != INVALID_HANDLE_VALUE) ? true : false;
}

void CSerialPort::Disconnect()
{
	if (overlapped_Read.hEvent != NULL)
	{
		CloseHandle(overlapped_Read.hEvent);
		memset(&overlapped_Read, 0, sizeof(OVERLAPPED));
	}

	if (overlapped_Write.hEvent != NULL)
	{
		CloseHandle(overlapped_Write.hEvent);
		memset(&overlapped_Write, 0, sizeof(OVERLAPPED));
	}

	if (device_handle != INVALID_HANDLE_VALUE)
	{
		FlushFileBuffers(device_handle);

		// back to original settings
		SetCommState(device_handle, &OriginalDCB);
		SetCommTimeouts(device_handle, &OriginalTimeouts);

		CloseHandle(device_handle);
		device_handle = INVALID_HANDLE_VALUE;
	}
}

void CSerialPort::flushRx()
{	// flush the receiver out
	char buf[128];
	DWORD num_of_bytes = 0;

	if (overlapped_Read.hEvent != NULL)
	{
		while ((device_handle != INVALID_HANDLE_VALUE) && ReadFile(device_handle, buf, sizeof(buf), &num_of_bytes, &overlapped_Read) == TRUE)
			if (num_of_bytes == 0)
				break;
	}
	else
	{
		while ((device_handle != INVALID_HANDLE_VALUE) && ReadFile(device_handle, buf, sizeof(buf), &num_of_bytes, NULL) == TRUE)
			if (num_of_bytes == 0)
				break;
	}
}

int CSerialPort::RxBytesAvailable()
{	// return number of bytes waiting to be read
	if (device_handle == INVALID_HANDLE_VALUE)
		return -1;	// not connected

	COMSTAT TempStat;
	DWORD TempDword;

	const BOOL res = ClearCommError(device_handle, &TempDword, &TempStat);
	if (res)
		return (int)TempStat.cbInQue;	// OK

	LastError = GetLastError();

	return -2;
}

int CSerialPort::RxBytes(void *buf, int num_of_bytes)
{
	if (device_handle == INVALID_HANDLE_VALUE)
		return -1;	// not connected

	DWORD ReadBytes = 0;
	COMSTAT TempStat;
	DWORD TempDword;

	BOOL res = ClearCommError(device_handle, &TempDword, &TempStat);
	if (res)
		ReadBytes = TempStat.cbInQue;	// OK

//	LastError = GetLastError();

	if (ReadBytes <= 0)
		return 0;

	if (num_of_bytes < (int)ReadBytes)
		num_of_bytes = ReadBytes;

	ReadBytes = 0;

	res = ReadFile(device_handle, buf, (DWORD)num_of_bytes, &ReadBytes, (overlapped_Read.hEvent != NULL) ? &overlapped_Read : NULL);
	if (!res)
	{
		LastError = GetLastError();
		if (LastError != ERROR_IO_PENDING)
		{
			COMSTAT TempStat;
			DWORD TempDword;
			ClearCommError(device_handle, &TempDword, &TempStat);
			ReadBytes = 0;
		}
		else
		{
			if (overlapped_Read.hEvent != NULL)
				WaitForSingleObject(overlapped_Read.hEvent, 1);
		}
	}

	return (ReadBytes > 0) ? (int)ReadBytes : 0;
}

int CSerialPort::TxBytesWaiting()
{	// return number of bytes waiting to be sent
	if (device_handle == INVALID_HANDLE_VALUE)
		return -1;	// not connected

	COMSTAT TempStat;
	DWORD TempDword;

	return (ClearCommError(device_handle, &TempDword, &TempStat)) ? (int)TempStat.cbOutQue : 0;
}

bool CSerialPort::TxChar(char c)
{
	if (device_handle == INVALID_HANDLE_VALUE)
		return false;	// not connected

	unsigned long BytesWritten = 0;

	const BOOL res = WriteFile(device_handle, &c, 1, &BytesWritten, (overlapped_Read.hEvent != NULL) ? &overlapped_Write : NULL);
	if (!res)
	{
		LastError = GetLastError();
		//	GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));

		if (overlapped_Write.hEvent == NULL)
			return false;

		if (LastError == ERROR_IO_PENDING)
		{
			if (WaitForSingleObject(overlapped_Write.hEvent, 1000))
			{
				BytesWritten = 0;
				return false;
			}
			GetOverlappedResult(device_handle, &overlapped_Write, &BytesWritten, FALSE);
			overlapped_Write.Offset += BytesWritten;
		}
	}

	return true;
}

int CSerialPort::TxBytes(void *buf, int bytes)
{
	if (device_handle == INVALID_HANDLE_VALUE)
		return -1;	// not connected

	if (buf == NULL || bytes <= 0)
		return 0;

	int i = 0;
	int Fails = 0;
	unsigned char *data = (unsigned char *)buf;

	while (i < bytes)
	{
		if (device_handle == INVALID_HANDLE_VALUE)
			return -2;	// not connected

		unsigned long BytesWritten = 0;
		DWORD j = bytes - i;

		const BOOL res = WriteFile(device_handle, data + i, j, &BytesWritten, (overlapped_Write.hEvent != NULL) ? &overlapped_Write : NULL);
		if (!res)
		{
			LastError = GetLastError();
			//	GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));

			if (overlapped_Write.hEvent == NULL)
			{
				COMSTAT TempStat;
				DWORD TempDword;
				ClearCommBreak(device_handle);
				ClearCommError(device_handle, &TempDword, &TempStat);
				BytesWritten = 0;
				if (++Fails >= MaxFails)
					return -3;
			}
			else
			if (LastError == ERROR_IO_PENDING)
			{
				if (WaitForSingleObject(overlapped_Write.hEvent, 1000))
				{
					COMSTAT TempStat;
					DWORD TempDword;
					ClearCommBreak(device_handle);
					ClearCommError(device_handle, &TempDword, &TempStat);
					BytesWritten = 0;
					if (++Fails >= MaxFails)
						return -3;
				}
				else
				{
					GetOverlappedResult(device_handle, &overlapped_Write, &BytesWritten, FALSE);
					overlapped_Write.Offset += BytesWritten;
					if (--Fails < 0)
						Fails = 0;
				}
			}
		}
		i += BytesWritten;
	}

	return i;
}

int CSerialPort::TxStr(char *s)
{
	if (device_handle == INVALID_HANDLE_VALUE)
		return -1;	// not connected

	if (s == NULL)
		return -2;

	int len = (int)strlen(s);
	if (len <= 0)
		return -3;

	return TxBytes(s, len);
}

void CSerialPort::Flush()
{
	if (device_handle != INVALID_HANDLE_VALUE)
		FlushFileBuffers(device_handle);
}

char * CSerialPort::GetDeviceName()
{
	return device_name;
}

HANDLE CSerialPort::GetDeviceHandle()
{
	return device_handle;
}

int CSerialPort::GetReceiveQueue()
{
	return ReceiveQueue;
}

void CSerialPort::SetReceiveQueue(int NewReceiveQueue)
{
	if (device_handle != INVALID_HANDLE_VALUE)
	{
		if (ReceiveQueue != NewReceiveQueue)
		{
			Disconnect();
			ReceiveQueue = NewReceiveQueue;
			Connect(device_name);
		}
	}
	else
		ReceiveQueue = NewReceiveQueue;
}

int CSerialPort::GetTxQueue()
{
	return TransmitQueue;
}

void CSerialPort::SetTxQueue(int NewTransmitQueue)
{
	if (device_handle != INVALID_HANDLE_VALUE)
	{
		if (TransmitQueue != NewTransmitQueue)
		{
			Disconnect();
			TransmitQueue = NewTransmitQueue;
			Connect(device_name);
		}
	}
	else
		TransmitQueue = NewTransmitQueue;
}

void CSerialPort::SetMaxFails(int NewMaxFails)
{
	if (device_handle != INVALID_HANDLE_VALUE)
	{
		Disconnect();
		MaxFails = NewMaxFails;
		Connect(device_name);
	}
	else
		MaxFails = NewMaxFails;
}

int CSerialPort::GetMaxFails()
{
	return MaxFails;
}

int CSerialPort::GetBaudRate()
{
	return MyDCB.BaudRate;
}

void CSerialPort::SetBaudRate(int NewBaudRate)
{
	if (device_handle != INVALID_HANDLE_VALUE)
	{
//		OriginalDCB.BaudRate = NewBaudRate;
//		MyDCB.BaudRate       = NewBaudRate;
//
//		SetCommState(device_handle, &MyDCB);

		if ((int)MyDCB.BaudRate != NewBaudRate)
		{
			Disconnect();
			MyDCB.BaudRate = NewBaudRate;
			Connect(device_name);
		}
	}
	else
		MyDCB.BaudRate = NewBaudRate;
}

void CSerialPort::SetRTS(bool set)
{
	if (device_handle == INVALID_HANDLE_VALUE)
		return;	// not connected

	if (set)
	{
		if (!EscapeCommFunction(device_handle, SETRTS))
		{
			LastError = GetLastError();
//			GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));
		}
	}
	else
	{
		if (!EscapeCommFunction(device_handle, CLRRTS))
		{
			LastError = GetLastError();
//			GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));
		}
	}
}

void CSerialPort::SetDTR(bool set)
{
	if (device_handle == INVALID_HANDLE_VALUE)
		return;	// not connected

	if (set)
	{
		if (!EscapeCommFunction(device_handle, SETDTR))
		{
			LastError = GetLastError();
//			GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));
		}
	}
	else
	{
		if (!EscapeCommFunction(device_handle, CLRDTR))
		{
			LastError = GetLastError();
//			GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));
		}
	}
}

bool CSerialPort::GetCTS()
{
	if (device_handle == INVALID_HANDLE_VALUE)
		return false;	// not connected

	if (!GetCommModemStatus(device_handle, &ModemState))
	{
		LastError = GetLastError();
//		GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));
		return false;
	}

	return (ModemState & MS_CTS_ON) ? true : false;
}

bool CSerialPort::GetDSR()
{
	if (device_handle == INVALID_HANDLE_VALUE)
		return false;	// not connected

	if (!GetCommModemStatus(device_handle, &ModemState))
	{
		LastError = GetLastError();
//		GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));
		return false;
	}

	return (ModemState & MS_DSR_ON) ? true : false;
}

bool CSerialPort::GetRING()
{
	if (device_handle == INVALID_HANDLE_VALUE)
		return false;	// not connected

	if (!GetCommModemStatus(device_handle, &ModemState))
	{
		LastError = GetLastError();
//		GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));
		return false;
	}

	return (ModemState & MS_RING_ON) ? true : false;
}

bool CSerialPort::GetRLSD()
{
	if (device_handle == INVALID_HANDLE_VALUE)
		return false;	// not connected

	if (!GetCommModemStatus(device_handle, &ModemState))
	{
		LastError = GetLastError();
//		GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));
		return false;
	}

	return (ModemState & MS_RLSD_ON) ? true : false;
}

#ifdef SERIAL_OVERLAPPED
	bool CSerialPort::GetBreak()
	{
		if (device_handle == INVALID_HANDLE_VALUE)
			return false;	// not connected

		DWORD event = 0;
		if (!WaitCommEvent(device_handle, &event, &overlapped_Read))
		{
			LastError = GetLastError();
//			GetLastErrorStr(LastError, LastErrorStr, sizeof(LastErrorStr));
			return false;
		}

		if (event & EV_ERR)
		{
			DWORD errors;
			COMSTAT stat;
			if (ClearCommError(device_handle, &errors, &stat))
			{
				if (errors & CE_BREAK)    return true;
//				if (errors & CE_FRAME)    return false;
//				if (errors & CE_OVERRUN)  return false;
//				if (errors & CE_RXOVER)   return false;
//				if (errors & CE_RXPARITY) return false;
			}
		}

//		return (event & EV_RING)     ? true : false;
//		return (event & EV_RXCHAR)   ? true : false;
		return (event & EV_BREAK)    ? true : false;
//		return (event & EV_CTS)      ? true : false;
//		return (event & & EV_DSR)    ? true : false;
//		return (event & & EV_RLSD)   ? true : false;
//		return (event & & EV_RXCHAR) ? true : false;
//		return (event & & EV_RXFLAG) ? true : false;
//		return (event & EV_TXEMPTY)  ? true : false;
	}
#endif

int CSerialPort::GetByteSize()
{
	return MyDCB.ByteSize;
}

void CSerialPort::SetByteSize(int NewByteSize)
{
	if (device_handle != INVALID_HANDLE_VALUE)
	{
//		OriginalDCB.ByteSize = NewByteSize;
//		MyDCB.ByteSize       = NewByteSize;
//		SetCommState(device_handle, &MyDCB);

		if (MyDCB.ByteSize != (BYTE)NewByteSize)
		{
			Disconnect();
			MyDCB.ByteSize = (BYTE)NewByteSize;
			Connect(device_name);
		}
	}
	else
		MyDCB.ByteSize = (BYTE)NewByteSize;
}

int CSerialPort::GetParity()
{
	return MyDCB.Parity;
}

void CSerialPort::SetParity(int NewParity)
{
	if (device_handle != INVALID_HANDLE_VALUE)
	{
//		OriginalDCB.Parity = NewParity;
//		MyDCB.Parity = NewParity;
//		SetCommState(device_handle, &MyDCB);

		if (MyDCB.Parity != (BYTE)NewParity)
		{
			Disconnect();
			MyDCB.Parity = (BYTE)NewParity;
			Connect(device_name);
		}
	}
	else
		MyDCB.Parity = (BYTE)NewParity;
}

int CSerialPort::GetStopBits()
{
	return MyDCB.StopBits;
}

void CSerialPort::SetStopBits(int NewStopBits)
{
	if (device_handle != INVALID_HANDLE_VALUE)
	{
//		OriginalDCB.StopBits = NewStopBits;
//		MyDCB.StopBits = NewStopBits;
//		SetCommState(device_handle, &MyDCB);

		if (MyDCB.StopBits != (BYTE)NewStopBits)
		{
			Disconnect();
			MyDCB.StopBits = (BYTE)NewStopBits;
			Connect(device_name);
		}
	}
	else
		MyDCB.StopBits = (BYTE)NewStopBits;
}

int CSerialPort::GetReadIntervalTimeout()
{
	return MyTimeouts.ReadIntervalTimeout;
}

void CSerialPort::SetReadIntervalTimeout(int NewReadIntervalTimeout)
{
	if (device_handle != INVALID_HANDLE_VALUE)
	{
//		OriginalTimeouts.ReadIntervalTimeout = NewReadIntervalTimeout;
//		MyTimeouts.ReadIntervalTimeout = NewReadIntervalTimeout;
//		SetCommTimeouts(device_handle, &MyTimeouts);

		if ((int)MyTimeouts.ReadIntervalTimeout != NewReadIntervalTimeout)
		{
			Disconnect();
			MyTimeouts.ReadIntervalTimeout = NewReadIntervalTimeout;
			Connect(device_name);
		}
	}
	else
		MyTimeouts.ReadIntervalTimeout = NewReadIntervalTimeout;
}

int CSerialPort::GetReadTotalTimeoutMultiplier()
{
	return MyTimeouts.ReadTotalTimeoutMultiplier;
}

void CSerialPort::SetReadTotalTimeoutMultiplier(int NewReadTotalTimeoutMultiplier)
{
	if (device_handle != INVALID_HANDLE_VALUE)
	{
//		OriginalTimeouts.ReadTotalTimeoutMultiplier = NewReadTotalTimeoutMultiplier;
//		MyTimeouts.ReadTotalTimeoutMultiplier = NewReadTotalTimeoutMultiplier;
//		SetCommTimeouts(device_handle, &MyTimeouts);

		if ((int)MyTimeouts.ReadTotalTimeoutMultiplier != NewReadTotalTimeoutMultiplier)
		{
			Disconnect();
			MyTimeouts.ReadTotalTimeoutMultiplier = NewReadTotalTimeoutMultiplier;
			Connect(device_name);
		}
	}
	else
		MyTimeouts.ReadTotalTimeoutMultiplier = NewReadTotalTimeoutMultiplier;
}

int CSerialPort::GetReadTotalTimeoutConstant()
{
	return MyTimeouts.ReadTotalTimeoutConstant;
}

void CSerialPort::SetReadTotalTimeoutConstant(int NewReadTotalTimeoutConstant)
{
	if (device_handle != INVALID_HANDLE_VALUE)
	{
//		OriginalTimeouts.ReadTotalTimeoutConstant = NewReadTotalTimeoutConstant;
//		MyTimeouts.ReadTotalTimeoutConstant = NewReadTotalTimeoutConstant;
//		SetCommTimeouts(device_handle, &MyTimeouts);

		if ((int)MyTimeouts.ReadTotalTimeoutConstant != NewReadTotalTimeoutConstant)
		{
			Disconnect();
			MyTimeouts.ReadTotalTimeoutConstant = NewReadTotalTimeoutConstant;
			Connect(device_name);
		}
	}
	else
		MyTimeouts.ReadTotalTimeoutConstant = NewReadTotalTimeoutConstant;
}

int CSerialPort::GetWriteTotalTimeoutMultiplier()
{
	return MyTimeouts.WriteTotalTimeoutMultiplier;
}

void CSerialPort::SetWriteTotalTimeoutMultiplier(int NewWriteTotalTimeoutMultiplier)
{
	if (device_handle != INVALID_HANDLE_VALUE)
	{
//		OriginalTimeouts.WriteTotalTimeoutMultiplier = NewWriteTotalTimeoutMultiplier;
//		MyTimeouts.WriteTotalTimeoutMultiplier = NewWriteTotalTimeoutMultiplier;
//		SetCommTimeouts(device_handle, &MyTimeouts);

		if ((int)MyTimeouts.WriteTotalTimeoutMultiplier != NewWriteTotalTimeoutMultiplier)
		{
			Disconnect();
			MyTimeouts.WriteTotalTimeoutMultiplier = NewWriteTotalTimeoutMultiplier;
			Connect(device_name);
		}
	}
	else
		MyTimeouts.WriteTotalTimeoutMultiplier = NewWriteTotalTimeoutMultiplier;
}

int CSerialPort::GetWriteTotalTimeoutConstant()
{
	return MyTimeouts.WriteTotalTimeoutConstant;
}

void CSerialPort::SetWriteTotalTimeoutConstant(int NewWriteTotalTimeoutConstant)
{
	if (device_handle != INVALID_HANDLE_VALUE)
	{
//		OriginalTimeouts.WriteTotalTimeoutConstant = NewWriteTotalTimeoutConstant;
//		MyTimeouts.WriteTotalTimeoutConstant = NewWriteTotalTimeoutConstant;
//		SetCommTimeouts(device_handle, &MyTimeouts);

		if ((int)MyTimeouts.WriteTotalTimeoutConstant != NewWriteTotalTimeoutConstant)
		{
			Disconnect();
			MyTimeouts.WriteTotalTimeoutConstant = NewWriteTotalTimeoutConstant;
			Connect(device_name);
		}
	}
	else
		MyTimeouts.WriteTotalTimeoutConstant = NewWriteTotalTimeoutConstant;
}

// ********************************************

