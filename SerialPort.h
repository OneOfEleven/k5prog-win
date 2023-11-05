
#ifndef SerialPortH
#define SerialPortH

#include <windows.h>

#include <vector>

typedef struct
{
	char name[MAX_PATH];
	char deviceID[MAX_PATH];
} T_SerialPortInfo;

class CSerialPort
{
private:
	HANDLE			device_handle;

	char			device_name[MAX_PATH];

	DCB				MyDCB;
	DCB				OriginalDCB;

	DWORD			ModemState;

	COMMTIMEOUTS	MyTimeouts;
	COMMTIMEOUTS	OriginalTimeouts;

	int				ReceiveQueue;
	int				TransmitQueue;

	unsigned char	ReceivedData[32768];
	int				ReceivedBytes;

	int				MaxFails;

	OVERLAPPED		overlapped_Read;
	OVERLAPPED		overlapped_Write;

	std::vector<T_SerialPortInfo> m_serialPortList;

	void Init();
	bool TxChar(char c);

	void GetSerialPortList();

	void Open();

public:
	CSerialPort();
	~CSerialPort();

	bool GetCommStat(DCB *dcb);

	bool isSerialPortPresent(char *name);

	bool Connect(char *newDeviceName);
	void Disconnect();

	bool Connected();

	void flushRx();

	int RxBytesAvailable();
	int RxBytes(void *buf, int num_of_bytes);

	int TxBytesWaiting();
	int TxBytes(void *buf, int bytes);
	int TxStr(char *s);

	char * GetDeviceName();

	HANDLE GetDeviceHandle();

	int GetReceiveQueue();
	void SetReceiveQueue(int NewReceiveQueue);

	int GetTxQueue();
	void SetTxQueue(int NewTransmitQueue);

	void SetMaxFails(int NewMaxFails);
	int GetMaxFails();

	int GetBaudRate();
	void SetBaudRate(int NewBaudRate);

	int GetByteSize();
	void SetByteSize(int NewByteSize);

	int GetParity();
	void SetParity(int NewParity);

	int GetStopBits();
	void SetStopBits(int NewStopBits);

	int GetReadIntervalTimeout();
	void SetReadIntervalTimeout(int NewReadIntervalTimeout);

	int GetReadTotalTimeoutMultiplier();
	void SetReadTotalTimeoutMultiplier(int NewReadTotalTimeoutMultiplier);

	int GetReadTotalTimeoutConstant();
	void SetReadTotalTimeoutConstant(int NewReadTotalTimeoutConstant);

	int GetWriteTotalTimeoutMultiplier();
	void SetWriteTotalTimeoutMultiplier(int NewWriteTotalTimeoutMultiplier);

	int GetWriteTotalTimeoutConstant();
	void SetWriteTotalTimeoutConstant(int NewWriteTotalTimeoutConstant);

	void SetRTS(bool set);
	void SetDTR(bool set);

	bool GetCTS();
	bool GetDSR();
	bool GetRING();
	bool GetRLSD();

	void Flush();

	void GetSerialPortList(std::vector<T_SerialPortInfo> &serialPortList);

	DWORD	LastError;
//	char	LastErrorStr[MAX_PATH];

	__property bool connected        = {read = Connected};

	__property int baudRate = {read = GetBaudRate, write = SetBaudRate};
	__property int byteSize         = {read = GetByteSize, write = SetByteSize};
	__property int parity           = {read = GetParity,   write = SetParity};
	__property int stopBits         = {read = GetStopBits, write = SetStopBits};

	__property bool rts             = {write = SetRTS};
	__property bool dtr             = {write = SetDTR};

	__property bool cts             = {read = GetCTS};
	__property bool dsr             = {read = GetDSR};
	__property bool ring            = {read = GetRING};
	__property bool rlsd            = {read = GetRLSD};
};

#endif

