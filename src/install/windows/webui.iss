; Installation script for NetXMS WebUI / Windows

#include "setup-webui.iss"
OutputBaseFilename=netxms-webui-1.2.9

[Files]
Source: files\prunsrv.exe; DestDir: "{app}\WebUI"; BeforeInstall: StopAllServices; Flags: ignoreversion; Components: webui
Source: winstone\winstone-0.9.10.jar; DestDir: "{app}\WebUI"; Flags: ignoreversion; Components: webui
Source: nxmc\nxmc.war; DestDir: "{app}\WebUI\nxmc"; Flags: ignoreversion; Components: webui
Source: files\jre\*; DestDir: "{app}\bin\jre"; Flags: ignoreversion recursesubdirs; Components: jre

#include "common-webui.iss"
