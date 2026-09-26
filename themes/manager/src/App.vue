<!--
  Copyright (c) Sticky Notes Refactoring Team (2026)
  Part of Sticky Notes Refactoring Project
  Licensed under the Apache License, Version 2.0
-->
<template>
  <el-config-provider :locale="elementPlusLocale">
    <div class="manager-layout select-none relative" :class="{ 'is-maximized': isMaximized }">
      <!-- Eight-directional Window Native Resize Handles -->
      <template v-if="!isMaximized">
        <div class="win-resize-handle win-resize-top" @mousedown.prevent.stop="onResizeStart('top', $event)" />
        <div class="win-resize-handle win-resize-bottom" @mousedown.prevent.stop="onResizeStart('bottom', $event)" />
        <div class="win-resize-handle win-resize-left" @mousedown.prevent.stop="onResizeStart('left', $event)" />
        <div class="win-resize-handle win-resize-right" @mousedown.prevent.stop="onResizeStart('right', $event)" />
        <div class="win-resize-handle win-resize-top-left" @mousedown.prevent.stop="onResizeStart('top-left', $event)" />
        <div class="win-resize-handle win-resize-top-right" @mousedown.prevent.stop="onResizeStart('top-right', $event)" />
        <div class="win-resize-handle win-resize-bottom-left" @mousedown.prevent.stop="onResizeStart('bottom-left', $event)" />
        <div class="win-resize-handle win-resize-bottom-right" @mousedown.prevent.stop="onResizeStart('bottom-right', $event)" />
      </template>

      <!-- Modern 38px TitleBar with Remix Icon & Native Drag -->
      <TitleBar
        v-model:searchQuery="filterStore.searchQuery"
        v-model:viewMode="filterStore.viewMode"
        v-model:sortBy="filterStore.sortBy"
        @create="notesStore.createNote()"
      />

      <!-- Absolute Layout Body with Resizable Splitter -->
      <div class="layout-body">
        <!-- Sidebar -->
        <aside class="layout-sidebar" :style="{ width: `${sidebarWidth}px` }">
          <StatSidebar
            :notes="notesStore.allNotes"
            :archivedCount="notesStore.archivedCount"
            :trashCount="notesStore.trashCount"
            v-model:activeFilter="filterStore.activeFilter"
            v-model:colorFilter="filterStore.colorFilter"
          />
        </aside>

        <!-- Sidebar Drag Resize Handle (Hit target with blue accent) -->
        <div
          class="resize-handle"
          :style="{ left: `${sidebarWidth}px` }"
          @mousedown="startResize"
        ></div>

        <!-- Main Workspace Content -->
        <main
          class="layout-main"
          :style="{ left: `${sidebarWidth + 1}px`, width: `calc(100% - ${sidebarWidth + 1}px)` }"
          @click="handleMainContentClick"
        >
          <!-- Settings Panel -->
          <SettingsPanel v-if="filterStore.activeFilter === 'settings'" />

          <!-- Notes Workspace Content -->
          <div v-else class="main-content-scroll">
            <!-- Section Bar -->
            <div class="section-bar">
              <div class="section-left">
                <h1 class="section-heading">{{ filterTitle }}</h1>
                <span class="section-count">
                  {{ notesStore.selectedNames.length > 0
                      ? t('batch.selectedCount', { n: notesStore.selectedNames.length })
                      : t('batch.totalCount', { n: filteredNotes.length }) }}
                </span>
              </div>
              <div class="section-right">
                <button
                  v-if="filterStore.activeFilter === 'trash' && filteredNotes.length > 0"
                  @click="handleClearTrashConfirm"
                  class="clear-trash-btn"
                  :title="t('confirm.clearTrashTitle')"
                >
                  <i class="ri-delete-bin-line mr-1"></i>{{ t('batch.permanentDeleteAll') }}
                </button>
                <button
                  v-if="filteredNotes.length > 0"
                  @click="toggleSelectAll"
                  class="select-all-btn"
                >
                  {{ isAllSelected ? t('batch.cancelSelection') : t('batch.selectAll') }}
                </button>
              </div>
            </div>

            <!-- Grid View -->
            <div
              v-if="filterStore.viewMode === 'grid' && filteredNotes.length > 0"
              class="cards-grid"
            >
              <NoteCard
                v-for="note in filteredNotes"
                :key="note.name"
                :note="note"
                :selected="notesStore.selectedNames.includes(note.name)"
                :searchKeyword="filterStore.debouncedSearchQuery"
                @select="notesStore.openDetailDrawer(note.name, isReadOnlyFilter)"
                @toggle-select="notesStore.toggleSelect(note.name)"
                @locate="notesStore.locateWindow(note.name)"
                @toggle-visible="notesStore.toggleVisible(note)"
                @toggle-topmost="notesStore.toggleTopmost(note)"
                @archive="(name) => notesStore.archiveNotes([name])"
                @unarchive="(name) => notesStore.unarchiveNotes([name])"
                @delete="deleteSingleNoteConfirm"
                @restore="(name) => notesStore.restoreNotes([name])"
                @permanent-delete="permanentDeleteSingleNoteConfirm"
              />
            </div>

            <!-- Table View -->
            <div v-else-if="filterStore.viewMode === 'table' && filteredNotes.length > 0" class="table-container">
              <NoteTable
                :notes="filteredNotes"
                :selectedNames="notesStore.selectedNames"
                :searchKeyword="filterStore.debouncedSearchQuery"
                :isTrash="filterStore.activeFilter === 'trash'"
                :isArchive="filterStore.activeFilter === 'archive'"
                @select="(n) => notesStore.openDetailDrawer(n.name, isReadOnlyFilter)"
                @toggle-select="notesStore.toggleSelect"
                @toggle-all="toggleSelectAll"
                @locate="notesStore.locateWindow"
                @toggle-visible="notesStore.toggleVisible"
                @toggle-topmost="notesStore.toggleTopmost"
                @archive="(name) => notesStore.archiveNotes([name])"
                @unarchive="(name) => notesStore.unarchiveNotes([name])"
                @delete="deleteSingleNoteConfirm"
                @restore="(name) => notesStore.restoreNotes([name])"
                @permanent-delete="permanentDeleteSingleNoteConfirm"
              />
            </div>

            <!-- Empty State -->
            <div
              v-else
              class="empty-state"
            >
              <template v-if="filterStore.activeFilter === 'trash'">
                <i class="ri-delete-bin-line empty-icon text-gray-500"></i>
                <h3 class="empty-title">{{ t('sidebar.trash') }}</h3>
                <p class="empty-desc">
                  {{ t('sidebar.trashDesc') }}
                </p>
              </template>
              <template v-else-if="filterStore.activeFilter === 'archive'">
                <i class="ri-inbox-archive-line empty-icon text-amber-500/70"></i>
                <h3 class="empty-title">{{ t('sidebar.archivedNotes') }}</h3>
                <p class="empty-desc">
                  {{ t('sidebar.archivedNotesDesc') }}
                </p>
              </template>
              <template v-else>
                <i class="ri-file-search-line empty-icon"></i>
                <h3 class="empty-title">{{ t('noteTable.emptyTable') }}</h3>
                <p class="empty-desc">
                  {{ t('sidebar.allNotesDesc') }}
                </p>
                <button
                  @click="notesStore.createNote()"
                  class="empty-create-btn"
                >
                  <i class="ri-add-line mr-1"></i>{{ t('titleBar.newNote') }}
                </button>
              </template>
            </div>
          </div>

          <!-- Detail Drawer with re-triggering key on note change -->
          <NoteDetailDrawer
            v-if="filterStore.activeFilter !== 'settings' && !isReadOnlyFilter"
            :key="notesStore.currentNoteDetail?.name || 'empty-drawer'"
            :visible="notesStore.drawerVisible"
            :note="notesStore.currentNoteDetail"
            @close="notesStore.closeDetailDrawer()"
            @update="notesStore.updateNote"
            @locate="notesStore.locateWindow"
            @delete="deleteSingleNoteConfirm"
            @export="(name) => notesStore.exportNotes([name], 'md')"
          />

          <!-- Floating Batch Action Bar -->
          <BatchActionBar
            v-if="filterStore.activeFilter !== 'settings'"
            :selectedCount="notesStore.selectedNames.length"
            :drawerOpen="notesStore.drawerVisible"
            :isTrash="filterStore.activeFilter === 'trash'"
            :isArchive="filterStore.activeFilter === 'archive'"
            @show-all="notesStore.batchSetVisible(true)"
            @hide-all="notesStore.batchSetVisible(false)"
            @export-md="notesStore.exportNotes(notesStore.selectedNames, 'md')"
            @export-json="notesStore.exportNotes(notesStore.selectedNames, 'json')"
            @archive-selected="batchArchiveConfirm"
            @unarchive-selected="notesStore.unarchiveNotes(notesStore.selectedNames)"
            @delete-selected="batchDeleteConfirm"
            @restore-selected="notesStore.restoreNotes(notesStore.selectedNames)"
            @permanent-delete-selected="batchPermanentDeleteConfirm"
            @clear-selection="notesStore.clearSelection()"
          />
        </main>

        <!-- Export Result Modal (Covers full body below TitleBar) -->
        <ExportModal
          :visible="notesStore.exportModalVisible"
          :content="notesStore.exportContent"
          :format="notesStore.exportFormat"
          @close="notesStore.exportModalVisible = false"
        />

        <!-- Two-Step Confirmation Modal (Covers full body below TitleBar) -->
        <ConfirmModal
          :visible="confirmState.visible"
          :title="confirmState.title"
          :message="confirmState.message"
          :confirmText="confirmState.confirmText"
          :cancelText="confirmState.cancelText"
          :danger="confirmState.danger"
          @confirm="handleConfirmAction"
          @cancel="confirmState.visible = false"
        />
      </div>
    </div>
  </el-config-provider>
</template>

<script setup lang="ts">
import { ref, reactive, computed, watch, onMounted, onUnmounted } from 'vue';
import { ElMessage, ElConfigProvider } from 'element-plus';
import { managerApi } from './api/managerApi';
import { isColorMatch } from './utils';
import { useNotesStore } from './stores/notesStore';
import { useFilterStore } from './stores/filterStore';
import { useI18n } from './locales';
import TitleBar from './components/manager/TitleBar.vue';
import StatSidebar from './components/manager/StatSidebar.vue';
import NoteCard from './components/manager/NoteCard.vue';
import NoteTable from './components/manager/NoteTable.vue';
import NoteDetailDrawer from './components/manager/NoteDetailDrawer.vue';
import BatchActionBar from './components/manager/BatchActionBar.vue';
import ExportModal from './components/manager/ExportModal.vue';
import ConfirmModal from './components/manager/ConfirmModal.vue';
import SettingsPanel from './components/manager/SettingsPanel.vue';

const notesStore = useNotesStore();
const filterStore = useFilterStore();
const { t, elementPlusLocale } = useI18n();

// ─── Dialog Confirmation State ────────────────────────────────────────────────
const confirmState = reactive({
  visible: false,
  title: '删除便签',
  message: '',
  confirmText: '确定删除',
  cancelText: '取消',
  danger: true,
  action: () => {},
});

function handleConfirmAction() {
  const fn = confirmState.action;
  confirmState.visible = false;
  if (typeof fn === 'function') {
    fn();
  }
}

// ─── Native Window Resize Start Handler ──────────────────────────────────────
function onResizeStart(direction: string, e: MouseEvent) {
  if (e.button !== 0) return;
  e.preventDefault();
  e.stopPropagation();
  managerApi.resize(direction);
}

// ─── Resizable Sidebar Logic ──────────────────────────────────────────────────
const SIDEBAR_MIN = 200;
const SIDEBAR_MAX = 380;
const sidebarWidth = ref(240);
const isResizing = ref(false);

const startResize = (e: MouseEvent) => {
  isResizing.value = true;
  document.addEventListener('mousemove', handleResize);
  document.addEventListener('mouseup', stopResize);
  e.preventDefault();
};

const handleResize = (e: MouseEvent) => {
  if (!isResizing.value) return;
  const raw = e.clientX;
  sidebarWidth.value = Math.max(SIDEBAR_MIN, Math.min(raw, SIDEBAR_MAX));
};

const stopResize = () => {
  isResizing.value = false;
  document.removeEventListener('mousemove', handleResize);
  document.removeEventListener('mouseup', stopResize);
};

// ─── Filter & Sort Logic ──────────────────────────────────────────────────────
const isReadOnlyFilter = computed(
  () => filterStore.activeFilter === 'trash' || filterStore.activeFilter === 'archive'
);

const filterTitle = computed(() => {
  if (filterStore.debouncedSearchQuery) {
    return t('titleBar.searchResults', { n: filteredNotes.value.length });
  }
  switch (filterStore.activeFilter) {
    case 'settings': return t('settings.headerTitle');
    case 'trash': return t('sidebar.trash');
    case 'archive': return t('sidebar.archivedNotes');
    case 'active': return t('sidebar.activeNotes');
    case 'hidden': return t('titleBar.hiddenOnly');
    case 'pending': return t('sidebar.activeNotes');
    case 'completed': return t('noteCard.allCompleted');
    default: return t('sidebar.allNotes');
  }
});

const filteredNotes = computed(() => {
  const source = filterStore.activeFilter === 'trash'
    ? notesStore.trashNotes
    : (filterStore.activeFilter === 'archive' ? notesStore.archivedNotes : notesStore.allNotes);

  let list = source.filter((note) => {
    if (filterStore.activeFilter === 'active' && !note.visible) return false;
    if (filterStore.activeFilter === 'hidden' && note.visible) return false;
    if (filterStore.activeFilter === 'pending' && note.pendingCount === 0) return false;
    if (filterStore.activeFilter === 'completed' && (note.pendingCount > 0 || note.itemCount === 0)) return false;

    if (filterStore.colorFilter && !isColorMatch(note.bgColor, filterStore.colorFilter)) {
      return false;
    }

    if (filterStore.debouncedSearchQuery.trim()) {
      const q = filterStore.debouncedSearchQuery.trim().toLowerCase();
      const titleMatch = (note.title || '').toLowerCase().includes(q);
      const itemsMatch = note.previewItems?.some((it) => (it.content || '').toLowerCase().includes(q));
      if (!titleMatch && !itemsMatch) return false;
    }

    return true;
  });

  list = [...list].sort((a, b) => {
    if (filterStore.activeFilter === 'trash') {
      const tA = a.deletedAt || 0;
      const tB = b.deletedAt || 0;
      if (tB !== tA) return tB - tA;
    }
    if (filterStore.activeFilter === 'archive') {
      const tA = a.archivedAt || 0;
      const tB = b.archivedAt || 0;
      if (tB !== tA) return tB - tA;
    }
    if (filterStore.sortBy === 'name_desc') return b.name.localeCompare(a.name);
    if (filterStore.sortBy === 'name_asc') return a.name.localeCompare(b.name);
    if (filterStore.sortBy === 'title') return (a.title || '').localeCompare(b.title || '');
    if (filterStore.sortBy === 'pending_desc') return b.pendingCount - a.pendingCount;
    if (filterStore.sortBy === 'items_desc') return b.itemCount - a.itemCount;
    return 0;
  });

  return list;
});

const isAllSelected = computed(() => {
  return filteredNotes.value.length > 0 && notesStore.selectedNames.length === filteredNotes.value.length;
});

function toggleSelectAll() {
  notesStore.toggleSelectAll(filteredNotes.value.map((n) => n.name));
}

watch(() => filterStore.activeFilter, () => {
  notesStore.closeDetailDrawer();
  notesStore.clearSelection();
});

function handleMainContentClick(e: MouseEvent) {
  if (!notesStore.drawerVisible) return;
  const target = e.target as HTMLElement;
  if (!target) return;
  if (
    target.closest(
      '.detail-drawer, .note-card, .note-table-row, .el-table__row, .batch-action-bar, .el-popover, .el-dialog, .el-message, .select-all-btn, .clear-trash-btn, .empty-create-btn'
    )
  ) {
    return;
  }
  notesStore.closeDetailDrawer();
}

// ─── Confirmation Wrappers for Destructive Actions ───────────────────────────
function deleteSingleNoteConfirm(name: string) {
  const target = notesStore.allNotes.find((n) => n.name === name);
  const noteTitle = target?.title || name || t('titleBar.newNote');
  confirmState.title = t('confirm.deleteTitle');
  confirmState.message = t('confirm.deleteMsg', { n: 1 });
  confirmState.confirmText = t('confirm.confirmBtn');
  confirmState.cancelText = t('confirm.cancelBtn');
  confirmState.danger = true;
  confirmState.action = () => {
    notesStore.deleteNotes([name]);
    ElMessage({
      message: t('confirm.deleteTitle'),
      type: 'success',
      duration: 2000,
    });
  };
  confirmState.visible = true;
}

function permanentDeleteSingleNoteConfirm(name: string) {
  const target = notesStore.trashNotes.find((n) => n.name === name);
  const noteTitle = target?.title || name || t('titleBar.newNote');
  confirmState.title = t('confirm.permanentDeleteTitle');
  confirmState.message = t('confirm.permanentDeleteMsg');
  confirmState.confirmText = t('confirm.confirmBtn');
  confirmState.cancelText = t('confirm.cancelBtn');
  confirmState.danger = true;
  confirmState.action = () => {
    notesStore.permanentDeleteNotes([name]);
    ElMessage({
      message: t('confirm.permanentDeleteTitle'),
      type: 'success',
      duration: 2000,
    });
  };
  confirmState.visible = true;
}

function batchDeleteConfirm() {
  if (notesStore.selectedNames.length === 0) return;
  const count = notesStore.selectedNames.length;
  confirmState.title = t('confirm.deleteTitle');
  confirmState.message = t('confirm.deleteMsg', { n: count });
  confirmState.confirmText = `${t('confirm.confirmBtn')} (${count})`;
  confirmState.cancelText = t('confirm.cancelBtn');
  confirmState.danger = true;
  confirmState.action = () => {
    notesStore.deleteNotes([...notesStore.selectedNames]);
    ElMessage({
      message: t('messages.batchVisibilitySuccess', { action: t('common.delete'), n: count }),
      type: 'success',
      duration: 2200,
    });
  };
  confirmState.visible = true;
}

function batchArchiveConfirm() {
  if (notesStore.selectedNames.length === 0) return;
  const count = notesStore.selectedNames.length;
  notesStore.archiveNotes([...notesStore.selectedNames]);
  ElMessage({
    message: t('messages.batchVisibilitySuccess', { action: t('noteCard.archive'), n: count }),
    type: 'success',
    duration: 2200,
  });
}

function batchPermanentDeleteConfirm() {
  if (notesStore.selectedNames.length === 0) return;
  const count = notesStore.selectedNames.length;
  confirmState.title = t('confirm.permanentDeleteTitle');
  confirmState.message = t('confirm.permanentDeleteMsg');
  confirmState.confirmText = `${t('confirm.confirmBtn')} (${count})`;
  confirmState.cancelText = t('confirm.cancelBtn');
  confirmState.danger = true;
  confirmState.action = () => {
    notesStore.permanentDeleteNotes([...notesStore.selectedNames]);
    ElMessage({
      message: t('confirm.permanentDeleteTitle'),
      type: 'success',
      duration: 2200,
    });
  };
  confirmState.visible = true;
}

function handleClearTrashConfirm() {
  if (notesStore.trashNotes.length === 0) return;
  confirmState.title = t('confirm.clearTrashTitle');
  confirmState.message = t('confirm.clearTrashMsg');
  confirmState.confirmText = t('confirm.confirmBtn');
  confirmState.cancelText = t('confirm.cancelBtn');
  confirmState.danger = true;
  confirmState.action = () => {
    notesStore.clearTrash();
    ElMessage({
      message: t('messages.trashCleared'),
      type: 'success',
      duration: 2200,
    });
  };
  confirmState.visible = true;
}

// ─── Keyboard Shortcuts ───────────────────────────────────────────────────────
function handleGlobalKeydown(e: KeyboardEvent) {
  if (confirmState.visible) {
    if (e.key === 'Escape') {
      confirmState.visible = false;
      return;
    }
    if (e.key === 'Enter') {
      handleConfirmAction();
      return;
    }
    return;
  }

  const target = e.target as HTMLElement;
  const isInInput = target && (
    target.tagName === 'INPUT' ||
    target.tagName === 'TEXTAREA' ||
    target.tagName === 'SELECT' ||
    target.isContentEditable ||
    target.closest('.hotkey-recorder-container') ||
    target.closest('.hotkey-box') ||
    target.closest('.el-popper') ||
    target.closest('.el-dialog') ||
    target.closest('.el-overlay') ||
    target.closest('.settings-panel')
  );

  if (isInInput) {
    return;
  }

  // Ctrl + N: 新建便签 (回收站与归档模式下不触发新建)
  if ((e.ctrlKey || e.metaKey) && (e.key === 'n' || e.key === 'N') && !e.altKey) {
    if (!isReadOnlyFilter.value) {
      e.preventDefault();
      notesStore.createNote();
    }
    return;
  }

  // Ctrl + F: 聚焦搜索框
  if ((e.ctrlKey || e.metaKey) && (e.key === 'f' || e.key === 'F') && !e.altKey) {
    e.preventDefault();
    const searchInput = document.querySelector('.search-input') as HTMLInputElement | null;
    if (searchInput) {
      searchInput.focus();
      searchInput.select();
    }
    return;
  }

  // Ctrl + ,: 切换到系统设置面板
  if ((e.ctrlKey || e.metaKey) && (e.key === ',' || e.key === '，')) {
    e.preventDefault();
    filterStore.activeFilter = 'settings';
    return;
  }

  // F5 or Ctrl + R: 刷新便签列表、归档列表与回收站
  if (e.key === 'F5' || ((e.ctrlKey || e.metaKey) && (e.key === 'r' || e.key === 'R'))) {
    e.preventDefault();
    managerApi.listAll();
    managerApi.listArchived();
    managerApi.listTrash();
    return;
  }

  // Escape: 依次关闭导出弹窗/抽屉/清空搜索/清空选择
  if (e.key === 'Escape') {
    if (notesStore.exportModalVisible) {
      notesStore.exportModalVisible = false;
      return;
    }
    if (notesStore.drawerVisible) {
      notesStore.closeDetailDrawer();
      return;
    }
    if (filterStore.searchQuery) {
      filterStore.searchQuery = '';
      return;
    }
    if (notesStore.selectedNames.length > 0) {
      notesStore.clearSelection();
      return;
    }
  }

  // Delete / Backspace (不在输入框中时): 批量删除选中的便签
  if (!isInInput && (e.key === 'Delete' || e.key === 'Backspace')) {
    if (notesStore.selectedNames.length > 0) {
      e.preventDefault();
      if (filterStore.activeFilter === 'trash') {
        batchPermanentDeleteConfirm();
      } else {
        batchDeleteConfirm();
      }
    }
  }

  // Ctrl + A (不在输入框中时): 全选当前便签
  if (!isInInput && (e.ctrlKey || e.metaKey) && (e.key === 'a' || e.key === 'A')) {
    e.preventDefault();
    toggleSelectAll();
  }
}

let cleanupIpc: (() => void) | null = null;
const isMaximized = ref(false);

function updateMaximizedState() {
  isMaximized.value = (window.innerWidth >= window.screen.availWidth - 4 && window.innerHeight >= window.screen.availHeight - 4);
}

onMounted(() => {
  window.addEventListener('keydown', handleGlobalKeydown);
  window.addEventListener('resize', updateMaximizedState);
  updateMaximizedState();
  cleanupIpc = notesStore.initIpc();
});

onUnmounted(() => {
  window.removeEventListener('keydown', handleGlobalKeydown);
  window.removeEventListener('resize', updateMaximizedState);
  stopResize();
  if (cleanupIpc) cleanupIpc();
});
</script>

<style scoped>
.manager-layout {
  display: flex;
  flex-direction: column;
  height: 100vh;
  width: 100vw;
  background: #0d1117;
  color: #f0f6fc;
  overflow: hidden;
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
  border-radius: 10px;
  border: none;
  box-shadow: inset 0 0 0 1px #30363d, 0 12px 40px rgba(0, 0, 0, 0.45);
  box-sizing: border-box;
  transition: border-radius 0.15s ease;
}

.manager-layout.is-maximized {
  border-radius: 0;
  border: none;
  box-shadow: none;
}

/* 8-Directional Window Resize Handles */
.win-resize-handle {
  position: absolute;
  z-index: 99999;
  background: transparent;
  user-select: none;
}
.win-resize-top {
  top: 0;
  left: 8px;
  right: 140px; /* 避开右上角窗口控制按钮 */
  height: 6px;
  cursor: ns-resize;
}
.win-resize-bottom {
  bottom: 0;
  left: 8px;
  right: 8px;
  height: 6px;
  cursor: ns-resize;
}
.win-resize-left {
  top: 8px;
  bottom: 8px;
  left: 0;
  width: 6px;
  cursor: ew-resize;
}
.win-resize-right {
  top: 38px; /* 避开右上角窗口控制按钮 */
  bottom: 8px;
  right: 0;
  width: 6px;
  cursor: ew-resize;
}
.win-resize-top-left {
  top: 0;
  left: 0;
  width: 8px;
  height: 8px;
  cursor: nwse-resize;
}
.win-resize-top-right {
  display: none; /* 避开右上角窗口控制按钮 */
}
.win-resize-bottom-left {
  bottom: 0;
  left: 0;
  width: 8px;
  height: 8px;
  cursor: nesw-resize;
}
.win-resize-bottom-right {
  bottom: 0;
  right: 0;
  width: 8px;
  height: 8px;
  cursor: nwse-resize;
}

.layout-body {
  position: relative;
  flex: 1;
  width: 100%;
  height: calc(100% - 38px);
  overflow: hidden;
  contain: strict;
  border-bottom-left-radius: 10px;
  border-bottom-right-radius: 10px;
}

.layout-sidebar {
  position: absolute;
  top: 0;
  left: 0;
  bottom: 0;
  background: #182030;
  border-right: none;
  overflow: hidden;
  white-space: nowrap;
  box-shadow: 2px 0 12px rgba(0, 0, 0, 0.25);
  z-index: 5;
  border-bottom-left-radius: 10px;
}

.resize-handle {
  position: absolute;
  top: 0;
  bottom: 0;
  width: 1px;
  background: #30363d;
  cursor: col-resize;
  z-index: 10;
  transition: background-color 0.2s ease;
}

.resize-handle::after {
  content: '';
  position: absolute;
  top: 0;
  bottom: 0;
  left: -4px;
  right: -4px;
  z-index: 1;
}

.resize-handle:hover,
.resize-handle:active {
  background: #60a5fa;
}

.layout-main {
  position: absolute;
  top: 0;
  bottom: 0;
  background: #1c2333;
  overflow: hidden;
  border-bottom-right-radius: 10px;
}

.main-content-scroll {
  width: 100%;
  height: 100%;
  overflow-y: auto;
  padding: 18px 22px 60px 22px;
  box-sizing: border-box;
}

.section-bar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
}

.section-left {
  display: flex;
  align-items: center;
  gap: 8px;
}

.section-heading {
  font-size: 14px;
  font-weight: 700;
  color: #f0f6fc;
  margin: 0;
}

.section-count {
  font-size: 11px;
  color: #8b949e;
}

.clear-trash-btn {
  display: inline-flex;
  align-items: center;
  font-size: 11px;
  color: #ff7b72;
  background: rgba(248, 81, 73, 0.12);
  border: 1px solid rgba(248, 81, 73, 0.3);
  padding: 3px 10px;
  border-radius: 5px;
  cursor: pointer;
  transition: all 0.15s ease;
  margin-right: 8px;
}

.clear-trash-btn:hover {
  background: rgba(248, 81, 73, 0.25);
  color: #fff;
  border-color: rgba(248, 81, 73, 0.6);
}

.select-all-btn {
  background: transparent;
  border: none;
  color: #60a5fa;
  font-size: 11px;
  cursor: pointer;
  padding: 0;
}

.select-all-btn:hover {
  text-decoration: underline;
}

.cards-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(240px, 1fr));
  gap: 12px;
}

.table-container {
  width: 100%;
}

.empty-state {
  height: 280px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  text-align: center;
  border: 1px dashed #30363d;
  border-radius: 8px;
  padding: 24px;
}

.empty-icon {
  font-size: 36px;
  color: #60a5fa;
  margin-bottom: 8px;
  opacity: 0.8;
}

.empty-title {
  font-size: 13px;
  font-weight: 700;
  color: #f0f6fc;
  margin: 0 0 4px 0;
}

.empty-desc {
  font-size: 11px;
  color: #8b949e;
  max-width: 320px;
  margin: 0 0 14px 0;
}

.empty-create-btn {
  display: inline-flex;
  align-items: center;
  padding: 6px 14px;
  background: #238636;
  border: 1px solid rgba(240, 246, 252, 0.1);
  color: #ffffff;
  font-size: 11px;
  font-weight: 600;
  border-radius: 6px;
  cursor: pointer;
  transition: background 0.15s;
}

.empty-create-btn:hover {
  background: #2ea043;
}
</style>

<style>
/* Toast Messages (ElMessage) anchored comfortably below the TitleBar */
.el-message {
  top: 48px !important;
  z-index: 9999 !important;
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.5) !important;
  border-radius: 8px !important;
}
</style>
