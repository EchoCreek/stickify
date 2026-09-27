; ------------------------------------------------------------------
; Stickify 现代化桌面便签 — Inno Setup 6 x64 生产级安装配置脚本
; 支持：中英双语选择、运行检测与平滑关闭、覆盖安装、用户数据保护、开机自启、Win+R快速唤起
; ------------------------------------------------------------------

#define MyAppName "Stickify"
#define MyAppVersion "1.0.1"
#define MyAppPublisher "EchoCreek & Stickify Contributors"
#define MyAppURL "https://github.com/EchoCreek/stickify"
#define MyAppExeName "Notes.exe"

[Setup]
AppId={{CD1E65B4-0A4C-4853-B4EA-F447CD00B780}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
DisableProgramGroupPage=yes
LicenseFile=LICENSE
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
OutputDir=output
OutputBaseFilename=Stickify.{#MyAppVersion}.x64
SetupIconFile=setup.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ShowLanguageDialog=yes
UsePreviousAppDir=yes
CloseApplications=yes
CloseApplicationsFilter=*.exe

[Languages]
Name: "chinesesimplified"; MessagesFile: "Languages\ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[CustomMessages]
chinesesimplified.AutoRunProgram=开机自动启动 Stickify
english.AutoRunProgram=Start Stickify automatically when Windows starts
chinesesimplified.AppRunningWarning=检测到 Stickify 正在运行。安装程序需要关闭它以继续安装。%n%n是否由安装程序自动关闭 Stickify 并继续？
english.AppRunningWarning=Stickify is currently running. Setup needs to close it to continue.%n%nDo you want Setup to automatically close Stickify and continue?
chinesesimplified.AppRunningUninstallWarning=检测到 Stickify 正在运行。卸载程序需要先关闭它。%n%n是否自动关闭 Stickify 并继续卸载？
english.AppRunningUninstallWarning=Stickify is currently running. The uninstaller needs to close it.%n%nDo you want to automatically close Stickify and continue?
chinesesimplified.KeepDataQuestion=是否保留您的便签数据和配置？%n%n点击“是”保留所有便签数据与设置（推荐升级/重装时保留）。%n点击“否”彻底清除所有本地数据。
english.KeepDataQuestion=Do you want to keep your notes data and settings?%n%nClick 'Yes' to preserve all notes and configuration (recommended).%nClick 'No' to remove all local data completely.
chinesesimplified.WebView2Missing=Stickify 需要 Microsoft Edge WebView2 Runtime 支持。%n%n检测到您的系统可能尚未安装该运行时，是否立即打开微软官方页面下载？
english.WebView2Missing=Stickify requires Microsoft Edge WebView2 Runtime to function.%n%nIt was not detected on your system. Would you like to open the official download page now?

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "autorun"; Description: "{cm:AutoRunProgram}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "x64\Release\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "x64\Release\WebView2Loader.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "themes\default\dist\*"; DestDir: "{app}\themes\Default"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "themes\simple\dist\*"; DestDir: "{app}\themes\Simple"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "themes\manager\dist\*"; DestDir: "{app}\themes\Manager"; Flags: ignoreversion recursesubdirs createallsubdirs

[Registry]
; 开机自启动注册表项
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "Sticky Note"; ValueData: """{app}\{#MyAppExeName}"""; Tasks: autorun; Flags: uninsdeletevalue
; Win + R 运行快捷命令支持 (notes / stickify)
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\{#MyAppExeName}"; ValueType: string; ValueData: "{app}\{#MyAppExeName}"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\Stickify.exe"; ValueType: string; ValueData: "{app}\{#MyAppExeName}"; Flags: uninsdeletekey

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Code]
// 关闭正在运行的 Stickify 进程
function CloseRunningStickify(const WarnMsg: String): Boolean;
var
  ErrorCode: Integer;
begin
  Result := True;
  while CheckForMutexes('HANCEL_STICKY_NOTES_APP,STICKY_NOTES_APP_MUTEX_SINGLETON') do
  begin
    if MsgBox(WarnMsg, mbConfirmation, MB_OKCANCEL) = IDOK then
    begin
      ShellExec('open', 'taskkill.exe', '/f /im {#MyAppExeName}', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);
      Sleep(600);
    end
    else
    begin
      Result := False;
      Exit;
    end;
  end;
end;

// 检测系统是否已安装 WebView2 Runtime
function IsWebView2Installed(): Boolean;
var
  Version: String;
begin
  Result := False;
  // 检查 64位 HKLM
  if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', Version) and (Version <> '') and (Version <> '0.0.0.0') then
  begin
    Result := True;
    Exit;
  end;
  // 检查 WOW6432Node HKLM
  if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', Version) and (Version <> '') and (Version <> '0.0.0.0') then
  begin
    Result := True;
    Exit;
  end;
  // 检查 HKCU 用户级安装
  if RegQueryStringValue(HKEY_CURRENT_USER, 'Software\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', Version) and (Version <> '') and (Version <> '0.0.0.0') then
  begin
    Result := True;
    Exit;
  end;
end;

function InitializeSetup(): Boolean;
begin
  Result := CloseRunningStickify(CustomMessage('AppRunningWarning'));
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ErrorCode: Integer;
begin
  if CurStep = ssPostInstall then
  begin
    if not IsWebView2Installed() then
    begin
      if MsgBox(CustomMessage('WebView2Missing'), mbConfirmation, MB_YESNO) = IDYES then
      begin
        ShellExec('open', 'https://go.microsoft.com/fwlink/p/?LinkId=2124703', '', '', SW_SHOWNORMAL, ewNoWait, ErrorCode);
      end;
    end;
  end;
end;

function InitializeUninstall(): Boolean;
begin
  Result := CloseRunningStickify(CustomMessage('AppRunningUninstallWarning'));
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
  begin
    // 询问用户是否保留便签数据
    if MsgBox(CustomMessage('KeepDataQuestion'), mbConfirmation, MB_YESNO) = IDNO then
    begin
      // 用户选择彻底清除
      DelTree(ExpandConstant('{app}\notes'), True, True, True);
      DelTree(ExpandConstant('{app}\data'), True, True, True);
      DelTree(ExpandConstant('{app}\{#MyAppExeName}.WebView2'), True, True, True);
      DeleteFile(ExpandConstant('{app}\setting.ini'));
    end;
  end;
end;