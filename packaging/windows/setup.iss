; Nexus Windows 安装程序 — Inno Setup 脚本
; 使用 Inno Setup 6 (https://jrsoftware.org/isinfo.php) 编译
;
; 编译命令:
;   ISCC.exe packaging/windows/setup.iss
;
; 或通过 CMake:
;   cmake --build build --target package

#define MyAppName "Nexus"
#define MyAppVersion "1.1.1"
#define MyAppPublisher "CherryClaw"
#define MyAppURL "https://github.com/hy7pc8gfmf-dotcom/nexus"
#define MyAppExeName "daemon.exe"

[Setup]
; ── 基本信息 ──
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
VersionInfoVersion={#MyAppVersion}

; ── 安装路径 ──
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog

; ── 输出 ──
OutputDir=..\..\dist
OutputBaseFilename=Nexus-{#MyAppVersion}-Setup

; ── 压缩 ──
Compression=lzma2/ultra64
SolidCompression=yes
InternalCompressLevel=ultra64
DiskSpanning=no

; ── 界面 ──
WizardStyle=modern
; WizardSmallImageFile=logo_small.bmp  ; 可选: 自定义左侧图片
DisableProgramGroupPage=no
DisableDirPage=no
UninstallDisplayIcon={app}\bin\{#MyAppExeName}
UninstallDisplayName={#MyAppName} {#MyAppVersion}

; ── 最小 Windows 版本 ──
MinVersion=10.0

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "zh"; MessagesFile: "ChineseSimplified.isl"

; ═══════════════════════════════════════════════════════════════════
; 自定义消息 — 中文安装说明
; ═══════════════════════════════════════════════════════════════════

[CustomMessages]
en.Nextraditional=CUDA Support (optional, enables GPU inference)
zh.Nextraditional=CUDA 支持（可选，启用 GPU 推理）
en.NexusDataDir=Data Directory for persistent state (%APPDATA%\Nexus)
zh.NexusDataDir=数据目录（持久化状态存储于 %APPDATA%\Nexus）
en.PathAdd=Add Nexus to system PATH (recommended)
zh.PathAdd=将 Nexus 添加到系统 PATH（推荐）

; ═══════════════════════════════════════════════════════════════════
; 文件安装清单
; ═══════════════════════════════════════════════════════════════════

[Files]
; ── 核心 EXE (11 个模块) ──
Source: "..\..\build\bin\Release\daemon.exe";         DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\..\build\bin\Release\core.exe";           DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\..\build\bin\Release\algo.exe";           DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\..\build\bin\Release\algo_engine.exe";    DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\..\build\bin\Release\env_checker.exe";    DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\..\build\bin\Release\psyche.exe";         DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\..\build\bin\Release\bridge.exe";         DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\..\build\bin\Release\coordinator.exe";    DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\..\build\bin\Release\quantum.exe";        DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\..\build\bin\Release\logic.exe";          DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\..\build\bin\Release\holographic.exe";    DestDir: "{app}\bin"; Flags: ignoreversion

; ── 语义数据库 & 知识图谱 (核心数据) ──
Source: "..\..\binary\.shell_d_semantic.mdb";  DestDir: "{app}\data"; Flags: ignoreversion
Source: "..\..\binary\.shell_d_seeds.json";    DestDir: "{app}\data"; Flags: ignoreversion
Source: "..\..\binary\.semantic_field.bin";    DestDir: "{app}\data"; Flags: ignoreversion
Source: "..\..\binary\.proof_index.json";      DestDir: "{app}\data"; Flags: ignoreversion
Source: "..\..\binary\.theorem_deps.json";     DestDir: "{app}\data"; Flags: ignoreversion
Source: "..\..\binary\lemma_seed_index.json";  DestDir: "{app}\data"; Flags: ignoreversion
Source: "..\..\binary\.unified_seed_bank.json"; DestDir: "{app}\data"; Flags: ignoreversion
Source: "..\..\binary\.engine_state.json";     DestDir: "{app}\data"; Flags: ignoreversion; Check: FileExists(ExpandConstant('..\..\binary\.engine_state.json'))

; ── 文档 ──
Source: "..\..\README.md";     DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\LICENSE";       DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\NOTICE";        DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\CHANGELOG.md";  DestDir: "{app}"; Flags: ignoreversion

; ── CUDA 运行时 (可选组件) ──
; 注意: cublas64_12.dll, cudart64_12.dll 等需要从 CUDA Toolkit 安装目录获取
; Source: "redist\cuda\*.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: cuda

[Components]
Name: "main";       Description: "Nexus Core Components";                  Types: full custom compact; Flags: fixed
Name: "cuda";       Description: "{cm:Nextraditional}";                     Types: full custom
Name: "docs";       Description: "Documentation and API References";        Types: full

[Types]
Name: "full";      Description: "Complete installation"
Name: "compact";   Description: "Core components only"
Name: "custom";    Description: "Custom installation"; Flags: iscustom

; ═══════════════════════════════════════════════════════════════════
; 目录和注册表
; ═══════════════════════════════════════════════════════════════════

[Dirs]
Name: "{app}\bin"
Name: "{app}\data"
Name: "{app}\logs"
Name: "{app}\conf"

[Registry]
; ── 注册 NEXUS_DATA_DIR 环境变量 ──
Root: HKCU; Subkey: "Environment"; ValueType: expandsz; ValueName: "NEXUS_DATA_DIR"; \
  ValueData: "{app}\data"; Flags: createvalueifdoesntexist; Check: WizardIsComponentSelected('main')

; ── 可选：添加到 PATH ──
Root: HKCU; Subkey: "Environment"; ValueType: expandsz; ValueName: "PATH"; \
  ValueData: "{olddata};{app}\bin"; Flags: preservestringtype; Check: PathAddCheck

[Run]
; ── 安装后选项 ──
Filename: "{app}\bin\env_checker.exe"; \
  Description: "Run environment check"; \
  Flags: postinstall nowait skipifsilent unchecked; \
  StatusMsg: "Checking system environment..."

Filename: "{app}\README.md"; \
  Description: "View README"; \
  Flags: postinstall nowait skipifsilent shellexec unchecked

; ═══════════════════════════════════════════════════════════════════
; 开始菜单和快捷键
; ═══════════════════════════════════════════════════════════════════

[Icons]
; ── Start Menu ──
Name: "{group}\Nexus Daemon (background service)";        Filename: "{app}\bin\{#MyAppExeName}"
Name: "{group}\Nexus Environment Checker";                Filename: "{app}\bin\env_checker.exe"
Name: "{group}\Nexus Coordinator (dialog interface)";     Filename: "{app}\bin\coordinator.exe"

; ── 工具分组 ──
Name: "{group}\Tools\Neural Core";          Filename: "{app}\bin\core.exe"
Name: "{group}\Tools\Algorithm Engine";    Filename: "{app}\bin\algo.exe"
Name: "{group}\Tools\Algorithm Worker";    Filename: "{app}\bin\algo_engine.exe"
Name: "{group}\Tools\Psyche Engine";       Filename: "{app}\bin\psyche.exe"
Name: "{group}\Tools\Bridge Service";      Filename: "{app}\bin\bridge.exe"
Name: "{group}\Tools\Logic Solver";        Filename: "{app}\bin\logic.exe"
Name: "{group}\Tools\Quantum Engine";      Filename: "{app}\bin\quantum.exe"
Name: "{group}\Tools\Holographic Engine";  Filename: "{app}\bin\holographic.exe"

; ── 文档 ──
Name: "{group}\Documentation\README";    Filename: "{app}\README.md"
Name: "{group}\Documentation\License";   Filename: "{app}\LICENSE"
Name: "{group}\Documentation\NOTICE";    Filename: "{app}\NOTICE"
Name: "{group}\Documentation\Changelog"; Filename: "{app}\CHANGELOG.md"

; ── 卸载 ──
Name: "{group}\Uninstall Nexus";        Filename: "{uninstallexe}"

; ═══════════════════════════════════════════════════════════════════
; 卸载行为
; ═══════════════════════════════════════════════════════════════════

[UninstallRun]
Filename: "{app}\bin\daemon.exe"; Parameters: "--stop"; Flags: runhidden nowait; RunOnceId: "StopDaemon"

[UninstallDelete]
Type: filesandordirs; Name: "{app}\logs"
Type: files; Name: "{app}\data\*"
Type: dirifempty; Name: "{app}\data"
Type: dirifempty; Name: "{app}"

; ═══════════════════════════════════════════════════════════════════
; 代码段 — 自定义逻辑
; ═══════════════════════════════════════════════════════════════════

[Code]

var
  PathCheckPage: TInputOptionWizardPage;

{ 初始化向导页 }
procedure InitializeWizard;
begin
  { PATH 添加选项页 }
  PathCheckPage := CreateInputOptionPage(
    wpSelectDir,
    'Additional Options',
    'Configure Nexus integration with your system.',
    'Select additional options to customize your Nexus installation:',
    True, False);
  PathCheckPage.Add('Add Nexus to system PATH (recommended)');
  PathCheckPage.Values[0] := True;
end;

{ 检查是否选择添加 PATH }
function PathAddCheck: Boolean;
begin
  Result := PathCheckPage.Values[0];
end;

{ 安装前检查 — 确保构建产物存在 }
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ExePath: String;
begin
  ExePath := ExpandConstant('{app}\bin\daemon.exe');
  if not FileExists(ExePath) then
  begin
    Result := 'Warning: Could not verify build artifacts.'#13#10 +
              'The installer may be incomplete.'#13#10#13#10 +
              'Please ensure you have built Nexus first:'#13#10 +
              '  cmake --build build --config Release';
  end;
end;

{ 安装后自动打开 README 的提示 }
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    { 创建空的 data 目录标记文件 }
    SaveStringToFile(ExpandConstant('{app}\data\.nexus_initialized'), '', False);
  end;
end;

{ 卸载前提醒 }
function InitializeUninstall: Boolean;
var
  Msg: String;
begin
  Msg := 'This will remove Nexus and all its components.'#13#13 +
         'Note: User data in %APPDATA%\Nexus\ will NOT be removed.'#13#13 +
         'Continue?';
  Result := MsgBox(Msg, mbConfirmation, MB_YESNO) = IDYES;
end;
