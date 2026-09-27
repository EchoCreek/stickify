// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#include "stdafx.h"
#include "host/WebViewEnvironmentManager.h"
#include "infra/AppSettingStore.h"
#include "ref/Path.h"
#include "ref/Log.h"
#include "WebView2EnvironmentOptions.h"

wil::com_ptr<ICoreWebView2Environment> WebViewEnvironmentManager::s_environment = nullptr;
bool WebViewEnvironmentManager::s_isCreating = false;
std::vector<std::function<void(HRESULT, ICoreWebView2Environment*)>> WebViewEnvironmentManager::s_callbacks;

void WebViewEnvironmentManager::GetOrCreateEnvironment(std::function<void(HRESULT, ICoreWebView2Environment*)> callback)
{
	if (s_environment != nullptr)
	{
		callback(S_OK, s_environment.get());
		return;
	}

	s_callbacks.push_back(std::move(callback));

	if (s_isCreating)
	{
		return;
	}

	s_isCreating = true;

	AppSetting currentSetting = AppSettingStore::Load();
	const wchar_t* browserExecutableFolder = nullptr;
	CString sCustomPath = Easy::Path::GetDirectory(currentSetting.sWebview2Path);
	if (currentSetting.bCustomWebview2 && !sCustomPath.IsEmpty() && Easy::Path::Exists(sCustomPath))
	{
		browserExecutableFolder = sCustomPath.GetString();
	}
	else if (currentSetting.bCustomWebview2 && !sCustomPath.IsEmpty())
	{
		CLogApp::Warn(_T("WebViewEnvironmentManager: Custom WebView2 path '%s' does not exist, falling back to system Evergreen runtime"), sCustomPath.GetString());
	}

	CString sUserDataDir = Easy::Path::GetCurDirectory(_T("data\\webview2"));
	if (!Easy::Path::Exists(sUserDataDir))
	{
		Easy::Path::Create(sUserDataDir);
	}

	CLogApp::Info(_T("WebViewEnvironmentManager: Creating single shared WebView2 Environment with browserPath='%s', userDataDir='%s'..."),
		browserExecutableFolder ? browserExecutableFolder : _T("(system default)"),
		sUserDataDir.GetString());

	auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
	if (options)
	{
		// 优化说明：
		// 1. 移除激进的 --enable-gpu-rasterization / --enable-zero-copy，交由 Chromium 依据设备 GPU 驱动自适应启用，杜绝老旧核显/虚拟机/RDP环境崩溃
		// 2. 移除对 CalculateNativeWinOcclusion 的禁用，允许 Chromium 自行感知窗口被遮挡状态并在遮挡时自动降低渲染帧率以节约 GPU 算力
		options->put_AdditionalBrowserArguments(
			L"--process-per-site "
			L"--renderer-process-limit=4 "
			L"--disable-features=Translate,MediaRouter,OptimizationHints "
			L"--disk-cache-size=10485760"
		);
	}

	HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
		browserExecutableFolder,
		sUserDataDir.GetString(),
		options.Get(),
		Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[browserExecutableFolder, sUserDataDir, options](HRESULT result, ICoreWebView2Environment* env) -> HRESULT
			{
				if (FAILED(result))
				{
					CLogApp::Error(_T("WebViewEnvironmentManager: Environment creation failed, hr=0x%08X"), result);
				}
				else
				{
					CLogApp::Info(_T("WebViewEnvironmentManager: Environment creation finished, hr=0x%08X"), result);
				}

				if (FAILED(result) && browserExecutableFolder != nullptr)
				{
					CLogApp::Warn(_T("WebViewEnvironmentManager: Custom path failed, retrying with system default Evergreen runtime..."));
					CreateCoreWebView2EnvironmentWithOptions(
						nullptr,
						sUserDataDir.GetString(),
						options.Get(),
						Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
							[](HRESULT retryResult, ICoreWebView2Environment* retryEnv) -> HRESULT
							{
								if (FAILED(retryResult))
								{
									CLogApp::Error(_T("WebViewEnvironmentManager: Fallback environment creation failed, hr=0x%08X"), retryResult);
								}
								else
								{
									CLogApp::Info(_T("WebViewEnvironmentManager: Fallback environment creation finished, hr=0x%08X"), retryResult);
								}
								s_isCreating = false;
								if (SUCCEEDED(retryResult) && retryEnv != nullptr)
								{
									s_environment = retryEnv;
								}

								auto pending = std::move(s_callbacks);
								s_callbacks.clear();

								for (auto& cb : pending)
								{
									if (cb) cb(retryResult, retryEnv);
								}
								return S_OK;
							}).Get());
					return S_OK;
				}

				s_isCreating = false;
				if (SUCCEEDED(result) && env != nullptr)
				{
					s_environment = env;
				}

				auto pending = std::move(s_callbacks);
				s_callbacks.clear();

				for (auto& cb : pending)
				{
					if (cb)
					{
						cb(result, env);
					}
				}
				return S_OK;
			}).Get());

	if (FAILED(hr))
	{
		CLogApp::Error(_T("WebViewEnvironmentManager: CreateCoreWebView2EnvironmentWithOptions FAILED immediately, hr=0x%08X"), hr);
		s_isCreating = false;
		auto pending = std::move(s_callbacks);
		s_callbacks.clear();
		for (auto& cb : pending)
		{
			if (cb) cb(hr, nullptr);
		}
	}
}

wil::com_ptr<ICoreWebView2Environment> WebViewEnvironmentManager::GetEnvironment()
{
	return s_environment;
}

void WebViewEnvironmentManager::ReleaseEnvironment()
{
	s_callbacks.clear();
	s_environment.reset();
}
