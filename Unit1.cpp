
/*
 * Use at your own risk
 *
 * Uses modified comm routines from
 *   Jacek Lipkowski <sq5bpf@lipkowski.org>
 *   https://github.com/sq5bpf/k5prog
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

#include <vcl.h>
#include <inifiles.hpp>

#include <stdio.h>
//#incluve <limits.h>

//#include <stdlib.h>     // srand, rand
//#include <time.h>       // time

#pragma hdrstop

#include "Unit1.h"

#pragma package(smart_init)
#pragma link "CGAUGES"
#pragma resource "*.dfm"

TForm1 *Form1 = NULL;

// ************************************************************************

// known serial commands:
//
// 0x0514 .. read firmware version
// 0x0517 .. read flash block
// 0x0518 .. radio streams this when in firmware update mode
// 0x0519 .. write flash block .. only in firmware update mode
// 0x051B .. read configuration block
// 0x051D .. write configuration block
// 0x0527 .. read RSSI, noise and glitch
// 0x0529 .. read battery ADC voltage and current values
// 0x052D ..
// 0x052F ..
// 0x0530 .. send firmware version .. only in firmware update mode
// 0x05DD .. reboot radio

const uint8_t session_id[] = {0x6a, 0x39, 0x57, 0x64};
//const uint8_t session_id[] = {0x46, 0x9c, 0x6f, 0x64};

// ************************************************************************

typedef struct
{
	uint16_t MajorVer;
	uint16_t MinorVer;
	uint16_t ReleaseVer;
	uint16_t BuildVer;
} TVersion;

bool __fastcall getBuildInfo(String filename, TVersion *version)
{
	DWORD ver_info_size;
	char *ver_info;
	UINT buffer_size;
	LPVOID buffer;
	DWORD dummy;

	if (version == NULL || filename.IsEmpty())
		return false;

	memset(version, 0, sizeof(TVersion));

	ver_info_size = ::GetFileVersionInfoSizeA(filename.c_str(), &dummy);
	if (ver_info_size == 0)
		return false;

	ver_info = new char [ver_info_size];
	if (ver_info == NULL)
		return false;

	if (::GetFileVersionInfoA(filename.c_str(), 0, ver_info_size, ver_info) == FALSE)
	{
		delete [] ver_info;
		return false;
	}

	if (::VerQueryValue(ver_info, _T("\\"), &buffer, &buffer_size) == FALSE)
	{
		delete [] ver_info;
		return false;
	}

	PVSFixedFileInfo ver = (PVSFixedFileInfo)buffer;
	version->MajorVer   = (ver->dwFileVersionMS >> 16) & 0xFFFF;
	version->MinorVer   = (ver->dwFileVersionMS >>  0) & 0xFFFF;
	version->ReleaseVer = (ver->dwFileVersionLS >> 16) & 0xFFFF;
	version->BuildVer   = (ver->dwFileVersionLS >>  0) & 0xFFFF;

	delete [] ver_info;

	return true;
}

// ************************************************************************

__fastcall TForm1::TForm1(TComponent* Owner)
	: TForm(Owner)
{
}

void __fastcall TForm1::FormCreate(TObject *Sender)
{
	{
//		char username[64];
//		DWORD size = sizeof(username);
//		if (::GetUserNameA(username, &size) != FALSE && size > 1)
//			m_ini_filename = ChangeFileExt(Application->ExeName, "_" + String(username) + ".ini");
//		else
			m_ini_filename = ChangeFileExt(Application->ExeName, ".ini");
	}

	::GetSystemInfo(&m_system_info);
//	sprintf(SystemInfoStr,
//					"OEM id: %u"crlf
//					"num of cpu's: %u"crlf
//					"page size: %u"crlf
//					"cpu type: %u"crlf
//					"min app addr: %lx"crlf
//					"max app addr: %lx"crlf
//					"active cpu mask: %u"crlf,
//					m_system_info.dwOemId,
//					m_system_info.dwNumberOfProcessors,
//					m_system_info.dwPageSize,
//					m_system_info.dwProcessorType,
//					m_system_info.lpMinimumApplicationAddress,
//					m_system_info.lpMaximumApplicationAddress,
//					m_system_info.dwActiveProcessorMask);
//	MemoAddString(Memo1, SystemInfoStr);

	// the screen size is the phsyical screen size
	m_screen_width  = 0;
	m_screen_height = 0;
	HDC hDC = GetDC(0);
	if (hDC != NULL)
	{
		//ScreenBitsPerPixel = ::GetDeviceCaps(hDC, BITSPIXEL);
		m_screen_width  = ::GetDeviceCaps(hDC, HORZRES);
		m_screen_height = ::GetDeviceCaps(hDC, VERTRES);
		ReleaseDC(0, hDC);
	}

	{
		String s;
		TVersion version;
		getBuildInfo(Application->ExeName, &version);
		#ifdef _DEBUG
			s.printf("%s v%u.%u.%u.debug", Application->Title.c_str(), version.MajorVer, version.MinorVer, version.ReleaseVer);
		#else
			s.printf("%s v%u.%u.%u", Application->Title.c_str(), version.MajorVer, version.MinorVer, version.ReleaseVer);
		#endif
		this->Caption = s;
	}

	this->DoubleBuffered  = true;
	Panel1->DoubleBuffered = true;
	Panel2->DoubleBuffered = true;
	Memo1->DoubleBuffered = true;

	Memo1->Clear();

	m_thread = NULL;

	m_verbose = VerboseTrackBar->Position;

	memset(m_config, 0, sizeof(m_config));

	m_rx_str = "";
	m_rx_mode = 0;
	
	m_serial.port_name = "";
	//m_serial.port;
	m_serial.rx_buffer.resize(2048);
	m_serial.rx_buffer_wr = 0;
	m_serial.rx_timer.mark();

	OpenDialog1->InitialDir = ExtractFilePath(Application->ExeName);
	SaveDialog1->InitialDir = ExtractFilePath(Application->ExeName);

	OpenDialog2->InitialDir = ExtractFilePath(Application->ExeName);
	SaveDialog2->InitialDir = ExtractFilePath(Application->ExeName);

	OpenDialog3->InitialDir = ExtractFilePath(Application->ExeName);

	CGauge1->Visible  = false;
	CGauge1->Progress = 0;
	CGauge1->Parent   = StatusBar1;
//	CGauge1->Parent   = StatusBar1->Panels->Items[2];
//	CGauge1->Align    = alClient;

//	make_CRC16_table();

	// ******************

	{
		updateSerialPortCombo();
		SerialPortComboBox->ItemIndex = 0;

		const TNotifyEvent ne = SerialSpeedComboBox->OnChange;
		SerialSpeedComboBox->OnChange = NULL;
		SerialSpeedComboBox->Clear();
		SerialSpeedComboBox->AddItem("300",     (TObject *)300);
		SerialSpeedComboBox->AddItem("600",     (TObject *)600);
		SerialSpeedComboBox->AddItem("1200",    (TObject *)1200);
		SerialSpeedComboBox->AddItem("2400",    (TObject *)2400);
		SerialSpeedComboBox->AddItem("4800",    (TObject *)4800);
		SerialSpeedComboBox->AddItem("9600",    (TObject *)9600);
		SerialSpeedComboBox->AddItem("19200",   (TObject *)19200);
		SerialSpeedComboBox->AddItem("38400 - k5/k6", (TObject *)38400);
		SerialSpeedComboBox->AddItem("57600",   (TObject *)57600);
		SerialSpeedComboBox->AddItem("76800",   (TObject *)76800);
		SerialSpeedComboBox->AddItem("115200",  (TObject *)115200);
		SerialSpeedComboBox->AddItem("230400",  (TObject *)230400);
		SerialSpeedComboBox->AddItem("250000",  (TObject *)250000);
		SerialSpeedComboBox->AddItem("460800",  (TObject *)460800);
		SerialSpeedComboBox->AddItem("500000",  (TObject *)500000);
		SerialSpeedComboBox->AddItem("921600",  (TObject *)921600);
		SerialSpeedComboBox->AddItem("1000000", (TObject *)1000000);
		SerialSpeedComboBox->AddItem("1843200", (TObject *)1843200);
		SerialSpeedComboBox->AddItem("2000000", (TObject *)2000000);
		SerialSpeedComboBox->AddItem("3000000", (TObject *)3000000);
		SerialSpeedComboBox->ItemIndex = SerialSpeedComboBox->Items->IndexOfObject((TObject *)DEFAULT_SERIAL_SPEED);
		SerialSpeedComboBox->OnChange = ne;
	}

	comboBoxAutoWidth(SerialPortComboBox);
	comboBoxAutoWidth(SerialSpeedComboBox);

	// ******************

	::PostMessage(this->Handle, WM_INIT_GUI, 0, 0);
}

void __fastcall TForm1::FormDestroy(TObject *Sender)
{
	//
}

void __fastcall TForm1::FormClose(TObject *Sender, TCloseAction &Action)
{
/*	if (m_thread != NULL)
	{	// we are busy reading/writing from/to the radio

		Application->BringToFront();
		Application->NormalizeTopMosts();
		const int res = Application->MessageBox("Busy readin/writing from/to the radio.\n\nStill close ?", Application->Title.c_str(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);
		Application->RestoreTopMosts();
		switch (res)
		{
			case IDYES:
				break;
			case IDNO:
			case IDCANCEL:
				Action = caNone;	// don't close this app'
				return;
		}
	}
*/
	Timer1->Enabled = false;

	disconnect();

	saveSettings();
}

void __fastcall TForm1::WMWindowPosChanging(TWMWindowPosChanging &msg)
{
	const int thresh = 8;

	RECT work_area;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0);

	const int dtLeft   = Screen->DesktopRect.left;
	const int dtRight  = Screen->DesktopRect.right;
	const int dtTop    = Screen->DesktopRect.top;
	const int dtBottom = Screen->DesktopRect.bottom;
	const int dtWidth  = dtRight - dtLeft;
	const int dtHeight = dtBottom - dtTop;

//	const int waLeft = work_area.left;
//	const int waTop = work_area.top;
//	const int waRight = work_area.right;
//	const int waBottom = work_area.bottom;
	const int waWidth = work_area.right - work_area.left;
	const int waHeight = work_area.bottom - work_area.top;

	int x = msg.WindowPos->x;
	int y = msg.WindowPos->y;
	int w = msg.WindowPos->cx;
	int h = msg.WindowPos->cy;

	{	// sticky screen edges
		if (std::abs((int)(x - work_area.left)) < thresh)
			x = work_area.left;			// stick left to left side
		else
		if (std::abs((int)((x + w) - work_area.right)) < thresh)
			x = work_area.right - w;	// stick right to right side

		if (std::abs((int)(y - work_area.top)) < thresh)
			y = work_area.top;			// stick top to top side
		else
		if (std::abs((int)((y + h) - work_area.bottom)) < thresh)
			y = work_area.bottom - h;	// stick bottom to bottm side

		// stick the right side to the right side of the screen if the left side is stuck to the left side of the screen
		if (x == work_area.left)
			if ((w >= (waWidth - thresh)) && (w <= (waWidth + thresh)))
				w = waWidth;

		// stick the bottom to the bottom of the screen if the top is stuck to the top of the screen
		if (y == work_area.top)
			if ((h >= (waHeight - thresh)) && (h <= (waHeight + thresh)))
				h = waHeight;
	}

	{	// limit minimum size
		if (w < Constraints->MinWidth)
			 w = Constraints->MinWidth;
		if (h < Constraints->MinHeight)
			 h = Constraints->MinHeight;
	}

	{	// limit maximum size
		if (w > Constraints->MaxWidth && Constraints->MaxWidth > Constraints->MinWidth)
			 w = Constraints->MaxWidth;
		if (h > Constraints->MaxHeight && Constraints->MaxHeight > Constraints->MinHeight)
			 h = Constraints->MaxHeight;
	}

	{	// limit maximum size
		if (w > dtWidth)
			 w = dtWidth;
		if (h > dtHeight)
			 h = dtHeight;
	}

	if (Application->MainForm && this != Application->MainForm)
	{	// stick to our main form sides
		const TRect rect = Application->MainForm->BoundsRect;

		if (std::abs((int)(x - rect.left)) < thresh)
			x = rect.left;			// stick to left to left side
		else
		if (std::abs((int)((x + w) - rect.left)) < thresh)
			x = rect.left - w;	// stick right to left side
		else
		if (std::abs((int)(x - rect.right)) < thresh)
			x = rect.right;		// stick to left to right side
		else
		if (std::abs((int)((x + w) - rect.right)) < thresh)
			x = rect.right - w;	// stick to right to right side

		if (std::abs((int)(y - rect.top)) < thresh)
			y = rect.top;			// stick top to top side
		else
		if (std::abs((int)((y + h) - rect.top)) < thresh)
			y = rect.top - h;		// stick bottom to top side
		else
		if (std::abs((int)(y - rect.bottom)) < thresh)
			y = rect.bottom;		// stick top to bottom side
		else
		if (std::abs((int)((y + h) - rect.bottom)) < thresh)
			y = rect.bottom - h;	// stick bottom to bottom side
	}

	{	// stop it completely leaving the desktop area
		if (x < (dtLeft - Width + (dtWidth / 15)))
			  x = dtLeft - Width + (dtWidth / 15);
		if (x > (dtWidth - (Screen->Width / 15)))
			  x = dtWidth - (Screen->Width / 15);
		if (y < dtTop)
			 y = dtTop;
		if (y > (dtBottom - (dtHeight / 10)))
			  y = dtBottom - (dtHeight / 10);
	}

	msg.WindowPos->x  = x;
	msg.WindowPos->y  = y;
	msg.WindowPos->cx = w;
	msg.WindowPos->cy = h;
}

void __fastcall TForm1::CMMouseEnter(TMessage &msg)
{
	TComponent *Comp = (TComponent *)msg.LParam;
	if (!Comp)
		return;

//	if (dynamic_cast<TControl *>(Comp) == NULL)
//		return;		// only interested in on screen controls

	if (dynamic_cast<TPaintBox *>(Comp) != NULL)
	{
//		TPaintBox *pb = (TPaintBox *)Comp;
//		pb->Invalidate();
	}
}

void __fastcall TForm1::CMMouseLeave(TMessage &msg)
{
	TComponent *Comp = (TComponent *)msg.LParam;
	if (!Comp)
		return;

//	if (dynamic_cast<TControl *>(Comp) == NULL)
//		return;		// only interested in on screen controls

	if (dynamic_cast<TPaintBox *>(Comp) != NULL)
	{
//		TPaintBox *pb = (TPaintBox *)Comp;
/*
		m_graph_mouse_x     = -1;
		m_graph_mouse_y     = -1;

		m_mouse_graph = -1;
		for (int i = 0; i < MAX_TRACES; i++)
			m_graph_mouse_index[i] = -1;
*/
//		pb->Invalidate();
	}
/*
	if (dynamic_cast<TPaintBox *>(Comp) != NULL)
	{
		TPaintBox *pb = PaintBox1;

		m_graph_mouse_x     = -1;
		m_graph_mouse_y     = -1;

		m_mouse_graph = -1;
		for (int i = 0; i < MAX_TRACES; i++)
			m_graph_mouse_index[i] = -1;

		pb->Invalidate();
	}
*/
}

void __fastcall TForm1::WMInitGUI(TMessage &msg)
{
	String s;

	loadSettings();

	//	BringToFront();
//	::SetForegroundWindow(Handle);

	if (Application->MainForm)
		Application->MainForm->Update();

	Timer1->Enabled = true;

	// test only
//	::PostMessage(this->Handle, WM_CONNECT, 0, 0);

	SerialPortComboBoxSelect(NULL);
}

void __fastcall TForm1::WMConnect(TMessage &msg)
{
	connect();
}

void __fastcall TForm1::WMDisconnect(TMessage &msg)
{
	disconnect();
}

void __fastcall TForm1::WMBreak(TMessage &msg)
{
	if (m_breaks < 10)
	{
		String s;
		s.printf("comm breaks %d", m_breaks);
		Memo1->Lines->Add(s);
	}
}

void __fastcall TForm1::loadSettings()
{
	int          i;
	float        f;
	String       s;
	bool         b;
	TNotifyEvent ne;

	TIniFile *ini = new TIniFile(m_ini_filename);
	if (ini == NULL)
		return;

	Top    = ini->ReadInteger("MainForm", "Top",     Top);
	Left   = ini->ReadInteger("MainForm", "Left",   Left);
	Width  = ini->ReadInteger("MainForm", "Width",  Width);
	Height = ini->ReadInteger("MainForm", "Height", Height);

	i = ini->ReadInteger("GUI", "Verbose", VerboseTrackBar->Position);
	m_verbose = (i < VerboseTrackBar->Min) ? 1 : (i > VerboseTrackBar->Max) ? 1 : i;
	VerboseTrackBar->Position = m_verbose;

	m_serial.port_name = ini->ReadString("SerialPort", "Name", SerialPortComboBox->Text);
	i = SerialPortComboBox->Items->IndexOf(m_serial.port_name);
	if (i >= 0)
	{
		ne = SerialPortComboBox->OnChange;
		SerialPortComboBox->OnChange = NULL;
		SerialPortComboBox->ItemIndex = i;
		SerialPortComboBox->OnChange = ne;
	}
	SerialPortComboBoxChange(SerialPortComboBox);

#if 0
	ne = SerialSpeedComboBox->OnChange;
	SerialSpeedComboBox->OnChange = NULL;
	i = ini->ReadInteger("SerialPort", "Speed", DEFAULT_SERIAL_SPEED);
	i = SerialSpeedComboBox->Items->IndexOfObject((TObject *)i);
	if (i < 0)
		i = SerialSpeedComboBox->Items->IndexOfObject((TObject *)DEFAULT_SERIAL_SPEED);
	SerialSpeedComboBox->ItemIndex = i;
	SerialSpeedComboBox->OnChange = ne;
#endif

	delete ini;
}

void __fastcall TForm1::saveSettings()
{
	String s;
	int    i;

	DeleteFile(m_ini_filename);

	TIniFile *ini = new TIniFile(m_ini_filename);
	if (ini == NULL)
		return;

	ini->WriteInteger("MainForm", "Top",    Top);
	ini->WriteInteger("MainForm", "Left",   Left);
	ini->WriteInteger("MainForm", "Width",  Width);
	ini->WriteInteger("MainForm", "Height", Height);

	ini->WriteInteger("GUI", "Verbose",      m_verbose);

	ini->WriteString("SerialPort", "Name",  m_serial.port_name);
	ini->WriteInteger("SerialPort", "Speed", (int)SerialSpeedComboBox->Items->Objects[SerialSpeedComboBox->ItemIndex]);

	delete ini;
}

void __fastcall TForm1::comboBoxAutoWidth(TComboBox *comboBox)
{
	if (!comboBox)
		return;

	#define COMBOBOX_HORIZONTAL_PADDING	4

	int itemsFullWidth = comboBox->Width;

	// get the max needed with of the items in dropdown state
	for (int i = 0; i < comboBox->Items->Count; i++)
	{
		int itemWidth = comboBox->Canvas->TextWidth(comboBox->Items->Strings[i]);
		itemWidth += 2 * COMBOBOX_HORIZONTAL_PADDING;
		if (itemsFullWidth < itemWidth)
			itemsFullWidth = itemWidth;
	}

	if (comboBox->DropDownCount < comboBox->Items->Count)
		itemsFullWidth += ::GetSystemMetrics(SM_CXVSCROLL);

	::SendMessage(comboBox->Handle, CB_SETDROPPEDWIDTH, itemsFullWidth, 0);
}

void __fastcall TForm1::updateSerialPortCombo()
{
	std::vector <T_SerialPortInfo> serial_port_list;
	m_serial.port.GetSerialPortList(serial_port_list);

	const TNotifyEvent ne = SerialPortComboBox->OnChange;
	SerialPortComboBox->OnChange = NULL;

	SerialPortComboBox->Clear();
	SerialPortComboBox->AddItem("None", (TObject *)0xffffffff);
	for (unsigned int i = 0; i < serial_port_list.size(); i++)
		SerialPortComboBox->AddItem(serial_port_list[i].name, (TObject *)i);

	const int i = SerialPortComboBox->Items->IndexOf(m_serial.port_name);
	SerialPortComboBox->ItemIndex = (!m_serial.port_name.IsEmpty() && i >= 0) ? i : 0;

	SerialPortComboBox->OnChange = ne;

	SerialPortComboBoxChange(SerialPortComboBox);
}

void __fastcall TForm1::SerialPortComboBoxDropDown(TObject *Sender)
{
	updateSerialPortCombo();
}

void __fastcall TForm1::clearRxPacket0()
{	// remove the 1st packet from the RX packet queue

	CCriticalSection cs(m_thread_cs);

	if (!m_rx_packet_queue.empty())
	{
		m_rx_packet_queue[0].clear();
		m_rx_packet_queue.erase(m_rx_packet_queue.begin() + 0);
	}
}

void __fastcall TForm1::clearRxPacketQueue()
{	// empty the RX packet queue

	CCriticalSection cs(m_thread_cs);

	while (!m_rx_packet_queue.empty())
	{
		m_rx_packet_queue[0].clear();
		m_rx_packet_queue.erase(m_rx_packet_queue.begin() + 0);
	}

	m_serial.rx_buffer_wr = 0;
}

void __fastcall TForm1::disconnect()
{
	// terminate the thread
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

	if (m_serial.port.connected)
	{
		m_serial.port.rts = false;
		m_serial.port.dtr = false;

		m_serial.port.Disconnect();

		String s;
		s.printf("'%s' closed", m_serial.port_name.c_str());
		Memo1->Lines->Add(s);
	}

	clearRxPacketQueue();

	m_serial.rx_buffer_wr = 0;

//	SerialPortComboBox->Enabled  = false;
//	SerialSpeedComboBox->Enabled = false;
}

bool __fastcall TForm1::connect(const bool clear_memo)
{
	String s;
	String port_name;

//	Beep(1000, 10);

	disconnect();

	if (clear_memo)
	{
		Memo1->Clear();
		Memo1->Update();
	}

	int i = SerialPortComboBox->ItemIndex;
	if (i <= 0)
		return false;
	port_name = SerialPortComboBox->Items->Strings[i].Trim();
	if (port_name.IsEmpty() || port_name.LowerCase() == "none")
	{
		return false;
	}

	i = SerialSpeedComboBox->ItemIndex;
	if (i < 0)
		return false;
	const int baudrate = (int)SerialSpeedComboBox->Items->Objects[i];
	if (baudrate <= 0)
	{	// error
		Memo1->Lines->Add("error: serial baudrate: [" + IntToStr(baudrate) + "]");
		return false;
	}

	m_breaks = 0;
	m_rx_str = "";
	{
		CCriticalSection cs(m_rx_lines_cs);
		m_rx_lines.resize(0);
	}

	m_serial.port.rts      = false;
	m_serial.port.dtr      = false;
	m_serial.port.byteSize = 8;
	m_serial.port.parity   = NOPARITY;
	m_serial.port.stopBits = ONESTOPBIT;
	m_serial.port.baudRate = baudrate;

	const bool res = m_serial.port.Connect(port_name.c_str());
	if (!res)
	{
		s.printf("error: serial port open error [%d]", res);
		Memo1->Lines->Add(s);

		SerialPortComboBox->Enabled  = true;
		SerialSpeedComboBox->Enabled = true;
		return false;
	}

	m_serial.port_name = port_name;

	s.printf("'%s' opened at %d Baud", m_serial.port_name.c_str(), baudrate);
	Memo1->Lines->Add(s);

	clearRxPacketQueue();

	m_serial.rx_buffer_wr = 0;
	m_serial.rx_timer.mark();

	// flush the RX
	while (m_serial.port.RxBytes(&m_serial.rx_buffer[0], m_serial.rx_buffer.size()) > 0)
		Sleep(50);

	if (m_thread == NULL)
	{	// create & start the thread
		m_thread = new CThread(&threadProcess, tpNormal, 1, true, false);
//		m_thread = new CThread(&threadProcess, tpNormal, 1, true, true);
		if (m_thread == NULL)
		{
			Memo1->Lines->Add("");
			disconnect();
			return false;
		}
	}

//	SerialPortComboBox->Enabled  = false;
//	SerialSpeedComboBox->Enabled = false;

	return true;
}

void __fastcall TForm1::threadProcess()
{
	if (m_thread == NULL)
		return;

	if (!m_serial.port.connected)
	{	// serial is closed
		return;
	}

	CCriticalSection cs(m_thread_cs, !m_thread->Sync);

	// *********
	// save any rx'ed bytes from the serial port into our RX buffer

	int num_bytes = 0;
	
#ifdef SERIAL_OVERLAPPED
	if (m_serial.port.GetBreak())
	{
		m_breaks++;
		::PostMessage(this->Handle, WM_BREAK, 0, 0);
		return;
	}
#endif

	const int rx_space = m_serial.rx_buffer.size() - m_serial.rx_buffer_wr;	// space we have left to store RX data
	if (rx_space > 0)
	{	// we have buffer space for more RX data

		num_bytes = m_serial.port.RxBytes(&m_serial.rx_buffer[m_serial.rx_buffer_wr], rx_space);
		if (num_bytes < 0)
		{	// error
			m_serial.port.Disconnect();
			::PostMessage(this->Handle, WM_DISCONNECT, 0, 0);
			m_serial.rx_buffer_wr = 0;
			return;
		}

		if (num_bytes > 0)
		{
			m_serial.rx_buffer_wr += num_bytes;
			m_serial.rx_timer.mark();
		}
	}

	// **********************************

	if (m_serial.rx_buffer_wr > 0 && m_serial.rx_timer.millisecs() >= 5000)
	{
		m_serial.rx_buffer_wr = 0;		// no data rx'ed for 2 seconds .. empty the rx buffer
		m_rx_str = "";
	}

	if (m_rx_mode == 0)
	{	// normal text mode

		unsigned int i = 0;

		while (i < m_serial.rx_buffer_wr && i < m_serial.rx_buffer.size())
		{
			const char c = m_serial.rx_buffer[i++];
			if (c < 32)
			{
				if (c == '\n')
				{
					CCriticalSection cs(m_rx_lines_cs);
					m_rx_lines.push_back(m_rx_str.TrimRight());
					m_rx_str = "";
				}
				else
				if (c != '\r')
				{
					if (m_rx_str.Length() < 512)
					{
						String s2;
						s2.printf("[%02X]", (uint8_t)c);
						m_rx_str += s2;
					}
				}
			}
			else
				m_rx_str += c;
		}

		m_serial.rx_buffer_wr = 0;
	}
	else
	{	// extract any rx'ed packets found in our RX buffer

		while (m_serial.rx_buffer_wr >= 8)   // '8' is the very minimum packet size
		{
			if (m_serial.rx_buffer[0] != 0xAB || m_serial.rx_buffer[1] != 0xCD)
			{
				// scan for a start byte
				int i = 1;
				while (m_serial.rx_buffer[i] != 0xAB && i < (int)m_serial.rx_buffer_wr)
					i++;

				if (i < (int)m_serial.rx_buffer_wr)
				{	// possible start byte found - remove all rx'ed bytes before it
					memmove(&m_serial.rx_buffer[0], &m_serial.rx_buffer[i], m_serial.rx_buffer_wr - i);
					m_serial.rx_buffer_wr -= i;
				}
				else
				{	// no start byte found - remove all rx'ed bytes
					m_serial.rx_buffer_wr = 0;
				}

				continue;
			}

			// fetch the payload size
			const int data_len   = ((uint16_t)m_serial.rx_buffer[3] << 8) | ((uint16_t)m_serial.rx_buffer[2] << 0);
			if (data_len > 270)
			{	// too big .. assume it's an error
				memmove(&m_serial.rx_buffer[0], &m_serial.rx_buffer[1], m_serial.rx_buffer_wr - 1);
				m_serial.rx_buffer_wr--;
				continue;
			}

			// complete packet length
			// includes the 4-byte header, payload, 2-byte CRC and 2-byte footer
			const int packet_len = 4 + data_len + 2 + 2;

			if ((int)m_serial.rx_buffer_wr < packet_len)
				break;	// not yet received the complete packet

			if (m_serial.rx_buffer[packet_len - 2] != 0xDC || m_serial.rx_buffer[packet_len - 1] != 0xBA)
			{	// invalid end bytes - slide the data down one byte
				memmove(&m_serial.rx_buffer[0], &m_serial.rx_buffer[1], m_serial.rx_buffer_wr - 1);
				m_serial.rx_buffer_wr--;
				continue;
			}

			// appear to have a complete packet

			if (data_len > 0)
			{	// save the RX'ed packet

				// copy/extract the packets payload into a separate buffer
				std::vector <uint8_t> rx_data(data_len + 2);
				memcpy(&rx_data[0], &m_serial.rx_buffer[4], data_len + 2);

				#if 1
					if (m_thread->Sync && m_verbose > 2)
					{	// show the raw packet
						String s;
						s.printf("rx [%3u] ", packet_len);
						for (int i = 0; i < packet_len; i++)
						{
							String s2;
							s2.printf("%02X ", m_serial.rx_buffer[i]);
							s += s2;
						}
						Memo1->Lines->Add(s.Trim());
						Memo1->Update();
					}
				#endif

				// CRC bytes before de-obfuscation
				const uint16_t crc0 = ((uint16_t)rx_data[rx_data.size() - 1] << 8) | ((uint16_t)rx_data[rx_data.size() - 2] << 0);

				// de-obfuscate (de-scramble) the payload (the data and 16-bit CRC are the scrambled bytes)
				k5_xor_payload(&rx_data[0], rx_data.size());

				// CRC bytes after de-obfuscation
				const uint16_t crc1 = ((uint16_t)rx_data[rx_data.size() - 1] << 8) | ((uint16_t)rx_data[rx_data.size() - 2] << 0);

				#if 1
					if (m_thread->Sync && m_verbose > 2)
					{	// show the de-scrambled payload
						String s;
						s.printf("rx [%3u]             ", m_rx_packet_queue.size());
						for (size_t i = 0; i < rx_data.size(); i++)
						{
							String s2;
							s2.printf("%02X ", rx_data[i]);
							s += s2;
						}
						Memo1->Lines->Add(s.Trim());
						Memo1->Update();
					}
				#endif

				if (crc0 != 0xffff && crc1 != 0xffff)
				{	// compute and check the CRC
					const uint16_t crc2 = crc16(&rx_data[0], rx_data.size() - 2);
					if (crc2 != crc1)
						rx_data.resize(0);	// CRC error .. dump the payload
				}

				if (!rx_data.empty())
				{	// we have a payload to save

					// drop the 16-bit CRC off the end
					//rx_data.resize(data_len);

					// append the RX'ed payload onto the RX queue
					if (m_rx_packet_queue.size() < 4)    // don't bother saving any more than 4 packets/payloads, the radios bootloader sends continuously when in firmware update mode
						m_rx_packet_queue.push_back(rx_data);
				}
			}

			// remove the spent packet from the RX buffer
			if (packet_len < (int)m_serial.rx_buffer_wr)
				memmove(&m_serial.rx_buffer[0], &m_serial.rx_buffer[packet_len], m_serial.rx_buffer_wr - packet_len);
			m_serial.rx_buffer_wr -= packet_len;

//			break;       // only extract one packet per thread loop - gives more time to the exec
		}
	}
}

std::vector <String> __fastcall TForm1::stringSplit(String s, String separator)
{
	std::vector <String> strings;

	if (separator.IsEmpty())
		return strings;

	while (s.Length() > 0)
	{
		const int p = s.Pos(separator);
		if (p <= 0)
			break;
		String s2 = s.SubString(1, p - 1);
		s = s.SubString(p + separator.Length(), s.Length());
		strings.push_back(s2);
	}

	if (!s.IsEmpty())
		strings.push_back(s);

	return strings;
}

void __fastcall TForm1::FormKeyDown(TObject *Sender, WORD &Key,
		TShiftState Shift)
{
	switch (Key)
	{
		case VK_ESCAPE:
			Key = 0;
			if (m_serial.port.connected)
				m_serial.port.Disconnect();
			else
				Close();
			break;
		case VK_SPACE:
//			Key = 0;
			break;
		case VK_UP:		// up arrow
//			Key = 0;
			break;
		case VK_DOWN:	// down arrow
//			Key = 0;
			break;
		case VK_LEFT:	// left arrow
//			Key = 0;
			break;
		case VK_RIGHT:	// right arrow
//			Key = 0;
			break;
		case VK_PRIOR:	// page up
//			Key = 0;
			break;
		case VK_NEXT:	// page down
//			Key = 0;
			break;
	}
}

void __fastcall TForm1::Timer1Timer(TObject *Sender)
{
	String s;

	// **************************

	bool updated = false;

	s = FormatDateTime(" dddd dd mmm yyyy  hh:nn:ss", Now());
	if (StatusBar1->Panels->Items[0]->Text != s)
	{
		StatusBar1->Panels->Items[0]->Text = s;
		updated = true;
	}

	#ifdef SERIAL_OVERLAPPED
		s = (m_serial.port.connected && m_serial.port.GetBreak()) ? "RX BREAK" : "";
		if (StatusBar1->Panels->Items[1]->Text != s)
		{
			StatusBar1->Panels->Items[1]->Text = s;
			updated = true;
		}
	#endif
	
	if (StatusBar1->Panels->Items[2]->Text != m_rx_str)
	{
		StatusBar1->Panels->Items[2]->Text = m_rx_str;
		updated = true;
	}

	{
		CCriticalSection cs(m_rx_lines_cs);
		while (!m_rx_lines.empty())
		{
			String s = m_rx_lines[0];
			m_rx_lines.erase(m_rx_lines.begin());
			Memo1->Lines->Add(s);
		}
	}
	
	if (updated)
		StatusBar1->Update();

	// **************************

//	while (m_serial.port.errorCount() > 0)
//	{
//		s = m_serial.port.pullError();
//		Memo1->Lines->Add("serial error: " + s);
//	}

	// **************************
}

int __fastcall TForm1::saveFile(String filename, const uint8_t *data, const size_t size)
{
	String s;

	if (data == NULL || size == 0)
		return 0;

	const int file_handle = FileCreate(filename);
	if (file_handle <= 0)
		return 0;

	const int bytes_written = FileWrite(file_handle, &data[0], size);

	FileClose(file_handle);

	return bytes_written;
}

size_t __fastcall TForm1::loadFile(String filename)
{	// load a saved file

	String s;

	// ****************************
	// load the file

	const int file_handle = FileOpen(filename, fmOpenRead | fmShareDenyNone);
	if (file_handle <= 0)
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		Application->MessageBox("error: file not opened", Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();
		return 0;
	}

	const int file_size = FileSeek(file_handle, 0, 2);
	FileSeek(file_handle, 0, 0);

	if (file_size <= 0)
	{
		FileClose(file_handle);

		Application->BringToFront();
		Application->NormalizeTopMosts();
		Application->MessageBox("error: empty file", Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();
		return 0;
	}

	std::vector <uint8_t> input_buffer(file_size);

	const int bytes_loaded = FileRead(file_handle, &input_buffer[0], file_size);

	FileClose(file_handle);

	if (bytes_loaded != (int)input_buffer.size())
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		Application->MessageBox("error: file not loaded", Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();
		return 0;
	}

	// ***************************

	m_loadfile_name = filename;
	m_loadfile_data = input_buffer;

	return m_loadfile_data.size();
}

void __fastcall TForm1::ClearButtonClick(TObject *Sender)
{
	Memo1->Clear();
}

// ************************************************************************
// CRC

#define CRC_POLY16 0x1021

#if 0
	// slow bit-bang based

	uint16_t __fastcall TForm1::crc16(const uint8_t *data, const int size)
	{
		uint16_t crc = 0;
		if (data != NULL && size > 0)
		{
			for (int i = 0; i < size; i++)
			{
				crc ^= (uint16_t)data[i] << 8;
				for (unsigned int k = 8; k > 0; k--)
					crc = (crc & 0x8000) ? (crc << 1) ^ CRC_POLY16 : crc << 1;
			}
		}
		return crc;
	}

#else
	// fast table based

	#if 0
		// small table

		#if 0

			uint16_t CRC16_TABLE[16];

			void __fastcall TForm1::make_CRC16_table()
			{
				for (uint16_t i = 0; i < 16; i++)
				{
					uint16_t crc = i << 8;
					for (unsigned int k = 8; k > 0; k--)
						crc = (crc & 0x8000) ? (crc << 1) ^ CRC_POLY16 : crc << 1;
					CRC16_TABLE[i] = crc;
				}
			}

		#else

			static const uint16_t CRC16_TABLE[] =
			{
				0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF
			};

		#endif

		uint16_t __fastcall TForm1::crc16(const uint8_t *data, const int size)
		{
			uint16_t crc = 0;
			if (data != NULL && size > 0)
			{
				for (int i = 0; i < size; i++)
				{
					crc ^= (uint16_t)data[i] << 8;
					crc = (crc << 4) ^ CRC16_TABLE[crc >> 12];
					crc = (crc << 4) ^ CRC16_TABLE[crc >> 12];
				}
			}
			return crc;
		}

	#else
		// large table

		#if 0

			uint16_t CRC16_TABLE[256];

			void __fastcall TForm1::make_CRC16_table()
			{
				for (uint16_t i = 0; i < 256; i++)
				{
					uint16_t crc = i << 8;
					for (unsigned int k = 8; k > 0; k--)
						crc = (crc & 0x8000) ? (crc << 1) ^ CRC_POLY16 : crc << 1;
					CRC16_TABLE[i] = crc;
				}
			}

		#else

			static const uint16_t CRC16_TABLE[] =
			{
				0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
				0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6, 0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
				0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485, 0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
				0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4, 0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
				0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823, 0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
				0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12, 0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
				0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41, 0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
				0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70, 0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
				0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F, 0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
				0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E, 0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
				0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D, 0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
				0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C, 0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
				0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB, 0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
				0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A, 0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
				0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9, 0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
				0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8, 0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
			};

		#endif

		uint16_t __fastcall TForm1::crc16(const uint8_t *data, const int size)
		{
			uint16_t crc = 0;
			if (data != NULL && size > 0)
				for (int i = 0; i < size; i++)
					crc = (crc << 8) ^ CRC16_TABLE[(crc >> 8) ^ data[i]];
			return crc;
		}

	#endif

#endif

// ************************************************************************

void __fastcall TForm1::k5_destroy_struct(struct k5_command *cmd)
{
	if (cmd == NULL)
		return;

	if (cmd->cmd != NULL)
	{
		free(cmd->cmd);
		cmd->cmd = NULL;
	}

	if (cmd->obfuscated_cmd != NULL)
	{
		free(cmd->obfuscated_cmd);
		cmd->obfuscated_cmd = NULL;
	}

	free(cmd);
}

void __fastcall TForm1::k5_hdump(const uint8_t *buf, const int len)
{
	String     s;
	char       adump[75];
	int        tmp3 = 0;
	const char hexz[] = "0123456789ABCDEF";
	int        last_tmp = 0;

	if (buf == NULL || len <= 0)
		return;

	#if 0
		s.printf("Offset  00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  Text", len);
		Memo1->Lines->Add(s);
		s.printf("-------------------------------------------------------------------------");
		Memo1->Lines->Add(s);
	#endif

	memset(adump, 0, sizeof(adump));
	memset(adump, ' ', 74);

	int i;
	int k;
	for (i = 0, k = 0; i < len; i++, k++)
	{
		if (k >= 16)
			 k = 0;
		if (k == 0)
		{
			if (i != 0)
			{
				s.printf("%06X  %.65s", tmp3, adump);
				Memo1->Lines->Add(s);
				last_tmp = i;
			}

			memset(adump, 0, sizeof(adump));
			memset(adump, ' ', 74);

			tmp3 = i;
		}

		const uint8_t b = buf[i];
		int m = k * 3;
		adump[m++] = hexz[b >> 4];
		adump[m]   = hexz[b & 0x0f];
		adump[((16 * 3) + 1) + k] = (b >= 32) ? b : '.';
	}

	#if 0
		if ((i % 16) != 0 || len == 16)
		{
			s.printf("%06X  %.65s", tmp3, adump);
			Memo1->Lines->Add(s);
		}
	#else
		if (last_tmp != i)
		{
			s.printf("%06X  %.65s", tmp3, adump);
			Memo1->Lines->Add(s);
		}
	#endif
}

void __fastcall TForm1::k5_hex_dump(const struct k5_command *cmd)
{
	String s;

	if (cmd == NULL)
		return;

	s.printf("command hex dump   obf_len %d   clear_len %d   crc_ok %d", cmd->obfuscated_len, cmd->len, cmd->crc_ok);
	Memo1->Lines->Add(s);

	if (cmd->obfuscated_cmd != NULL)
	{
		Memo1->Lines->Add("* obfuscated");
		k5_hdump(cmd->obfuscated_cmd, cmd->obfuscated_len);
	}

	if (cmd->cmd != NULL)
	{
		Memo1->Lines->Add("* clear");
		k5_hdump(cmd->cmd, cmd->len);
	}
}

void __fastcall TForm1::k5_hex_dump2(const struct k5_command *cmd, const bool tx)
{
	String s;

	if (cmd == NULL)
		return;

	s.printf("%s crc_ok %d", tx ? "tx" : "rx", cmd->crc_ok);
	Memo1->Lines->Add(s);

	if (cmd->obfuscated_cmd != NULL)
	{
		s.printf("%s obfuscated [%3d] ", tx ? "tx" : "rx", cmd->obfuscated_len);
		for (int i = 0; i < cmd->obfuscated_len; i++)
		{
			String s2;
			s2.printf("%02X ", cmd->obfuscated_cmd[i]);
			s += s2;
		}
		Memo1->Lines->Add(s.Trim());
	}

	if (cmd->cmd != NULL)
	{
		s.printf("%s clear      [%3d]             ", tx ? "tx" : "rx", cmd->len);
		for (int i = 0; i < cmd->len; i++)
		{
			String s2;
			s2.printf("%02X ", cmd->cmd[i]);
			s += s2;
		}
		{
			String s2;
			s2.printf("- %04X", cmd->crc_clear);
			s += s2;
		}
		Memo1->Lines->Add(s.Trim());
	}
}

// (de)obfuscate firmware data
void __fastcall TForm1::k5_xor_firmware(uint8_t *data, const int len)
{
	static const uint8_t xor_pattern[] =
	{
		0x47, 0x22, 0xc0, 0x52, 0x5d, 0x57, 0x48, 0x94, 0xb1, 0x60, 0x60, 0xdb, 0x6f, 0xe3, 0x4c, 0x7c,
		0xd8, 0x4a, 0xd6, 0x8b, 0x30, 0xec, 0x25, 0xe0, 0x4c, 0xd9, 0x00, 0x7f, 0xbf, 0xe3, 0x54, 0x05,
		0xe9, 0x3a, 0x97, 0x6b, 0xb0, 0x6e, 0x0c, 0xfb, 0xb1, 0x1a, 0xe2, 0xc9, 0xc1, 0x56, 0x47, 0xe9,
		0xba, 0xf1, 0x42, 0xb6, 0x67, 0x5f, 0x0f, 0x96, 0xf7, 0xc9, 0x3c, 0x84, 0x1b, 0x26, 0xe1, 0x4e,
		0x3b, 0x6f, 0x66, 0xe6, 0xa0, 0x6a, 0xb0, 0xbf, 0xc6, 0xa5, 0x70, 0x3a, 0xba, 0x18, 0x9e, 0x27,
		0x1a, 0x53, 0x5b, 0x71, 0xb1, 0x94, 0x1e, 0x18, 0xf2, 0xd6, 0x81, 0x02, 0x22, 0xfd, 0x5a, 0x28,
		0x91, 0xdb, 0xba, 0x5d, 0x64, 0xc6, 0xfe, 0x86, 0x83, 0x9c, 0x50, 0x1c, 0x73, 0x03, 0x11, 0xd6,
		0xaf, 0x30, 0xf4, 0x2c, 0x77, 0xb2, 0x7d, 0xbb, 0x3f, 0x29, 0x28, 0x57, 0x22, 0xd6, 0x92, 0x8b
	};

	if (data == NULL || len <= 0)
		return;

	for (int i = 0; i < len; i++)
		data[i] ^= xor_pattern[i % sizeof(xor_pattern)];
}

// (de)obfuscate communications data
void __fastcall TForm1::k5_xor_payload(uint8_t *data, const int len)
{
	static const uint8_t xor_pattern[] =
	{
		0x16, 0x6c, 0x14, 0xe6, 0x2e, 0x91, 0x0d, 0x40, 0x21, 0x35, 0xd5, 0x40, 0x13, 0x03, 0xe9, 0x80
	};

	if (data == NULL || len <= 0)
		return;

	for (int i = 0; i < len; i++)
		data[i] ^= xor_pattern[i % sizeof(xor_pattern)];
}

int __fastcall TForm1::k5_obfuscate(struct k5_command *cmd)
{
	if (cmd == NULL)
		return 0;

	if (cmd->cmd == NULL)
		return 0;

	if (cmd->obfuscated_cmd != NULL)
	{
		free(cmd->obfuscated_cmd);
		cmd->obfuscated_cmd = NULL;
	}

	cmd->obfuscated_len    = cmd->len + 8;       // header + length + data + crc + footer
	cmd->obfuscated_cmd    = (uint8_t *)calloc(cmd->obfuscated_len, 1);

	cmd->obfuscated_cmd[0] = 0xAB;
	cmd->obfuscated_cmd[1] = 0xCD;

	cmd->obfuscated_cmd[2] = (cmd->len >> 0) & 0xff;
	cmd->obfuscated_cmd[3] = (cmd->len >> 8) & 0xff;

	memcpy(cmd->obfuscated_cmd + 4, cmd->cmd, cmd->len);

	// add the CRC
	cmd->crc_clear = crc16(cmd->obfuscated_cmd + 4, cmd->len);
	cmd->obfuscated_cmd[cmd->len + 4] = (cmd->crc_clear >> 0) & 0xff;
	cmd->obfuscated_cmd[cmd->len + 5] = (cmd->crc_clear >> 8) & 0xff;

	k5_xor_payload(cmd->obfuscated_cmd + 4, cmd->len + 2);

	cmd->obfuscated_cmd[cmd->len + 6] = 0xDC;
	cmd->obfuscated_cmd[cmd->len + 7] = 0xBA;

	cmd->crc_ok = 1;

	return 1;
}

// deobfuscate a k5 datagram and verify it
int __fastcall TForm1::k5_deobfuscate(struct k5_command *cmd)
{
	String s;

	if (cmd == NULL)
		return 0;

	if (cmd->obfuscated_cmd == NULL)
		return 0;

	if (cmd->cmd != NULL)
	{
		free(cmd->cmd);
		cmd->cmd = NULL;
	}

	// check the obfuscated datagram

	if (cmd->obfuscated_cmd[0] != 0xAB || cmd->obfuscated_cmd[1] != 0xCD)
	{	// invalid header
		if (m_verbose > 2)
		{
			Memo1->Lines->Add("error: invalid header");
			k5_hex_dump(cmd);
		}
		return 0;
	}

	if (cmd->obfuscated_cmd[cmd->obfuscated_len - 2] != 0xDC || cmd->obfuscated_cmd[cmd->obfuscated_len - 1] != 0xBA)
	{	// invalid footer
		if (m_verbose > 2)
		{
			Memo1->Lines->Add("error: invalid footer");
			k5_hex_dump(cmd);
		}
		return 0;
	}

	cmd->len = cmd->obfuscated_len - 6;       // header + length + data + crc + footer
	cmd->cmd = (uint8_t *)calloc(cmd->len, 1);
	if (cmd->cmd == NULL)
	{
		Memo1->Lines->Add("error: calloc()");
		return 0;
	}

	memcpy(cmd->cmd, cmd->obfuscated_cmd + 4, cmd->len);

	// de-obfuscate
	k5_xor_payload(cmd->cmd, cmd->len);

	// save the decrypted CRC
	cmd->crc_clear = ((uint8_t)cmd->cmd[cmd->len - 1] << 8) | ((uint8_t)cmd->cmd[cmd->len - 2] << 0);

	if (m_verbose > 2)
	{
		s.printf("de-obfuscate [%d]:", cmd->len);
		Memo1->Lines->Add("");
		Memo1->Lines->Add(s);
		Memo1->Update();

		k5_hdump(cmd->cmd, cmd->len);
	}

	const uint16_t crc1 = crc16(cmd->cmd, cmd->len - 2);
	const uint16_t crc2 = cmd->crc_clear;

	if (crc2 == 0xffff)
	{
		cmd->crc_ok = 1;
		cmd->len -= 2;    // drop the CRC
		return 1;
	}

	if (crc2 == crc1)
	{
		Memo1->Lines->Add("* the protocol actually uses proper crc on datagrams from the radio, please inform the author of the radio/firmware version");
		k5_hex_dump(cmd);

		cmd->crc_ok = 1;
		cmd->len -= 2;    // drop the CRC
		return 1;
	}

	cmd->crc_ok = 0;

	if (m_verbose > 2)
	{
		s.printf("invalid crc   rx'ed 0x%04X   computed 0x%04X", crc2, crc1);
		Memo1->Lines->Add(s);
		k5_hex_dump(cmd);
	}

	cmd->len -= 2;    	// drop the CRC

	return 0;
}

// obfuscate a command, send it
int __fastcall TForm1::k5_send_cmd(struct k5_command *cmd)
{
	String s;

	if (!m_serial.port.connected || cmd == NULL)
		return 0;

	if (k5_obfuscate(cmd) <= 0)
	{
		Memo1->Lines->Add("error: k5_obfuscate()");
		return 0;
	}

	if (m_verbose > 1)
//		k5_hex_dump(cmd);
		k5_hex_dump2(cmd, true);

	if (cmd->obfuscated_cmd != NULL && cmd->obfuscated_len > 0)
	{
		const int num_sent = m_serial.port.TxBytes(cmd->obfuscated_cmd, cmd->obfuscated_len);
		if (num_sent != cmd->obfuscated_len)
			return -2;
	}

	return 1;
}

int __fastcall TForm1::k5_send_buf(const uint8_t *buf, const int len)
{
	if (!m_serial.port.connected || buf == NULL || len <= 0)
		return 0;

	// delete any previously rx'ed/saved packets
	clearRxPacketQueue();

	struct k5_command *cmd = (struct k5_command *)calloc(sizeof(struct k5_command), 1);
	if (cmd == NULL)
		return 0;

	cmd->len = len;
	cmd->cmd = (uint8_t *)malloc(cmd->len);
	memcpy(cmd->cmd, buf, len);

	const int s_len = k5_send_cmd(cmd);

	k5_destroy_struct(cmd);

	return s_len;
}

int __fastcall TForm1::k5_read_eeprom(uint8_t *buf, const int len, const int offset)
{
	String             s;
	int                l;
	uint8_t            buffer[4 + 4 + 4];
	struct k5_command *cmd;

	if (!m_serial.port.connected || buf == NULL || len <= 0 || len > 128)
		return 0;

	if (m_verbose > 0)
	{
		s.printf("read_config  offset %04X  len %d", offset, len);
		Memo1->Lines->Add(s);
		Memo1->Update();
	}

	buffer[0] = 0x1B;                     // LS-Byte command
	buffer[1] = 0x05;                     // MS-Byte command
	buffer[2] = ((8) >> 0) & 0xff;        // LS-Byte size
	buffer[3] = ((8) >> 8) & 0xff;        // MS-Byte size

	buffer[4] = (offset >> 0) & 0xff;     // LS-Byte addr
	buffer[5] = (offset >> 8) & 0xff;     // MS-Byte addr
	buffer[6] = len;                      // len
	buffer[7] = 0;                        // ?

	memcpy(&buffer[8], session_id, 4);

	int r = k5_send_buf(buffer, sizeof(buffer));
	if (r <= 0)
		return 0;

	// wait for a rx'ed packet
	const DWORD tick = GetTickCount();
	while ((GetTickCount() - tick) <= 3000 && m_serial.port.connected)
	{
		if (m_thread->Sync)
			Application->ProcessMessages();
		else
			Sleep(1);

		if (m_rx_packet_queue.empty())
			continue;

		// fetch the rx'ed packet
		const int      rx_data_size = m_rx_packet_queue[0].size() - 2;
		const uint8_t *rx_data      = &m_rx_packet_queue[0][0];

		if (m_verbose > 2)
		{
			Memo1->Lines->Add("rx ..");
			k5_hdump(rx_data, rx_data_size + 2);
		}

		if (rx_data[0] == 0x18 && rx_data[1] == 0x05)
		{	// radio is in firmware update mode
			clearRxPacket0();		// remove spent packet
			return -1;
		}

		if (rx_data_size < (8 + len) ||
			 rx_data[0] != 0x1C ||
			 rx_data[1] != 0x05 ||
			 rx_data[4] != buffer[4] ||
			 rx_data[5] != buffer[5])
		{
			clearRxPacket0();		// remove spent packet
			continue;
		}

		memcpy(buf, &rx_data[8], len);

		clearRxPacket0();			// remove spent packet

		return 1;
	}

	if (m_verbose > 1)
		Memo1->Lines->Add("error: no valid packet received");

	return 0;
}

int __fastcall TForm1::k5_write_eeprom(uint8_t *buf, const int len, const int offset)
{
	String  s;
	uint8_t buffer[4 + 8 + 128];

	if (!m_serial.port.connected || buf == NULL || len <= 0 || len > 128)
		return 0;

	if (m_verbose > 0)
	{
		s.printf("write_config  offset %04X  len %d", offset, len);
		Memo1->Lines->Add(s);
		Memo1->Update();
	}

	// header
	buffer[0] = 0x1D;                      // LS-Byte command
	buffer[1] = 0x05;                      // MS-Byte command
	buffer[2] = ((8 + len) >> 0) & 0xff;   // LS-Byte size
	buffer[3] = ((8 + len) >> 8) & 0xff;   // MS-Byte size

	buffer[4] = (offset >> 0) & 0xff;      // addr LS-Byte
	buffer[5] = (offset >> 8) & 0xff;      // addr MS-Byte
	buffer[6] = len;                       // length
	buffer[7] = 1;                         // allow password

	memcpy(&buffer[8], session_id, 4);

	memcpy(&buffer[4 + 8], buf, len);

	const int r = k5_send_buf(buffer, 4 + 8 + len);
	if (r <= 0)
		return 0;

	// wait for a rx'ed packet
	const DWORD tick = GetTickCount();
	while ((GetTickCount() - tick) <= 2000 && m_serial.port.connected)
	{
		if (m_thread->Sync)
			Application->ProcessMessages();
		else
			Sleep(1);

		if (m_rx_packet_queue.empty())
			continue;

		// fetch the rx'ed packet
		const int      rx_data_size = m_rx_packet_queue[0].size();
		const uint8_t *rx_data      = &m_rx_packet_queue[0][0];

		if (m_verbose > 2)
		{
			Memo1->Lines->Add("rx ..");
			k5_hdump(rx_data, rx_data_size);
		}

		if (rx_data[0] == 0x18 && rx_data[1] == 0x05)
		{	// radio is in firmware update mode
			clearRxPacket0();		// remove spent packet
			return -1;
		}

		if (rx_data_size < 6 ||
			 rx_data[0] != 0x1E ||
			 rx_data[1] != 0x05 ||
			 rx_data[4] != buffer[4] ||
			 rx_data[5] != buffer[5])
		{
			clearRxPacket0();		// remove spent packet
			continue;
		}

		clearRxPacket0();			// remove spent packet

		return 1;
	}

	if (m_verbose > 1)
		Memo1->Lines->Add("error: no valid packet received");

	return 0;
}

int __fastcall TForm1::k5_wait_flash_message()
{
//	String s;

	if (!m_serial.port.connected)
		return 0;

	m_bootloader_ver = "";

	// delete any previously rx'ed/saved packets
	clearRxPacketQueue();

	if (m_verbose > 0)
	{
		Memo1->Lines->Add("");
		Memo1->Lines->Add("listening for firmware update mode packet ..");
		Memo1->Update();
	}

	// wait for a rx'ed packet
	const DWORD tick = GetTickCount();
	while ((GetTickCount() - tick) <= 3000 && m_serial.port.connected)
	{
		if (m_thread->Sync)
			Application->ProcessMessages();
		else
			Sleep(1);

		if (m_rx_packet_queue.empty())
			continue;

		// fetch the rx'ed packet
		const int      rx_data_size = m_rx_packet_queue[0].size() - 2;
		const uint8_t *rx_data      = &m_rx_packet_queue[0][0];

		if (m_verbose > 2)
		{
			Memo1->Lines->Add("rx ..");
			k5_hdump(rx_data, rx_data_size + 2);
		}

		// 18 05 20 00 01 02 02 06 1c 53 50 4a 37 47 ff 0f   .. ......SPJ7G..
		// 8c 00 53 00 32 2e 30 30 2e 30 36 00 34 0a 00 00   ..S.2.00.06.4...
		// 00 00 00 20                                       ...

		// or ..

		// 18 05 00 00 01 02 02 02 0E 53 50 4A 37 47 FF 01 90 00 8A 00     .........SPJ7G......

		#if 0
		{
//			uint8_t data[] = {
//				0x0E,0x69,0x14,0xE6,0x2F,0x93,0x0F,0x42,0x2F,0x66,0x85,0x0A,0x24,0x44,0x16,0x81,0x86,0x6C,0x9E,0xE6
//			};
			uint8_t data[] = {
				0x0C,0x69,0x1C,0xE6,0xD8,0x5A,0x0A,0x51,0x21,0x35,0xD5,0x40
			};


			// de-obfuscate (de-scramble) the payload (the data and 16-bit CRC are the scrambled bytes)
			k5_xor_payload(&data[0], ARRAY_SIZE(data));

			String s;
			for (unsigned int i = 0; i < ARRAY_SIZE(data); i++)
			{
				String s2;
				s2.printf("%02X ", data[i]);
				s += s2;
			}
			s += "    ";
			for (unsigned int i = 0; i < ARRAY_SIZE(data); i++)
			{
				const char c = data[i];
				if (c < 32)
					s += '.';
				else
					s += String(c);
			}
			Memo1->Lines->Add("TEST: " + s);
		}
		#endif







		if ((rx_data_size != 20 && rx_data_size != 36) || rx_data[0] != 0x18 || rx_data[1] != 0x05)
		{
			clearRxPacket0();		// remove spent packet
			continue;
		}

		if (rx_data_size >= 36)
		{
			char buf[17];
			memset(buf, 0, sizeof(buf));

			for (int i = 0; i < ((int)sizeof(buf) - 1); i++)
			{
				const int k = i + 20;
				if (k >= rx_data_size)
					break;
				const char c = rx_data[k];
				if (!isprint(c))
					break;
				buf[i] = c;
			}

			m_bootloader_ver = String(buf);

			Memo1->Lines->Add("");
			Memo1->Lines->Add("Bootloader version '" + m_bootloader_ver + "'");
		}
		else
		{
			Memo1->Lines->Add("");
			Memo1->Lines->Add("Bootloader version unknown (short packet rx'ed)");
		}

		clearRxPacket0();		// remove spent packet

		return 1;
	}

//	if (m_verbose > 1)
//		Memo1->Lines->Add("error: no valid packet received");

	return 0;
}

int __fastcall TForm1::k5_send_flash_version_message(const char *ver)
{
	String  s;
	uint8_t buffer[4 + 16 + 1];

	if (!m_serial.port.connected || ver == NULL || strlen(ver) > 16)
		return 0;

	memset(buffer, 0, sizeof(buffer));

	buffer[0] = 0x30;                      // LS-Byte command
	buffer[1] = 0x05;                      // MS-Byte command
	buffer[2] = ((16) >> 0) & 0xff;        // LS-Byte size
	buffer[3] = ((16) >> 8) & 0xff;        // MS-Byte size

	strcpy(buffer + 4, ver);

//	if (m_verbose > 0)
	{
//		s.printf("sending firmware version '%s' ..", ver);
//		Memo1->Lines->Add("");
//		Memo1->Lines->Add(s);
//		Memo1->Update();
	}

	const int r = k5_send_buf(buffer, 4 + 16);
	if (r <= 0)
		return 0;

	// wait for a rx'ed packet
	const DWORD tick = GetTickCount();
	while ((GetTickCount() - tick) <= 2000 && m_serial.port.connected)
	{
		if (m_thread->Sync)
			Application->ProcessMessages();
		else
			Sleep(1);

		if (m_rx_packet_queue.empty())
			continue;

		// fetch the rx'ed packet
		const int      rx_data_size = m_rx_packet_queue[0].size() - 2;
		const uint8_t *rx_data      = &m_rx_packet_queue[0][0];

		if (m_verbose > 2)
		{
			Memo1->Lines->Add("rx ..");
			k5_hdump(rx_data, rx_data_size + 2);
		}

		// 18 05 20 00 01 02 02 06 1c 53 50 4a 37 47 ff 0f   .. ......SPJ7G..
		// 8c 00 53 00 32 2e 30 30 2e 30 36 00 34 0a 00 00   ..S.2.00.06.4...
		// 00 00 00 20                                       ...

		// or ..

		// 18 05 00 00 01 02 02 02 0E 53 50 4A 37 47 FF 01 90 00 8A 00     .........SPJ7G......

//		if (rx_data_size < 36 || rx_data[0] != 0x18 || rx_data[1] != 0x05)
		if ((rx_data_size != 20 && rx_data_size != 36) || rx_data[0] != 0x18 || rx_data[1] != 0x05)
		{
			clearRxPacket0();		// remove spent packet
			continue;
		}

		clearRxPacket0();			// remove spent packet

		return 1;   // all OK
	}

	s.printf("error: no valid packet received, bootloader refused firmware version '%s' ..", ver);
	Memo1->Lines->Add(s);

	return 0;
}
/*
int __fastcall TForm1::k5_read_flash(uint8_t *buf, const int len, const int offset)
{
	String s;
	uint8_t buffer[16];
	int ok = 0;
	int r;

	if (buf == NULL || len <= 0 || len > 256)
		return 0;

	if (m_verbose > 1)
	{
		s.printf("read_flash offset %04X  len %d", offset, len);
		Memo1->Lines->Add(s);
	}

	memset(buffer, 0, sizeof(buffer));

	buffer[ 0] = 0x17;
	buffer[ 1] = 0x05;
//	buffer[ 2] = 0x0C;      // bytes 2, 3: length is 0x010C (12 + 256)
//	buffer[ 3] = 0x01;




}
*/
int __fastcall TForm1::k5_write_flash(const uint8_t *buf, const int len, const int offset, const int firmware_size)
{
	String s;
	uint8_t buffer[4 + 12 + 256];

	if (!m_serial.port.connected || buf == NULL || len <= 0 || len > 256)
		return 0;

	if (m_verbose > 0)
	{
		s.printf("write_flash  offset %04X  len %d", offset, len);
		Memo1->Lines->Add(s);
		Memo1->Update();
	}

	const uint16_t flash_max_block_addr = (firmware_size & 0xff) ? (firmware_size & 0xff00) + UVK5_FLASH_BLOCKSIZE : firmware_size & 0xff00;

	memset(buffer, 0, sizeof(buffer));

	buffer[ 0] = 0x19;                      // LS-Byte command
	buffer[ 1] = 0x05;                      // MS-Byte command
	buffer[ 2] = ((12 + 256) >> 0) & 0xff;  // LS-Byte size
	buffer[ 3] = ((12 + 256) >> 8) & 0xff;  // MS-Byte size

	buffer[ 4] = 0x8A;
	buffer[ 5] = 0x8D;
	buffer[ 6] = 0x9F;
	buffer[ 7] = 0x1D;

	buffer[ 8] = (offset >> 8) & 0xff;
	buffer[ 9] = (offset >> 0) & 0xff;
//	buffer[10] = 0xF0;
	buffer[10] = (flash_max_block_addr >> 8) & 0xff;
	buffer[11] = 0x00;

//	buffer[12] = (len >> 8) & 0xff;
//	buffer[13] = (len >> 0) & 0xff;
	buffer[12] = (len >> 0) & 0xff;
	buffer[13] = (len >> 8) & 0xff;
	buffer[14] = 0x00;
	buffer[15] = 0x00;

	// add up to 256 bytes of data
	memcpy(&buffer[16], buf, len);

	int r = k5_send_buf(buffer, sizeof(buffer));
	if (r <= 0)
		return 0;

	// wait for a rx'ed packet
	const DWORD tick = GetTickCount();
	while ((GetTickCount() - tick) <= 3000 && m_serial.port.connected)
	{
		if (m_thread->Sync)
			Application->ProcessMessages();
		else
			Sleep(1);

		if (m_rx_packet_queue.empty())
			continue;

		// fetch the 1st packet in the queue
		const int      rx_data_size = m_rx_packet_queue[0].size() - 2;
		const uint8_t *rx_data      = &m_rx_packet_queue[0][0];

		if (m_verbose > 2)
		{
			Memo1->Lines->Add("rx ..");
			k5_hdump(rx_data, rx_data_size + 2);
		}

		// good replies:
		// 1A 05 08 00 8A 8D 9F 1D 00 00 00 00
		// 1A 05 08 00 8A 8D 9F 1D 01 00 00 00
		// 1A 05 08 00 8A 8D 9F 1D 02 00 00 00

		// error replies:
		// 1A 05 08 00 00 00 00 00 00 00 01 00
		// 1A 05 08 00 00 00 00 00 00 00 01 00
		// 1A 05 08 00 00 00 00 00 00 00 01 00

		if (rx_data_size < 12  ||
			 rx_data[0] != 0x1A ||
			 rx_data[1] != 0x05 ||
			 rx_data[2] != 8    ||
			 rx_data[3] != 0    ||
			 rx_data[4] != buffer[4] ||
			 rx_data[5] != buffer[5] ||
			 rx_data[6] != buffer[6] ||
			 rx_data[7] != buffer[7] ||
			 rx_data[8] != buffer[8] ||
			 rx_data[9] != buffer[9])
		{
			clearRxPacket0();		// remove spent packet
			continue;
		}

		clearRxPacket0();			// remove spent packet

		return 1;
	}

	if (m_verbose > 1)
		Memo1->Lines->Add("error: no valid packet received");

	return 0;
}

int __fastcall TForm1::k5_hello()
{
	String s;
	uint8_t buffer[8];

	if (!m_serial.port.connected)
		return 0;

	m_firmware_ver       = "";
	m_has_custom_AES_key = false;
	m_is_in_lock_screen  = false;
	memset(m_challenge, 0, sizeof(m_challenge));

	buffer[0] = 0x14;                      // LS-Byte command
	buffer[1] = 0x05;                      // MS-Byte command
	buffer[2] = ((4) >> 0) & 0xff;         // LS-Byte size
	buffer[3] = ((4) >> 8) & 0xff;         // MS-Byte size

	memcpy(&buffer[4], session_id, 4);

	if (m_verbose > 0)
	{
		Memo1->Lines->Add("");
		Memo1->Lines->Add("sending k5_hello ..");
		Memo1->Update();
	}

	const int r = k5_send_buf(buffer, sizeof(buffer));
	if (r <= 0)
		return 0;

	// wait for a rx'ed packet
	const DWORD tick = GetTickCount();
	while ((GetTickCount() - tick) <= 1000 && m_serial.port.connected)
	{
		if (m_thread->Sync)
			Application->ProcessMessages();
		else
			Sleep(1);

		if (m_rx_packet_queue.empty())
			continue;

		// fetch the rx'ed packet
		const int      rx_data_size = m_rx_packet_queue[0].size() - 2;
		const uint8_t *rx_data      = &m_rx_packet_queue[0][0];

		if (m_verbose > 2)
		{
			Memo1->Lines->Add("rx ..");
			k5_hdump(rx_data, rx_data_size + 2);
		}

		// reply ..
		// 000000  15 05 24 00 6B 35 5F 32 2E 30 31 2E 32 36 00 00  ..$.k5_2.01.26..
		// 000010  E4 E1 00 00 00 00 00 00 42 3D 28 55 36 49 D8 27  ‰·......B=(U6Iÿ'
		// 000020  51 DF 0C 54 99 DB 2B 7F                          Qﬂ.Tô€+

		if (rx_data[0] == 0x18 && rx_data[1] == 0x05)
		{	// radio is in firmware update mode
			clearRxPacket0();		// remove spent packet
			return -1;
		}

		if (rx_data_size < (4 + 16 + 2 + 2 + sizeof(m_challenge)) || rx_data[0] != 0x15 || rx_data[1] != 0x05)
		{
			clearRxPacket0();		// remove spent packet
			continue;
		}

		char buf[17] = {0};
		memcpy(buf, &rx_data[4], 16);
		m_firmware_ver = String(buf);

		m_has_custom_AES_key = rx_data[4 + 16 + 0];
		m_is_in_lock_screen  = rx_data[4 + 16 + 1];

		memcpy(m_challenge, &rx_data[4 + 16 + 2 + 2], sizeof(m_challenge));

		Memo1->Lines->Add("");
		Memo1->Lines->Add("  firmware version: '" + m_firmware_ver + "'");
		s.printf("has custom AES key: %s", m_has_custom_AES_key ? "yes" : "no");
		Memo1->Lines->Add(s);
		s.printf(" is in lock screen: %s", m_is_in_lock_screen ? "yes" : "no");
		Memo1->Lines->Add(s);
		s = "         challenge: ";
		for (size_t i = 0; i < sizeof(m_challenge); i++)
		{
			String s2;
			s2.printf("%02X ", m_challenge[i]);
			s += s2;
		}
		Memo1->Lines->Add(s.TrimRight());

		clearRxPacket0();			// remove spent packet

		return 1;
	}

	if (m_verbose > 1)
		Memo1->Lines->Add("error: no valid packet received");

	return 0;
}

int __fastcall TForm1::k5_readADC()
{
	String s;
	uint8_t buffer[8];

	if (!m_serial.port.connected)
		return 0;

	buffer[0] = 0x29;                      // LS-Byte command
	buffer[1] = 0x05;                      // MS-Byte command
	buffer[2] = ((4) >> 0) & 0xff;         // LS-Byte size
	buffer[3] = ((4) >> 8) & 0xff;         // MS-Byte size

	memcpy(&buffer[4], session_id, 4);

	if (m_verbose > 0)
	{
		Memo1->Lines->Add("");
		Memo1->Lines->Add("sending k5_readADC ..");
		Memo1->Update();
	}

	const int r = k5_send_buf(buffer, sizeof(buffer));
	if (r <= 0)
		return 0;

	// wait for a rx'ed packet
	const DWORD tick = GetTickCount();
	while ((GetTickCount() - tick) <= 1000 && m_serial.port.connected)
	{
		if (m_thread->Sync)
			Application->ProcessMessages();
		else
			Sleep(1);

		if (m_rx_packet_queue.empty())
			continue;

		// fetch the rx'ed packet
		const int      rx_data_size = m_rx_packet_queue[0].size() - 2;
		const uint8_t *rx_data      = &m_rx_packet_queue[0][0];

		// 2A 05 04 00 AC 07 00 00

		if (m_verbose > 2)
		{
			Memo1->Lines->Add("rx ..");
			k5_hdump(rx_data, rx_data_size + 2);
		}

		if (rx_data[0] == 0x18 && rx_data[1] == 0x05)
		{	// radio is in firmware update mode
			clearRxPacket0();		// remove spent packet
			return -1;
		}

		if (rx_data_size < 8 || rx_data[0] != 0x2A || rx_data[1] != 0x05)
		{
			clearRxPacket0();		// remove spent packet
			continue;
		}

		const uint16_t adc_vol = ((uint16_t)rx_data[5] << 8) | ((uint16_t)rx_data[4] << 0);
//		const uint16_t adc_cur = ((uint16_t)rx_data[7] << 8) | ((uint16_t)rx_data[6] << 0);

		const unsigned int adc_ref_addr = 0x1F40 + (sizeof(uint16_t) * 3);
		const uint16_t adc_ref = ((uint16_t)m_config[adc_ref_addr + 1] << 8) | ((uint16_t)m_config[adc_ref_addr + 0] << 0);

		// battery calibration data in the config area
		// 0x1F40  DE 04 FA 06 45 07 5E 07 C5 07 FC 08 FF FF FF FF
		//
		//             ADC     V = (7.6 * ADC) / [3]
		// [0] 04DE .. 1246 .. 5.021V .. empty battery
		// [1] 06FA .. 1786 .. 7.197V .. 1 bar
		// [2] 0745 .. 1861 .. 7.499V .. 2 bar
		// [3] 075E .. 1886 .. 7.600V .. 3 bar
		// [4] 07C5 .. 1989 .. 8.015V .. 4 bar
		// [5] 08FC .. 2300 .. 9.268V .. overwritten by radio to 2300
		// [6] FFFF
		// [7] FFFF
		//
		// actual battery voltage = (7.6 * ADC) / [3]
		//
		// once voltage falls to 6.6V, bat icon flashes and voltage starts rapidly falling
		// radio turns off at about 5.7V

		if (adc_ref > 1000 && adc_ref < 2300)
			s.printf("battery ADC 0x%04X %u %0.3fV", adc_vol, adc_vol, (7.6f * adc_vol) / adc_ref);
		else
			s.printf("battery ADC 0x%04X %u", adc_vol, adc_vol);
		Memo1->Lines->Add("");
		Memo1->Lines->Add(s);

		clearRxPacket0();			// remove spent packet

		return 1;
	}

	if (m_verbose > 1)
		Memo1->Lines->Add("error: no valid packet received");

	return 0;
}

int __fastcall TForm1::k5_readRSSI()
{
	String s;
	uint8_t buffer[8];

	if (!m_serial.port.connected)
		return 0;

	buffer[0] = 0x27;                      // LS-Byte command
	buffer[1] = 0x05;                      // MS-Byte command
	buffer[2] = ((4) >> 0) & 0xff;         // LS-Byte size
	buffer[3] = ((4) >> 8) & 0xff;         // MS-Byte size

	memcpy(&buffer[4], session_id, 4);

	if (m_verbose > 0)
	{
		Memo1->Lines->Add("");
		Memo1->Lines->Add("sending k5_readRSSI ..");
		Memo1->Update();
	}

	const int r = k5_send_buf(buffer, sizeof(buffer));
	if (r <= 0)
		return 0;

	// wait for a rx'ed packet
	const DWORD tick = GetTickCount();
	while ((GetTickCount() - tick) <= 1000 && m_serial.port.connected)
	{
		if (m_thread->Sync)
			Application->ProcessMessages();
		else
			Sleep(1);

		if (m_rx_packet_queue.empty())
			continue;

		// fetch the rx'ed packet
		const int      rx_data_size = m_rx_packet_queue[0].size() - 2;
		const uint8_t *rx_data      = &m_rx_packet_queue[0][0];

		// 28 05 04 00 8E 00 50 42

		if (m_verbose > 2)
		{
			Memo1->Lines->Add("rx ..");
			k5_hdump(rx_data, rx_data_size + 2);
		}

		if (rx_data[0] == 0x18 && rx_data[1] == 0x05)
		{	// radio is in firmware update mode
			clearRxPacket0();		// remove spent packet
			return -1;
		}

		if (rx_data_size < 8 || rx_data[0] != 0x28 || rx_data[1] != 0x05)
		{
			clearRxPacket0();		// remove spent packet
			continue;
		}

		const uint16_t rssi_raw   = (((uint16_t)rx_data[5] << 8) | ((uint16_t)rx_data[4] << 0)) & 0x01FF;
		const uint8_t  noise_raw  = rx_data[6] & 0x7F;
		const uint8_t  glitch_raw = rx_data[7];

		const float rssi = ((float)rssi_raw / 2) - 160;

		s.printf("RSSI %u %0.1fdBm, Noise 0x%02X %u, Glitch 0x%02X %u", rssi_raw, rssi, noise_raw, noise_raw, glitch_raw, glitch_raw);
		Memo1->Lines->Add("");
		Memo1->Lines->Add(s);

		clearRxPacket0();			// remove spent packet

		return 1;
	}

	if (m_verbose > 1)
		Memo1->Lines->Add("error: no valid packet received");

	return 0;
}

int __fastcall TForm1::k5_reboot()
{
	uint8_t buffer[4];

	if (!m_serial.port.connected)
		return 0;

	buffer[0] = 0xDD;                      // LS-Byte command
	buffer[1] = 0x05;                      // MS-Byte command
	buffer[2] = ((0) >> 0) & 0xff;         // LS-Byte size
	buffer[3] = ((0) >> 8) & 0xff;         // MS-Byte size

//	if (m_verbose > 0)
	{
		Memo1->Lines->Add("");
		Memo1->Lines->Add("sending reboot radio ..");
	}

	return k5_send_buf(buffer, sizeof(buffer));
}

void __fastcall TForm1::VerboseTrackBarChange(TObject *Sender)
{
	m_verbose = VerboseTrackBar->Position;
}

void __fastcall TForm1::ReadConfigButtonClick(TObject *Sender)
{
	String  s;

	disconnect();

	// *******************************************
	// download the radios configuration data

	Memo1->Clear();
	Memo1->Lines->Add("");
	Memo1->Update();

	m_rx_mode = 1;

	if (!connect(false))
		return;

	SerialPortComboBoxChange(NULL);

	Memo1->Lines->Add("");
	Memo1->Lines->Add("Downloading configuration data from the radio ..");
	Memo1->Update();

	int r = 0;
	for (int i = 0; i < UVK5_HELLO_TRIES; i++)
	{
		r = k5_hello();
		if (r != 0)
			break;
		Application->ProcessMessages();
	}
	Memo1->Lines->Add("");
	if (r == 0)
	{
		disconnect();
		Memo1->Lines->Add("");
		Memo1->Lines->Add("radio not detected");
		SerialPortComboBoxChange(NULL);

		m_rx_mode = 0;
		connect(false);

		return;
	}
	if (r < 0)
	{
		disconnect();
		Memo1->Lines->Add("");
		Memo1->Lines->Add("error: radio is in firmware update mode - turn the radio off, then back on whilst NOT pressing the PTT");
		SerialPortComboBoxChange(NULL);

		m_rx_mode = 0;
		connect(false);

		return;
	}

	memset(m_config, 0, sizeof(m_config));

	const int size = sizeof(m_config);
//	const int size = UVK5_CONFIG_SIZE;

	const int block_len = UVK5_CONFIG_BLOCKSIZE;

	CGauge1->MaxValue = size;
	CGauge1->Progress = 0;
	CGauge1->Update();
	CGauge1->Visible = true;

	for (unsigned int i = 0; i < size; i += block_len)
	{
		r = k5_read_eeprom(&m_config[i], block_len, i);
		if (r <= 0)
		{
			s.printf("error: k5_read_eeprom() [%d]", r);
			Memo1->Lines->Add(s);
			Memo1->Lines->Add("");
			CGauge1->Visible = false;
			StatusBar1->Update();
			disconnect();
			SerialPortComboBoxChange(NULL);

			m_rx_mode = 0;
			connect(false);

			return;
		}

		CGauge1->Progress = i + block_len;
		CGauge1->Update();

		Application->ProcessMessages();
	}

	Memo1->Lines->Add("read CONFIG complete");
	Memo1->Lines->Add("");

	CGauge1->Visible = false;
	StatusBar1->Update();

	disconnect();

	SerialPortComboBoxChange(NULL);

	Memo1->Lines->Add("");
	Memo1->Update();

	// *******************************************
	// save the radios configuration data

	if (m_verbose >= 1)
	{	// show the config contents
		Memo1->Lines->BeginUpdate();
		Memo1->Lines->Add("");
		k5_hdump(&m_config[0], size);
		Memo1->Lines->EndUpdate();
		Memo1->Lines->Add("");
		Memo1->Update();
	}

	Application->BringToFront();
	Application->NormalizeTopMosts();
	SaveDialog1->Title = "Save configuration file ..";
	const bool ok = SaveDialog1->Execute();
	Application->RestoreTopMosts();
	if (!ok)
	{
		m_rx_mode = 0;
		connect(false);

		return;
	}

	String name = SaveDialog1->FileName;
	String ext  = ExtractFileExt(name).LowerCase();
	if (ext.IsEmpty())
	{
		ext = ".bin";
		name += ext;
	}

	saveFile(name, &m_config[0], size);

	// *******************************************

	m_rx_mode = 0;
	connect(false);
}

void __fastcall TForm1::WriteFirmwareButtonClick(TObject *Sender)
{
	String s;
	uint8_t flash[UVK5_MAX_FLASH_SIZE];

	disconnect();

	// *******************************************
	// load the firmware file in

	Application->BringToFront();
	Application->NormalizeTopMosts();
	OpenDialog3->Title = "Select a FIRMWARE file to upload";
	const bool ok = OpenDialog3->Execute();
	Application->RestoreTopMosts();
	if (!ok)
	{
		m_rx_mode = 0;
		connect(false);

		return;
	}

	String name = OpenDialog3->FileName;
	String ext  = ExtractFileExt(name).LowerCase();
	if (ext.IsEmpty())
	{
		ext = ".bin";
		name += ext;
	}

	m_bootloader_ver = "";
	m_firmware_ver   = "";

	if (loadFile(name) == 0)
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		Application->MessageBox("No data loaded", Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();

		m_rx_mode = 0;
		connect(false);

		return;
	}

	Memo1->Clear();
	Memo1->Lines->Add("");

	s.printf("Loaded %u (0x%04X) bytes (max 0x%04X) from ..", m_loadfile_data.size(), m_loadfile_data.size(), UVK5_FLASH_SIZE);
	Memo1->Lines->Add(s);
	Memo1->Lines->Add(m_loadfile_name);
	Memo1->Update();

	if (m_loadfile_data.size() < 2000)
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		Application->MessageBox("File appears to be to small to be a firmware file", Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();

		m_rx_mode = 0;
		connect(false);

		return;
	}

	bool encrypted = true;

	const uint16_t crc1 = crc16(&m_loadfile_data[0], m_loadfile_data.size() - 2);
	const uint16_t crc2 = ((uint16_t)m_loadfile_data[m_loadfile_data.size() - 1] << 8) | ((uint16_t)m_loadfile_data[m_loadfile_data.size() - 2] << 0);

	#if 0
		if (m_loadfile_data[0] == 0x88 && m_loadfile_data[1] == 0x13 && m_loadfile_data[2] == 0x00 && m_loadfile_data[3] == 0x20)
			encrypted = false;
		else
		if (m_loadfile_data[0] == 0x88 && m_loadfile_data[1] == 0x11 && m_loadfile_data[2] == 0x00 && m_loadfile_data[3] == 0x20)
			encrypted = false;
		else
		if (m_loadfile_data[0] == 0xF0 && m_loadfile_data[1] == 0x3F && m_loadfile_data[2] == 0x00 && m_loadfile_data[3] == 0x20)
			encrypted = false;
	#else
		if (m_loadfile_data[ 2] == 0x00 &&
			 m_loadfile_data[ 3] == 0x20 &&
			 m_loadfile_data[ 6] == 0x00 &&
			 m_loadfile_data[10] == 0x00 &&
			 m_loadfile_data[14] == 0x00)
			encrypted = false;
	#endif

	if (encrypted && crc1 == crc2)
	{	// the file appears to be encrypted

		// drop the 16-bit CRC
		m_loadfile_data.resize(m_loadfile_data.size() - 2);

		// decrypt it
		k5_xor_firmware(&m_loadfile_data[0], m_loadfile_data.size());

		#if 0
			if (m_loadfile_data[0] == 0x88 && m_loadfile_data[1] == 0x13 && m_loadfile_data[2] == 0x00 && m_loadfile_data[3] == 0x20)
				encrypted = false;
			else
			if (m_loadfile_data[0] == 0x88 && m_loadfile_data[1] == 0x11 && m_loadfile_data[2] == 0x00 && m_loadfile_data[3] == 0x20)
				encrypted = false;
			else
			if (m_loadfile_data[0] == 0xF0 && m_loadfile_data[1] == 0x3F && m_loadfile_data[2] == 0x00 && m_loadfile_data[3] == 0x20)
				encrypted = false;
		#else
			if (m_loadfile_data[ 2] == 0x00 &&
				 m_loadfile_data[ 3] == 0x20 &&
				 m_loadfile_data[ 6] == 0x00 &&
				 m_loadfile_data[10] == 0x00 &&
				 m_loadfile_data[14] == 0x00)
				encrypted = false;
		#endif

		if (!encrypted)
		{
			Memo1->Lines->Add("");
			Memo1->Lines->Add("firmware file de-obfuscated");
		}

		if (!encrypted && m_loadfile_data.size() >= (0x2000 + 16))
		{	// extract and remove the 16-byte version string

			char firmware_ver[17] = {0};
			memcpy(firmware_ver, &m_loadfile_data[0x2000], 16);

			if (m_loadfile_data.size() > (0x2000 + 16))
				memmove(&m_loadfile_data[0x2000], &m_loadfile_data[0x2000 + 16], m_loadfile_data.size() - 0x2000 - 16);
			m_loadfile_data.resize(m_loadfile_data.size() - 16);

			m_firmware_ver = String(firmware_ver);

			Memo1->Lines->Add("");
			Memo1->Lines->Add("firmware file version '" + m_firmware_ver + "'");
		}
	}

	if (encrypted)
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		Application->MessageBox("File doesn't appear to be valid for uploading", Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();

		m_rx_mode = 0;
		connect(false);

		return;
	}

	if (m_loadfile_data.size() > UVK5_MAX_FLASH_SIZE)
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		s.printf("File is to large to be a firmware file (max 0x%05X)", UVK5_MAX_FLASH_SIZE);
		Application->MessageBox(s.c_str(), Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();

		m_rx_mode = 0;
		connect(false);

		return;
	}

	if (m_loadfile_data.size() > UVK5_FLASH_SIZE)
	{
		#if 0
			Application->BringToFront();
			Application->NormalizeTopMosts();
			s.printf("File runs into bootloader area (0x%04X)\n\nStill upload ?", UVK5_FLASH_SIZE);
			const int res = Application->MessageBox(s.c_str(), Application->Title.c_str(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);
			Application->RestoreTopMosts();
			switch (res)
			{
				case IDYES:
					break;
				case IDNO:
				case IDCANCEL:

					m_rx_mode = 0;
					connect(false);

					return;
			}
		#else
			Application->BringToFront();
			Application->NormalizeTopMosts();
			s.printf("File runs into bootloader area (0x%04X)\n\nUpload cancelled", UVK5_FLASH_SIZE);
			Application->MessageBox(s.c_str(), Application->Title.c_str(), MB_ICONERROR | MB_OK);
			Application->RestoreTopMosts();

			m_rx_mode = 0;
			connect(false);

			return;
		#endif
	}

	if (m_verbose > 2)
	{	// show the files contents - takes a short while to display

		Memo1->Lines->Add("");
		Memo1->Lines->Add("creating firmware hex dump ..");
		Memo1->Update();

		Memo1->Lines->BeginUpdate();
		Memo1->Lines->Add("");
		k5_hdump(&m_loadfile_data[0], m_loadfile_data.size());
		Memo1->Lines->EndUpdate();
	}

	// *******************************************
	// upload the firmware to the radio

	Memo1->Lines->Add("");

	m_rx_mode = 1;

	if (!connect(false))
	{
		m_rx_mode = 0;
		connect(false);

		return;
	}

	SerialPortComboBoxChange(NULL);

	Memo1->Lines->Add("");
	Memo1->Lines->Add("Uploading firmware to the radio ..");
	Memo1->Update();

	int r = k5_wait_flash_message();
	if (r <= 0)
	{	// radio is either turned off or is not in firmware update mode

		for (int i = 0; i < UVK5_HELLO_TRIES; i++)
		{
			r = k5_hello();
			if (r != 0)
				break;
			Application->ProcessMessages();
		}
		Memo1->Lines->Add("");
		if (r == 0)
		{
			disconnect();
			Memo1->Lines->Add("");
			Memo1->Lines->Add("radio not detected");
			SerialPortComboBoxChange(NULL);

			m_rx_mode = 0;
			connect(false);

			return;
		}
		if (r > 0)
		{
			disconnect();
			Memo1->Lines->Add("");
			Memo1->Lines->Add("error: radio is not in firmware update mode - turn the radio off, then back on whilst pressing the PTT");
			SerialPortComboBoxChange(NULL);

			m_rx_mode = 0;
			connect(false);

			return;
		}
	}
	else
		Memo1->Lines->Add("");

	// the bootloader will at times refuse the firmware version
	if (m_firmware_ver.Length() >= 2 && m_bootloader_ver.Length() >= 2)
	{
		if (m_firmware_ver[1] >= '0' && m_firmware_ver[1] <= '9')
			if (m_firmware_ver[2] == '.' && m_bootloader_ver[2] == '.')
//				if (m_firmware_ver[1] > m_bootloader_ver[1])
				if (m_firmware_ver[1] != m_bootloader_ver[1])
					m_firmware_ver[1] = '*';
	}
	else
		m_firmware_ver = '*';

	// pass the firmware version to the bootloader
	// it will either allow the upload, or not
	r = k5_send_flash_version_message(m_firmware_ver.c_str());
	if (r <= 0)
	{
		Memo1->Lines->Add("error: firmware upload refused");
		Memo1->Lines->Add("");
		disconnect();
		SerialPortComboBoxChange(NULL);

		m_rx_mode = 0;
		connect(false);

		return;
	}

	Memo1->Lines->Add("");

	if (m_verbose <= 0)
		Memo1->Lines->Add("writing firmware ..");

	CGauge1->MaxValue = m_loadfile_data.size();
	CGauge1->Progress = 0;
	CGauge1->Visible = true;
	CGauge1->Update();

	for (int i = 0; i < (int)m_loadfile_data.size(); i += UVK5_FLASH_BLOCKSIZE)
	{
		int len = (int)m_loadfile_data.size() - i;
		if (len > UVK5_FLASH_BLOCKSIZE)
			 len = UVK5_FLASH_BLOCKSIZE;

		r = k5_write_flash(&m_loadfile_data[i], len, i, m_loadfile_data.size());
		if (r <= 0)
		{
			s.printf("error: k5_write_flash() [%d]", r);
			Memo1->Lines->Add(s);
			Memo1->Lines->Add("");
			CGauge1->Visible = false;
			StatusBar1->Update();
			disconnect();
			SerialPortComboBoxChange(NULL);

			m_rx_mode = 0;
			connect(false);

			return;
		}

		CGauge1->Progress = i + len;
		CGauge1->Update();

		Application->ProcessMessages();
	}

	Memo1->Lines->Add("write FLASH complete");
	Memo1->Update();

	CGauge1->Visible = false;
	StatusBar1->Update();

	k5_reboot();
	{	// give the serial port time to complete the data TX
		const DWORD tick = GetTickCount();
		while ((GetTickCount() - tick) < 300)
		{
			Application->ProcessMessages();
			Sleep(1);
		}
	}

	Memo1->Lines->Add("");

	disconnect();
	SerialPortComboBoxChange(NULL);

	// *******************************************

	m_rx_mode = 0;
	connect(false);
}

void __fastcall TForm1::WriteConfigButtonClick(TObject *Sender)
{
	String s;

	disconnect();

	// *******************************************
	// load the configuration file in

	Memo1->Lines->Add("");

	Application->BringToFront();
	Application->NormalizeTopMosts();
	OpenDialog1->Title = "Load configuration file ..";
	const bool ok = OpenDialog1->Execute();
	Application->RestoreTopMosts();
	if (!ok)
	{
		m_rx_mode = 0;
		connect(false);

		return;
	}

	String name = OpenDialog1->FileName;
	String ext  = ExtractFileExt(name).LowerCase();
	if (ext.IsEmpty())
	{
		ext = ".bin";
		name += ext;
	}

	if (loadFile(name) == 0)
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		Application->MessageBox("No data loaded", Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();

		m_rx_mode = 0;
		connect(false);

		return;
	}

	Memo1->Clear();
	Memo1->Lines->Add("");

	s.printf("Loaded %u bytes from ..", m_loadfile_data.size());
	Memo1->Lines->Add(s);
	Memo1->Lines->Add(m_loadfile_name);
	Memo1->Update();

	if (m_loadfile_data.size() <= 1000)
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		Application->MessageBox("File appears to be to small to be an config file", Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();

		m_rx_mode = 0;
		connect(false);

		return;
	}

	if (m_loadfile_data.size() > UVK5_MAX_CONFIG_SIZE)
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		s.printf("File is to large to be an config file (max 0x%04X)", UVK5_MAX_CONFIG_SIZE);
		Application->MessageBox(s.c_str(), Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();

		m_rx_mode = 0;
		connect(false);

		return;
	}

	if (m_loadfile_data.size() > UVK5_CONFIG_SIZE)
	{
		#if 1
			Application->BringToFront();
			Application->NormalizeTopMosts();
			s.printf("File is larger than normal (0x%04X)\n\nStill upload ?", UVK5_CONFIG_SIZE);
			const int res = Application->MessageBox(s.c_str(), Application->Title.c_str(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);
			Application->RestoreTopMosts();
			switch (res)
			{
				case IDYES:
					break;
				case IDNO:
				case IDCANCEL:

					m_rx_mode = 0;
					connect(false);

					return;
			}
		#else
			Application->BringToFront();
			Application->NormalizeTopMosts();
			s.printf("File is to large to be an config file (max 0x%04X)\n\nUpload cancelled", UVK5_CONFIG_SIZE);
			Application->MessageBox(s.c_str(), Application->Title.c_str(), MB_ICONERROR | MB_OK);
			Application->RestoreTopMosts();

			m_rx_mode = 0;
			connect(false);

			return;
		#endif
	}

	// *******************************************
	// upload the configuration data to the radio

	Memo1->Lines->Add("");

	m_rx_mode = 1;

	if (!connect(false))
	{
		m_rx_mode = 0;
		connect(false);

		return;
	}

	SerialPortComboBoxChange(NULL);

	Memo1->Lines->Add("");
	Memo1->Lines->Add("Uploading configuration data to the radio ..");
	Memo1->Update();

	int r = 0;
	for (int i = 0; i < UVK5_HELLO_TRIES; i++)
	{
		r = k5_hello();
		if (r != 0)
			break;
		Application->ProcessMessages();
	}
	Memo1->Lines->Add("");
	if (r == 0)
	{
		disconnect();
		Memo1->Lines->Add("");
		Memo1->Lines->Add("radio not detected");
		SerialPortComboBoxChange(NULL);

		m_rx_mode = 0;
		connect(false);

		return;
	}
	if (r < 0)
	{
		disconnect();
		Memo1->Lines->Add("");
		Memo1->Lines->Add("error: radio is in firmware update mode - turn the radio off, then back on whilst NOT pressing the PTT");
		SerialPortComboBoxChange(NULL);

		m_rx_mode = 0;
		connect(false);

		return;
	}

	if (m_verbose > 2)
	{	//	show the files contents - takes a short while to display
		Memo1->Lines->BeginUpdate();
		Memo1->Lines->Add("");
		k5_hdump(&m_loadfile_data[0], m_loadfile_data.size());
		Memo1->Lines->EndUpdate();
		Memo1->Lines->Add("");
	}

	Memo1->Lines->Add("writing config area ..");

	const int size = m_loadfile_data.size();

	CGauge1->MaxValue = size;
	CGauge1->Progress = 0;
	CGauge1->Visible = true;
	CGauge1->Update();

	for (int i = 0; i < size; i += UVK5_CONFIG_BLOCKSIZE)
	{
		const int len  = UVK5_CONFIG_BLOCKSIZE;

		const int r = k5_write_eeprom(&m_loadfile_data[i], len, i);
		if (r <= 0)
		{
			s.printf("error: k5_write_eeprom() [%d]", r);
			Memo1->Lines->Add(s);
			Memo1->Lines->Add("");
			CGauge1->Visible = false;
			StatusBar1->Update();
			disconnect();
			SerialPortComboBoxChange(NULL);

			m_rx_mode = 0;
			connect(false);

			return;
		}

		CGauge1->Progress = i + len;
		CGauge1->Update();

		Application->ProcessMessages();
	}

	Memo1->Lines->Add("write CONFIG complete");
	Memo1->Update();

	CGauge1->Visible = false;
	StatusBar1->Update();

	k5_reboot();
	{	// give the serial port time to complete the data TX
		const DWORD tick = GetTickCount();
		while ((GetTickCount() - tick) < 300)
		{
			Application->ProcessMessages();
			Sleep(1);
		}
	}

	Memo1->Lines->Add("");

	disconnect();

	SerialPortComboBoxChange(NULL);

	m_rx_mode = 0;
	connect(false);
}

void __fastcall TForm1::SerialPortComboBoxChange(TObject *Sender)
{
	const bool enabled = !m_serial.port.connected && (SerialPortComboBox->ItemIndex > 0);

	ReadConfigButton->Enabled       = enabled;
	WriteConfigButton->Enabled      = enabled;
	ReadCalibrationButton->Enabled  = enabled;
	WriteCalibrationButton->Enabled = enabled;
	WriteFirmwareButton->Enabled    = enabled;
	ReadADCButton->Enabled          = enabled;
	ReadRSSIButton->Enabled         = enabled;

//	SerialPortComboBox->Enabled     = !m_serial.port.connected;
//	SerialSpeedComboBox->Enabled    = !m_serial.port.connected;
}

void __fastcall TForm1::ReadADCButtonClick(TObject *Sender)
{
	String s;

	disconnect();

	Memo1->Lines->Add("");

	m_rx_mode = 1;

	if (!connect(false))
	{
		m_rx_mode = 0;
		connect(false);

		return;
	}

	SerialPortComboBoxChange(NULL);

	Memo1->Lines->Add("");
	Memo1->Lines->Add("Reading ADC ..");
	Memo1->Update();

#if 0
	int r = 0;
	for (int i = 0; i < UVK5_HELLO_TRIES; i++)
	{
		r = k5_hello();
		if (r != 0)
			break;
		Application->ProcessMessages();
	}
	Memo1->Lines->Add("");
	if (r == 0)
	{
		Memo1->Lines->Add("");
		Memo1->Lines->Add("radio not detected");
		disconnect();
		SerialPortComboBoxChange(NULL);

		m_rx_mode = 0;
		connect(false);

		return;
	}
	if (r < 0)
	{
		Memo1->Lines->Add("");
		Memo1->Lines->Add("error: radio is in firmware update mode - turn the radio off, then back on whilst NOT pressing the PTT");
		disconnect();
		SerialPortComboBoxChange(NULL);

		m_rx_mode = 0;
		connect(false);

		return;
	}
#else
	int r;
#endif

	r = k5_readADC();
	if (r <= 0)
	{
		s.printf("error: k5_readADC() [%d]", r);
		Memo1->Lines->Add(s);
		Memo1->Lines->Add("");
		disconnect();
		SerialPortComboBoxChange(NULL);

		m_rx_mode = 0;
		connect(false);

		return;
	}

	disconnect();
	SerialPortComboBoxChange(NULL);

	m_rx_mode = 0;
	connect(false);
}

void __fastcall TForm1::ReadRSSIButtonClick(TObject *Sender)
{
	String s;

	disconnect();

	Memo1->Lines->Add("");

	m_rx_mode = 1;

	if (!connect(false))
	{
		m_rx_mode = 0;
		connect(false);

		return;
	}

	SerialPortComboBoxChange(NULL);

	Memo1->Lines->Add("");
	Memo1->Lines->Add("Reading RSSI ..");
	Memo1->Update();

#if 0
	int r = 0;
	for (int i = 0; i < UVK5_HELLO_TRIES; i++)
	{
		r = k5_hello();
		if (r != 0)
			break;
		Application->ProcessMessages();
	}
	Memo1->Lines->Add("");
	if (r == 0)
	{
		Memo1->Lines->Add("");
		Memo1->Lines->Add("radio not detected");
		disconnect();
		SerialPortComboBoxChange(NULL);

		m_rx_mode = 0;
		connect(false);

		return;
	}
	if (r < 0)
	{
		Memo1->Lines->Add("");
		Memo1->Lines->Add("error: radio is in firmware update mode - turn the radio off, then back on whilst NOT pressing the PTT");
		disconnect();
		SerialPortComboBoxChange(NULL);

		m_rx_mode = 0;
		connect(false);

		return;
	}
#else
	int r;
#endif

	r = k5_readRSSI();
	if (r <= 0)
	{
		s.printf("error: k5_readRSSI() [%d]", r);
		Memo1->Lines->Add(s);
		Memo1->Lines->Add("");
		disconnect();
		SerialPortComboBoxChange(NULL);

		m_rx_mode = 0;
		connect(false);

		return;
	}

	disconnect();
	SerialPortComboBoxChange(NULL);

	m_rx_mode = 0;
	connect(false);
}

void __fastcall TForm1::SerialPortComboBoxSelect(TObject *Sender)
{
	// get radio details

	String s;

	if (SerialPortComboBox->ItemIndex <= 0 || m_serial.port.connected)
		return;

	{
		CCriticalSection cs(m_rx_lines_cs);
		m_rx_lines.resize(0);
	}

	Memo1->Clear();
	Memo1->Lines->Add("");
	Memo1->Update();

#if 0
	m_rx_mode = 1;

	if (!connect(false))
	{
		m_rx_mode = 0;
		connect(false);

		return;
	}

	SerialPortComboBoxChange(NULL);

	Memo1->Lines->Add("");
	Memo1->Lines->Add("Fetching radio details ..");
	Memo1->Update();

	int r = k5_wait_flash_message();
	if (r <= 0)
	{	// radio is either turned off or is not in firmware update mode

		for (int i = 0; i < UVK5_HELLO_TRIES; i++)
		{
			r = k5_hello();
			if (r != 0)
				break;
			Application->ProcessMessages();
		}
		Memo1->Lines->Add("");
		if (r == 0)
		{
			disconnect();
			Memo1->Lines->Add("");
			Memo1->Lines->Add("radio not detected");
			SerialPortComboBoxChange(NULL);

			m_rx_mode = 0;
			connect(false);

			return;
		}
		if (r > 0)
		{
			disconnect();
			Memo1->Lines->Add("");
			Memo1->Lines->Add("error: radio is in user mode");
			SerialPortComboBoxChange(NULL);

			m_rx_mode = 0;
			connect(false);

			return;
		}
	}
	else
	{
		disconnect();

		Memo1->Lines->Add("");
		if (r > 0)
			Memo1->Lines->Add("radio is in firmware update mode");
		else
			Memo1->Lines->Add("radio not detected");

		SerialPortComboBoxChange(NULL);

		m_rx_mode = 0;
		connect(false);

		return;
	}

	disconnect();

	SerialPortComboBoxChange(NULL);
#endif

	m_rx_mode = 0;
	connect(false);
}

void __fastcall TForm1::SerialSpeedComboBoxSelect(TObject *Sender)
{
	SerialPortComboBoxSelect(NULL);
}

void __fastcall TForm1::ReadCalibrationButtonClick(TObject *Sender)
{
	String  s;

	disconnect();

	// *******************************************
	// download the radios calibration data

	Memo1->Clear();
	Memo1->Lines->Add("");
	Memo1->Update();

	m_rx_mode = 1;

	if (!connect(false))
	{
		m_rx_mode = 0;
		connect(false);

		return;
	}

	SerialPortComboBoxChange(NULL);

	Memo1->Lines->Add("");
	Memo1->Lines->Add("Downloading calibration data from the radio ..");
	Memo1->Update();

	int r = 0;
	for (int i = 0; i < UVK5_HELLO_TRIES; i++)
	{
		r = k5_hello();
		if (r != 0)
			break;
		Application->ProcessMessages();
	}
	Memo1->Lines->Add("");
	if (r == 0)
	{
		disconnect();
		Memo1->Lines->Add("");
		Memo1->Lines->Add("radio not detected");
		SerialPortComboBoxChange(NULL);

		m_rx_mode = 0;
		connect(false);

		return;
	}
	if (r < 0)
	{
		disconnect();
		Memo1->Lines->Add("");
		Memo1->Lines->Add("error: radio is in firmware update mode - turn the radio off, then back on whilst NOT pressing the PTT");
		SerialPortComboBoxChange(NULL);

		m_rx_mode = 0;
		connect(false);

		return;
	}

	const uint32_t calib_addr = UVK5_MAX_CONFIG_SIZE - UVK5_CALIB_SIZE;
	const int      size       = sizeof(m_calib);
	const int      block_len  = UVK5_CONFIG_BLOCKSIZE;

	memset(m_calib, 0, sizeof(m_calib));

	CGauge1->MaxValue = size;
	CGauge1->Progress = 0;
	CGauge1->Update();
	CGauge1->Visible = true;

	for (unsigned int i = 0; i < size; i += block_len)
	{
		r = k5_read_eeprom(&m_calib[i], block_len, calib_addr + i);
		if (r <= 0)
		{
			s.printf("error: k5_read_eeprom() [%d]", r);
			Memo1->Lines->Add(s);
			Memo1->Lines->Add("");
			CGauge1->Visible = false;
			StatusBar1->Update();
			disconnect();
			SerialPortComboBoxChange(NULL);

			m_rx_mode = 0;
			connect(false);

			return;
		}

		CGauge1->Progress = i + block_len;
		CGauge1->Update();

		Application->ProcessMessages();
	}

	Memo1->Lines->Add("read CALIBRATION complete");
	Memo1->Lines->Add("");

	CGauge1->Visible = false;
	StatusBar1->Update();

	disconnect();

	SerialPortComboBoxChange(NULL);

	Memo1->Lines->Add("");
	Memo1->Update();

	// *******************************************
	// save the radios calibration data

//	if (m_verbose > 0)
	{	// show the calibration contents
		Memo1->Lines->BeginUpdate();
		//Memo1->Lines->Add("");
		k5_hdump(&m_calib[0], size);
		Memo1->Lines->EndUpdate();
		Memo1->Lines->Add("");
		Memo1->Update();
	}

	Application->BringToFront();
	Application->NormalizeTopMosts();
	SaveDialog2->Title = "Save CALIBRATION data file ..";
	const bool ok = SaveDialog2->Execute();
	Application->RestoreTopMosts();
	if (!ok)
	{
		m_rx_mode = 0;
		connect(false);

		return;
	}

	String name = SaveDialog2->FileName;
	String ext  = ExtractFileExt(name).LowerCase();
	if (ext.IsEmpty())
	{
		ext = ".bin";
		name += ext;
	}

	saveFile(name, &m_calib[0], size);

	// *******************************************

	{
//		t_calibration *calibration = (t_calibration *)&m_calib;




		// battery calibration
		// 0x1F40  DE 04 FA 06 45 07 5E 07 C5 07 FC 08 FF FF FF FF
		//
		//             ADC     V = (760 * ADC) / [3] / 100
		// [0] 04DE .. 1246 .. 5.021V .. empty battery
		// [1] 06FA .. 1786 .. 7.197V .. 1 bar
		// [2] 0745 .. 1861 .. 7.499V .. 2 bar
		// [3] 075E .. 1886 .. 7.600V .. 3 bar
		// [4] 07C5 .. 1989 .. 8.015V .. 4 bar
		// [5] 08FC .. 2300 .. 9.268V .. overwritten by radio to 2300
		// [6] FFFF
		// [7] FFFF
		//
		// actual battery volt = (760 * ADC) / [3] / 100
	}

	// *******************************************

	m_rx_mode = 0;
	connect(false);
}

void __fastcall TForm1::StatusBar1Resize(TObject *Sender)
{
	const int panel = 2;

	int x = StatusBar1->Left;
	for (int i = 0; i < StatusBar1->Panels->Count; i++)
		if (i < panel)
			x += StatusBar1->Panels->Items[i]->Width;

	StatusBar1->Panels->Items[panel]->Width = StatusBar1->Width - x;

	RECT Rect;
	StatusBar1->Perform(SB_GETRECT, 0, (LPARAM)&Rect);

	CGauge1->Top    = Rect.top + 4;
	CGauge1->Left   = x + 10;
	CGauge1->Width  = StatusBar1->Panels->Items[panel]->Width - 36;
	CGauge1->Height = Rect.bottom - Rect.top - 8;
}

void __fastcall TForm1::WriteCalibrationButtonClick(TObject *Sender)
{
	String s;

	disconnect();

	// *******************************************
	// load the firmware file in

	Application->BringToFront();
	Application->NormalizeTopMosts();
	OpenDialog2->Title = "Select a CALIBRATION file to upload";
	const bool ok = OpenDialog2->Execute();
	Application->RestoreTopMosts();
	if (!ok)
	{
		m_rx_mode = 0;
		connect(false);

		return;
	}

	String name = OpenDialog2->FileName;
	String ext  = ExtractFileExt(name).LowerCase();
	if (ext.IsEmpty())
	{
		ext = ".bin";
		name += ext;
	}

	if (loadFile(name) == 0)
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		Application->MessageBox("No data loaded", Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();

		m_rx_mode = 0;
		connect(false);

		return;
	}

	Memo1->Clear();
	Memo1->Lines->Add("");

	s.printf("Loaded %u (0x%04X) bytes (max 0x%04X) from '%s'", m_loadfile_data.size(), m_loadfile_data.size(), UVK5_FLASH_SIZE, m_loadfile_name.c_str());
	Memo1->Lines->Add(s);
	Memo1->Update();

	if (m_loadfile_data.size() != UVK5_CALIB_SIZE)
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		Application->MessageBox("File size is invalid", Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();

		m_rx_mode = 0;
		connect(false);

		return;
	}

	if (m_verbose > 2)
	{	// show the files contents - takes a short while to display
		Memo1->Lines->BeginUpdate();
		Memo1->Lines->Add("");
		k5_hdump(&m_loadfile_data[0], m_loadfile_data.size());
		Memo1->Lines->EndUpdate();
	}

	// *******************************************
	// upload the calibration data to the radio

	Memo1->Lines->Add("");

	m_rx_mode = 1;

	if (!connect(false))
	{
		m_rx_mode = 0;
		connect(false);

		return;
	}

	SerialPortComboBoxChange(NULL);

	Memo1->Lines->Add("");
	Memo1->Lines->Add("Uploading calibration data to the radio ..");
	Memo1->Update();

	int r = 0;
	for (int i = 0; i < UVK5_HELLO_TRIES; i++)
	{
		r = k5_hello();
		if (r != 0)
			break;
		Application->ProcessMessages();
	}
	Memo1->Lines->Add("");
	if (r == 0)
	{
		disconnect();
		Memo1->Lines->Add("");
		Memo1->Lines->Add("radio not detected");
		SerialPortComboBoxChange(NULL);

		m_rx_mode = 0;
		connect(false);

		return;
	}
	if (r < 0)
	{
		disconnect();
		Memo1->Lines->Add("");
		Memo1->Lines->Add("error: radio is in firmware update mode - turn the radio off, then back on whilst NOT pressing the PTT");
		SerialPortComboBoxChange(NULL);

		m_rx_mode = 0;
		connect(false);

		return;
	}

	if (m_verbose > 2)
	{	//	show the files contents - takes a short while to display
		Memo1->Lines->BeginUpdate();
		Memo1->Lines->Add("");
		k5_hdump(&m_loadfile_data[0], m_loadfile_data.size());
		Memo1->Lines->EndUpdate();
		Memo1->Lines->Add("");
	}

	Memo1->Lines->Add("writing config area ..");

	const uint32_t calib_addr = UVK5_MAX_CONFIG_SIZE - UVK5_CALIB_SIZE;

	int size = m_loadfile_data.size();

	CGauge1->MaxValue = size;
	CGauge1->Progress = 0;
	CGauge1->Visible = true;
	CGauge1->Update();

	for (int i = 0; i < size; i += UVK5_CONFIG_BLOCKSIZE)
	{
		const int len  = UVK5_CONFIG_BLOCKSIZE;

		const int r = k5_write_eeprom(&m_loadfile_data[i], len, calib_addr + i);
		if (r <= 0)
		{
			s.printf("error: k5_write_eeprom() [%d]", r);
			Memo1->Lines->Add(s);
			Memo1->Lines->Add("");
			CGauge1->Visible = false;
			StatusBar1->Update();
			disconnect();
			SerialPortComboBoxChange(NULL);

			m_rx_mode = 0;
			connect(false);

			return;
		}

		CGauge1->Progress = i + len;
		CGauge1->Update();

		Application->ProcessMessages();
	}

	Memo1->Lines->Add("write CALIBRATION complete");
	Memo1->Update();

	CGauge1->Visible = false;
	StatusBar1->Update();

	k5_reboot();
	{	// give the serial port time to complete the data TX
		const DWORD tick = GetTickCount();
		while ((GetTickCount() - tick) < 300)
		{
			Application->ProcessMessages();
			Sleep(1);
		}
	}

	Memo1->Lines->Add("");

	disconnect();

	SerialPortComboBoxChange(NULL);

	// *******************************************

	m_rx_mode = 0;
	connect(false);
}

