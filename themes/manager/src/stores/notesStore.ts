// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
import { defineStore } from 'pinia';
import { ref, computed } from 'vue';
import { ElMessage } from 'element-plus';
import {
  managerApi,
  type NoteSummary,
  type NoteDetail,
  type ExportResult,
  type DataChangedEvent,
} from '../api/managerApi';
import { t } from '../locales';

const CROSS_LIST_ACTIONS = new Set<DataChangedEvent['action']>([
  'delete',
  'archive',
  'unarchive',
  'restore',
  'permanent_delete',
  'clear_trash',
  'import_db',
]);

export const useNotesStore = defineStore('notes', () => {
  // ─── State ────────────────────────────────────────────────────────────────
  const allNotes = ref<NoteSummary[]>([]);
  const archivedNotes = ref<NoteSummary[]>([]);
  const trashNotes = ref<NoteSummary[]>([]);
  const selectedNames = ref<string[]>([]);
  const drawerVisible = ref(false);
  const currentNoteDetail = ref<NoteDetail | null>(null);

  const exportModalVisible = ref(false);
  const exportContent = ref('');
  const exportFormat = ref('md');

  // ─── Getters ──────────────────────────────────────────────────────────────
  const activeCount = computed(() => allNotes.value.filter((n) => n.visible).length);
  const archivedCount = computed(() => archivedNotes.value.length);
  const trashCount = computed(() => trashNotes.value.length);

  // ─── Actions: Selection ───────────────────────────────────────────────────
  function toggleSelect(name: string) {
    const idx = selectedNames.value.indexOf(name);
    if (idx >= 0) {
      selectedNames.value.splice(idx, 1);
    } else {
      selectedNames.value.push(name);
    }
  }

  function toggleSelectAll(currentFilteredNames: string[]) {
    if (
      selectedNames.value.length === currentFilteredNames.length &&
      currentFilteredNames.length > 0
    ) {
      selectedNames.value = [];
    } else {
      selectedNames.value = [...currentFilteredNames];
    }
  }

  function clearSelection() {
    selectedNames.value = [];
  }

  // ─── Actions: Detail Drawer ───────────────────────────────────────────────
  function openDetailDrawer(name: string, isReadOnly = false) {
    if (isReadOnly) return;
    if (currentNoteDetail.value?.name !== name) {
      currentNoteDetail.value = null;
    }
    managerApi.getNote(name);
    drawerVisible.value = true;
  }

  function closeDetailDrawer() {
    drawerVisible.value = false;
  }

  // ─── Actions: IPC Operations ──────────────────────────────────────────────
  function createNote(title?: string, color?: string) {
    managerApi.createNote({ title: title || t('titleBar.newNote'), color });
  }

  function updateNote(updated: NoteDetail) {
    managerApi.updateNote(updated);
  }

  function locateWindow(name: string) {
    managerApi.locateWindow(name);
    const target = allNotes.value.find((n) => n.name === name);
    const title = target?.title || target?.name || t('titleBar.newNote');
    ElMessage({
      message: t('messages.locatedSuccess', { title }),
      type: 'success',
      duration: 2200,
    });
  }

  function toggleVisible(note: NoteSummary) {
    managerApi.toggleVisible([note.name], !note.visible);
  }

  function batchSetVisible(visible: boolean) {
    if (selectedNames.value.length === 0) return;
    managerApi.toggleVisible(selectedNames.value, visible);
    const action = visible ? t('messages.actionShow') : t('messages.actionHide');
    ElMessage({
      message: t('messages.batchVisibilitySuccess', { action, n: selectedNames.value.length }),
      type: 'success',
    });
    selectedNames.value = [];
  }

  function toggleTopmost(note: NoteSummary) {
    managerApi.updateNote({
      name: note.name,
      topMost: !note.topMost,
    });
  }

  function deleteNotes(names: string[]) {
    if (currentNoteDetail.value && names.includes(currentNoteDetail.value.name)) {
      drawerVisible.value = false;
    }
    selectedNames.value = selectedNames.value.filter((n) => !names.includes(n));
    managerApi.deleteNotes(names);
  }

  function archiveNotes(names: string[]) {
    if (currentNoteDetail.value && names.includes(currentNoteDetail.value.name)) {
      drawerVisible.value = false;
    }
    selectedNames.value = selectedNames.value.filter((n) => !names.includes(n));
    managerApi.archiveNotes(names);
  }

  function unarchiveNotes(names: string[]) {
    if (currentNoteDetail.value && names.includes(currentNoteDetail.value.name)) {
      drawerVisible.value = false;
    }
    selectedNames.value = selectedNames.value.filter((n) => !names.includes(n));
    managerApi.unarchiveNotes(names);
  }

  function restoreNotes(names: string[]) {
    if (currentNoteDetail.value && names.includes(currentNoteDetail.value.name)) {
      drawerVisible.value = false;
    }
    selectedNames.value = selectedNames.value.filter((n) => !names.includes(n));
    managerApi.restoreNotes(names);
  }

  function permanentDeleteNotes(names: string[]) {
    if (currentNoteDetail.value && names.includes(currentNoteDetail.value.name)) {
      drawerVisible.value = false;
    }
    selectedNames.value = selectedNames.value.filter((n) => !names.includes(n));
    managerApi.permanentDelete(names);
  }

  function clearTrash() {
    drawerVisible.value = false;
    selectedNames.value = [];
    managerApi.clearTrash();
  }

  function exportNotes(names: string[], format: 'md' | 'json' = 'md') {
    managerApi.exportNotes(names, format);
  }

  // ─── Lifecycle & Subscriptions ────────────────────────────────────────────
  function initIpc() {
    const unbindList = managerApi.onNoteList((list) => {
      allNotes.value = list;
    });

    const unbindArchived = managerApi.onArchivedList((list) => {
      archivedNotes.value = list;
    });

    const unbindTrash = managerApi.onTrashList((list) => {
      trashNotes.value = list;
    });

    const unbindDetail = managerApi.onNoteDetail((detail) => {
      currentNoteDetail.value = detail;
    });

    const unbindDataChanged = managerApi.onDataChanged((event) => {
      // 精准刷新：根据 action 类型判断哪些列表会受影响
      // 跨列表移动类操作（删除/归档/还原/清空回收站/导入）需要同时刷新多个列表；
      // 常规操作（新建/修改/显隐/设置）仅影响活跃便签列表。
      if (CROSS_LIST_ACTIONS.has(event.action)) {
        managerApi.listAll();
        managerApi.listArchived();
        managerApi.listTrash();
      } else {
        // create / update / visibility / settings —— 只影响活跃列表
        managerApi.listAll();
      }

      // 若当前抽屉处于打开状态，同步更新抽屉内的便签详情与待办项
      if (
        drawerVisible.value &&
        currentNoteDetail.value &&
        (!event.name || event.name === currentNoteDetail.value.name)
      ) {
        managerApi.getNote(currentNoteDetail.value.name);
      }
    });

    const unbindExport = managerApi.onExportResult((res: ExportResult) => {
      if (res.success && res.content) {
        exportContent.value = res.content;
        exportFormat.value = res.format || 'md';
        exportModalVisible.value = true;
      }
    });

    managerApi.listAll();
    managerApi.listArchived();
    managerApi.listTrash();

    return () => {
      unbindList();
      unbindArchived();
      unbindTrash();
      unbindDetail();
      unbindDataChanged();
      unbindExport();
    };
  }

  return {
    allNotes,
    archivedNotes,
    trashNotes,
    selectedNames,
    drawerVisible,
    currentNoteDetail,
    exportModalVisible,
    exportContent,
    exportFormat,
    activeCount,
    archivedCount,
    trashCount,
    toggleSelect,
    toggleSelectAll,
    clearSelection,
    openDetailDrawer,
    closeDetailDrawer,
    createNote,
    updateNote,
    locateWindow,
    toggleVisible,
    batchSetVisible,
    toggleTopmost,
    deleteNotes,
    archiveNotes,
    unarchiveNotes,
    restoreNotes,
    permanentDeleteNotes,
    clearTrash,
    exportNotes,
    initIpc,
  };
});
