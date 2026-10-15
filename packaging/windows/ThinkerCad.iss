#ifndef Version
  #define Version "0.3.0"
#endif
#ifndef StagingDir
  #define StagingDir "..\..\out\stage"
#endif
[Setup]
AppId={{D184B517-892D-48D8-A537-F54197762D2F}
AppName=ThinkerCad
AppVersion={#Version}
AppPublisher=Lyvt-Dev
AppPublisherURL=https://github.com/Lyvt-Dev/ThinkerCad
AppSupportURL=https://github.com/Lyvt-Dev/ThinkerCad/issues
DefaultDirName={localappdata}\Programs\ThinkerCad
DefaultGroupName=ThinkerCad
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\..\out\installer
OutputBaseFilename=ThinkerCad-Setup-{#Version}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupIconFile=..\..\assets\thinkercad.ico
UninstallDisplayIcon={app}\ThinkerCad.exe
AppMutex=ThinkerCadDesktopMain
CloseApplications=yes
CloseApplicationsFilter=ThinkerCad.exe
RestartApplications=no
DisableProgramGroupPage=yes
[Languages]
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"
[Tasks]
Name: "desktopicon"; Description: "Desktop-Verknüpfung erstellen"; GroupDescription: "Verknüpfungen:"; Flags: unchecked
[Files]
Source: "{#StagingDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{group}\ThinkerCad"; Filename: "{app}\ThinkerCad.exe"
Name: "{autodesktop}\ThinkerCad"; Filename: "{app}\ThinkerCad.exe"; Tasks: desktopicon
[Run]
Filename: "{app}\ThinkerCad.exe"; Description: "ThinkerCad starten"; Flags: nowait postinstall skipifsilent
Filename: "{app}\ThinkerCad.exe"; Flags: nowait; Check: UpdateRestartRequested
[Code]
function UpdateRestartRequested: Boolean;
begin
  Result := WizardSilent and (ExpandConstant('{param:RESTARTTHINKERCAD|0}') = '1');
end;
