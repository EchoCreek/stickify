// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#include "stdafx.h"
#include <shobjidl.h>
#include "host/NoteManager.h"
#include "host/NoteManagerWindow.h"
#include "infra/AppSettingStore.h"
#include "infra/DataMigrator.h"
#include "infra/SqliteMigrator.h"

NoteManager::NoteManager()
	: m_sqliteRepo()
	, m_repo(m_sqliteRepo)
	, m_service(m_repo)
{
	AppSetting setting = AppSettingStore::Load();
	if (!setting.sNoteDir.IsEmpty())
	{
		CString dbPath = Easy::Path::Resolve(setting.sNoteDir, _T("notes.db"));
		m_sqliteRepo.SetDbPath(dbPath);
	}
}

NoteManager::NoteManager(INoteRepository& repo)
	: m_sqliteRepo()
	, m_repo(repo)
	, m_service(m_repo)
{
	AppSetting setting = AppSettingStore::Load();
	if (!setting.sNoteDir.IsEmpty())
	{
		CString dbPath = Easy::Path::Resolve(setting.sNoteDir, _T("notes.db"));
		m_sqliteRepo.SetDbPath(dbPath);
	}
}

NoteManager::~NoteManager()
{
	Cleanup();
}

void NoteManager::Cleanup()
{
	// 1. 立即隐藏所有窗口与管理器，给用户即时的退出反馈，彻底消除假死/卡顿残留
	if (m_managerWindow && m_managerWindow->GetSafeHwnd() && ::IsWindow(m_managerWindow->GetSafeHwnd()))
	{
		m_managerWindow->ShowWindow(SW_HIDE);
	}
	for (auto& win : m_windows)
	{
		if (win && win->GetSafeHwnd() && ::IsWindow(win->GetSafeHwnd()))
		{
			win->ShowWindow(SW_HIDE);
		}
	}

	// 2. 平稳销毁管理器窗口
	if (m_managerWindow)
	{
		if (m_managerWindow->GetSafeHwnd() && ::IsWindow(m_managerWindow->GetSafeHwnd()))
		{
			m_managerWindow->DestroyWindow();
		}
		m_managerWindow.reset();
	}

	// 3. 平稳销毁所有便签窗口
	for (auto& win : m_windows)
	{
		if (win)
		{
			win->SetOnClosedCallback(nullptr);
			if (win->GetSafeHwnd() && ::IsWindow(win->GetSafeHwnd()))
			{
				win->DestroyWindow();
			}
		}
	}
	m_windows.clear();
}

void NoteManager::Init()
{
	AppSetting setting = AppSettingStore::Load();
	CString notesDir = setting.sNoteDir.IsEmpty() ? Easy::Path::GetCurDirectory(_T("notes\\")) : setting.sNoteDir;

	DataMigrator::MigrateIfNeeded(m_repo);
	SqliteMigrator::MigrateIfNeeded(m_sqliteRepo, notesDir);

	ReloadNotesFromRepo();
}

void NoteManager::ReloadNotesFromRepo()
{
	// Close and destroy existing active note windows safely
	for (auto& win : m_windows)
	{
		if (win && win->GetSafeHwnd() && ::IsWindow(win->GetSafeHwnd()))
		{
			win->DestroyWindow();
		}
	}
	m_windows.clear();

	std::vector<CString> noteNames = m_repo.ListAll();
	if (noteNames.empty())
	{
		New();
	}
	else
	{
		for (const auto& name : noteNames)
		{
			New(name);
		}
	}
}

NoteHostWindow* NoteManager::New(const CString& noteName)
{
	CLogApp::Info(_T("NoteManager::New: creating note, requestedName='%s', existingCount=%d"),
		noteName.GetString(), static_cast<int>(m_windows.size()));

	auto win = std::make_unique<NoteHostWindow>(m_service, m_repo);
	if (!win->Init(noteName))
	{
		CLogApp::Error(_T("NoteManager::New: win->Init failed"));
		return nullptr;
	}

	if (noteName.IsEmpty() && !m_windows.empty())
	{
		CRect r = win->GetNote().rect;
		int offset = static_cast<int>(m_windows.size() * 30) % 240;
		r.OffsetRect(offset, offset);
		m_service.UpdateRect(const_cast<Note&>(win->GetNote()), r);
	}

	win->SetOnClosedCallback([this](NoteHostWindow* w) {
		OnWindowClosed(w);
	});

	if (win->GetNote().visible)
	{
		if (!win->Create(NoteHostWindow::IDD, nullptr))
		{
			CLogApp::Error(_T("NoteManager::New: win->Create failed, error=%d"), GetLastError());
			return nullptr;
		}
		win->SetMouseThrough(false);
		win->ShowWindow(SW_SHOW);
		win->SetForegroundWindow();
		NoteHostWindow* pWin = win.get();
		m_windows.push_back(std::move(win));
		CLogApp::Info(_T("NoteManager::New: Success! HWND=0x%p, totalWindows=%d"), pWin->GetSafeHwnd(), static_cast<int>(m_windows.size()));
		return pWin;
	}

	return nullptr;
}

bool NoteManager::CheckEdit()
{
	CPoint mousePt;
	GetCursorPos(&mousePt);
	for (auto& win : m_windows)
	{
		if (win && win->GetSafeHwnd() && ::IsWindow(win->GetSafeHwnd()))
		{
			CRect rc;
			win->GetWindowRect(&rc);
			if (rc.PtInRect(mousePt))
			{
				win->SetMouseThrough(false);
				win->ShowWindow(SW_SHOW);
				win->SetForegroundWindow();
				return true;
			}
		}
	}
	return false;
}

void NoteManager::FocusFirstNote()
{
	for (auto& win : m_windows)
	{
		if (win && win->GetSafeHwnd() && ::IsWindow(win->GetSafeHwnd()))
		{
			win->SetMouseThrough(false);
			win->ShowWindow(SW_SHOW);
			::SetWindowPos(win->GetSafeHwnd(), win->GetNote().topMost ? HWND_TOPMOST : HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
			win->SetForegroundWindow();
			return;
		}
	}
}

void NoteManager::SetMouseThrough(bool through)
{
	for (auto& win : m_windows)
	{
		if (win && win->GetSafeHwnd() && ::IsWindow(win->GetSafeHwnd()))
		{
			win->SetMouseThrough(through);
		}
	}
}

void NoteManager::SetVisible(bool show)
{
	if (show)
	{
		std::vector<CString> noteNames = m_repo.ListAll();
		for (const auto& name : noteNames)
		{
			Note note;
			if (m_repo.Load(name, note))
			{
				note.visible = true;
				m_repo.Save(note);
			}

			bool alreadyOpen = false;
			for (const auto& win : m_windows)
			{
				if (win && win->GetNote().name == name)
				{
					win->UpdateNoteFromExternal(note);
					win->SetForegroundWindow();
					alreadyOpen = true;
					break;
				}
			}
			if (!alreadyOpen)
			{
				New(name);
			}
		}

		if (m_windows.empty())
		{
			New();
		}
	}
	else
	{
		std::vector<CString> noteNames = m_repo.ListAll();
		for (const auto& name : noteNames)
		{
			Note note;
			if (m_repo.Load(name, note))
			{
				note.visible = false;
				m_repo.Save(note);
			}
		}

		for (auto& win : m_windows)
		{
			if (win && win->GetSafeHwnd() && ::IsWindow(win->GetSafeHwnd()))
			{
				const_cast<Note&>(win->GetNote()).visible = false;
				win->ShowWindow(SW_HIDE);
			}
		}
	}

	BroadcastDataChanged(_T("update"), _T(""));
}

void NoteManager::ShowManager()
{
	if (!m_managerWindow)
	{
		m_managerWindow = std::make_unique<NoteManagerWindow>(*this, m_sqliteRepo);
	}
	m_managerWindow->ShowManagerWindow();
}

void NoteManager::BroadcastDataChanged(const CString& action, const CString& noteName)
{
	if (m_managerWindow)
	{
		m_managerWindow->BroadcastDataChanged(action, noteName);
	}
}

void NoteManager::SetNoteVisible(const CString& noteName, bool visible)
{
	Note note;
	if (!m_repo.Load(noteName, note)) return;
	note.visible = visible;
	m_repo.Save(note);

	if (visible)
	{
		bool found = false;
		for (auto& win : m_windows)
		{
			if (win && win->GetNote().name == noteName)
			{
				win->UpdateNoteFromExternal(note);
				win->SetForegroundWindow();
				found = true;
				break;
			}
		}
		if (!found)
		{
			New(noteName);
		}
	}
	else
	{
		for (auto& win : m_windows)
		{
			if (win && win->GetNote().name == noteName)
			{
				win->UpdateNoteFromExternal(note);
				win->ShowWindow(SW_HIDE);
				break;
			}
		}
	}
}

void NoteManager::CloseNoteWindow(const CString& noteName)
{
	for (auto it = m_windows.begin(); it != m_windows.end(); ++it)
	{
		if ((*it) && (*it)->GetNote().name == noteName)
		{
			// 关键防崩：
			// 1. 先将 unique_ptr 转移出来并从 m_windows 移除，防止 DestroyWindow 触发的回调发生重入 Double Erase
			auto win = std::move(*it);
			m_windows.erase(it);

			// 2. 解除窗口关闭回调
			win->SetOnClosedCallback(nullptr);

			// 3. 销毁窗口 HWND
			if (win->GetSafeHwnd() && ::IsWindow(win->GetSafeHwnd()))
			{
				win->DestroyWindow();
			}
			// win 离开作用域安全析构
			break;
		}
	}
}

void NoteManager::LocateNoteWindow(const CString& noteName)
{
	for (auto& win : m_windows)
	{
		if (win && win->GetNote().name == noteName)
		{
			// 1. 若便签当前在桌面处于隐藏状态，自动切换为可见并同步数据库与管理器
			Note note = win->GetNote();
			if (!note.visible)
			{
				note.visible = true;
				m_repo.Save(note);
				win->UpdateNoteFromExternal(note);
				if (m_managerWindow)
				{
					m_managerWindow->BroadcastDataChanged(_T("update"), noteName);
				}
			}

			// 2. 触发桌面窗口定位与呼吸光环高亮动效
			win->LocateAndHighlight();
			return;
		}
	}

	// 若该便签尚未在桌面打开窗口，则从数据库读取并创建
	Note note;
	if (m_repo.Load(noteName, note))
	{
		note.visible = true;
		m_repo.Save(note);
		NoteHostWindow* pWin = New(noteName);
		if (pWin && pWin->GetSafeHwnd() && ::IsWindow(pWin->GetSafeHwnd()))
		{
			pWin->LocateAndHighlight();
			if (m_managerWindow)
			{
				m_managerWindow->BroadcastDataChanged(_T("update"), noteName);
			}
		}
	}
}

void NoteManager::SyncActiveNoteWindow(const Note& note)
{
	for (auto& win : m_windows)
	{
		if (win && win->GetNote().name == note.name)
		{
			win->UpdateNoteFromExternal(note);
			break;
		}
	}
}

void NoteManager::OnWindowClosed(NoteHostWindow* win)
{
	for (auto it = m_windows.begin(); it != m_windows.end(); ++it)
	{
		if (it->get() == win)
		{
			auto p = std::move(*it);
			m_windows.erase(it);
			p->SetOnClosedCallback(nullptr);
			break;
		}
	}
}

bool NoteManager::SetNoteDir(const CString& newDir)
{
	if (newDir.IsEmpty()) return false;

	CString normalizedDir = newDir;
	if (normalizedDir.Right(1) != _T("\\") && normalizedDir.Right(1) != _T("/"))
	{
		normalizedDir += _T("\\");
	}

	Easy::Path::Create(normalizedDir);
	CString newDbPath = Easy::Path::Resolve(normalizedDir, _T("notes.db"));

	CString currentDbPath = m_sqliteRepo.GetDbPath();
	if (currentDbPath.IsEmpty())
	{
		AppSetting cur = AppSettingStore::Load();
		currentDbPath = Easy::Path::Resolve(cur.sNoteDir.IsEmpty() ? Easy::Path::GetCurDirectory(_T("notes\\")) : cur.sNoteDir, _T("notes.db"));
	}

	if (currentDbPath.CompareNoCase(newDbPath) == 0)
	{
		return true; // Already on this path
	}

	// Switch database (auto copy if target does not exist)
	bool switched = m_sqliteRepo.SwitchDatabase(newDbPath, true);
	if (!switched)
	{
		CLogApp::Error(_T("NoteManager::SetNoteDir: SwitchDatabase failed to '%s'"), newDbPath.GetString());
		return false;
	}

	// Run migration on new directory if legacy json notes exist there
	SqliteMigrator::MigrateIfNeeded(m_sqliteRepo, normalizedDir);

	// Reload active note windows from the new repository
	ReloadNotesFromRepo();

	// Broadcast data changed event so Note Manager UI refreshes
	BroadcastDataChanged(_T("reload"), _T(""));

	return true;
}

void NoteManager::ApplySettings(const AppSetting& setting)
{
	AppSetting curSetting = GetSettings();
	bool dirChanged = !setting.sNoteDir.IsEmpty() && setting.sNoteDir.CompareNoCase(curSetting.sNoteDir) != 0;
	bool themeChanged = !setting.sTheme.IsEmpty() && setting.sTheme.CompareNoCase(curSetting.sTheme) != 0;

	if (dirChanged)
	{
		SetNoteDir(setting.sNoteDir);
	}

	if (m_onSettingsChanged)
	{
		m_onSettingsChanged(setting);
	}
	else
	{
		AppSettingStore::Save(setting);
	}

	if (themeChanged && !dirChanged)
	{
		ReloadNotesFromRepo();
	}
	else
	{
		// 动态向所有当前已打开的桌面便签窗口同步推送最新设置与语言变更
		for (auto& win : m_windows)
		{
			if (win && win->GetSafeHwnd() && ::IsWindow(win->GetSafeHwnd()))
			{
				win->PushNoteSetting();
			}
		}
	}
}

AppSetting NoteManager::GetSettings() const
{
	return AppSettingStore::Load();
}

CString NoteManager::BrowseFolder(HWND hWnd, const CString& defaultPath)
{
	CString root = defaultPath;
	if (root.IsEmpty())
	{
		root = GetSettings().sNoteDir;
	}
	return Easy::Path::Folder(hWnd, root);
}

void NoteManager::OpenFolder(const CString& path)
{
	CString targetPath = path;
	if (targetPath.IsEmpty())
	{
		targetPath = GetSettings().sNoteDir;
	}
	if (!targetPath.IsEmpty())
	{
		Easy::Path::Create(targetPath);
		ShellExecute(NULL, _T("open"), targetPath.GetString(), NULL, NULL, SW_SHOWNORMAL);
	}
}

CString NoteManager::ExportDatabaseDialog(HWND hWnd)
{
	CString resultPath = _T("");
	Microsoft::WRL::ComPtr<IFileSaveDialog> pFileSave;
	HRESULT hr = ::CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileSave));
	if (SUCCEEDED(hr) && pFileSave)
	{
		COMDLG_FILTERSPEC rgSpec[] = {
			{ L"SQLite Database (*.db)", L"*.db" },
			{ L"All Files (*.*)", L"*.*" }
		};
		pFileSave->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);
		pFileSave->SetDefaultExtension(L"db");

		CTime now = CTime::GetCurrentTime();
		CString defaultName = now.Format(_T("Stickify_Backup_%Y%m%d_%H%M%S.db"));
		pFileSave->SetFileName(defaultName.GetString());
		pFileSave->SetTitle(L"导出 SQLite 完整数据库备份");

		hr = pFileSave->Show(hWnd);
		if (SUCCEEDED(hr))
		{
			Microsoft::WRL::ComPtr<IShellItem> pItem;
			hr = pFileSave->GetResult(&pItem);
			if (SUCCEEDED(hr) && pItem)
			{
				PWSTR pszFilePath = nullptr;
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
				if (SUCCEEDED(hr) && pszFilePath)
				{
					CString savePath(pszFilePath);
					::CoTaskMemFree(pszFilePath);

					if (m_sqliteRepo.ExportDatabase(savePath))
					{
						resultPath = savePath;
					}
				}
			}
		}
	}
	else
	{
		OPENFILENAMEW ofn = { sizeof(ofn) };
		wchar_t szFile[MAX_PATH] = { 0 };
		CTime now = CTime::GetCurrentTime();
		CString defaultName = now.Format(_T("Stickify_Backup_%Y%m%d_%H%M%S.db"));
		wcsncpy_s(szFile, defaultName.GetString(), _TRUNCATE);

		ofn.hwndOwner = hWnd;
		ofn.lpstrFilter = L"SQLite Database (*.db)\0*.db\0All Files (*.*)\0*.*\0";
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrDefExt = L"db";
		ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

		if (::GetSaveFileNameW(&ofn))
		{
			CString savePath(szFile);
			if (m_sqliteRepo.ExportDatabase(savePath))
			{
				resultPath = savePath;
			}
		}
	}

	return resultPath;
}

bool NoteManager::ImportDatabaseDialog(HWND hWnd, int& outNoteCount)
{
	outNoteCount = 0;
	bool imported = false;

	Microsoft::WRL::ComPtr<IFileOpenDialog> pFileOpen;
	HRESULT hr = ::CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileOpen));
	if (SUCCEEDED(hr) && pFileOpen)
	{
		COMDLG_FILTERSPEC rgSpec[] = {
			{ L"SQLite Database (*.db)", L"*.db" },
			{ L"All Files (*.*)", L"*.*" }
		};
		pFileOpen->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);
		pFileOpen->SetDefaultExtension(L"db");
		pFileOpen->SetTitle(L"选择要导入的 SQLite 数据库备份文件");

		hr = pFileOpen->Show(hWnd);
		if (SUCCEEDED(hr))
		{
			Microsoft::WRL::ComPtr<IShellItem> pItem;
			hr = pFileOpen->GetResult(&pItem);
			if (SUCCEEDED(hr) && pItem)
			{
				PWSTR pszFilePath = nullptr;
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
				if (SUCCEEDED(hr) && pszFilePath)
				{
					CString srcPath(pszFilePath);
					::CoTaskMemFree(pszFilePath);

					if (m_sqliteRepo.ImportDatabase(srcPath))
					{
						ReloadNotesFromRepo();
						outNoteCount = static_cast<int>(m_sqliteRepo.ListAll().size());
						imported = true;
					}
				}
			}
		}
	}
	else
	{
		OPENFILENAMEW ofn = { sizeof(ofn) };
		wchar_t szFile[MAX_PATH] = { 0 };
		ofn.hwndOwner = hWnd;
		ofn.lpstrFilter = L"SQLite Database (*.db)\0*.db\0All Files (*.*)\0*.*\0";
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrDefExt = L"db";
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

		if (::GetOpenFileNameW(&ofn))
		{
			CString srcPath(szFile);
			if (m_sqliteRepo.ImportDatabase(srcPath))
			{
				ReloadNotesFromRepo();
				outNoteCount = static_cast<int>(m_sqliteRepo.ListAll().size());
				imported = true;
			}
		}
	}

	return imported;
}


