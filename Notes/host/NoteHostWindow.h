// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

#include <afxwin.h>
#include <afxdialogex.h>
#include <wrl.h>
#include <wil/com.h>
#include <functional>
#include <vector>
#include <memory>
#include "WebView2.h"
#include "core/domain/Note.h"
#include "core/ports/INoteRepository.h"
#include "core/services/NoteService.h"
#include "core/services/NoteEventBus.h"
#include "host/NoteHostProtocol.h"
#include "Resource.h"

class NoteHostWindow : public CDialogEx
{
	DECLARE_DYNAMIC(NoteHostWindow)
public:
	enum { IDD = IDD_DLG_NOTE };

	NoteHostWindow(NoteService& service,
	               INoteRepository& repo,
	               CWnd* pParent = nullptr);
	virtual ~NoteHostWindow();

	// 对外控制接口（供 NoteManager 调用）
	bool  Init(const CString& noteName);   // 加载 Note 数据并初始化窗口
	void  SetMouseThrough(bool through);
	bool  IsMouseThrough() const;
	void  SetWindowAlpha(float alpha);     // 0.0~100.0，封装 SetLayeredWindowAttributes
	const Note& GetNote() const;
	void  UpdateNoteFromExternal(const Note& updatedNote);
	void  LocateAndHighlight();
	void  PushNoteSetting();               // 向 Web 端主动推送最新便签设置与语言配置
	void  SetOnClosedCallback(std::function<void(NoteHostWindow*)> cb);

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;
	// MFC 消息映射（仅保留 Win32 必要消息）
	virtual BOOL OnInitDialog() override;
	virtual void OnOK() override {}
	virtual void OnCancel() override;
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg void OnNcMouseMove(UINT nHitTest, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnRawInput(UINT nInputcode, HRAWINPUT hRawInput);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwpos);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwpos);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg void OnExitSizeMove();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg LRESULT OnDisplayChange(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	DECLARE_MESSAGE_MAP()

private:
	// WebView2 初始化（私有，外部不感知）
	void InitWebView();
	HRESULT OnCreateCoreWebView2ControllerCompleted(HRESULT result, ICoreWebView2Controller* controller);
	HRESULT OnWebMessageReceived(ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args);     // 仅转发给 m_dispatcher
	HRESULT OnDocumentReady(ICoreWebView2* webview, ICoreWebView2NavigationCompletedEventArgs* args);

	// 向 Web 推送数据的私有方法
	void PushNoteItems();
	void PushMouseThrough();
	void PushEdgeDockState();
	void PostWebMessage(const CString& event, const CString& dataJson);

	// Win32 窗口辅助（私有）
	void ApplyTopMost(bool topMost);
	void OnMouseMoving(CPoint pt);
	void CheckEdgeDockState();
	void RetractToEdge();
	void RestoreFromEdge();
	void UpdateEdgeDockRegion(bool bHidden);
	void UpdateEdgeTimer();  // 按需启停 1002 号贴边检测定时器，避免在非贴边状态空转

	// Handler 注册
	void RegisterHandlers();
	void RegisterHandler(NoteHostMessageType type, std::function<void(const NoteHostMessage&)> fn);

public:
	enum class EdgeDockState {
		None = 0,
		Left,
		Right,
		Top
	};

private:
	NoteService&       m_service;
	INoteRepository&   m_repo;
	Note               m_note;
	NoteHostMessageDispatcher m_dispatcher;    // 注入 NoteHostProtocol 分发器

	// WebView2 COM 对象（私有）
	Microsoft::WRL::ComPtr<ICoreWebView2Controller>  m_controller;
	Microsoft::WRL::ComPtr<ICoreWebView2>            m_webView;

	// 窗口状态（私有）
	CBrush  m_brush;
	bool    m_bMoveWindow    = false;
	bool    m_bMouseThrough  = false;
	CRect   m_BeginMoveRect;
	CPoint  m_BeginMovePoint;

	// 贴边自动收缩隐藏与拉手状态
	EdgeDockState m_edgeDockState = EdgeDockState::None;
	bool          m_bEdgeHidden        = false;
	bool          m_bEdgeLocked        = false;
	bool          m_bRegionApplied     = false; // 防止 UpdateEdgeDockRegion(true) 在稳定后重复调用
	CRect         m_rcRestored;
	int           m_nLeaveCount          = 0;
	int           m_nStartupWarmupTicks  = 10; // 启动 1 秒缓冲期 (10 x 100ms)
	int           m_nEdgeHiddenTicks     = 0;  // 贴边隐入帧计数（用于动画完成后精准裁剪物理区域）
	UINT          m_nCachedDpi           = 96; // 缓存窗口 DPI，避免高频符号查找

	// Handler 内存持有
	std::vector<std::unique_ptr<INoteHostMessageHandler>> m_handlers;

	// 关闭回调
	std::function<void(NoteHostWindow*)> m_onClosed;
};
