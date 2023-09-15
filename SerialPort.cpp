
/*
 * Use at your own risk
 *
 *
 * This program is licensed under the GNU GENERAL PUBLIC LICENSE v3
 * License text avaliable at: http://www.gnu.org/copyleft/gpl.html 
 */

/*
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef _MSC_VER
	#include "StdAfx.h"
#endif

#include <string.h>
#include <stdio.h>

#ifdef __BORLANDC__
	#pragma hdrstop
#endif

#ifdef _DEBUG
	#ifdef _MSC_VER
		#define new DEBUG_NEW
		#undef THIS_FILE
		static char THIS_FILE[] = __FILE__;
	#endif
#endif

#include "SerialPort.h"

#ifdef __BORLANDC__
	#pragma package(smart_init)
#endif

#include <stdint.h>

// ***********************************************************

void __fastcall GetLastErrorStr(DWORD err, char *err_str, int max_size)
{
	if (!err_str || max_size <= 0)
		return;

	memset(err_str, 0, max_size);

	char *buf = NULL;

	WORD prevErrorMode = ::SetErrorMode(SEM_FAILCRITICALERRORS);	// ignore critical errors

	HMODULE wnet_handle = ::GetModuleHandle(_T("wininet.dll"));

	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM;
	if (wnet_handle) flags |= FORMAT_MESSAGE_FROM_HMODULE;		// retrieve message from specified DLL

	DWORD res = ::FormatMessageA(flags, wnet_handle, err, 0, (LPSTR)&buf, 0, NULL);

	if (wnet_handle)
		::FreeLibrary(wnet_handle);

	if (buf != NULL)
	{
		#if (__BORLANDC__ < 0x0600)
			if (res > 0)
				sprintf(err_str, "[%d] %s", err, buf);
		#else
			if (res > 0)
				sprintf_s(err_str, max_size, "[%d] %s", err, buf);
		#endif

		::LocalFree(buf);
	}

	::SetErrorMode(prevErrorMode);
}

// ********************************************

CSerialPort::CSerialPort()
{
	m_device_handle = INVALID_HANDLE_VALUE;
	m_mutex         = ::CreateMutex(NULL, TRUE, NULL);
	m_thread        = NULL;

	m_last_error = ERROR_SUCCESS;
	memset(m_last_error_str, 0, sizeof(m_last_error_str));

	memset(&m_rx.overlapped, 0, sizeof(OVERLAPPED));
	memset(&m_tx.overlapped, 0, sizeof(OVERLAPPED));
	m_rx.overlapped.hEvent  = INVALID_HANDLE_VALUE;
	m_tx.overlapped.hEvent  = INVALID_HANDLE_VALUE;
	m_rx.overlapped_waiting = false;
	m_tx.overlapped_waiting = false;

	GetSerialPortList();

	memset(&m_dcb, 0, sizeof(m_dcb));
	m_dcb.DCBlength     = sizeof(m_dcb);
	m_dcb.BaudRate      = 115200;
	m_dcb.fBinary       = true;
	m_dcb.fParity       = false;
	m_dcb.fOutxCtsFlow  = false;
	m_dcb.fOutxDsrFlow  = false;
	m_dcb.fDtrControl   = DTR_CONTROL_ENABLE;	// DTR_CONTROL_DISABLE, DTR_CONTROL_ENABLE, DTR_CONTROL_HANDSHAKE
//	m_dcb.fDsrSensitivity:1;   // DSR sensitivity
//	m_dcb.fTXContinueOnXoff:1; // XOFF continues Tx
	m_dcb.fOutX         = false;
	m_dcb.fInX          = false;
//	m_dcb.fErrorChar: 1;       // enable error replacement
	m_dcb.fNull         = false;
	m_dcb.fRtsControl   = RTS_CONTROL_ENABLE;	// RTS_CONTROL_DISABLE, RTS_CONTROL_ENABLE, RTS_CONTROL_HANDSHAKE
	m_dcb.fAbortOnError = false;
//	m_dcb.fDummy2:17;          // reserved
//	m_dcb.XonLim;
//	m_dcb.XoffLim;
	m_dcb.ByteSize      = 8;	// 8 bits per byte - when was it ever any different ??????
	m_dcb.Parity        = NOPARITY;
	m_dcb.StopBits      = ONESTOPBIT;
//	m_dcb.XonChar;
//	m_dcb.XoffChar;
//	m_dcb.ErrorChar;
//	m_dcb.EofChar;
//	m_dcb.EvtChar;

	memset(&m_timeouts, 0, sizeof(m_timeouts));
	m_timeouts.ReadIntervalTimeout           = MAXDWORD;
	m_timeouts.ReadTotalTimeoutMultiplier    = 0;
	m_timeouts.ReadTotalTimeoutConstant      = 0;
	m_timeouts.WriteTotalTimeoutMultiplier   = 0;
	m_timeouts.WriteTotalTimeoutConstant     = 0;

	m_rx.queue_size = 32768;
	m_tx.queue_size = 32768;

	m_buffer.resize(m_rx.queue_size);

	m_rx.buffer.resize(1000000);
	m_rx.buffer_rd = 0;
	m_rx.buffer_wr = 0;

	m_tx.buffer.resize(1000000);
	m_tx.buffer_rd = 0;
	m_tx.buffer_wr = 0;

	m_rx.overlapped_waiting = false;
	m_tx.overlapped_waiting = false;

/*
	#ifdef USE_THREAD
		m_thread = new CSerialPortThread(&threadProcess);
	#else
		m_thread = NULL;
	#endif
*/
}

CSerialPort::~CSerialPort()
{
	if (m_thread != NULL)
	{
		if (!m_thread->FreeOnTerminate)
		{
			m_thread->Terminate();
			m_thread->WaitFor();
			delete m_thread;
		}
		else
			m_thread->Terminate();
		m_thread = NULL;
	}

	Disconnect();

	if (m_mutex != NULL)
		::CloseHandle(m_mutex);
	m_mutex = NULL;
}

void __fastcall CSerialPort::clearErrors()
{
	#ifdef USE_THREAD
		const DWORD res = ::WaitForSingleObject(m_mutex, 100);
	#endif

	m_errors.resize(0);

	#ifdef USE_THREAD
		if (res == WAIT_OBJECT_0)
			::ReleaseMutex(m_mutex);
	#endif
}

void __fastcall CSerialPort::pushError(const DWORD error, STRING leading_text)
{
	#ifdef USE_THREAD
		const DWORD res = ::WaitForSingleObject(m_mutex, 10);
	#endif

	m_last_error = error;
	GetLastErrorStr(error, m_last_error_str, sizeof(m_last_error_str));
	m_errors.push_back(leading_text + STRING(m_last_error_str));

	#ifdef USE_THREAD
		if (res == WAIT_OBJECT_0)
			::ReleaseMutex(m_mutex);
	#endif
}

unsigned int __fastcall CSerialPort::errorCount()
{
	#ifdef USE_THREAD
		const DWORD res = ::WaitForSingleObject(m_mutex, 10);
	#endif

	const unsigned int size = m_errors.size();

	#ifdef USE_THREAD
		if (res == WAIT_OBJECT_0)
			::ReleaseMutex(m_mutex);
	#endif

	return size;
}

STRING __fastcall CSerialPort::pullError()
{
	STRING s;

	#ifdef USE_THREAD
		const DWORD res = ::WaitForSingleObject(m_mutex, 100);
	#endif

	if (!m_errors.empty())
	{
		s = m_errors[0];
		m_errors.erase(m_errors.begin() + 0);
	}

	#ifdef USE_THREAD
		if (res == WAIT_OBJECT_0)
			::ReleaseMutex(m_mutex);
	#endif

	return s;
}

void __fastcall CSerialPort::getState()
{
	if (m_device_handle == INVALID_HANDLE_VALUE)
		return;

	if (::GetCommModemStatus(m_device_handle, &m_modem_state_read) == FALSE)
		pushError(::GetLastError(), "GetCommModemStatus ");

	if (::GetCommState(m_device_handle, &m_dcb) == FALSE)
		pushError(::GetLastError(), "GetCommState ");

	if (::ClearCommError(m_device_handle, &m_errors_read, &m_stat_read) == FALSE)
		pushError(::GetLastError(), "ClearCommError ");
	else
	{
		m_rx_break = (m_errors_read & CE_BREAK) ? true : false;
	}
}

void __fastcall CSerialPort::GetSerialPortList()
{  // enumerate the com-ports - non-WMI
	#define MAX_KEY_LENGTH 255
	#define MAX_VALUE_NAME 16383

	DWORD err;

	m_serialPortList.clear();

	HKEY hKey = NULL;

	err = ::RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_QUERY_VALUE, &hKey);
	if (err != ERROR_SUCCESS || hKey == NULL)
	{
		pushError(err, "reg ");
		if (hKey != NULL)
			::RegCloseKey(hKey);
		return;
	}

//	TCHAR    achKey[MAX_KEY_LENGTH];		// buffer for subkey name
//	DWORD    cbName;							// size of name string
	char     achClass[MAX_PATH];			// buffer for class name
	DWORD    cchClassName = MAX_PATH;	// size of class string
	DWORD    cSubKeys = 0;					// number of subkeys
	DWORD    cbMaxSubKey;					// longest subkey size
	DWORD    cchMaxClass;					// longest class string
	DWORD    cValues;							// number of values for key
	DWORD    cchMaxValue;					// longest value name
	DWORD    cbMaxValueData;				// longest value data
	DWORD    cbSecurityDescriptor;		// size of security descriptor
	FILETIME ftLastWriteTime;				// last write time

	char achValue[MAX_VALUE_NAME];
	DWORD cchValue = MAX_VALUE_NAME;

	// get the class name and the value count
	err = ::RegQueryInfoKeyA(
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
	if (err != ERROR_SUCCESS)
	{
		pushError(err, "reg ");
		::RegCloseKey(hKey);
		return;
	}

	if (cValues <= 0)
	{	// no serial ports
		::RegCloseKey(hKey);
		return;
	}

	for (int i = 0; i < (int)cValues; i++)
	{
		cchValue = MAX_VALUE_NAME;
		achValue[0] = '\0';

		err = ::RegEnumValueA(hKey, i, achValue, &cchValue, NULL, NULL, NULL, NULL);
		if (err != ERROR_SUCCESS)
		{
			pushError(err, "reg ");
			break;
//			continue;
		}

		DWORD type;
		char str[MAX_PATH];
		DWORD size = sizeof(str);

		err = ::RegQueryValueExA(hKey, achValue, NULL, &type, (BYTE *)str, &size);
		if (err != ERROR_SUCCESS)
		{
			pushError(err, "reg ");
			break;
//			continue;
		}

		// check to see if we already have it
		unsigned int k = 0;
		while (k < m_serialPortList.size())
		{
			if (STRING(m_serialPortList[k].name).LowerCase() == STRING(str).LowerCase())
				break;	// already in the list
			k++;
		}

		if (k >= m_serialPortList.size())
		{	// save it into our list
			T_SerialPortInfo serialPortInfo;
			memset(&serialPortInfo, 0, sizeof(T_SerialPortInfo));
			#if (__BORLANDC__ < 0x0600)
				strcpy(serialPortInfo.name, str);
				strcpy(serialPortInfo.deviceID, achValue);
			#else
				strcpy_s(serialPortInfo.name, sizeof(serialPortInfo.name), str);
				strcpy_s(serialPortInfo.deviceID, sizeof(serialPortInfo.deviceID), achValue);
			#endif
			m_serialPortList.push_back(serialPortInfo);
		}
	}

	::RegCloseKey(hKey);
}

void __fastcall CSerialPort::GetSerialPortList(std::vector <T_SerialPortInfo> &serialPortList)
{
	GetSerialPortList();

	serialPortList.clear();

	for (int i = 0; i < (int)m_serialPortList.size(); i++)
		serialPortList.push_back(m_serialPortList[i]);
}

bool __fastcall CSerialPort::Connected()
{
	if (m_device_handle == INVALID_HANDLE_VALUE)
		return false;

	if (::ClearCommError(m_device_handle, &m_errors_read, &m_stat_read) == FALSE)
	{	// close the comm port

		const DWORD err = ::GetLastError();
		if (err != ERROR_INVALID_FUNCTION)
		{
//			char err_str[256];
//			GetLastErrorStr(err, err_str, sizeof(err_str));

			pushError(err, "connected ClearCommError ");

//			if (err == ERROR_ACCESS_DENIED)
//			{	// tends to mean they have unplugged the USB serial port, or we're no longer connected
//			}

			Disconnect();
		}
	}
	else
	{
		m_rx_break = (m_errors_read & CE_BREAK) ? true : false;
	}

	return (m_device_handle != INVALID_HANDLE_VALUE) ? true : false;
}

int __fastcall CSerialPort::Connect(STRING device_name, const bool use_overlapped)
{
	if (m_device_handle != INVALID_HANDLE_VALUE)
		Disconnect();

	clearErrors();

	memset(&m_rx.overlapped, 0, sizeof(m_rx.overlapped));
	memset(&m_tx.overlapped, 0, sizeof(m_tx.overlapped));
	m_rx.overlapped.hEvent  = INVALID_HANDLE_VALUE;
	m_tx.overlapped.hEvent  = INVALID_HANDLE_VALUE;
	m_rx.overlapped_waiting = false;
	m_tx.overlapped_waiting = false;

	m_rx.buffer_rd = 0;
	m_rx.buffer_wr = 0;

	m_tx.buffer_rd = 0;
	m_tx.buffer_wr = 0;

	m_rx_break = false;

	if (device_name.IsEmpty())
		return -1;

	m_device_name = device_name;

	#if defined(__BORLANDC__)
		STRING name = (device_name.Pos("\\\\") != 1) ? STRING("\\\\.\\") + device_name : device_name;
	#elif defined(_MSC_VER)
		// this needs fixing for MS Visual Studio
		STRING name = (device_name.Pos("\\\\") != 0) ? STRING("\\\\.\\") + device_name : device_name;
	#endif

	#if defined(__BORLANDC__)
		#ifdef SERIAL_OVERLAPPED
			if (use_overlapped)
				m_device_handle = ::CreateFileA(name.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0 | FILE_FLAG_OVERLAPPED, NULL);
			else
				m_device_handle = ::CreateFileA(name.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		#else
			m_device_handle = ::CreateFileA(name.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		#endif
	#elif defined(_MSC_VER)
		#ifdef SERIAL_OVERLAPPED
			if (use_overlapped)
				m_device_handle = ::CreateFileA(name.GetString(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0 | FILE_FLAG_OVERLAPPED, NULL);
			else
				m_device_handle = ::CreateFileA(name.GetString(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		#else
			m_device_handle = ::CreateFileA(name.GetString(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		#endif
	#else
		#error "FIX ME"
	#endif

	if (m_device_handle == INVALID_HANDLE_VALUE)
	{
		pushError(::GetLastError(), "connect CreateFile ");
		return m_last_error;
	}

	if (::GetCommState(m_device_handle, &m_original_dcb) == FALSE)
		pushError(::GetLastError(), "connect GetCommState ");

	if (::GetCommTimeouts(m_device_handle, &m_original_timeouts) == FALSE)
		pushError(::GetLastError(), "connect GetCommTimeouts ");

	if (::GetCommModemStatus(m_device_handle, &m_modem_state_read) == FALSE)
		pushError(::GetLastError(), "connect GetCommModemStatus ");

	#if 1
		::SetCommState(m_device_handle, &m_dcb);
	#else
		if (::SetCommState(m_device_handle, &m_dcb) == FALSE)
			pushError(::GetLastError(), "connect SetCommState ");
	#endif

	if (::SetCommTimeouts(m_device_handle, &m_timeouts) == FALSE)
		pushError(::GetLastError(), "connect SetCommTimeouts ");

	if (::SetupComm(m_device_handle, m_rx.queue_size, m_tx.queue_size) == FALSE)
		pushError(::GetLastError(), "connect SetupComm ");

	#ifdef SERIAL_OVERLAPPED
		if (use_overlapped)
		{
			m_rx.overlapped.hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
			if (m_rx.overlapped.hEvent == NULL)
				pushError(::GetLastError(), "connect CreateEvent ");

			m_tx.overlapped.hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
			if (m_tx.overlapped.hEvent == NULL)
				pushError(::GetLastError(), "connect CreateEvent ");

/*			//if (::SetCommMask(m_device_handle, EV_TXEMPTY | EV_RXCHAR) == FALSE)
			if (::SetCommMask(m_device_handle, EV_RXCHAR | EV_BREAK) == FALSE)
				pushError(::GetLastError(), "connect SetCommMask ");
*/
		}
	#endif

	// flush the Tx/Rx buffers
	if (::PurgeComm(m_device_handle, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR) == FALSE)
		pushError(::GetLastError(), "connect PurgeComm ");

	if (::ClearCommError(m_device_handle, &m_errors_read, &m_stat_read) == FALSE)
		pushError(::GetLastError(), "connect ClearCommError ");
	else
	{
		m_rx_break = (m_errors_read & CE_BREAK) ? true : false;
	}

	#ifdef USE_THREAD
		m_thread = new CSerialPortThread(&threadProcess);
	#endif

	return ERROR_SUCCESS;	// OK
}

void __fastcall CSerialPort::Disconnect()
{
	if (m_thread != NULL)
	{
		if (!m_thread->FreeOnTerminate)
		{
			m_thread->Terminate();
			m_thread->WaitFor();
			delete m_thread;
		}
		else
			m_thread->Terminate();
		m_thread = NULL;
	}

	const HANDLE handle = m_device_handle;
	m_device_handle = INVALID_HANDLE_VALUE;	// stops the thread accessing the serial port

	m_rx.buffer_rd = 0;
	m_rx.buffer_wr = 0;

	m_tx.buffer_rd = 0;
	m_tx.buffer_wr = 0;

	if (m_rx.overlapped.hEvent != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(m_rx.overlapped.hEvent);
		memset(&m_rx.overlapped, 0, sizeof(m_rx.overlapped));
		m_rx.overlapped.hEvent = INVALID_HANDLE_VALUE;
	}

	if (m_tx.overlapped.hEvent != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(m_tx.overlapped.hEvent);
		memset(&m_tx.overlapped, 0, sizeof(m_tx.overlapped));
		m_tx.overlapped.hEvent = INVALID_HANDLE_VALUE;
	}

	if (handle != INVALID_HANDLE_VALUE)
	{
		::PurgeComm(handle, PURGE_RXABORT | PURGE_RXCLEAR);
		::ClearCommError(handle, NULL, NULL);
//		::SetCommState(handle, &m_original_dcb);
		::SetCommTimeouts(handle, &m_original_timeouts);
		::ClearCommError(handle, NULL, NULL);
		::CloseHandle(handle);
	}
}

void __fastcall CSerialPort::processTx()
{
	if (m_device_handle == INVALID_HANDLE_VALUE)
		return;

	DWORD bytes_written = 0;

	const int wr = m_tx.buffer_wr;
	int rd       = m_tx.buffer_rd;
	int len      = (wr >= rd) ? wr - rd : m_tx.buffer.size() - rd;

	if (m_tx.overlapped.hEvent == INVALID_HANDLE_VALUE)
	{	// non-overlapped

		if (::ClearCommError(m_device_handle, &m_errors_read, &m_stat_read) == FALSE)
		{
			const DWORD err = ::GetLastError();
			if (err != ERROR_INVALID_FUNCTION)
				pushError(err, "tx ClearCommError ");
		}
		else
		{
			m_rx_break = (m_errors_read & CE_BREAK) ? true : false;
		}

		if (len > 0)
		{
			if (::WriteFile(m_device_handle, &m_tx.buffer[rd], len, &bytes_written, NULL) == FALSE)
			{
				const DWORD err = ::GetLastError();
				if (err != ERROR_INVALID_FUNCTION)
				{
					pushError(err, "tx WriteFile ");
					//::ClearCommBreak(m_device_handle);
					::ClearCommError(m_device_handle, &m_errors_read, &m_stat_read);
					return;
				}
			}
		}
	}
	else
	{	// overlapped

		if (!m_tx.overlapped_waiting)
		{
			if (::ClearCommError(m_device_handle, &m_errors_read, &m_stat_read) == FALSE)
			{
				const DWORD err = ::GetLastError();
				if (err != ERROR_INVALID_FUNCTION)
					pushError(err, "tx ClearCommError ");
			}
			else
			{
				m_rx_break = (m_errors_read & CE_BREAK) ? true : false;
			}

			if (len > 0)
			{
				if (::WriteFile(m_device_handle, &m_tx.buffer[rd], len, &bytes_written, &m_tx.overlapped) == FALSE)
				{
					const DWORD error = ::GetLastError();
					if (error != ERROR_INVALID_FUNCTION)
					{
						if (error != ERROR_IO_PENDING)
						{	// error
							//::ClearCommBreak(m_device_handle);
							::ClearCommError(m_device_handle, &m_errors_read, &m_stat_read);
							pushError(error, "tx WriteFile ");
							return;
						}
					}
					m_tx.overlapped_waiting = true;
				}
			}
		}

		if (m_tx.overlapped_waiting)
		{
			if (::GetOverlappedResult(m_device_handle, &m_tx.overlapped, &bytes_written, FALSE) == FALSE)
			{
				const DWORD error = ::GetLastError();
				::ClearCommError(m_device_handle, &m_errors_read, &m_stat_read);
				//if (error == ERROR_SEM_TIMEOUT)
				//	m_tx.overlapped_waiting = false;
				if (error != ERROR_IO_INCOMPLETE)
				{
					pushError(error, "tx GetOverlappedResult ");
					m_rx.overlapped_waiting = false;
				}
			}
			else
				m_tx.overlapped_waiting = false;
		}
	}

	if (bytes_written > 0)
	{
		rd += bytes_written;
		if (rd >= (int)m_tx.buffer.size())
			rd -= m_tx.buffer.size();
		m_tx.buffer_rd = rd;
	}
}

void __fastcall CSerialPort::processRx()
{
	if (m_device_handle == INVALID_HANDLE_VALUE)
		return;

	DWORD bytes_read = 0;

	if (m_rx.overlapped.hEvent == INVALID_HANDLE_VALUE)
	{	// non-overlapped

		if (::ClearCommError(m_device_handle, &m_errors_read, &m_stat_read) == FALSE)
		{
			const DWORD error = ::GetLastError();
			if (error != ERROR_INVALID_FUNCTION)
				pushError(error, "rx ClearCommError ");
		}
		else
		{
			m_rx_break = (m_errors_read & CE_BREAK) ? true : false;
		}
		
		const DWORD bytes_available = m_stat_read.cbInQue;

//		if (bytes_available > 0)
		{
			if (m_buffer.size() <= bytes_available)
				m_buffer.resize(bytes_available * 4);

			if (::ReadFile(m_device_handle, &m_buffer[0], m_buffer.size(), &bytes_read, NULL) == FALSE)
//			if (::ReadFile(m_device_handle, &m_buffer[0], bytes_available, &bytes_read, NULL) == FALSE)
			{
				pushError(::GetLastError(), "rx ReadFile ");
				::ClearCommError(m_device_handle, &m_errors_read, &m_stat_read);
				return;
			}
		}
	}
	else
	{	// overlapped

		if (!m_rx.overlapped_waiting)
		{
			if (::ClearCommError(m_device_handle, &m_errors_read, &m_stat_read) == FALSE)
			{
				const DWORD error = ::GetLastError();
				if (error != ERROR_INVALID_FUNCTION)
					pushError(error, "rx ClearCommError ");
			}
			else
			{
				m_rx_break  = (m_errors_read & CE_BREAK) ? true : false;
			}

			const DWORD bytes_available = m_stat_read.cbInQue;

//			if (bytes_available > 0)
			{
				if (m_buffer.size() <= bytes_available)
					m_buffer.resize(bytes_available * 4);

				if (::ReadFile(m_device_handle, &m_buffer[0], m_buffer.size(), &bytes_read, &m_rx.overlapped) == FALSE)
				{
					const DWORD error = ::GetLastError();
					if (error != ERROR_INVALID_FUNCTION)
					{
						if (error != ERROR_IO_PENDING)
						{
							::ClearCommError(m_device_handle, &m_errors_read, &m_stat_read);
							pushError(error, "rx ReadFile ");
							return;
						}
					}
					m_rx.overlapped_waiting = true;
				}
			}
		}

		if (m_rx.overlapped_waiting)
		{
			if (::GetOverlappedResult(m_device_handle, &m_rx.overlapped, &bytes_read, FALSE) == FALSE)
			{
				const DWORD error = ::GetLastError();
				::ClearCommError(m_device_handle, &m_errors_read, &m_stat_read);
				//if (error == ERROR_SEM_TIMEOUT)
				//	m_rx.overlapped_waiting = false;
				if (error != ERROR_IO_INCOMPLETE)
				{
					pushError(error, "rx GetOverlappedResult ");
					m_rx.overlapped_waiting = false;
				}
			}
			else
				m_rx.overlapped_waiting = false;
		}
	}

	if (bytes_read > 0)
	{	// copy the received bytes into our rx buffer
		int loops = 0;
		int rd = 0;
		while (loops++ < 100 && rd < (int)bytes_read)
		{
			int buf_rd = m_rx.buffer_rd;
			int buf_wr = m_rx.buffer_wr;
			int len = (buf_wr >= buf_rd) ? (int)m_rx.buffer.size() - buf_wr : buf_rd - buf_wr;
			if (len > ((int)bytes_read - rd))
				len = (int)bytes_read - rd;
			if (len > 0)
			{
				memcpy(&m_rx.buffer[buf_wr], &m_buffer[rd], len);
				rd += len;
				buf_wr += len;
				if (buf_wr >= (int)m_rx.buffer.size())
					buf_wr -= m_rx.buffer.size();
				m_rx.buffer_wr = buf_wr;
			}
			else
			{	// wait a bit for the exec to free up some buffer space
				Sleep(1);
			}
		}
	}
}

void __fastcall CSerialPort::threadProcess()
{
	if (m_device_handle == INVALID_HANDLE_VALUE || m_thread == NULL)
		return;

	processTx();
	processRx();
}

int __fastcall CSerialPort::RxBytesAvailable()
{	// return number of available received bytes
	if (m_device_handle == INVALID_HANDLE_VALUE)
		return -1;

#ifdef USE_THREAD
	const int wr = m_rx.buffer_wr;
	const int rd = m_rx.buffer_rd;
	return (wr >= rd) ? wr - rd : (m_rx.buffer.size() - rd) + wr;
#else

#endif
}

int __fastcall CSerialPort::RxByte()
{
	if (m_device_handle == INVALID_HANDLE_VALUE)
		return -1;

#ifdef USE_THREAD
	const int wr = m_rx.buffer_wr;

	int rd = m_rx.buffer_rd;
	if (rd == wr)
		return -2;

	const int i = m_rx.buffer[rd];
	if (++rd >= (int)m_rx.buffer.size())
		rd -= m_rx.buffer.size();

	m_rx.buffer_rd = rd;
#else


#endif

	return i;
}

int __fastcall CSerialPort::RxBytes(void *buf, int size)
{
	if (m_device_handle == INVALID_HANDLE_VALUE)
		return -1;

#ifdef USE_THREAD
	uint8_t *buffer = (uint8_t *)buf;
	int bytes_written = 0;

	while (bytes_written < size)
	{
		int buf_wr = m_rx.buffer_wr;
		int buf_rd = m_rx.buffer_rd;
		int len = (buf_wr >= buf_rd) ? buf_wr - buf_rd : m_rx.buffer.size() - buf_rd;
		if (len > (size - bytes_written))
			len = size - bytes_written;
		if (len <= 0)
			break;
		memcpy(&buffer[bytes_written], &m_rx.buffer[buf_rd], len);
		bytes_written += len;
		buf_rd        += len;
		if (buf_rd >= (int)m_rx.buffer.size())
			buf_rd -= m_rx.buffer.size();
		m_rx.buffer_rd = buf_rd;
	}

	return bytes_written;
#else


#endif
}

#ifdef USE_THREAD

int __fastcall CSerialPort::RxBytePeek()
{	// fetch a byte without removing it from the rx buffer
	if (m_device_handle == INVALID_HANDLE_VALUE)
		return -1;

	const int wr = m_rx.buffer_wr;

	int rd = m_rx.buffer_rd;
	if (rd == wr)
		return -2;

	return (int)m_rx.buffer[rd];
}

int __fastcall CSerialPort::RxBytesPeek(void *buf, int size)
{	// fetch bytes without removing them from the rx buffer
	if (m_device_handle == INVALID_HANDLE_VALUE)
		return -1;

	uint8_t *buffer = (uint8_t *)buf;
	int bytes_written = 0;

	int buf_rd = m_rx.buffer_rd;

	while (bytes_written < size)
	{
		int buf_wr = m_rx.buffer_wr;
		int len = (buf_wr >= buf_rd) ? buf_wr - buf_rd : m_rx.buffer.size() - buf_rd;
		if (len > (size - bytes_written))
			len = size - bytes_written;
		if (len <= 0)
			break;
		memcpy(&buffer[bytes_written], &m_rx.buffer[buf_rd], len);
		bytes_written += len;
		buf_rd        += len;
		if (buf_rd >= (int)m_rx.buffer.size())
			buf_rd -= m_rx.buffer.size();
	}

	return bytes_written;
}

bool __fastcall CSerialPort::TxEmpty()
{
	return (m_device_handle == INVALID_HANDLE_VALUE || m_tx.buffer_rd == m_tx.buffer_wr) ? true : false;
}

int __fastcall CSerialPort::TxBytesWaiting()
{	// return number of queued transmit bytes
	if (m_device_handle == INVALID_HANDLE_VALUE)
		return -1;

	const int rd = m_tx.buffer_rd;
	const int wr = m_tx.buffer_wr;
	const int bytes_available = (wr >= rd) ? wr - rd : (m_tx.buffer.size() - rd) + wr;

	return bytes_available;
}

#endif

int __fastcall CSerialPort::TxByte(const int b)
{
	if (m_device_handle == INVALID_HANDLE_VALUE)
		return -1;

#ifdef USE_THREAD
	const int buf_rd = m_tx.buffer_rd;
	int       buf_wr = m_tx.buffer_wr;
	int space_available = (buf_wr >= buf_rd) ? (int)m_tx.buffer.size() - (buf_wr - buf_rd) : buf_rd - buf_wr;
	if (space_available > 0)
		space_available--;
	if (space_available < 1)
		return 0;

	m_tx.buffer[buf_wr] = (uint8_t)b;
	if (++buf_wr >= (int)m_tx.buffer.size())
		buf_wr -= m_tx.buffer.size();

	m_tx.buffer_wr = buf_wr;
#else

#endif

	return 1;
}

int __fastcall CSerialPort::TxBytes(const void *buf, int bytes)
{
	if (m_device_handle == INVALID_HANDLE_VALUE)
		return -1;

	if (buf == NULL || bytes <= 0)
		return 0;

#ifdef USE_THREAD
	uint8_t *buffer = (uint8_t *)buf;

	const int buf_rd = m_tx.buffer_rd;
	int       buf_wr = m_tx.buffer_wr;

	int space_available = (buf_wr >= buf_rd) ? (int)m_tx.buffer.size() - (buf_wr - buf_rd) - 1 : buf_rd - buf_wr - 1;
	if (space_available <= 0)
		return 0;

	if (bytes > space_available)
		bytes = space_available;

	int bytes_written = 0;

	while (bytes_written < bytes)
	{
		int len = (buf_wr >= buf_rd) ? (int)m_tx.buffer.size() - buf_wr : buf_rd - buf_wr;
		if (len > (bytes - bytes_written))
			len = bytes - bytes_written;
		memcpy(&m_tx.buffer[buf_wr], &buffer[bytes_written], len);
		bytes_written += len;
		buf_wr        += len;
		if (buf_wr >= (int)m_tx.buffer.size())
			buf_wr -= m_tx.buffer.size();
	}

	m_tx.buffer_wr = buf_wr;

#else
#endif

	return bytes_written;
}

int __fastcall CSerialPort::TxBytes(const char *s)
{
	if (m_device_handle == INVALID_HANDLE_VALUE)
		return -1;

	if (s == NULL)
		return 0;

	const int len = (int)strlen(s);
	if (len <= 0)
		return 0;

	return TxBytes(s, len);
}

int __fastcall CSerialPort::TxBytes(const STRING s)
{
	if (m_device_handle == INVALID_HANDLE_VALUE)
		return -1;

	if (s.IsEmpty())
		return 0;

	#if defined(__BORLANDC__)
		return TxBytes(s.c_str(), s.Length());
	#elif defined(_MSC_VER)
		return TxBytes(s.GetString(), s.GetLength());
	#else
		#error "FIX ME"
	#endif
}

int __fastcall CSerialPort::GetBaudRate()
{
	getState();

	return m_dcb.BaudRate;
}

void __fastcall CSerialPort::SetBaudRate(int value)
{
	if (m_device_handle == INVALID_HANDLE_VALUE)
	{
		m_dcb.BaudRate = value;
		return;
	}

	if ((int)m_dcb.BaudRate == value)
		return;

	m_dcb.BaudRate = value;
	if (::SetCommState(m_device_handle, &m_dcb) == FALSE)
		pushError(::GetLastError(), "SetCommState ");

	getState();
}

void __fastcall CSerialPort::SetRTS(bool value)
{
	if (m_device_handle == INVALID_HANDLE_VALUE)
		return;

	if (::EscapeCommFunction(m_device_handle, value ? SETRTS : CLRRTS) == FALSE)
		pushError(::GetLastError(), "EscapeCommFunction ");

	getState();
}

void __fastcall CSerialPort::SetDTR(bool value)
{
	if (m_device_handle == INVALID_HANDLE_VALUE)
		return;

	if (::EscapeCommFunction(m_device_handle, value ? SETDTR : CLRDTR) == FALSE)
		pushError(::GetLastError(), "EscapeCommFunction ");

	getState();
}

bool __fastcall CSerialPort::GetCTS()
{
	if (m_device_handle == INVALID_HANDLE_VALUE)
		return false;

	getState();

	return (m_modem_state_read & MS_CTS_ON) ? true : false;
}

bool __fastcall CSerialPort::GetDSR()
{
	if (m_device_handle == INVALID_HANDLE_VALUE)
		return false;

	getState();

	return (m_modem_state_read & MS_DSR_ON) ? true : false;
}

bool __fastcall CSerialPort::GetRING()
{
	if (m_device_handle == INVALID_HANDLE_VALUE)
		return false;

	getState();

	return (m_modem_state_read & MS_RING_ON) ? true : false;
}

bool __fastcall CSerialPort::GetRLSD()
{
	if (m_device_handle == INVALID_HANDLE_VALUE)
		return false;

	getState();

	return (m_modem_state_read & MS_RLSD_ON) ? true : false;
}

int __fastcall CSerialPort::GetByteSize()
{
	getState();

	return m_dcb.ByteSize;
}

void __fastcall CSerialPort::SetByteSize(int value)
{
	if (m_device_handle == INVALID_HANDLE_VALUE)
	{
		m_dcb.ByteSize = (BYTE)value;
		return;
	}

	if (m_dcb.ByteSize == (BYTE)value)
		return;

	m_dcb.ByteSize = (BYTE)value;
	if (::SetCommState(m_device_handle, &m_dcb) == FALSE)
		pushError(::GetLastError(), "SetCommState ");

	getState();
}

int __fastcall CSerialPort::GetParity()
{
	getState();

	return m_dcb.Parity;
}

void __fastcall CSerialPort::SetParity(int value)
{
	if (m_device_handle == INVALID_HANDLE_VALUE)
	{
		m_dcb.Parity = (BYTE)value;
		return;
	}

	if (m_dcb.Parity == (BYTE)value)
		return;

	m_dcb.Parity = (BYTE)value;
	if (::SetCommState(m_device_handle, &m_dcb) == FALSE)
		pushError(::GetLastError(), "SetCommState ");

	getState();
}

int __fastcall CSerialPort::GetStopBits()
{
	getState();

	return m_dcb.StopBits;
}

void __fastcall CSerialPort::SetStopBits(int value)
{
	if (m_device_handle == INVALID_HANDLE_VALUE)
	{
		m_dcb.StopBits = (BYTE)value;
		return;
	}

	if (m_dcb.StopBits == (BYTE)value)
		return;

	m_dcb.StopBits = (BYTE)value;
	if (::SetCommState(m_device_handle, &m_dcb) == FALSE)
		pushError(::GetLastError(), "SetCommState ");

	getState();
}

// ********************************************

