[Dirs]
Name: "{app}\etc"
Name: "{app}\etc\nxagentd.conf.d"
Name: "{app}\lib"
Name: "{app}\log"
Name: "{app}\var"

[Tasks]
Name: fspermissions; Description: "Set hardened file system permissions"
Name: sessionagent; Description: "Install session agent (will run in every user session)"; Flags: unchecked
Name: useragent; Description: "Install user support application (will run in every user session)"; Flags: unchecked

[Registry]
Root: HKLM; Subkey: "Software\NetXMS"; Flags: uninsdeletekeyifempty
Root: HKLM; Subkey: "Software\NetXMS\Agent"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\NetXMS\Agent"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"
Root: HKLM; Subkey: "Software\NetXMS\Agent"; ValueType: string; ValueName: "ConfigFile"; ValueData: "{app}\etc\nxagentd.conf"
Root: HKLM; Subkey: "Software\NetXMS\Agent"; ValueType: string; ValueName: "ConfigIncludeDir"; ValueData: "{app}\etc\nxagentd.conf.d"
Root: HKLM; Subkey: "Software\NetXMS\Agent"; ValueType: dword; ValueName: "WithUserAgent"; ValueData: "1"; Tasks: useragent
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "NetXMSSessionAgent"; ValueData: """{app}\bin\nxsagent.exe"" -c ""{app}\etc\nxagentd.conf"" -H"; Flags: uninsdeletevalue; Tasks: sessionagent
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "NetXMSUserAgent"; ValueData: """{app}\bin\nxuseragent.exe"""; Flags: uninsdeletevalue; Tasks: useragent

[Run]
Filename: "icacls.exe"; Parameters: """{app}"" /inheritance:r"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}"" /grant:r *S-1-5-18:(OI)(CI)F"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}"" /grant:r *S-1-5-32-544:(OI)(CI)F"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\*"" /reset /T"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-Z ""{app}\etc\nxagentd.conf"" {code:GetForceCreateConfigFlag} ""{code:GetMasterServer}"" ""{code:GetLogFile}"" ""{code:GetFileStore}"" ""{code:GetConfigIncludeDir}"" {code:GetSubagentList} {code:GetExtraConfigValues}"; WorkingDir: "{app}\bin"; StatusMsg: "Creating agent's config..."; Flags: runhidden
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-c ""{app}\etc\nxagentd.conf"" {code:GetCentralConfigOption} {code:GetCustomCmdLine} -I"; WorkingDir: "{app}\bin"; StatusMsg: "Installing service..."; Flags: runhidden
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-s"; WorkingDir: "{app}\bin"; StatusMsg: "Starting service..."; Flags: runhidden
Filename: "{tmp}\nxreload.exe"; Parameters: "-finish"; WorkingDir: "{tmp}"; StatusMsg: "Waiting for external processes..."; Flags: runhidden waituntilterminated

[UninstallRun]
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-K"; StatusMsg: "Stopping external subagents..."; RunOnceId: "StopExternalAgents"; Flags: runhidden
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-S"; StatusMsg: "Stopping service..."; RunOnceId: "StopService"; Flags: runhidden
Filename: "taskkill.exe"; Parameters: "/F /IM nxagentd.exe /T"; StatusMsg: "Killing agent processes..."; RunOnceId: "KillProcess"; Flags: runhidden
Filename: "taskkill.exe"; Parameters: "/F /IM nxsagent.exe /T"; StatusMsg: "Killing session agent processes..."; RunOnceId: "KillSessionAgentProcess"; Flags: runhidden
Filename: "taskkill.exe"; Parameters: "/F /IM nxuseragent.exe /T"; StatusMsg: "Killing user agent processes..."; RunOnceId: "KillUserAgentProcess"; Flags: runhidden
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-R"; StatusMsg: "Uninstalling service..."; RunOnceId: "DelService"; Flags: runhidden

[UninstallDelete]
Type: filesandordirs; Name: "{app}\etc\*"
Type: filesandordirs; Name: "{app}\log\*"
Type: filesandordirs; Name: "{app}\var\*"

[Code]
Var
  ServerSelectionPage: TWizardPage;
  editServerName: TNewEdit;
  cbDownloadConfig: TNewCheckBox;
  SubagentSelectionPage: TInputOptionWizardPage;
  serverName, sbECS, sbFileMgr, sbLogWatch, sbPing, sbPortCheck, 
    sbWinPerf, sbWMI, sbUPS, sbDownloadConfig, extraConfigValues: String;
  backupFileSuffix: String;
  logFile: String;
  fileSTore: String;
  configIncludeDir: String;
  forceCreateConfig: Boolean;

#include "..\..\install\windows\firewall.iss"

Procedure ExecAndLog(execName, options, workingDir: String);
Var
  iResult : Integer;
Begin
  Log('Executing ' + execName + ' ' + options);
  Log('   Working directory: ' + workingDir);
  Exec(execName, options, workingDir, 0, ewWaitUntilTerminated, iResult);
  Log('   Result: ' + IntToStr(iResult));
End;

Procedure StopService;
Var
  strExecName : String;
Begin
  strExecName := ExpandConstant('{app}\bin\nxagentd.exe');
  If FileExists(strExecName) Then
  Begin
    Log('Stopping agent service');
    FileCopy(ExpandConstant('{tmp}\nxreload.exe'), ExpandConstant('{app}\bin\nxreload.exe'), False);
    ExecAndLog(strExecName, '-k', ExpandConstant('{app}\bin'));
    ExecAndLog(strExecName, '-S', ExpandConstant('{app}\bin'));
    ExecAndLog('taskkill.exe', '/IM nxagentd.exe /F', ExpandConstant('{app}\bin'));
    ExecAndLog('taskkill.exe', '/IM nxsagent.exe /F', ExpandConstant('{app}\bin'));
    ExecAndLog('taskkill.exe', '/IM nxuseragent.exe /F', ExpandConstant('{app}\bin'));
  End;
End;

Function PrepareToInstall(var NeedsRestart: Boolean): String;
Begin
  RegDeleteValue(HKEY_LOCAL_MACHINE, 'Software\NetXMS\Agent', 'ReloadFlag');
  ExecAndLog('taskkill.exe', '/IM nxreload.exe /F', ExpandConstant('{tmp}'));
  ExtractTemporaryFile('nxreload.exe');
  StopService;
  Result := '';
End;

Function BoolToStr(Val: Boolean): String;
Begin
  If Val Then
    Result := 'TRUE'
  Else
    Result := 'FALSE';
End;

Function StrToBool(Val: String): Boolean;
Begin
  If Val = 'TRUE' Then
    Result := TRUE
  Else
    Result := FALSE;
End;

Function InitializeSetup(): Boolean;
Var
  i, nCount : Integer;
  param : String;
Begin
  // Common suffix for backup files
  backupFileSuffix := '.bak.' + GetDateTimeString('yyyymmddhhnnss', #0, #0);

  // Check if we are running on 64-bit Windows
  If (NOT Is64BitInstallMode) AND (ProcessorArchitecture = paX64) Then Begin
    If MsgBox('You are trying to install 32-bit version of NetXMS agent on 64-bit Windows. It is recommended to install 64-bit version instead. Do you really wish to continue installation?', mbConfirmation, MB_YESNO) = IDYES Then
      Result := TRUE
    Else
      Result := FALSE;
  End Else Begin
    Result := TRUE;
  End;
  
  // Empty values for installation data
  serverName := '';
  sbECS := 'FALSE';
  sbFileMgr := 'FALSE';
  sbLogWatch := 'FALSE';
  sbPing := 'FALSE';
  sbPortCheck := 'FALSE';
  sbWinPerf := 'TRUE';
  sbWMI := 'FALSE';
  sbUPS := 'FALSE';
  sbDownloadConfig := 'FALSE';
  extraConfigValues := '';
  logFile := '';
  fileStore := '';
  configIncludeDir := '';
  forceCreateConfig := FALSE;
  
  // Parse command line parameters
  nCount := ParamCount;
  For i := 1 To nCount Do Begin
    param := ParamStr(i);

    If Pos('/SERVER=', param) = 1 Then Begin
      serverName := param;
      Delete(serverName, 1, 8);
    End;

    If Pos('/CENTRALCONFIG', param) = 1 Then Begin
      sbDownloadConfig := 'TRUE';
    End;

    If Pos('/LOCALCONFIG', param) = 1 Then Begin
      sbDownloadConfig := 'FALSE';
    End;

    If Pos('/SUBAGENT=', param) = 1 Then Begin
      Delete(param, 1, 10);
      param := Uppercase(param);
      If param = 'ECS' Then
        sbECS := 'TRUE';
      If param = 'FILEMGR' Then
        sbFileMgr := 'TRUE';
      If param = 'LOGWATCH' Then
        sbLogWatch := 'TRUE';
      If param = 'PING' Then
        sbPing := 'TRUE';
      If param = 'PORTCHECK' Then
        sbPortCheck := 'TRUE';
      If param = 'WINPERF' Then
        sbWinPerf := 'TRUE';
      If param = 'WMI' Then
        sbWMI := 'TRUE';
      If param = 'UPS' Then
        sbUPS := 'TRUE';
    End;

    If Pos('/NOSUBAGENT=', param) = 1 Then Begin
      Delete(param, 1, 12);
      param := Uppercase(param);
      If param = 'ECS' Then
        sbECS := 'FALSE';
      If param = 'FILEMGR' Then
        sbFileMgr := 'FALSE';
      If param = 'LOGWATCH' Then
        sbLogWatch := 'FALSE';
      If param = 'PING' Then
        sbPing := 'FALSE';
      If param = 'PORTCHECK' Then
        sbPortCheck := 'FALSE';
      If param = 'WINPERF' Then
        sbWinPerf := 'FALSE';
      If param = 'WMI' Then
        sbWMI := 'FALSE';
      If param = 'UPS' Then
        sbUPS := 'FALSE';
    End;

    If param = '/FORCECREATECONFIG' Then Begin
      forceCreateConfig := TRUE;
    End;

    If Pos('/LOGFILE=', param) = 1 Then Begin
      Delete(param, 1, 9);
      logFile := param;
    End;

    If Pos('/FILESTORE=', param) = 1 Then Begin
      Delete(param, 1, 11);
      fileStore := param;
    End;

    If Pos('/CONFIGINCLUDEDIR=', param) = 1 Then Begin
      Delete(param, 1, 18);
      configIncludeDir := param;
    End;

    If Pos('/CONFIGENTRY=', param) = 1 Then Begin
      Delete(param, 1, 13);
      extraConfigValues := extraConfigValues + '~~' + param;
    End;
  End;
End;

Procedure InitializeWizard;
Var
  static: TNewStaticText;
Begin
  ServerSelectionPage := CreateCustomPage(wpSelectTasks,
    'NetXMS Server', 'Select your management server.');

  static := TNewStaticText.Create(ServerSelectionPage);
  static.Top := ScaleY(8);
  static.Caption := 'Please enter host name or IP address of your NetXMS server.';
  static.AutoSize := True;
  static.Parent := ServerSelectionPage.Surface;

  editServerName := TNewEdit.Create(ServerSelectionPage);
  editServerName.Top := static.Top + static.Height + ScaleY(8);
  editServerName.Width := ServerSelectionPage.SurfaceWidth - ScaleX(8);
  editServerName.Text := GetPreviousData('MasterServer', serverName);
  editServerName.Parent := ServerSelectionPage.Surface;

  cbDownloadConfig := TNewCheckBox.Create(ServerSelectionPage);
  cbDownloadConfig.Top := editServerName.Top + editServerName.Height + ScaleY(12);
  cbDownloadConfig.Width := ServerSelectionPage.SurfaceWidth;
  cbDownloadConfig.Height := ScaleY(17);
  cbDownloadConfig.Caption := 'Download configuration file from management server on startup';
  cbDownloadConfig.Checked := StrToBool(GetPreviousData('DownloadConfig', sbDownloadConfig));
  cbDownloadConfig.Parent := ServerSelectionPage.Surface;

  SubagentSelectionPage := CreateInputOptionPage(ServerSelectionPage.Id,
    'Subagent Selection', 'Select desired subagents.',
    'Please select additional subagents you wish to load.', False, False);
  SubagentSelectionPage.Add('Extended Checksum Subagent - ecs.nsm');
  SubagentSelectionPage.Add('File Manager Subagent - filemgr.nsm');
  SubagentSelectionPage.Add('ICMP Pinger Subagent - ping.nsm');
  SubagentSelectionPage.Add('Log Monitoring Subagent - logwatch.nsm');
  SubagentSelectionPage.Add('Port Checker Subagent - portcheck.nsm');
  SubagentSelectionPage.Add('Windows Performance Subagent - winperf.nsm');
  SubagentSelectionPage.Add('WMI Subagent - wmi.nsm');
  SubagentSelectionPage.Add('UPS Monitoring Subagent - ups.nsm');
  SubagentSelectionPage.Values[0] := StrToBool(GetPreviousData('Subagent_ECS', sbECS));
  SubagentSelectionPage.Values[1] := StrToBool(GetPreviousData('Subagent_FILEMGR', sbFileMgr));
  SubagentSelectionPage.Values[2] := StrToBool(GetPreviousData('Subagent_PING', sbPing));
  SubagentSelectionPage.Values[3] := StrToBool(GetPreviousData('Subagent_LOGWATCH', sbLogWatch));
  SubagentSelectionPage.Values[4] := StrToBool(GetPreviousData('Subagent_PORTCHECK', sbPortCheck));
  SubagentSelectionPage.Values[5] := StrToBool(GetPreviousData('Subagent_WINPERF', sbWinPerf));
  SubagentSelectionPage.Values[6] := StrToBool(GetPreviousData('Subagent_WMI', sbWMI));
  SubagentSelectionPage.Values[7] := StrToBool(GetPreviousData('Subagent_UPS', sbUPS));
End;

Procedure RegisterPreviousData(PreviousDataKey: Integer);
Begin
  SetPreviousData(PreviousDataKey, 'MasterServer', editServerName.Text);
  SetPreviousData(PreviousDataKey, 'DownloadConfig', BoolToStr(cbDownloadConfig.Checked));
  SetPreviousData(PreviousDataKey, 'Subagent_ECS', BoolToStr(SubagentSelectionPage.Values[0]));
  SetPreviousData(PreviousDataKey, 'Subagent_FILEMGR', BoolToStr(SubagentSelectionPage.Values[1]));
  SetPreviousData(PreviousDataKey, 'Subagent_PING', BoolToStr(SubagentSelectionPage.Values[2]));
  SetPreviousData(PreviousDataKey, 'Subagent_LOGWATCH', BoolToStr(SubagentSelectionPage.Values[3]));
  SetPreviousData(PreviousDataKey, 'Subagent_PORTCHECK', BoolToStr(SubagentSelectionPage.Values[4]));
  SetPreviousData(PreviousDataKey, 'Subagent_WINPERF', BoolToStr(SubagentSelectionPage.Values[5]));
  SetPreviousData(PreviousDataKey, 'Subagent_WMI', BoolToStr(SubagentSelectionPage.Values[6]));
  SetPreviousData(PreviousDataKey, 'Subagent_UPS', BoolToStr(SubagentSelectionPage.Values[7]));
End;

Function GetMasterServer(Param: String): String;
Begin
  Result := editServerName.Text;
End;

Function GetSubagentList(Param: String): String;
Begin
  Result := '';
  If SubagentSelectionPage.Values[0] Then
    Result := Result + 'ecs.nsm ';
  If SubagentSelectionPage.Values[1] Then
    Result := Result + 'filemgr.nsm ';
  If SubagentSelectionPage.Values[2] Then
    Result := Result + 'ping.nsm ';
  If SubagentSelectionPage.Values[3] Then
    Result := Result + 'logwatch.nsm ';
  If SubagentSelectionPage.Values[4] Then
    Result := Result + 'portcheck.nsm ';
  If SubagentSelectionPage.Values[5] Then
    Result := Result + 'winperf.nsm ';
  If SubagentSelectionPage.Values[6] Then
    Result := Result + 'wmi.nsm ';
  If SubagentSelectionPage.Values[7] Then
    Result := Result + 'ups.nsm ';
End;

Function GetExtraConfigValues(Param: String): String;
Begin
  If extraConfigValues <> '' Then
    Result := '-z "' + extraConfigValues + '"'
  Else
    Result := ''
End;

Function GetLogFile(Param: String): String;
Begin
  If logFile <> '' Then
    Result := logFile
  Else
    Result := '{syslog}'
End;

Function GetConfigIncludeDir(Param: String): String;
Begin
  If configIncludeDir <> '' Then
    Result := configIncludeDir
  Else
    Result := ExpandConstant('{app}\etc\nxagentd.conf.d')
End;

Function GetFileStore(Param: String): String;
Begin
  If fileStore <> '' Then
    Result := fileStore
  Else
    Result := ExpandConstant('{app}\var')
End;

Function GetForceCreateConfigFlag(Param: String): String;
Begin
  If forceCreateConfig Then
    Result := '-F'
  Else
    Result := ''
End;

Function GetCentralConfigOption(Param: String): String;
Var
  server: String;
  position: Integer;
Begin
  If cbDownloadConfig.Checked Then Begin
    server := editServerName.Text;
    position := Pos(',', server);
    If position > 0 Then Begin
      Delete(server, position, 100000);
    End;
    Result := '-M "' + server + '"';
  End
  Else
    Result := '';
End;

Procedure RenameOldFile;
Begin
  RenameFile(ExpandConstant(CurrentFileName), ExpandConstant(CurrentFileName) + backupFileSuffix);
End;

Procedure DeleteBackupFiles;
Var
  fd: TFindRec;
  basePath: String;
Begin
  basePath := ExpandConstant('{app}\bin\');
  If FindFirst(basePath + '*.bak.*', fd) Then
  Begin
    Try
      Repeat
        DeleteFile(basePath + fd.Name);
      Until Not FindNext(fd);
    Finally
      FindClose(fd);
    End;
  End;
End;

Procedure CurStepChanged(CurStep: TSetupStep);
Begin
  If CurStep = ssPostInstall Then Begin
    SetFirewallException('NetXMS Agent', ExpandConstant('{app}')+'\bin\nxagentd.exe');
    DeleteBackupFiles;
  End;
End;

Procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
Begin
  If CurUninstallStep = usPostUninstall Then Begin
     RemoveFirewallException(ExpandConstant('{app}')+'\bin\nxagentd.exe');
  End;
End;
