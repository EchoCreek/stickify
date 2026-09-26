<!--
  Copyright (c) Sticky Notes Refactoring Team (2026)
  Part of Sticky Notes Refactoring Project
  Licensed under the Apache License, Version 2.0
-->
<template>
  <transition name="batch-float">
    <div
      v-if="selectedCount > 0"
      class="batch-toolbar select-none"
      :class="{ 'drawer-active': drawerOpen }"
    >
      <!-- Selection Indicator -->
      <div class="selection-pill shrink-0">
        <div class="pulse-dot"></div>
        <span class="selection-text">{{ t('batch.selectedCount', { n: selectedCount }) }}</span>
      </div>

      <div class="divider shrink-0"></div>

      <!-- Action Button Groups -->
      <div class="actions-group">
        <!-- Trash Mode Batch Actions -->
        <template v-if="isTrash">
          <button
            type="button"
            @click="$emit('restore-selected')"
            class="action-item"
            :title="t('batch.restoreAll')"
          >
            <i class="ri-restart-line text-blue-400"></i>
            <span>{{ t('batch.restoreAll') }}</span>
          </button>

          <button
            type="button"
            @click="$emit('permanent-delete-selected')"
            class="action-item danger-item"
            :title="t('batch.permanentDeleteAll')"
          >
            <i class="ri-delete-bin-2-line"></i>
            <span>{{ t('batch.permanentDeleteAll') }}</span>
          </button>
        </template>

        <!-- Archive Mode Batch Actions -->
        <template v-else-if="isArchive">
          <button
            type="button"
            @click="$emit('unarchive-selected')"
            class="action-item"
            :title="t('batch.unarchiveAll')"
          >
            <i class="ri-inbox-unarchive-line text-amber-400"></i>
            <span>{{ t('batch.unarchiveAll') }}</span>
          </button>

          <!-- Export Controls in Archive Mode -->
          <button
            type="button"
            @click="$emit('export-md')"
            class="action-item"
            title="Markdown (.md)"
          >
            <i class="ri-markdown-line text-blue-400"></i>
            <span>MD</span>
          </button>

          <button
            type="button"
            @click="$emit('export-json')"
            class="action-item"
            title="JSON (.json)"
          >
            <i class="ri-code-s-slash-line text-purple-400"></i>
            <span>JSON</span>
          </button>

          <div class="divider shrink-0"></div>

          <button
            type="button"
            @click="$emit('delete-selected')"
            class="action-item danger-item"
            :title="t('batch.deleteAll')"
          >
            <i class="ri-delete-bin-line"></i>
            <span>{{ t('batch.deleteAll') }}</span>
          </button>
        </template>

        <!-- Normal Active Mode Batch Actions -->
        <template v-else>
          <!-- Visibility Controls -->
          <button
            type="button"
            @click="$emit('show-all')"
            class="action-item"
            :title="t('batch.showAll')"
          >
            <i class="ri-eye-line text-emerald-400"></i>
            <span>{{ t('batch.showAll') }}</span>
          </button>

          <button
            type="button"
            @click="$emit('hide-all')"
            class="action-item"
            :title="t('batch.hideAll')"
          >
            <i class="ri-eye-off-line text-gray-400"></i>
            <span>{{ t('batch.hideAll') }}</span>
          </button>

          <div class="divider shrink-0"></div>

          <!-- Export Controls -->
          <button
            type="button"
            @click="$emit('export-md')"
            class="action-item"
            title="Markdown (.md)"
          >
            <i class="ri-markdown-line text-blue-400"></i>
            <span>MD</span>
          </button>

          <button
            type="button"
            @click="$emit('export-json')"
            class="action-item"
            title="JSON (.json)"
          >
            <i class="ri-code-s-slash-line text-purple-400"></i>
            <span>JSON</span>
          </button>

          <div class="divider shrink-0"></div>

          <!-- Archive Control -->
          <button
            type="button"
            @click="$emit('archive-selected')"
            class="action-item"
            :title="t('batch.archiveAll')"
          >
            <i class="ri-inbox-archive-line text-amber-400"></i>
            <span>{{ t('batch.archiveAll') }}</span>
          </button>

          <!-- Danger Control: Delete (Soft Delete) -->
          <button
            type="button"
            @click="$emit('delete-selected')"
            class="action-item danger-item"
            :title="t('batch.deleteAll')"
          >
            <i class="ri-delete-bin-line"></i>
            <span>{{ t('batch.deleteAll') }}</span>
          </button>
        </template>

        <!-- Clear Selection Button -->
        <button
          type="button"
          @click="$emit('clear-selection')"
          class="close-item shrink-0"
          :title="t('batch.cancelSelection')"
        >
          <i class="ri-close-line"></i>
        </button>
      </div>
    </div>
  </transition>
</template>

<script setup lang="ts">
import { useI18n } from '../../locales';

defineProps<{
  selectedCount: number;
  drawerOpen?: boolean;
  isTrash?: boolean;
  isArchive?: boolean;
}>();

defineEmits<{
  (e: 'show-all'): void;
  (e: 'hide-all'): void;
  (e: 'export-md'): void;
  (e: 'export-json'): void;
  (e: 'archive-selected'): void;
  (e: 'unarchive-selected'): void;
  (e: 'delete-selected'): void;
  (e: 'restore-selected'): void;
  (e: 'permanent-delete-selected'): void;
  (e: 'clear-selection'): void;
}>();

const { t } = useI18n();
</script>

<style scoped>
.batch-toolbar {
  position: absolute;
  bottom: 24px;
  left: 50%;
  transform: translateX(-50%);
  max-width: calc(100% - 32px);
  background: rgba(18, 24, 38, 0.94);
  backdrop-filter: blur(16px);
  -webkit-backdrop-filter: blur(16px);
  border: 1px solid rgba(96, 165, 250, 0.3);
  box-shadow: 0 16px 36px -6px rgba(0, 0, 0, 0.65), 0 0 0 1px rgba(255, 255, 255, 0.06);
  border-radius: 10px;
  padding: 4px 8px 4px 12px;
  display: flex;
  align-items: center;
  gap: 8px;
  z-index: 60;
  white-space: nowrap;
  transition: left 0.2s cubic-bezier(0.16, 1, 0.3, 1), max-width 0.2s ease;
  overflow-x: auto;
  scrollbar-width: none;
}

.batch-toolbar::-webkit-scrollbar {
  display: none;
}

/* When detail drawer is opened (360px on right), automatically center in the visible viewport */
.batch-toolbar.drawer-active {
  left: calc((100% - 360px) / 2);
  max-width: calc(100% - 390px);
}

.selection-pill {
  display: flex;
  align-items: center;
  gap: 4px;
  padding-right: 2px;
}

.pulse-dot {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  background: #60a5fa;
  box-shadow: 0 0 8px #60a5fa;
}

.selection-text {
  font-size: 11px;
  color: #8b949e;
  font-weight: 500;
}

.selection-count {
  font-size: 13px;
  font-weight: 700;
  color: #60a5fa;
  line-height: 1;
}

.selection-unit {
  font-size: 11px;
  color: #8b949e;
}

.divider {
  width: 1px;
  height: 14px;
  background: rgba(255, 255, 255, 0.12);
}

.actions-group {
  display: flex;
  align-items: center;
  gap: 4px;
}

.action-item {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 4px 8px;
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.08);
  border-radius: 6px;
  color: #f0f6fc;
  font-size: 11px;
  font-weight: 500;
  cursor: pointer;
  transition: all 0.15s cubic-bezier(0.16, 1, 0.3, 1);
}

.action-item:hover {
  background: rgba(255, 255, 255, 0.14);
  border-color: rgba(96, 165, 250, 0.4);
  transform: translateY(-1px);
}

.action-item:active {
  transform: translateY(0);
  background: rgba(255, 255, 255, 0.08);
}

.danger-item {
  color: #f85149;
  background: rgba(248, 81, 73, 0.1);
  border-color: rgba(248, 81, 73, 0.25);
}

.danger-item:hover {
  background: rgba(248, 81, 73, 0.25);
  border-color: #f85149;
  color: #ff7b72;
}

.close-item {
  width: 24px;
  height: 24px;
  border-radius: 6px;
  background: transparent;
  border: 1px solid transparent;
  color: #8b949e;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 13px;
  cursor: pointer;
  transition: all 0.15s ease;
  margin-left: 2px;
}

.close-item:hover {
  background: rgba(255, 255, 255, 0.1);
  border-color: rgba(255, 255, 255, 0.15);
  color: #f0f6fc;
}

/* Smooth Slide-up & Zoom Transition */
.batch-float-enter-active,
.batch-float-leave-active {
  transition: all 0.22s cubic-bezier(0.16, 1, 0.3, 1);
}

.batch-float-enter-from,
.batch-float-leave-to {
  opacity: 0;
  transform: translate(-50%, 18px) scale(0.96);
}
</style>
