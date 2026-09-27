; ------------------------------------------------------------------
; Stickify 现代化桌面便签 — Inno Setup 6 x64 生产级安装配置脚本
; 支持：中英双语选择、全方位旧版检测与覆盖升级、进程运行检测与平滑关闭、
;       用户数据保护、开机自启、Win+R快速唤起、WebView2运行时检测
; ------------------------------------------------------------------

#define MyAppName "Stickify"
#define MyAppVersion "1.0.1"
#define MyAppPublisher "EchoCreek & Stickify Contributors"
#define MyAppURL "https://github.com/EchoCreek/stickify"
#define MyAppExeName "Notes.exe"
#define RawAppId "{CD1E65B4-0A4C-4853-B4EA-F447CD00B780}"
#define SetupAppId "{" + RawAppId

[Setup]
AppId={#SetupAppId}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={code:GetDefaultInstallDir}
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
DisableDirPage=no
UsePreviousAppDir=no
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
chinesesimplified.ExistingInstallPrompt=检测到系统中已安装 Stickify。%n%n已安装版本: %1%n当前所在路径: %2%n%n是否覆盖升级现有安装？%n%n点击【是】：自动覆盖并升级至该目录，完整保留您的所有便签与设置（推荐）。%n点击【否】：全新安装，您可以自选新的安装目录。
english.ExistingInstallPrompt=An existing installation of Stickify was detected.%n%nInstalled Version: %1%nInstalled Location: %2%n%nDo you want to upgrade and overwrite this installation?%n%nClick [Yes]: Overwrite and upgrade to this location, preserving all your notes (Recommended).%nClick [No]: Clean install, allowing you to choose a new location.
chinesesimplified.UpgradeDirLabel=安装程序已检测到旧版本，将在此目录上执行覆盖升级安装：
english.UpgradeDirLabel=Setup detected an existing installation and will overwrite and upgrade in this directory:
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
var
  ExistingPath: String;
  ExistingVersion: String;
  ExistingDetected: Boolean;
  IsUpgradeMode: Boolean;

// 辅助检测某目录是否包含 Stickify 主程序
function CheckDirHasNotes(const DirPath: String): Boolean;
begin
  Result := (DirPath <> '') and FileExists(AddBackslash(DirPath) + '{#MyAppExeName}');
end;

// 尝试从指定的注册表键读取安装路径与版本
function TryGetPathFromKey(RootKey: Integer; const SubKeyName: String; var OutPath, OutVer: String): Boolean;
var
  PathVal, VerVal: String;
begin
  Result := False;
  OutPath := '';
  OutVer := '';

  if RegQueryStringValue(RootKey, SubKeyName, 'InstallLocation', PathVal) and (PathVal <> '') then
  begin
    OutPath := RemoveQuotes(PathVal);
    RegQueryStringValue(RootKey, SubKeyName, 'DisplayVersion', VerVal);
    OutVer := VerVal;
    if CheckDirHasNotes(OutPath) then
    begin
      Result := True;
      Exit;
    end;
  end;

  if RegQueryStringValue(RootKey, SubKeyName, 'Inno Setup: App Path', PathVal) and (PathVal <> '') then
  begin
    OutPath := RemoveQuotes(PathVal);
    RegQueryStringValue(RootKey, SubKeyName, 'DisplayVersion', VerVal);
    OutVer := VerVal;
    if CheckDirHasNotes(OutPath) then
    begin
      Result := True;
      Exit;
    end;
  end;
end;

// 全方位地毯式扫描已有安装路径
function FindExistingInstall(): Boolean;
var
  AppIdGuid: String;
  KeyName: String;
  PathVal: String;
begin
  Result := False;
  ExistingPath := '';
  ExistingVersion := '';
  AppIdGuid := '{#RawAppId}';
  KeyName := 'Software\Microsoft\Windows\CurrentVersion\Uninstall\' + AppIdGuid + '_is1';

  // 1. 当前用户注册表卸载项 (HKCU)
  if TryGetPathFromKey(HKEY_CURRENT_USER, KeyName, ExistingPath, ExistingVersion) then
  begin
    Result := True;
    Exit;
  end;

  // 2. 本地机器 64位注册表卸载项 (HKLM)
  if TryGetPathFromKey(HKEY_LOCAL_MACHINE, KeyName, ExistingPath, ExistingVersion) then
  begin
    Result := True;
    Exit;
  end;

  // 3. 本地机器 32位(WOW6432Node)注册表卸载项 (旧版 32位 安装包遗留)
  if TryGetPathFromKey(HKEY_LOCAL_MACHINE, 'Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\' + AppIdGuid + '_is1', ExistingPath, ExistingVersion) then
  begin
    Result := True;
    Exit;
  end;

  // 4. HKCU App Paths
  if RegQueryStringValue(HKEY_CURRENT_USER, 'Software\Microsoft\Windows\CurrentVersion\App Paths\{#MyAppExeName}', '', PathVal) and (PathVal <> '') then
  begin
    PathVal := ExtractFilePath(RemoveQuotes(PathVal));
    if CheckDirHasNotes(PathVal) then
    begin
      ExistingPath := PathVal;
      Result := True;
      Exit;
    end;
  end;

  // 5. HKLM App Paths
  if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'Software\Microsoft\Windows\CurrentVersion\App Paths\{#MyAppExeName}', '', PathVal) and (PathVal <> '') then
  begin
    PathVal := ExtractFilePath(RemoveQuotes(PathVal));
    if CheckDirHasNotes(PathVal) then
    begin
      ExistingPath := PathVal;
      Result := True;
      Exit;
    end;
  end;

  // 6. 开机启动项 Run
  if RegQueryStringValue(HKEY_CURRENT_USER, 'Software\Microsoft\Windows\CurrentVersion\Run', 'Sticky Note', PathVal) and (PathVal <> '') then
  begin
    PathVal := ExtractFilePath(RemoveQuotes(PathVal));
    if CheckDirHasNotes(PathVal) then
    begin
      ExistingPath := PathVal;
      Result := True;
      Exit;
    end;
  end;

  // 7. 常见已知物理目录
  PathVal := ExpandConstant('{localappdata}\Programs\Stickify');
  if CheckDirHasNotes(PathVal) then
  begin
    ExistingPath := PathVal;
    Result := True;
    Exit;
  end;

  PathVal := ExpandConstant('{autopf}\Stickify');
  if CheckDirHasNotes(PathVal) then
  begin
    ExistingPath := PathVal;
    Result := True;
    Exit;
  end;

  PathVal := ExpandConstant('{commonpf32}\Stickify');
  if CheckDirHasNotes(PathVal) then
  begin
    ExistingPath := PathVal;
    Result := True;
    Exit;
  end;
end;

// 动态决定默认安装路径
function GetDefaultInstallDir(Param: String): String;
begin
  if IsUpgradeMode and (ExistingPath <> '') then
    Result := ExistingPath
  else
    Result := ExpandConstant('{autopf}\{#MyAppName}');
end;

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
  if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', Version) and (Version <> '') and (Version <> '0.0.0.0') then
  begin
    Result := True;
    Exit;
  end;
  if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', Version) and (Version <> '') and (Version <> '0.0.0.0') then
  begin
    Result := True;
    Exit;
  end;
  if RegQueryStringValue(HKEY_CURRENT_USER, 'Software\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', Version) and (Version <> '') and (Version <> '0.0.0.0') then
  begin
    Result := True;
    Exit;
  end;
end;

function InitializeSetup(): Boolean;
var
  PromptMsg: String;
begin
  Result := True;
  IsUpgradeMode := False;

  // 1. 若旧实例正在运行，先友好提示并关闭
  if not CloseRunningStickify(CustomMessage('AppRunningWarning')) then
  begin
    Result := False;
    Exit;
  end;

  // 2. 探测旧版安装路径，并弹窗提示用户选择是否覆盖升级
  ExistingDetected := FindExistingInstall();
  if ExistingDetected and (ExistingPath <> '') then
  begin
    if ExistingVersion = '' then
      ExistingVersion := '1.0.0';
    PromptMsg := FmtMessage(CustomMessage('ExistingInstallPrompt'), [ExistingVersion, ExistingPath]);
    if MsgBox(PromptMsg, mbConfirmation, MB_YESNO) = IDYES then
    begin
      IsUpgradeMode := True;
    end;
  end;
end;

procedure InitializeWizard();
begin
  // 如果确认是覆盖升级，在向导的目录选择框直接填入该已有路径
  if IsUpgradeMode and (ExistingPath <> '') then
  begin
    WizardForm.DirEdit.Text := ExistingPath;
    WizardForm.SelectDirLabel.Caption := CustomMessage('UpgradeDirLabel');
  end;
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
      DelTree(ExpandConstant('{app}\notes'), True, True, True);
      DelTree(ExpandConstant('{app}\data'), True, True, True);
      DelTree(ExpandConstant('{app}\{#MyAppExeName}.WebView2'), True, True, True);
      DeleteFile(ExpandConstant('{app}\setting.ini'));
    end;
  end;
end;