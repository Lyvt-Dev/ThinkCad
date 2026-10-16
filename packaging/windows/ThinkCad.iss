#ifndef Version
  #define Version "1.2.0"
#endif
#ifndef StagingDir
  #define StagingDir "..\..\out\stage"
#endif
[Setup]
AppId={{D184B517-892D-48D8-A537-F54197762D2F}
AppName=ThinkCad
AppVersion={#Version}
AppPublisher=Lyvt-Dev
AppPublisherURL=https://github.com/Lyvt-Dev/ThinkCad
AppSupportURL=https://github.com/Lyvt-Dev/ThinkCad/issues
DefaultDirName={localappdata}\Programs\ThinkCad
DefaultGroupName=ThinkCad
UsePreviousGroup=no
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\..\out\installer
OutputBaseFilename=ThinkCad-Setup-{#Version}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupIconFile=..\..\assets\thinkercad.ico
UninstallDisplayIcon={app}\ThinkCad.exe
AppMutex=ThinkerCadDesktopMain
CloseApplications=yes
CloseApplicationsFilter=ThinkCad.exe,ThinkerCad.exe
RestartApplications=no
DisableProgramGroupPage=yes
[InstallDelete]
Type: files; Name: "{app}\ThinkerCad.exe"
Type: files; Name: "{app}\ThinkerCadUpdater.exe"
Type: files; Name: "{userprograms}\ThinkerCad\ThinkerCad.lnk"
Type: files; Name: "{userdesktop}\ThinkerCad.lnk"
[Languages]
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"
[Tasks]
Name: "desktopicon"; Description: "Desktop-Verknüpfung erstellen"; GroupDescription: "Verknüpfungen:"; Flags: unchecked
[Files]
Source: "{#StagingDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{group}\ThinkCad"; Filename: "{app}\ThinkCad.exe"
Name: "{autodesktop}\ThinkCad"; Filename: "{app}\ThinkCad.exe"; Tasks: desktopicon
[Run]
Filename: "{app}\ThinkCad.exe"; Description: "ThinkCad starten"; Flags: nowait postinstall skipifsilent
Filename: "{app}\ThinkCad.exe"; Flags: nowait; Check: UpdateRestartRequested
[Code]
function UpdateRestartRequested: Boolean;
begin
  Result := WizardSilent and (ExpandConstant('{param:RESTARTTHINKERCAD|0}') = '1');
end;
