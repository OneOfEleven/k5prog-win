object Form1: TForm1
  Left = 297
  Top = 139
  Width = 730
  Height = 300
  Caption = 'Form1'
  Color = clBtnFace
  Constraints.MinHeight = 300
  Constraints.MinWidth = 730
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
    Top = 233
    Width = 714
    Height = 28
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBtnText
    Font.Height = -13
    Font.Name = 'Segoe UI'
    Font.Style = []
    Panels = <
      item
        Alignment = taCenter
        Bevel = pbRaised
        Width = 220
      end
      item
        Alignment = taCenter
        Bevel = pbRaised
        Width = 100
      end
      item
        Alignment = taCenter
        Bevel = pbRaised
        Width = 200
      end>
    SimplePanel = False
    UseSystemFont = False
    OnResize = StatusBar1Resize
  end
  object Panel1: TPanel
    Left = 0
    Top = 85
    Width = 714
    Height = 148
    Align = alClient
    TabOrder = 1
    object Memo1: TMemo
      Left = 1
      Top = 1
      Width = 712
      Height = 144
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
    Width = 714
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
      Left = 227
      Top = 16
      Width = 61
      Height = 13
      Alignment = taRightJustify
      Caption = 'Serial speed '
    end
    object CGauge1: TCGauge
      Left = 572
      Top = 12
      Width = 89
      Height = 25
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = [fsBold]
      BorderStyle = bsNone
      ForeColor = clGreen
      BackColor = clBtnFace
      ParentFont = False
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
      Left = 296
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
      Left = 646
      Top = 12
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
    object ReadConfigButton: TButton
      Left = 8
      Top = 50
      Width = 130
      Height = 25
      Cursor = crHandPoint
      Hint = 'Download configuration data from radio'
      Caption = 'Read Configuration'
      ParentShowHint = False
      ShowHint = True
      TabOrder = 3
      OnClick = ReadConfigButtonClick
    end
    object WriteFirmwareButton: TButton
      Left = 570
      Top = 50
      Width = 130
      Height = 25
      Cursor = crHandPoint
      Hint = 'Upload firmware file to radio'
      Caption = 'Write Firmware'
      ParentShowHint = False
      ShowHint = True
      TabOrder = 5
      OnClick = WriteFirmwareButtonClick
    end
    object VerboseTrackBar: TTrackBar
      Left = 444
      Top = 4
      Width = 141
      Height = 41
      Cursor = crHandPoint
      Hint = 'Verbose level (0..3)'
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
    object WriteConfigButton: TButton
      Left = 144
      Top = 50
      Width = 130
      Height = 25
      Cursor = crHandPoint
      Hint = 'Upload configuration data to radio'
      Caption = 'Write Configuration'
      ParentShowHint = False
      ShowHint = True
      TabOrder = 4
      OnClick = WriteConfigButtonClick
    end
    object ReadADCButton: TButton
      Left = 714
      Top = 12
      Width = 75
      Height = 25
      Cursor = crHandPoint
      Hint = 'Test only'
      Caption = 'Read ADC'
      ParentShowHint = False
      ShowHint = True
      TabOrder = 7
      Visible = False
      OnClick = ReadADCButtonClick
    end
    object ReadRSSIButton: TButton
      Left = 714
      Top = 50
      Width = 75
      Height = 25
      Cursor = crHandPoint
      Hint = 'Test only'
      Caption = 'Read RSSI'
      ParentShowHint = False
      ShowHint = True
      TabOrder = 8
      Visible = False
      OnClick = ReadRSSIButtonClick
    end
    object ReadCalibrationButton: TButton
      Left = 290
      Top = 50
      Width = 130
      Height = 25
      Cursor = crHandPoint
      Hint = 'Read the radios calibration settings'
      Caption = 'Read Calibration'
      ParentShowHint = False
      ShowHint = True
      TabOrder = 9
      OnClick = ReadCalibrationButtonClick
    end
    object WriteCalibrationButton: TButton
      Left = 424
      Top = 50
      Width = 130
      Height = 25
      Cursor = crHandPoint
      Hint = 'Write the radios calibration settings'
      Caption = 'Write Calibration'
      ParentShowHint = False
      ShowHint = True
      TabOrder = 10
      OnClick = WriteCalibrationButtonClick
    end
  end
  object OpenDialog1: TOpenDialog
    DefaultExt = '.bin'
    FileName = 'my_config.bin'
    Filter = 
      'All files|*.*|Supported files *.bin *.raw|*.bin;*.raw|BIN files|' +
      '*.bin|RAW files|*.raw'
    FilterIndex = 2
    Options = [ofReadOnly, ofPathMustExist, ofFileMustExist, ofEnableSizing]
    Left = 104
    Top = 128
  end
  object SaveDialog1: TSaveDialog
    DefaultExt = '.bin'
    FileName = 'my_config.bin'
    Filter = 
      'All files|*.*|Supported files *.bin *.raw|*.bin;*.raw|BIN files|' +
      '*.bin|RAW files|*.raw'
    FilterIndex = 2
    Options = [ofOverwritePrompt, ofHideReadOnly, ofPathMustExist, ofEnableSizing]
    Left = 136
    Top = 128
  end
  object Timer1: TTimer
    Enabled = False
    Interval = 100
    OnTimer = Timer1Timer
    Left = 104
    Top = 92
  end
  object SaveDialog2: TSaveDialog
    DefaultExt = '.bin'
    FileName = 'my_calibration.bin'
    Filter = 
      'All files|*.*|Supported files *.bin *.raw|*.bin;*.raw|BIN files|' +
      '*.bin|RAW files|*.raw'
    FilterIndex = 2
    Options = [ofOverwritePrompt, ofHideReadOnly, ofPathMustExist, ofEnableSizing]
    Left = 136
    Top = 164
  end
  object OpenDialog2: TOpenDialog
    DefaultExt = '.bin'
    FileName = 'my_calibration.bin'
    Filter = 
      'All files|*.*|Supported files *.bin *.raw|*.bin;*.raw|BIN files|' +
      '*.bin|RAW files|*.raw'
    FilterIndex = 2
    Options = [ofReadOnly, ofPathMustExist, ofFileMustExist, ofEnableSizing]
    Left = 104
    Top = 164
  end
  object OpenDialog3: TOpenDialog
    DefaultExt = '.bin'
    Filter = 
      'All files|*.*|Supported files *.bin *.raw|*.bin;*.raw|BIN files|' +
      '*.bin|RAW files|*.raw'
    FilterIndex = 2
    Options = [ofReadOnly, ofPathMustExist, ofFileMustExist, ofEnableSizing]
    Left = 200
    Top = 136
  end
end
