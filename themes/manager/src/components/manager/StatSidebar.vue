<!--
  Copyright (c) Sticky Notes Refactoring Team (2026)
  Part of Sticky Notes Refactoring Project
  Licensed under the Apache License, Version 2.0
-->
<template>
  <div class="side-menu select-none">
    <!-- Overview Stats Section -->
    <div class="menu-section stats-overview">
      <div class="stat-card">
        <div class="stat-header">
          <span class="stat-label">{{ t('sidebar.totalNotes') }}</span>
          <span class="stat-value">{{ totalNotes }}</span>
        </div>
        <div class="stat-sub">
          <span>{{ t('detail.todoProgress') }}</span>
          <span class="stat-rate">{{ completedItems }}/{{ totalItems }} ({{ completionRate }}%)</span>
        </div>
        <div class="progress-track">
          <div class="progress-bar" :style="{ width: `${completionRate}%` }"></div>
        </div>
      </div>
    </div>

    <!-- Category Filters Section (Remix Icon) -->
    <div class="menu-section">
      <div class="section-title">{{ t('common.filter') }}</div>
      <div
        v-for="item in navItems"
        :key="item.key"
        class="menu-item"
        :class="{ active: activeFilter === item.key }"
        @click="$emit('update:activeFilter', item.key)"
      >
        <i :class="item.icon" class="menu-icon"></i>
        <span class="menu-text">{{ item.label }}</span>
        <span class="menu-count">{{ item.count }}</span>
      </div>
    </div>

    <!-- Storage Lifecycle Section (Archive & Trash) -->
    <div class="menu-section storage-section">
      <div class="section-header">
        <span class="section-title">{{ t('sidebar.archivedNotes') }} & {{ t('sidebar.trash') }}</span>
        <span class="storage-tag">STORAGE</span>
      </div>
      <div class="storage-box">
        <!-- Archive Category -->
        <div
          class="menu-item storage-item archive-item"
          :class="{ active: activeFilter === 'archive' }"
          @click="$emit('update:activeFilter', 'archive')"
        >
          <div class="icon-wrap archive-icon-wrap">
            <i class="ri-inbox-archive-line"></i>
          </div>
          <span class="menu-text">{{ t('sidebar.archivedNotes') }}</span>
          <span class="menu-count archive-count" v-if="(archivedCount || 0) > 0">{{ archivedCount }}</span>
          <span class="menu-count" v-else>0</span>
        </div>

        <!-- Recycle Bin Category -->
        <div
          class="menu-item storage-item trash-item"
          :class="{ active: activeFilter === 'trash' }"
          @click="$emit('update:activeFilter', 'trash')"
        >
          <div class="icon-wrap trash-icon-wrap">
            <i class="ri-delete-bin-line"></i>
          </div>
          <span class="menu-text">{{ t('sidebar.trash') }}</span>
          <span class="menu-count trash-count" v-if="(trashCount || 0) > 0">{{ trashCount }}</span>
          <span class="menu-count" v-else>0</span>
        </div>
      </div>
    </div>

    <!-- Theme Colors Palette Section -->
    <div class="menu-section">
      <div class="section-header">
        <span class="section-title">{{ t('detail.colorPalette') }}</span>
        <button
          v-if="colorFilter"
          @click="$emit('update:colorFilter', '')"
          class="clear-filter-btn"
          :title="t('common.reset')"
        >
          {{ t('common.reset') }}
        </button>
      </div>

      <div class="color-grid">
        <button
          v-for="c in presetColors"
          :key="c.hex"
          @click="onColorClick(c.hex)"
          class="color-btn"
          :class="{ active: colorFilter.toLowerCase() === c.hex.toLowerCase() }"
          :style="{ backgroundColor: c.hex, borderColor: c.border }"
          :title="`${getColorName(c.hex)} (${getColorCount(c.hex)})`"
        >
          <i
            v-if="colorFilter.toLowerCase() === c.hex.toLowerCase()"
            class="ri-check-line check-icon"
            :class="isLightColor(c.hex) ? 'text-gray-800' : 'text-white'"
          ></i>
        </button>
      </div>
    </div>

    <!-- System Settings Section -->
    <div class="menu-section settings-nav-section">
      <div
        class="menu-item settings-item"
        :class="{ active: activeFilter === 'settings' }"
        @click="$emit('update:activeFilter', 'settings')"
      >
        <i class="ri-settings-3-line menu-icon text-blue-400"></i>
        <span class="menu-text font-medium">{{ t('settings.headerTitle') }}</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import type { NoteSummary } from '../../api/managerApi';
import { PRESET_COLORS, isColorMatch, isLightColor, getColorName } from '../../utils';
import { useI18n } from '../../locales';

const props = defineProps<{
  notes: NoteSummary[];
  activeFilter: string;
  colorFilter: string;
  archivedCount?: number;
  trashCount?: number;
}>();

const emit = defineEmits<{
  (e: 'update:activeFilter', val: string): void;
  (e: 'update:colorFilter', val: string): void;
}>();

const { t } = useI18n();

const totalNotes = computed(() => props.notes.length);

const totalItems = computed(() => {
  return props.notes.reduce((sum, n) => sum + (n.itemCount || 0), 0);
});

const completedItems = computed(() => {
  return props.notes.reduce((sum, n) => sum + (n.completedCount || 0), 0);
});

const completionRate = computed(() => {
  if (totalItems.value === 0) return 0;
  return Math.round((completedItems.value / totalItems.value) * 100);
});

const navItems = computed(() => {
  const all = props.notes.length;
  const active = props.notes.filter(n => n.visible).length;
  const hidden = props.notes.filter(n => !n.visible).length;
  const pending = props.notes.filter(n => n.pendingCount > 0).length;
  const completed = props.notes.filter(n => n.pendingCount === 0 && n.itemCount > 0).length;

  return [
    { key: 'all', label: t('sidebar.allNotes'), icon: 'ri-apps-2-line', count: all },
    { key: 'active', label: t('titleBar.visibleOnly'), icon: 'ri-computer-line', count: active },
    { key: 'hidden', label: t('titleBar.hiddenOnly'), icon: 'ri-eye-off-line', count: hidden },
    { key: 'pending', label: t('sidebar.activeNotes'), icon: 'ri-time-line', count: pending },
    { key: 'completed', label: t('noteCard.allCompleted'), icon: 'ri-checkbox-circle-line', count: completed },
  ];
});

const presetColors = PRESET_COLORS;

function onColorClick(hex: string) {
  if (props.colorFilter.toLowerCase() === hex.toLowerCase()) {
    emit('update:colorFilter', '');
  } else {
    emit('update:colorFilter', hex);
  }
}

function getColorCount(hex: string): number {
  return props.notes.filter((n) => isColorMatch(n.bgColor, hex)).length;
}
</script>

<style scoped>
.side-menu {
  width: 100%;
  height: 100%;
  padding: 12px 10px;
  display: flex;
  flex-direction: column;
  gap: 16px;
  overflow-y: auto;
  box-sizing: border-box;
}

.menu-section {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.stats-overview {
  margin-bottom: 2px;
}

.stat-card {
  background: #0d1117;
  border: 1px solid #30363d;
  border-radius: 8px;
  padding: 10px 12px;
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.stat-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.stat-label {
  font-size: 11px;
  color: #8b949e;
}

.stat-value {
  font-size: 16px;
  font-weight: 700;
  color: #f0f6fc;
}

.stat-sub {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 10px;
  color: #8b949e;
}

.stat-rate {
  color: #60a5fa;
  font-weight: 600;
}

.progress-track {
  width: 100%;
  height: 4px;
  background: #21262d;
  border-radius: 999px;
  overflow: hidden;
}

.progress-bar {
  height: 100%;
  background: #60a5fa;
  border-radius: 999px;
  transition: width 0.3s ease;
}

.section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 2px 6px;
}

.section-title {
  font-size: 10px;
  font-weight: 600;
  color: #8b949e;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  padding: 2px 6px;
}

.clear-filter-btn {
  background: transparent;
  border: none;
  color: #60a5fa;
  font-size: 10px;
  cursor: pointer;
  padding: 0;
}

.clear-filter-btn:hover {
  text-decoration: underline;
}

.menu-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 7px 10px;
  border-radius: 6px;
  cursor: pointer;
  color: #c9d1d9;
  font-size: 12px;
  transition: all 0.15s ease;
  border-left: 3px solid transparent;
}

.menu-item:hover {
  background: #21262d;
  color: #f0f6fc;
}

.menu-item.active {
  background: rgba(96, 165, 250, 0.12);
  color: #60a5fa;
  font-weight: 600;
  border-left: 3px solid #60a5fa;
}

.menu-icon {
  font-size: 15px;
  line-height: 1;
}

.menu-text {
  flex: 1;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.menu-count {
  font-size: 10px;
  padding: 1px 6px;
  border-radius: 999px;
  background: #21262d;
  color: #8b949e;
}

.menu-item.active .menu-count {
  background: rgba(96, 165, 250, 0.2);
  color: #93c5fd;
}

.color-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 6px;
  padding: 4px 6px;
}

.color-btn {
  width: 100%;
  aspect-ratio: 1;
  border-radius: 6px;
  border: 1px solid #30363d;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: all 0.15s ease;
}

.color-btn:hover {
  transform: scale(1.06);
}

.color-btn.active {
  box-shadow: 0 0 0 2px #60a5fa;
}

.check-icon {
  font-size: 11px;
}

.storage-section {
  margin-top: -2px;
}

.storage-tag {
  font-size: 8px;
  font-weight: 700;
  padding: 1px 4px;
  border-radius: 3px;
  background: rgba(110, 118, 129, 0.18);
  color: #8b949e;
  letter-spacing: 0.8px;
  line-height: 1;
}

.storage-box {
  background: rgba(13, 17, 23, 0.8);
  border: 1px solid #30363d;
  border-radius: 8px;
  padding: 3px;
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.storage-item {
  border-radius: 5px;
  padding: 6px 8px;
}

.icon-wrap {
  width: 22px;
  height: 22px;
  border-radius: 5px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 13px;
  flex-shrink: 0;
  transition: all 0.15s ease;
}

.archive-icon-wrap {
  background: rgba(251, 191, 36, 0.12);
  color: #fbbf24;
  border: 1px solid rgba(251, 191, 36, 0.22);
}

.trash-icon-wrap {
  background: rgba(248, 81, 73, 0.12);
  color: #f85149;
  border: 1px solid rgba(248, 81, 73, 0.22);
}

.archive-item:hover .archive-icon-wrap {
  background: rgba(251, 191, 36, 0.22);
  box-shadow: 0 0 8px rgba(251, 191, 36, 0.3);
}

.archive-item.active {
  background: rgba(251, 191, 36, 0.15);
  color: #fbbf24;
  font-weight: 600;
  border-left: 3px solid #fbbf24;
}

.archive-item.active .archive-icon-wrap {
  background: #fbbf24;
  color: #0d1117;
  border-color: #fbbf24;
}

.archive-item.active .menu-count {
  background: rgba(251, 191, 36, 0.25);
  color: #fde68a;
}

.archive-count {
  background: rgba(251, 191, 36, 0.15);
  color: #fde68a;
}

.trash-item:hover .trash-icon-wrap {
  background: rgba(248, 81, 73, 0.22);
  box-shadow: 0 0 8px rgba(248, 81, 73, 0.3);
}

.trash-item.active {
  background: rgba(248, 81, 73, 0.15);
  color: #f85149;
  font-weight: 600;
  border-left: 3px solid #f85149;
}

.trash-item.active .trash-icon-wrap {
  background: #f85149;
  color: #0d1117;
  border-color: #f85149;
}

.trash-item.active .menu-count {
  background: rgba(248, 81, 73, 0.25);
  color: #ff7b72;
}

.trash-count {
  background: rgba(248, 81, 73, 0.15);
  color: #ff7b72;
}

.settings-nav-section {
  margin-top: auto;
  padding-top: 10px;
  border-top: 1px solid rgba(48, 54, 61, 0.6);
}

.settings-item.active {
  background: rgba(88, 166, 255, 0.15);
  color: #58a6ff;
  font-weight: 600;
  border-left: 3px solid #58a6ff;
}
</style>
