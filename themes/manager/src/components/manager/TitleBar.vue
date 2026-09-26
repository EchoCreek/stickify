<!--
  Copyright (c) Sticky Notes Refactoring Team (2026)
  Part of Sticky Notes Refactoring Project
  Licensed under the Apache License, Version 2.0
-->
<template>
  <div
    class="titlebar"
    :class="{ 'is-maximized': isMaximized }"
    @mousedown="startDrag"
    @dblclick="handleTitleBarDblClick"
  >
    <!-- Left: App Icon & Brand Title (Full row is draggable) -->
    <div class="titlebar-left">
      <div class="app-brand">
        <div class="logo-wrapper">
          <img src="/logo.svg" class="logo-img" alt="Logo" />
        </div>
        <span class="app-name">Stickify</span>
        <span class="app-subtitle">Manager</span>
      </div>
    </div>

    <!-- Center: Search & View Controls (Spacious & Non-overlapping) -->
    <div class="titlebar-center">
      <!-- Search Input -->
      <div class="search-box">
        <i class="ri-search-line search-icon"></i>
        <input
          type="text"
          :value="searchQuery"
          @input="$emit('update:searchQuery', ($event.target as HTMLInputElement).value)"
          :placeholder="t('titleBar.searchPlaceholder')"
          class="search-input"
        />
        <button
          v-if="searchQuery"
          @click="$emit('update:searchQuery', '')"
          class="search-clear"
          :title="t('titleBar.clearSearch')"
        >
          <i class="ri-close-line"></i>
        </button>
      </div>

      <!-- View Switcher -->
      <div class="view-switch">
        <button
          @click="$emit('update:viewMode', 'grid')"
          class="view-btn"
          :class="{ active: viewMode === 'grid' }"
          :title="t('titleBar.viewGrid')"
        >
          <i class="ri-grid-line"></i>
          <span>{{ lang === 'en-US' ? 'Cards' : '卡片' }}</span>
        </button>
        <button
          @click="$emit('update:viewMode', 'table')"
          class="view-btn"
          :class="{ active: viewMode === 'table' }"
          :title="t('titleBar.viewTable')"
        >
          <i class="ri-list-check-2"></i>
          <span>{{ lang === 'en-US' ? 'Table' : '列表' }}</span>
        </button>
      </div>

      <!-- Custom Modern Sort Selector Dropdown via Teleported ElPopover -->
      <el-popover
        v-model:visible="isSortOpen"
        trigger="click"
        placement="bottom-start"
        width="auto"
        :show-arrow="false"
        :offset="6"
        transition="dropdown-slide"
        popper-class="custom-sort-popover"
        :teleported="true"
      >
        <template #reference>
          <button
            type="button"
            class="sort-trigger-btn"
            :class="{ active: isSortOpen }"
            :title="currentSortLabel"
          >
            <i :class="currentSortIcon" class="sort-prefix-icon"></i>
            <span class="sort-current-label">{{ currentSortLabel }}</span>
            <i class="ri-arrow-down-s-line sort-caret" :class="{ 'is-open': isSortOpen }"></i>
          </button>
        </template>

        <div class="sort-popover-menu">
          <div class="sort-menu-title">{{ t('common.filter') }}</div>
          <div
            v-for="opt in sortOptions"
            :key="opt.value"
            class="sort-option-row"
            :class="{ 'is-active': sortBy === opt.value }"
            @click="handleSelectSort(opt.value)"
          >
            <i :class="opt.icon" class="option-icon"></i>
            <span class="option-text">{{ opt.label }}</span>
            <i v-if="sortBy === opt.value" class="ri-check-line option-check"></i>
          </div>
        </div>
      </el-popover>
    </div>

    <!-- Right: New Note Action & Windows Controls -->
    <div class="titlebar-right">
      <button
        @click="$emit('create')"
        class="create-btn"
      >
        <i class="ri-add-line text-sm"></i>
        <span>{{ t('titleBar.newNote') }}</span>
      </button>

      <div class="divider"></div>

      <!-- Window Control Buttons -->
      <div class="window-controls">
        <div class="titlebar-button" @click.stop="minimize" :title="t('titleBar.minWindow')">
          <i class="ri-subtract-line text-sm"></i>
        </div>
        <div class="titlebar-button" @click.stop="toggleMaximize" :title="isMaximized ? (lang === 'en-US' ? 'Restore Window' : '向下还原') : t('titleBar.maxWindow')">
          <i :class="isMaximized ? 'ri-file-copy-line' : 'ri-checkbox-blank-line'" class="text-xs"></i>
        </div>
        <div class="titlebar-button titlebar-close" @click.stop="close" :title="t('titleBar.closeWindow')">
          <i class="ri-close-line text-base"></i>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue';
import { managerApi } from '../../api/managerApi';
import { useI18n } from '../../locales';

const props = defineProps<{
  searchQuery: string;
  viewMode: 'grid' | 'table';
  sortBy: string;
}>();

const emit = defineEmits<{
  (e: 'update:searchQuery', val: string): void;
  (e: 'update:viewMode', val: 'grid' | 'table'): void;
  (e: 'update:sortBy', val: string): void;
  (e: 'create'): void;
}>();

const { t, lang } = useI18n();

const isMaximized = ref(false);
const isSortOpen = ref(false);

const sortOptions = computed(() => [
  { value: 'name_desc', label: t('titleBar.sortByTimeDesc'), icon: 'ri-time-line' },
  { value: 'name_asc', label: t('titleBar.sortByTimeAsc'), icon: 'ri-history-line' },
  { value: 'title', label: t('titleBar.sortByTitleAsc'), icon: 'ri-sort-alphabet-asc' },
  { value: 'pending_desc', label: t('sidebar.activeNotes'), icon: 'ri-checkbox-circle-line' },
  { value: 'items_desc', label: t('sidebar.totalNotes'), icon: 'ri-list-ordered' },
]);

const currentSortOption = computed(() => {
  return sortOptions.value.find((o) => o.value === props.sortBy) || sortOptions.value[0];
});

const currentSortLabel = computed(() => currentSortOption.value.label);
const currentSortIcon = computed(() => currentSortOption.value.icon);

const handleSelectSort = (val: string) => {
  emit('update:sortBy', val);
  isSortOpen.value = false;
};

let lastMouseDownTime = 0;
let lastMouseDownX = 0;
let lastMouseDownY = 0;

const startDrag = (e: MouseEvent) => {
  if (e.button !== 0) return;
  const target = e.target as HTMLElement;
  if (target && target.closest('input, select, button, .titlebar-button, a, .search-box, .view-switch')) {
    return;
  }

  const now = Date.now();
  const dx = Math.abs(e.clientX - lastMouseDownX);
  const dy = Math.abs(e.clientY - lastMouseDownY);
  const isDblClick = (now - lastMouseDownTime < 350 && dx < 6 && dy < 6) || e.detail === 2;

  lastMouseDownTime = now;
  lastMouseDownX = e.clientX;
  lastMouseDownY = e.clientY;

  if (isDblClick) {
    lastMouseDownTime = 0; // 重置防止连续三击触发
    e.preventDefault();
    e.stopPropagation();
    toggleMaximize();
    return;
  }

  e.preventDefault();
  managerApi.move(true);
};

const handleTitleBarDblClick = (e: MouseEvent) => {
  if (e.button !== 0) return;
  const target = e.target as HTMLElement;
  if (target && target.closest('input, select, button, .titlebar-button, a, .search-box, .view-switch')) {
    return;
  }
  e.preventDefault();
  e.stopPropagation();
  toggleMaximize();
};

const minimize = () => {
  managerApi.minWindow();
};

const toggleMaximize = () => {
  managerApi.maxWindow();
};

const close = () => {
  managerApi.closeWindow();
};

const updateMaximizedState = () => {
  isMaximized.value = (window.innerWidth >= window.screen.availWidth - 4 && window.innerHeight >= window.screen.availHeight - 4);
};

onMounted(() => {
  window.addEventListener('resize', updateMaximizedState);
  updateMaximizedState();
});

onUnmounted(() => {
  window.removeEventListener('resize', updateMaximizedState);
});
</script>

<style scoped>
.titlebar {
  height: 38px;
  background: #161b22;
  user-select: none;
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-shrink: 0;
  border-bottom: 1px solid #30363d;
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.2);
  z-index: 9990;
  position: relative;
  padding-left: 14px;
  cursor: default;
}

.titlebar-left {
  height: 100%;
  display: flex;
  align-items: center;
  flex-shrink: 0;
}

.app-brand {
  display: flex;
  align-items: center;
  gap: 8px;
  cursor: default;
}

.logo-wrapper {
  width: 22px;
  height: 22px;
  border-radius: 5px;
  background: rgba(96, 165, 250, 0.15);
  border: 1px solid rgba(96, 165, 250, 0.3);
  display: flex;
  align-items: center;
  justify-content: center;
  pointer-events: none;
}

.logo-img {
  width: 14px;
  height: 14px;
  object-fit: contain;
}

.logo-icon {
  font-size: 13px;
  color: #60a5fa;
}

.app-name {
  font-size: 12px;
  font-weight: 700;
  color: #f0f6fc;
  letter-spacing: -0.2px;
  pointer-events: none;
}

.app-subtitle {
  font-size: 10px;
  font-weight: 500;
  color: #8b949e;
  background: #21262d;
  padding: 1px 6px;
  border-radius: 4px;
  border: 1px solid #30363d;
  pointer-events: none;
}

.titlebar-center {
  height: 100%;
  flex: 1;
  display: flex;
  justify-content: center;
  align-items: center;
  gap: 14px;
  padding: 0 16px;
  min-width: 0;
}

.search-box {
  position: relative;
  width: 280px;
  max-width: 320px;
  display: flex;
  align-items: center;
  flex-shrink: 1;
}

.search-icon {
  position: absolute;
  left: 9px;
  font-size: 13px;
  color: #6e7681;
  pointer-events: none;
}

.search-input {
  width: 100%;
  background: #161b22;
  border: 1px solid #30363d;
  border-radius: 6px;
  padding: 4px 26px 4px 28px;
  font-size: 11px;
  color: #f0f6fc;
  outline: none;
  transition: all 0.2s ease;
}

.search-input::placeholder {
  color: #6e7681;
}

.search-input:focus {
  border-color: #60a5fa;
  background: #1c2333;
  box-shadow: 0 0 0 1px #60a5fa;
}

.search-clear {
  position: absolute;
  right: 5px;
  background: transparent;
  border: none;
  color: #8b949e;
  font-size: 14px;
  cursor: pointer;
  padding: 0 2px;
  display: flex;
  align-items: center;
}

.search-clear:hover {
  color: #f0f6fc;
}

.view-switch {
  display: flex;
  background: #161b22;
  border: 1px solid #30363d;
  border-radius: 6px;
  padding: 2px;
  gap: 2px;
  flex-shrink: 0;
}

.view-btn {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 3px 8px;
  border-radius: 4px;
  font-size: 11px;
  font-weight: 500;
  color: #8b949e;
  background: transparent;
  border: none;
  cursor: pointer;
  transition: all 0.15s ease;
  white-space: nowrap;
}

.view-btn:hover {
  color: #f0f6fc;
}

.view-btn.active {
  background: #252d3d;
  color: #60a5fa;
}

/* Modern Sort Dropdown */
.sort-dropdown-wrap {
  position: relative;
  flex-shrink: 0;
}

.sort-trigger-btn {
  display: flex;
  align-items: center;
  gap: 4px;
  background: #161b22;
  border: 1px solid #30363d;
  color: #c9d1d9;
  font-size: 11px;
  font-weight: 500;
  border-radius: 6px;
  padding: 4px 8px;
  cursor: pointer;
  transition: all 0.18s ease;
  white-space: nowrap;
  user-select: none;
  width: 114px;
  min-width: 114px;
  justify-content: space-between;
  box-sizing: border-box;
  flex-shrink: 0;
}

.sort-trigger-btn:hover {
  background: #1c2333;
  border-color: #485468;
  color: #f0f6fc;
}

.sort-trigger-btn.active {
  border-color: #60a5fa;
  background: #1c2333;
  color: #60a5fa;
  box-shadow: 0 0 0 1px rgba(96, 165, 250, 0.4);
}

.sort-prefix-icon {
  font-size: 13px;
  color: #60a5fa;
  flex-shrink: 0;
}

.sort-current-label {
  line-height: 1;
  flex: 1;
  text-align: left;
  margin: 0 2px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.sort-caret {
  font-size: 12px;
  color: #8b949e;
  transition: transform 0.2s ease;
  flex-shrink: 0;
}

.sort-caret.is-open {
  transform: rotate(180deg);
  color: #60a5fa;
}

/* Dropdown Popover Menu */
.sort-popover-menu {
  min-width: 170px;
  width: 100%;
  box-sizing: border-box;
  padding: 2px;
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.sort-menu-title {
  font-size: 11px;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.05em;
  color: #8b949e;
  padding: 4px 8px 3px 8px;
  margin-bottom: 2px;
  user-select: none;
}

.sort-option-row {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 7px 10px;
  box-sizing: border-box;
  width: 100%;
  border-radius: 6px;
  font-size: 12.5px;
  line-height: 1.4;
  color: #c9d1d9;
  cursor: pointer;
  transition: all 0.15s ease;
  user-select: none;
}

.sort-option-row:hover {
  background: #21283b;
  color: #f0f6fc;
}

.sort-option-row.is-active {
  background: rgba(96, 165, 250, 0.14);
  color: #60a5fa;
  font-weight: 500;
}

.option-icon {
  font-size: 14px;
  color: #8b949e;
  flex-shrink: 0;
  width: 16px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
}

.sort-option-row.is-active .option-icon {
  color: #60a5fa;
}

.option-text {
  flex: 1;
  white-space: nowrap;
  padding-right: 4px;
}

.option-check {
  font-size: 14px;
  color: #60a5fa;
  flex-shrink: 0;
  margin-left: auto;
}
</style>

<style>
/* Global Popover styling for teleported sort dropdown */
.custom-sort-popover.el-popper {
  background: #161b22 !important;
  border: 1px solid #30363d !important;
  border-radius: 8px !important;
  box-shadow: 0 12px 32px rgba(0, 0, 0, 0.65), 0 0 0 1px rgba(255, 255, 255, 0.06) !important;
  padding: 4px !important;
  width: auto !important;
  min-width: 170px !important;
  box-sizing: border-box !important;
  transform-origin: top left !important;
}

.custom-sort-popover.el-popper .el-popper__arrow {
  display: none !important;
}

/* Fast and snappy drop-down slide animation */
.dropdown-slide-enter-active {
  transition: opacity 0.12s ease-out, transform 0.12s cubic-bezier(0, 0, 0.2, 1) !important;
}

.dropdown-slide-leave-active {
  transition: opacity 0.08s ease-in, transform 0.08s cubic-bezier(0.4, 0, 1, 1) !important;
}

.dropdown-slide-enter-from {
  opacity: 0 !important;
  transform: translateY(-8px) scaleY(0.92) !important;
}

.dropdown-slide-leave-to {
  opacity: 0 !important;
  transform: translateY(-4px) scaleY(0.96) !important;
}
</style>
<style scoped>

.titlebar-right {
  display: flex;
  height: 100%;
  align-items: center;
  justify-content: flex-end;
  flex-shrink: 0;
  gap: 8px;
}

.create-btn {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 4px 12px;
  background: #238636;
  border: 1px solid rgba(240, 246, 252, 0.1);
  color: #ffffff;
  font-size: 11px;
  font-weight: 600;
  border-radius: 6px;
  cursor: pointer;
  transition: background 0.15s ease;
  white-space: nowrap;
}

.create-btn:hover {
  background: #2ea043;
}

.create-btn:active {
  background: #1f7f32;
}

.divider {
  width: 1px;
  height: 16px;
  background: #30363d;
}

.window-controls {
  display: flex;
  height: 100%;
  align-items: stretch;
  margin: 0;
  border-radius: 0 !important;
}

.titlebar-button {
  display: inline-flex;
  justify-content: center;
  align-items: center;
  width: 46px;
  height: 100%;
  color: #8b949e;
  transition: background 0.15s ease, color 0.15s ease;
  cursor: pointer;
  border-radius: 0 !important;
  border: none;
  outline: none;
}

.titlebar-button:hover {
  background: rgba(255, 255, 255, 0.08);
  color: #f0f6fc;
}

.titlebar-button:active {
  background: rgba(255, 255, 255, 0.14);
}

.titlebar-close {
  border-radius: 0 !important;
}

.titlebar-close:hover {
  background: #e81123 !important;
  color: #ffffff !important;
  border-radius: 0 !important;
}

.titlebar-close:active {
  background: #bf0f1d !important;
  color: #ffffff !important;
  border-radius: 0 !important;
}
</style>
