
/*
 * Use at your own risk
 *
 * Uses modified comm routines from Jacek Lipkowski <sq5bpf@lipkowski.org>
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

#ifndef Unit1H
#define Unit1H

#define VC_EXTRALEAN
#define WIN32_EXTRA_LEAN
#define WIN32_LEAN_AND_MEAN

#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <ComCtrls.hpp>
#include <Buttons.hpp>
#include <Menus.hpp>
#include <Dialogs.hpp>
#include "CGAUGES.h"

#include <vector>
#include <stdint.h>

#include "SerialPort.h"
#include "HighResolutionTick.h"
#include "CriticalSection.h"

#define WM_INIT_GUI          (WM_USER + 100)
#define WM_CONNECT           (WM_USER + 101)
#define WM_DISCONNECT        (WM_USER + 102)

#define ARRAY_SIZE(array)    (sizeof(array) / sizeof(array[0]))

struct k5_command
{
	uint8_t *cmd;
	int      len;
	uint8_t *obfuscated_cmd;
	int      obfuscated_len;
	int      crc_ok;
};

// ******************************************************************************

typedef void __fastcall (__closure *mainForm_threadProcess)();

class CThread : public TThread
{
	private:
		mainForm_threadProcess m_process;
		bool                   m_sync;
		DWORD                  m_sleep_ms;
		HANDLE                 m_mutex;

	protected:
		void __fastcall Execute()
		{
			DWORD res;
			while (!Terminated)
			{
				res = WAIT_FAILED;
				if (m_mutex)
				{
					res = WaitForSingleObject(m_mutex, m_sleep_ms);
					if (res == WAIT_TIMEOUT)
					{
					}
					else
					if (res == WAIT_OBJECT_0)
					{
						ReleaseMutex(m_mutex);
					}
				}
				else
				{
					Sleep(m_sleep_ms);
				}

				if (m_process != NULL)
				{
					if (!m_sync)
						m_process();
					else
						Synchronize(m_process);
				}
			}
			ReturnValue = 0;
		}

	public:
		__fastcall CThread(mainForm_threadProcess process, TThreadPriority priority, DWORD sleep_ms, bool start, bool sync) : TThread(!start)
		{
			m_sleep_ms = sleep_ms;
			m_process  = process;
			m_sync     = sync;

			FreeOnTerminate = false;
			Priority        = priority;

			m_mutex = CreateMutex(NULL, TRUE, NULL);
		}

		virtual __fastcall ~CThread()
		{
			m_process = NULL;

			if (m_mutex)
			{
				WaitForSingleObject(m_mutex, 100);		// wait for upto 100ms
				CloseHandle(m_mutex);
				m_mutex = NULL;
			}
		}

		__property bool Sync     = {read = m_sync,     write = m_sync};
		__property DWORD SleepMS = {read = m_sleep_ms, write = m_sleep_ms};
		__property HANDLE Mutex  = {read = m_mutex};
};

// ******************************************************************************

class TForm1 : public TForm
{
__published:	// IDE-managed Components
	TStatusBar *StatusBar1;
	TOpenDialog *OpenDialog1;
	TSaveDialog *SaveDialog1;
	TTimer *Timer1;
	TPanel *Panel1;
	TMemo *Memo1;
	TPanel *Panel2;
	TLabel *Label1;
	TLabel *Label2;
	TComboBox *SerialPortComboBox;
	TComboBox *SerialSpeedComboBox;
	TButton *ClearButton;
	TButton *ReadEEPROMButton;
	TButton *WriteFirmwareButton;
	TTrackBar *VerboseTrackBar;
	TCGauge *CGauge1;
	TButton *WriteEEPROMButton;
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall SerialPortComboBoxDropDown(TObject *Sender);
	void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
			 TShiftState Shift);
	void __fastcall Timer1Timer(TObject *Sender);
	void __fastcall ClearButtonClick(TObject *Sender);
	void __fastcall VerboseTrackBarChange(TObject *Sender);
	void __fastcall ReadEEPROMButtonClick(TObject *Sender);
	void __fastcall WriteFirmwareButtonClick(TObject *Sender);
	void __fastcall WriteEEPROMButtonClick(TObject *Sender);
	void __fastcall SerialPortComboBoxChange(TObject *Sender);

private:	// User declarations

	String                m_ini_filename;

	int                   m_screen_width;
	int                   m_screen_height;
	SYSTEM_INFO           m_system_info;

	CCriticalSectionObj   m_thread_cs;
	CThread              *m_thread;

	String                m_loadfile_name;
	std::vector <uint8_t> m_loadfile_data;

	String                m_bootloader_ver;
	String                m_firmware_ver;

	int                   m_verbose;

	std::vector < std::vector <uint8_t> > m_rx_packet_queue;

	struct
	{
		String                port_name;
		CSerialPort           port;
		std::vector <uint8_t> rx_buffer;
		volatile uint32_t     rx_buffer_wr;
		CHighResolutionTick   rx_timer;
	} m_serial;

	void __fastcall loadSettings();
	void __fastcall saveSettings();

	int    __fastcall saveFile(String filename, const uint8_t *data, const size_t size);
	size_t __fastcall loadFile(String filename);

	HANDLE __fastcall openBleService();

	void __fastcall comboBoxAutoWidth(TComboBox *comboBox);
	void __fastcall updateSerialPortCombo();

	void __fastcall disconnect();
	bool __fastcall connect(const bool clear_memo = true);

	void __fastcall threadProcess();

	void __fastcall clearRxPacketQueue();

	std::vector <String> __fastcall stringSplit(String s, String param);

	void __fastcall hex_dump(const struct k5_command *cmd, const bool tx);

	uint16_t __fastcall crc16xmodem(const uint8_t *data, const int size);
	void __fastcall destroy_k5_struct(struct k5_command *cmd);
	void __fastcall hdump(const uint8_t *buf, const int len);
	void __fastcall k5_hexdump(const struct k5_command *cmd);
	void __fastcall xor_firmware(uint8_t *data, const int len);
	void __fastcall xor_payload(uint8_t *data, const int len);
	int  __fastcall k5_obfuscate(struct k5_command *cmd);
	int  __fastcall k5_deobfuscate(struct k5_command *cmd);
	int  __fastcall k5_send_cmd(struct k5_command *cmd);
	int  __fastcall k5_send_buf(const uint8_t *buf, const int len);
	int  __fastcall k5_read_eeprom(uint8_t *buf, const int len, const int offset);
	int  __fastcall k5_write_eeprom(uint8_t *buf, const int len, const int offset);
	int  __fastcall wait_flash_message();
	int  __fastcall k5_send_flash_version_message(const char *ver);
//	int  __fastcall k5_readflash(uint8_t *buf, const int len, const int offset);
	int  __fastcall k5_write_flash(const uint8_t *buf, const int len, const int offset, const int firmware_size);
	int  __fastcall k5_prepare(const int retry);
	int  __fastcall k5_reboot();

	void __fastcall WMWindowPosChanging(TWMWindowPosChanging &msg);
	void __fastcall CMMouseEnter(TMessage &msg);
	void __fastcall CMMouseLeave(TMessage &msg);
	void __fastcall WMInitGUI(TMessage &msg);
	void __fastcall WMConnect(TMessage &msg);
	void __fastcall WMDisconnect(TMessage &msg);

protected:
	#pragma option push -vi-
	BEGIN_MESSAGE_MAP
		VCL_MESSAGE_HANDLER(WM_WINDOWPOSCHANGING, TWMWindowPosMsg, WMWindowPosChanging);

		VCL_MESSAGE_HANDLER(CM_MOUSELEAVE, TMessage, CMMouseLeave);
		VCL_MESSAGE_HANDLER(CM_MOUSEENTER, TMessage, CMMouseEnter);

		VCL_MESSAGE_HANDLER(WM_INIT_GUI, TMessage, WMInitGUI);

		VCL_MESSAGE_HANDLER(WM_CONNECT, TMessage, WMConnect);
		VCL_MESSAGE_HANDLER(WM_DISCONNECT, TMessage, WMDisconnect);

	END_MESSAGE_MAP(TForm)
	#pragma option pop

public:		// User declarations
	__fastcall TForm1(TComponent* Owner);
};

extern PACKAGE TForm1 *Form1;

#endif

