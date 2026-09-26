// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
// ipc.ts — Web↔Native IPC typed contract
// Must stay in sync with host/WebMessageDispatcher.h :: WebMessageType

export type WebMessageType =
  | 'move' | 'resize' | 'lock' | 'top' | 'opacityable' | 'bgcolor' | 'title'
  | 'close' | 'add' | 'task' | 'update' | 'update_all' | 'remove'
  | 'hide' | 'clear' | 'listen' | 'restore_dock' | 'edge_lock';

export interface WebMessage<T = any> {
  event: WebMessageType;
  data: T;
}

declare global {
  interface Window {
    chrome?: {
      webview?: {
        postMessage: (message: string) => void;
        addEventListener: (type: string, listener: (e: any) => void) => void;
        removeEventListener: (type: string, listener: (e: any) => void) => void;
      };
    };
  }
}

export const isWebview = !!(typeof window !== 'undefined' && window.chrome && window.chrome.webview);

/** 向 Native 发送 IPC 消息 */
export function sendToNative<T = any>(event: WebMessageType, data?: T): void {
  if (!isWebview) return;
  const msg: WebMessage<T | null> = { event, data: data !== undefined ? data : null };
  window.chrome!.webview!.postMessage(JSON.stringify(msg));
}

/** 注册 Native→Web 消息接收监听器 */
export function onNativeMessage(
  handler: (msg: WebMessage) => void
): () => void {
  if (!isWebview) return () => {};
  const listener = (e: Event) => {
    const raw = (e as MessageEvent).data;
    try {
      handler(typeof raw === 'string' ? JSON.parse(raw) : raw);
    } catch { /* ignore malformed messages */ }
  };
  window.chrome!.webview!.addEventListener('message', listener);
  return () => window.chrome!.webview!.removeEventListener('message', listener);
}
