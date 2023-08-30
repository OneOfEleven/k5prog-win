
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

#ifndef SerialPortH
#define SerialPortH

#pragma once

#define VC_EXTRALEAN
#define WIN32_EXTRA_LEAN
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <Classes.hpp>
#include <stdint.h>

#ifdef __BORLANDC__
	#include <system.hpp>	// for AnsiString
#endif

#include <vector>

#if defined(__BORLANDC__)
	#define STRING  AnsiString
#elif defined(_MSC_VER)
	#define STRING  CString
#else
	#error "FIX ME"
#endif

#define SERIAL_OVERLAPPED

typedef struct
{
	char name[MAX_PATH];
	char deviceID[MAX_PATH];
} T_SerialPortInfo;

typedef void __fastcall (__closure *serialPort_threadProcess)();

class CSerialPortThread : public TThread
{
	private:
		serialPort_threadProcess m_process;

	protected:
		void __fastcall Execute()
		{
			while (!Terminated)
			{
				Sleep(1);
				if (m_process)
					m_process();
			}
			ReturnValue = 0;
		}

	public:
		__fastcall CSerialPortThread(serialPort_threadProcess process) :
			TThread(false)
		{
			Priority        = tpNormal;
			FreeOnTerminate = false;
			m_process       = process;
		}
		virtual __fastcall ~CSerialPortThread()
		{
			m_process = NULL;
		}
};

class CSerialPort
{
private:
	HANDLE			m_device_handle;

	HANDLE			m_mutex;
	
	DWORD				m_last_error;
	char				m_last_error_str[256];

	STRING			m_device_name;

	DCB				m_original_dcb;
	DCB				m_dcb;

	COMMTIMEOUTS	m_original_timeouts;
	COMMTIMEOUTS	m_timeouts;

	DWORD				m_modem_state_read;

	COMSTAT			m_stat_read;
	DWORD				m_errors_read;

	std::vector <T_SerialPortInfo> m_serialPortList;

	std::vector <uint8_t> m_buffer;

	std::vector <STRING> m_errors;

	struct
	{
		bool overlapped_waiting;
		OVERLAPPED overlapped;
		int queue_size;
		std::vector <uint8_t> buffer;
		volatile int buffer_rd;
		volatile int buffer_wr;
	} m_rx;

	struct
	{
		bool overlapped_waiting;
		OVERLAPPED overlapped;
		int queue_size;
		std::vector <uint8_t> buffer;
		volatile int buffer_rd;
		volatile int buffer_wr;
	} m_tx;

	CSerialPortThread *m_thread;

	void __fastcall clearErrors();
	void __fastcall pushError(const DWORD error, STRING leading_text);

	STRING __fastcall GetLastErrorString()
	{
		return AnsiString(m_last_error_str);
	}

	void __fastcall getState();

	void __fastcall GetSerialPortList();

	bool __fastcall Connected();

	void __fastcall processTx();
	void __fastcall processRx();
	void __fastcall threadProcess();

	int __fastcall RxBytesAvailable();
	int __fastcall RxByte();
	int __fastcall RxBytePeek();

	bool __fastcall TxEmpty();
	int __fastcall TxBytesWaiting();
	int __fastcall TxByte(const int b);

	int __fastcall GetBaudRate();
	void __fastcall SetBaudRate(int value);

	int __fastcall GetByteSize();
	void __fastcall SetByteSize(int value);

	int __fastcall GetParity();
	void __fastcall SetParity(int value);

	int __fastcall GetStopBits();
	void __fastcall SetStopBits(int value);

	void __fastcall SetRTS(bool value);
	void __fastcall SetDTR(bool value);

	bool __fastcall GetCTS();
	bool __fastcall GetDSR();
	bool __fastcall GetRING();
	bool __fastcall GetRLSD();

public:
	CSerialPort();
	~CSerialPort();

	unsigned int __fastcall errorCount();
	STRING __fastcall pullError();

	int __fastcall Connect(STRING device_name, const bool use_overlapped);
	void __fastcall Disconnect();

	int __fastcall RxBytes(void *buf, int size);
	int __fastcall RxBytesPeek(void *buf, int size);

	int __fastcall TxBytes(const void *buf, int bytes);
	int __fastcall TxBytes(const char *s);
	int __fastcall TxBytes(const STRING s);

	void __fastcall GetSerialPortList(std::vector <T_SerialPortInfo> &serialPortList);

	__property bool connected = {read = Connected};

	__property STRING deviceName    = {read = m_device_name};
	__property HANDLE deviceHandle  = {read = m_device_handle};

	__property DWORD lastError      = {read = m_last_error};
	__property STRING lastErrorStr  = {read = GetLastErrorString};

	__property int rxBytesAvailable = {read = RxBytesAvailable};
	__property int rxByte           = {read = RxByte};
	__property int rxBytePeek       = {read = RxBytePeek};

	__property bool txEmpty         = {read = TxEmpty};
	__property int txBytesWaiting   = {read = TxBytesWaiting};
	__property int txByte           = {write = TxByte};

	__property int baudRate = {read = GetBaudRate, write = SetBaudRate};
	__property int byteSize = {read = GetByteSize, write = SetByteSize};
	__property int parity   = {read = GetParity,   write = SetParity};
	__property int stopBits = {read = GetStopBits, write = SetStopBits};

	__property bool rts  = {write = SetRTS};
	__property bool dtr  = {write = SetDTR};

	__property bool cts  = {read = GetCTS};
	__property bool dsr  = {read = GetDSR};
	__property bool ring = {read = GetRING};
	__property bool rlsd = {read = GetRLSD};
};

#endif

