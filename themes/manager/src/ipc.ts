// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
// ipc.ts — Typed IPC protocol for Note Manager
// Must stay in sync with host/NoteManagerProtocol.h :: NoteManagerMessageType

export type NoteManagerMessageType =
  | 'mgr_list_all'
  | 'mgr_get_note'
  | 'mgr_create_note'
  | 'mgr_delete_notes'
  | 'mgr_toggle_visible'
  | 'mgr_update_note'
  | 'mgr_locate_window'
  | 'mgr_export'
  | 'mgr_close'
  | 'mgr_min'
  | 'mgr_max'
  | 'mgr_move'
  | 'mgr_resize'
  | 'mgr_get_app_settings'
  | 'mgr_save_app_settings'
  | 'mgr_browse_folder'
  | 'mgr_open_folder'
  | 'mgr_list_trash'
  | 'mgr_restore_note'
  | 'mgr_permanent_delete'
  | 'mgr_clear_trash'
  | 'mgr_trash_list'
  | 'mgr_list_archived'
  | 'mgr_archive_note'
  | 'mgr_unarchive_note'
  | 'mgr_export_db'
  | 'mgr_import_db'
  | 'mgr_export_db_result'
  | 'mgr_import_db_result'
  | 'mgr_archived_list'
  | 'mgr_note_list'
  | 'mgr_note_detail'
  | 'mgr_export_result'
  | 'mgr_app_settings'
  | 'mgr_folder_selected'
  | 'mgr_save_settings_result'
  | 'mgr_data_changed';

export interface NoteManagerMessage<T = unknown> {
  event: NoteManagerMessageType | string;
  data: T;
}

declare global {
  interface Window {
    chrome?: {
      webview?: {
        postMessage: (message: string) => void;
        addEventListener: (type: string, listener: (e: MessageEvent | Event) => void) => void;
        removeEventListener: (type: string, listener: (e: MessageEvent | Event) => void) => void;
      };
    };
  }
}

export const isWebview = !!(typeof window !== 'undefined' && window.chrome && window.chrome.webview);

/** 向 Native 发送 NoteManager IPC 消息 */
export function sendToNative<T = unknown>(event: NoteManagerMessageType, data?: T): void {
  if (!isWebview) return;
  const msg: NoteManagerMessage<T | null> = { event, data: data !== undefined ? data : null };
  window.chrome!.webview!.postMessage(JSON.stringify(msg));
}

/** 注册 Native→Web 消息接收监听器 */
export function onNativeMessage<T = unknown>(
  handler: (msg: NoteManagerMessage<T>) => void
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
