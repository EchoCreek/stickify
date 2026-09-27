// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

#include <wrl.h>
#include <wil/com.h>
#include <WebView2.h>
#include <functional>
#include <vector>

class WebViewEnvironmentManager
{
public:
	static void GetOrCreateEnvironment(std::function<void(HRESULT, ICoreWebView2Environment*)> callback);
	static wil::com_ptr<ICoreWebView2Environment> GetEnvironment();
	static void ReleaseEnvironment();

private:
	static wil::com_ptr<ICoreWebView2Environment> s_environment;
	static bool s_isCreating;
	static std::vector<std::function<void(HRESULT, ICoreWebView2Environment*)>> s_callbacks;
};
