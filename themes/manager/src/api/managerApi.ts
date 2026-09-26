// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
import { sendToNative, onNativeMessage, type NoteManagerMessage } from '../ipc';

export interface NotePreviewItem {
  id: number;
  content: string;
  finished: boolean;
}

export interface NoteSummary {
  name: string;
  title: string;
  rect: { left: number; top: number; right: number; bottom: number };
  bgColor: string;
  opacity: number;
  opacityEnabled: boolean;
  visible: boolean;
  topMost: boolean;
  isDeleted?: boolean;
  deletedAt?: number;
  isArchived?: boolean;
  archivedAt?: number;
  itemCount: number;
  pendingCount: number;
  completedCount: number;
  previewItems: NotePreviewItem[];
}

export interface NoteItemDto {
  id: number;
  content: string;
  finished: boolean;
}

export interface NoteDetail {
  name: string;
  title: string;
  rect: { left: number; top: number; right: number; bottom: number };
  bgColor: string;
  opacity: number;
  opacityEnabled: boolean;
  visible: boolean;
  topMost: boolean;
  isDeleted?: boolean;
  deletedAt?: number;
  isArchived?: boolean;
  archivedAt?: number;
  items: NoteItemDto[];
}

export interface ExportResult {
  success: boolean;
  format: string;
  content?: string;
}

export interface ExportDbResult {
  success: boolean;
  path?: string;
}

export interface ImportDbResult {
  success: boolean;
  count?: number;
}

export interface HotkeyItem {
  value: number;
  text: string;
}

export interface AppSettingsDto {
  autoRun: boolean;
  customWebview2: boolean;
  webview2Path: string;
  noteDir: string;
  theme: string;
  defaultBgColor: string;
  language?: string;
  appVersion?: string;
  themes: string[];
  hotkeys: {
    newNote: HotkeyItem;
    editNote: HotkeyItem;
    hideAll: HotkeyItem;
    showAll: HotkeyItem;
  };
}

export interface SaveSettingsResult {
  success: boolean;
  message: string;
}

export interface DataChangedEvent {
  action: 'create' | 'update' | 'delete' | 'visibility' | 'settings' | 'restore' | 'permanent_delete' | 'clear_trash' | 'archive' | 'unarchive' | 'import_db';
  name: string;
}

export const managerApi = {
  listAll(params?: { keyword?: string; filter?: string }): void {
    sendToNative('mgr_list_all', params || {});
  },

  listTrash(): void {
    sendToNative('mgr_list_trash', {});
  },

  listArchived(): void {
    sendToNative('mgr_list_archived', {});
  },

  getNote(name: string): void {
    sendToNative('mgr_get_note', { name });
  },

  createNote(data?: { title?: string; color?: string; bgColor?: string }): void {
    const payload = data ? { title: data.title, bgColor: data.bgColor || data.color } : {};
    sendToNative('mgr_create_note', payload);
  },

  deleteNotes(names: string[]): void {
    sendToNative('mgr_delete_notes', { names });
  },

  restoreNote(name: string): void {
    sendToNative('mgr_restore_note', { name });
  },

  restoreNotes(names: string[]): void {
    for (const name of names) {
      sendToNative('mgr_restore_note', { name });
    }
  },

  permanentDelete(names: string[]): void {
    sendToNative('mgr_permanent_delete', { names });
  },

  clearTrash(): void {
    sendToNative('mgr_clear_trash', {});
  },

  archiveNote(name: string): void {
    sendToNative('mgr_archive_note', { name });
  },

  archiveNotes(names: string[]): void {
    for (const name of names) {
      sendToNative('mgr_archive_note', { name });
    }
  },

  unarchiveNote(name: string): void {
    sendToNative('mgr_unarchive_note', { name });
  },

  unarchiveNotes(names: string[]): void {
    for (const name of names) {
      sendToNative('mgr_unarchive_note', { name });
    }
  },

  toggleVisible(names: string[], visible: boolean): void {
    sendToNative('mgr_toggle_visible', { names, visible });
  },

  updateNote(note: Partial<NoteDetail> & { name: string }): void {
    sendToNative('mgr_update_note', note);
  },

  locateWindow(name: string): void {
    sendToNative('mgr_locate_window', { name });
  },

  exportNotes(paramsOrNames: { format: 'json' | 'md'; names?: string[] } | string[], format: 'md' | 'json' = 'md'): void {
    if (Array.isArray(paramsOrNames)) {
      sendToNative('mgr_export', { names: paramsOrNames, format });
    } else {
      sendToNative('mgr_export', paramsOrNames);
    }
  },

  getAppSettings(): void {
    sendToNative('mgr_get_app_settings');
  },

  saveAppSettings(settings: Partial<AppSettingsDto>): void {
    sendToNative('mgr_save_app_settings', settings);
  },

  browseFolder(defaultPath?: string): void {
    sendToNative('mgr_browse_folder', { defaultPath: defaultPath || '' });
  },

  openFolder(folder?: string): void {
    sendToNative('mgr_open_folder', { folder: folder || '' });
  },

  exportDb(): void {
    sendToNative('mgr_export_db');
  },

  importDb(): void {
    sendToNative('mgr_import_db');
  },

  move(start: boolean): void {
    sendToNative('mgr_move', start);
  },

  resize(direction: string): void {
    sendToNative('mgr_resize', direction);
  },

  closeManager(): void {
    sendToNative('mgr_close');
  },

  closeWindow(): void {
    sendToNative('mgr_close');
  },

  minimize(): void {
    sendToNative('mgr_min');
  },

  minWindow(): void {
    sendToNative('mgr_min');
  },

  maximize(): void {
    sendToNative('mgr_max');
  },

  maxWindow(): void {
    sendToNative('mgr_max');
  },

  onNoteList(handler: (list: NoteSummary[]) => void): () => void {
    return onNativeMessage<NoteSummary[]>((msg) => {
      if (msg.event === 'mgr_note_list' && Array.isArray(msg.data)) {
        handler(msg.data);
      }
    });
  },

  onTrashList(handler: (list: NoteSummary[]) => void): () => void {
    return onNativeMessage<NoteSummary[]>((msg) => {
      if (msg.event === 'mgr_trash_list' && Array.isArray(msg.data)) {
        handler(msg.data);
      }
    });
  },

  onArchivedList(handler: (list: NoteSummary[]) => void): () => void {
    return onNativeMessage<NoteSummary[]>((msg) => {
      if (msg.event === 'mgr_archived_list' && Array.isArray(msg.data)) {
        handler(msg.data);
      }
    });
  },

  onNoteDetail(handler: (detail: NoteDetail) => void): () => void {
    return onNativeMessage<NoteDetail>((msg) => {
      if (msg.event === 'mgr_note_detail' && msg.data) {
        handler(msg.data);
      }
    });
  },

  onDataChanged(handler: (event: DataChangedEvent) => void): () => void {
    return onNativeMessage<DataChangedEvent>((msg) => {
      if ((msg.event === 'mgr_data_changed' || msg.event === 'data_changed') && msg.data) {
        handler(msg.data);
      }
    });
  },

  onExportResult(handler: (result: ExportResult) => void): () => void {
    return onNativeMessage<ExportResult>((msg) => {
      if (msg.event === 'mgr_export_result' && msg.data) {
        handler(msg.data);
      }
    });
  },

  onAppSettings(handler: (settings: AppSettingsDto) => void): () => void {
    return onNativeMessage<AppSettingsDto>((msg) => {
      if (msg.event === 'mgr_app_settings' && msg.data) {
        handler(msg.data);
      }
    });
  },

  onFolderSelected(handler: (data: { folder: string }) => void): () => void {
    return onNativeMessage<{ folder: string }>((msg) => {
      if (msg.event === 'mgr_folder_selected' && msg.data) {
        handler(msg.data);
      }
    });
  },

  onSaveSettingsResult(handler: (result: SaveSettingsResult) => void): () => void {
    return onNativeMessage<SaveSettingsResult>((msg) => {
      if (msg.event === 'mgr_save_settings_result' && msg.data) {
        handler(msg.data);
      }
    });
  },

  onExportDbResult(handler: (result: ExportDbResult) => void): () => void {
    return onNativeMessage<ExportDbResult>((msg) => {
      if (msg.event === 'mgr_export_db_result' && msg.data) {
        handler(msg.data);
      }
    });
  },

  onImportDbResult(handler: (result: ImportDbResult) => void): () => void {
    return onNativeMessage<ImportDbResult>((msg) => {
      if (msg.event === 'mgr_import_db_result' && msg.data) {
        handler(msg.data);
      }
    });
  },
};
