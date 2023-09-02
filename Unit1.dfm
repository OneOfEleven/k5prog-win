object Form1: TForm1
  Left = 297
  Top = 139
  Width = 690
  Height = 300
  Caption = 'Form1'
  Color = clBtnFace
  Constraints.MinHeight = 300
  Constraints.MinWidth = 690
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  KeyPreview = True
  OldCreateOrder = False
  OnClose = FormClose
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  OnKeyDown = FormKeyDown
  PixelsPerInch = 96
  TextHeight = 13
  object StatusBar1: TStatusBar
    Left = 0
    Top = 242
    Width = 674
    Height = 19
    Panels = <
      item
        Alignment = taCenter
        Width = 200
      end
      item
        Width = 50
      end
      item
        Width = 50
      end>
    SimplePanel = False
  end
  object Panel1: TPanel
    Left = 0
    Top = 85
    Width = 674
    Height = 157
    Align = alClient
    TabOrder = 1
    object Memo1: TMemo
      Left = 1
      Top = 1
      Width = 672
      Height = 153
      Align = alTop
      Anchors = [akLeft, akTop, akRight, akBottom]
      Font.Charset = ANSI_CHARSET
      Font.Color = clWindowText
      Font.Height = -13
      Font.Name = 'Consolas'
      Font.Style = []
      Lines.Strings = (
        'Memo1')
      ParentFont = False
      ReadOnly = True
      ScrollBars = ssBoth
      TabOrder = 0
      WordWrap = False
    end
  end
  object Panel2: TPanel
    Left = 0
    Top = 0
    Width = 674
    Height = 85
    Align = alTop
    TabOrder = 2
    object Label1: TLabel
      Left = 18
      Top = 16
      Width = 50
      Height = 13
      Alignment = taRightJustify
      Caption = 'Serial port '
    end
    object Label2: TLabel
      Left = 219
      Top = 16
      Width = 61
      Height = 13
      Alignment = taRightJustify
      Caption = 'Serial speed '
    end
    object CGauge1: TCGauge
      Left = 362
      Top = 52
      Width = 133
      Height = 21
      ForeColor = clGreen
      Progress = 50
    end
    object SerialPortComboBox: TComboBox
      Left = 76
      Top = 14
      Width = 125
      Height = 21
      Cursor = crHandPoint
      AutoComplete = False
      AutoDropDown = True
      Style = csDropDownList
      DropDownCount = 20
      ItemHeight = 13
      TabOrder = 0
      OnChange = SerialPortComboBoxChange
      OnDropDown = SerialPortComboBoxDropDown
      OnSelect = SerialPortComboBoxSelect
    end
    object SerialSpeedComboBox: TComboBox
      Left = 288
      Top = 14
      Width = 125
      Height = 21
      Cursor = crHandPoint
      AutoComplete = False
      AutoDropDown = True
      Style = csDropDownList
      DropDownCount = 20
      ItemHeight = 13
      TabOrder = 1
      OnSelect = SerialSpeedComboBoxSelect
    end
    object ClearButton: TButton
      Left = 504
      Top = 50
      Width = 53
      Height = 25
      Cursor = crHandPoint
      Hint = 'Clear the memo'
      Caption = 'Clear'
      ParentShowHint = False
      ShowHint = True
      TabOrder = 6
      OnClick = ClearButtonClick
    end
    object ReadEEPROMButton: TButton
      Left = 12
      Top = 50
      Width = 109
      Height = 25
      Cursor = crHandPoint
      Hint = 'Read and save the EEPROM'
      Caption = 'Read EEPROM'
      ParentShowHint = False
      ShowHint = True
      TabOrder = 3
      OnClick = ReadEEPROMButtonClick
    end
    object WriteFirmwareButton: TButton
      Left = 244
      Top = 50
      Width = 109
      Height = 25
      Cursor = crHandPoint
      Hint = 'Write firmware file'
      Caption = 'Write Firmware'
      ParentShowHint = False
      ShowHint = True
      TabOrder = 5
      OnClick = WriteFirmwareButtonClick
    end
    object VerboseTrackBar: TTrackBar
      Left = 428
      Top = 4
      Width = 137
      Height = 41
      Cursor = crHandPoint
      Hint = 'Verbose level'
      Max = 3
      Orientation = trHorizontal
      ParentShowHint = False
      PageSize = 1
      Frequency = 1
      Position = 1
      SelEnd = 0
      SelStart = 0
      ShowHint = True
      TabOrder = 2
      TickMarks = tmBoth
      TickStyle = tsAuto
      OnChange = VerboseTrackBarChange
    end
    object WriteEEPROMButton: TButton
      Left = 128
      Top = 50
      Width = 109
      Height = 25
      Cursor = crHandPoint
      Hint = 'Load and write the EEPROM'
      Caption = 'Write EEPROM'
      ParentShowHint = False
      ShowHint = True
      TabOrder = 4
      OnClick = WriteEEPROMButtonClick
    end
    object ReadADCButton: TButton
      Left = 576
      Top = 12
      Width = 75
      Height = 25
      Cursor = crHandPoint
      Caption = 'Read ADC'
      TabOrder = 7
      OnClick = ReadADCButtonClick
    end
    object ReadRSSIButton: TButton
      Left = 576
      Top = 50
      Width = 75
      Height = 25
      Cursor = crHandPoint
      Caption = 'Read RSSI'
      TabOrder = 8
      OnClick = ReadRSSIButtonClick
    end
  end
  object OpenDialog1: TOpenDialog
    Filter = 
      'All files|*.*|Supported files *.bin *.raw|*.bin;*.raw|BIN files|' +
      '*.bin|RAW files|*.raw'
    FilterIndex = 2
    Options = [ofReadOnly, ofPathMustExist, ofFileMustExist, ofEnableSizing]
    Left = 104
    Top = 172
  end
  object SaveDialog1: TSaveDialog
    Filter = 
      'All files|*.*|Supported files *.bin *.raw|*.bin;*.raw|BIN files|' +
      '*.bin|RAW files|*.raw'
    FilterIndex = 2
    Options = [ofOverwritePrompt, ofHideReadOnly, ofPathMustExist, ofEnableSizing]
    Left = 172
    Top = 172
  end
  object Timer1: TTimer
    Enabled = False
    Interval = 100
    OnTimer = Timer1Timer
    Left = 240
    Top = 168
  end
end
