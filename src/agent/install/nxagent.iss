; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
#include "setup.iss"
OutputBaseFilename=nxagent-2.1.8

[Files]
; Installer helpers
Source: "..\..\..\Release\nxreload.exe"; DestDir: "{tmp}"; Flags: dontcopy signonce
; Agent
Source: "..\..\..\Release\libnetxms.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\libnxagent.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\libnxdb.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\libnxjava.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\libnxlp.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\libnxsnmp.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\appagent.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxagentd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxsagent.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\db2.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\dbquery.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\devemu.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\ecs.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\filemgr.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\informix.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\java.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\logwatch.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\netsvc.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\odbcquery.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\oracle.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\ping.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\portcheck.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\sms.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\ssh.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\ups.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\winnt.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\winperf.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\wmi.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\db2.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\informix.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\mssql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\mysql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\oracle.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\pgsql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\sqlite.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\contrib\nxagentd.conf-dist"; DestDir: "{app}\etc"; Flags: ignoreversion
Source: "..\..\..\Release\libexpat.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\libpng.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\libtre.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxsqlite.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxzlib.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\jansson.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\libeay32.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\ssleay32.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\libcurl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\libssh.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\Microsoft.VC80.CRT\*"; DestDir: "{app}\bin\Microsoft.VC80.CRT"; Flags: ignoreversion
Source: "..\..\libnxjava\java\target\netxms-java-bridge.jar"; DestDir: "{app}\lib"; Flags: ignoreversion
Source: "..\subagents\bind9\target\bind9.jar"; DestDir: "{app}\lib"; Flags: ignoreversion
Source: "..\subagents\java\java\target\netxms-agent.jar"; DestDir: "{app}\lib"; Flags: ignoreversion
Source: "..\subagents\jmx\target\jmx.jar"; DestDir: "{app}\lib"; Flags: ignoreversion
Source: "..\subagents\ubntlw\target\ubntlw.jar"; DestDir: "{app}\lib"; Flags: ignoreversion
; Command-line tools
Source: "..\..\..\Release\nxappget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxapush.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxevent.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxpush.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\libnxclient.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce

;#include "custom.iss"

#include "common.iss"

Function GetCustomCmdLine(Param: String): String;
Begin
  Result := '';
End;
