<!--
  Copyright (c) Sticky Notes Refactoring Team (2026)
  Part of Sticky Notes Refactoring Project
  Licensed under the Apache License, Version 2.0
-->
<template>
  <div
    class="note-card"
    :class="{
      'is-active': selected,
      'is-light': isLight
    }"
    :style="{ backgroundColor: note.bgColor || '#161b22', borderLeftColor: isLight ? '#0969da' : '#60a5fa' }"
    @click="handleCardClick"
  >
    <!-- Header -->
    <div class="card-header">
      <div class="header-main" @click.stop>
        <input
          type="checkbox"
          :checked="selected"
          @change="$emit('toggle-select', note.name)"
          class="card-checkbox"
        />
        <h3
          class="card-title truncate"
          v-html="highlightKeyword(note.title || t('titleBar.newNote'), searchKeyword)"
        ></h3>
      </div>

      <!-- Action buttons on hover -->
      <div class="card-actions" @click.stop>
        <!-- Trash Mode Actions -->
        <template v-if="note.isDeleted">
          <button
            type="button"
            class="action-btn restore-btn"
            :title="t('noteCard.restore')"
            @click="$emit('restore', note.name)"
          >
            <i class="ri-restart-line"></i>
          </button>
          <button
            type="button"
            class="action-btn delete-btn"
            :title="t('noteCard.permanentDelete')"
            @click="$emit('permanent-delete', note.name)"
          >
            <i class="ri-delete-bin-2-line"></i>
          </button>
        </template>

        <!-- Archived Mode Actions -->
        <template v-else-if="note.isArchived">
          <button
            type="button"
            class="action-btn restore-btn"
            :title="t('noteCard.unarchive')"
            @click="$emit('unarchive', note.name)"
          >
            <i class="ri-inbox-unarchive-line"></i>
          </button>
          <button
            type="button"
            class="action-btn delete-btn"
            :title="t('noteCard.moveToTrash')"
            @click="$emit('delete', note.name)"
          >
            <i class="ri-delete-bin-line"></i>
          </button>
        </template>

        <!-- Normal Active Mode Actions -->
        <template v-else>
          <button
            type="button"
            class="action-btn locate-btn"
            :title="t('noteCard.locate')"
            @click="$emit('locate', note.name)"
          >
            <i class="ri-focus-3-line"></i>
          </button>
          <button
            type="button"
            class="action-btn"
            :title="note.topMost ? t('noteCard.toggleTopmostOff') : t('noteCard.toggleTopmostOn')"
            :class="{ active: note.topMost }"
            @click="$emit('toggle-topmost', note)"
          >
            <i :class="note.topMost ? 'ri-pushpin-2-fill' : 'ri-pushpin-2-line'"></i>
          </button>
          <button
            type="button"
            class="action-btn"
            :title="note.visible ? t('noteCard.toggleVisibleHide') : t('noteCard.toggleVisibleShow')"
            @click="$emit('toggle-visible', note)"
          >
            <i :class="note.visible ? 'ri-eye-line' : 'ri-eye-off-line'"></i>
          </button>
          <button
            type="button"
            class="action-btn"
            :title="t('noteCard.archive')"
            @click="$emit('archive', note.name)"
          >
            <i class="ri-inbox-archive-line"></i>
          </button>
          <button
            type="button"
            class="action-btn delete-btn"
            :title="t('noteCard.moveToTrash')"
            @click="$emit('delete', note.name)"
          >
            <i class="ri-delete-bin-line"></i>
          </button>
        </template>
      </div>
    </div>

    <!-- Preview Items List -->
    <div class="card-body">
      <div
        v-for="item in note.previewItems"
        :key="item.id"
        class="preview-item"
      >
        <i
          :class="item.finished ? 'ri-checkbox-circle-fill text-emerald-500' : 'ri-checkbox-blank-circle-line opacity-40'"
          class="item-icon"
        ></i>
        <span
          class="item-text truncate"
          :class="{ finished: item.finished }"
          v-html="highlightKeyword(item.content, searchKeyword)"
        ></span>
      </div>
      <div v-if="!note.previewItems || note.previewItems.length === 0" class="empty-preview">
        {{ t('noteCard.noItems') }}
      </div>
    </div>

    <!-- Footer -->
    <div class="card-footer">
      <div class="footer-meta">
        <template v-if="note.isDeleted">
          <span class="status-tag deleted">
            <i class="ri-delete-bin-line text-[11px]"></i>
            {{ formatDeletedTime(note.deletedAt) }}
          </span>
        </template>
        <template v-else-if="note.isArchived">
          <span class="status-tag archived">
            <i class="ri-inbox-archive-line text-[11px]"></i>
            {{ formatArchivedTime(note.archivedAt) }}
          </span>
        </template>
        <template v-else>
          <span
            class="status-tag"
            :class="note.visible ? 'visible' : 'hidden'"
          >
            <i :class="note.visible ? 'ri-computer-line' : 'ri-eye-off-line'" class="text-[11px]"></i>
            {{ note.visible ? t('noteCard.visible') : t('noteCard.hidden') }}
          </span>

          <span v-if="note.topMost" class="topmost-tag">
            <i class="ri-pushpin-2-fill text-[10px]"></i>
            {{ t('noteCard.topmost') }}
          </span>
        </template>
      </div>

      <!-- Progress Stats -->
      <div class="footer-progress">
        <div class="progress-bar-wrap">
          <div class="progress-bar-fill" :style="{ width: `${progressPercent}%` }"></div>
        </div>
        <span class="progress-text">{{ note.completedCount }}/{{ note.itemCount }}</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import type { NoteSummary } from '../../api/managerApi';
import { formatDeletedTime, formatArchivedTime, isLightColor, highlightKeyword } from '../../utils';
import { useI18n } from '../../locales';

const props = defineProps<{
  note: NoteSummary;
  selected: boolean;
  searchKeyword?: string;
}>();

const { t } = useI18n();

const emit = defineEmits<{
  (e: 'select', note: NoteSummary): void;
  (e: 'toggle-select', name: string): void;
  (e: 'locate', name: string): void;
  (e: 'toggle-visible', note: NoteSummary): void;
  (e: 'toggle-topmost', note: NoteSummary): void;
  (e: 'archive', name: string): void;
  (e: 'unarchive', name: string): void;
  (e: 'delete', name: string): void;
  (e: 'restore', name: string): void;
  (e: 'permanent-delete', name: string): void;
}>();

function handleCardClick() {
  if (props.note.isDeleted || props.note.isArchived) {
    emit('toggle-select', props.note.name);
  } else {
    emit('select', props.note);
  }
}

const isLight = computed(() => isLightColor(props.note.bgColor));

const progressPercent = computed(() => {
  if (!props.note.itemCount || props.note.itemCount === 0) return 0;
  return Math.round((props.note.completedCount / props.note.itemCount) * 100);
});
</script>

<style scoped>
.note-card {
  position: relative;
  padding: 12px 14px;
  background: #161b22;
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.2s ease;
  border: 1px solid #30363d;
  border-left: 3px solid #60a5fa;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  min-height: 140px;
  box-sizing: border-box;
  content-visibility: auto;
  contain-intrinsic-size: 280px 180px;
}

.note-card:hover {
  border-color: #8b949e;
  transform: translateY(-1px);
  box-shadow: 0 4px 14px rgba(0, 0, 0, 0.2);
}

.note-card.is-active {
  border-color: #60a5fa !important;
  box-shadow: 0 0 0 1px #60a5fa, 0 4px 16px rgba(0, 0, 0, 0.25);
}

.note-card.is-light {
  color: #1f2328;
}

.card-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8px;
  margin-bottom: 8px;
}

.header-main {
  display: flex;
  align-items: center;
  gap: 8px;
  flex: 1;
  min-width: 0;
}

.card-checkbox {
  width: 14px;
  height: 14px;
  cursor: pointer;
  accent-color: #60a5fa;
  flex-shrink: 0;
}

.card-title {
  font-size: 13px;
  font-weight: 600;
  color: #f0f6fc;
  margin: 0;
  line-height: 1.3;
}

.is-light .card-title {
  color: #1f2328;
}

.card-actions {
  display: flex;
  align-items: center;
  gap: 3px;
  opacity: 0.6;
  transition: opacity 0.15s ease;
}

.note-card:hover .card-actions {
  opacity: 1;
}

.action-btn {
  background: rgba(255, 255, 255, 0.06);
  border: 1px solid rgba(255, 255, 255, 0.1);
  color: #8b949e;
  border-radius: 4px;
  width: 22px;
  height: 22px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  font-size: 12px;
  cursor: pointer;
  transition: all 0.15s ease;
}

.action-btn:hover {
  background: rgba(255, 255, 255, 0.15);
  color: #f0f6fc;
}

.action-btn.active {
  background: rgba(96, 165, 250, 0.2);
  border-color: #60a5fa;
  color: #60a5fa;
}

.restore-btn:hover {
  background: rgba(96, 165, 250, 0.25) !important;
  color: #60a5fa !important;
  border-color: #60a5fa !important;
}

.delete-btn:hover {
  background: rgba(248, 81, 73, 0.2) !important;
  color: #f85149 !important;
  border-color: #f85149 !important;
}

.card-body {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 4px;
  margin-bottom: 10px;
}

.preview-item {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 11px;
  color: #c9d1d9;
}

.is-light .preview-item {
  color: #484f58;
}

.item-icon {
  font-size: 12px;
  flex-shrink: 0;
}

.item-text {
  flex: 1;
}

.item-text.finished {
  text-decoration: line-through;
  opacity: 0.5;
}

.empty-preview {
  font-size: 11px;
  color: #6e7681;
  font-style: italic;
  padding: 4px 0;
}

.card-footer {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8px;
  padding-top: 8px;
  border-top: 1px solid rgba(255, 255, 255, 0.08);
}

.is-light .card-footer {
  border-top-color: rgba(0, 0, 0, 0.08);
}

.footer-meta {
  display: flex;
  align-items: center;
  gap: 6px;
}

.status-tag {
  display: inline-flex;
  align-items: center;
  gap: 4px;
  font-size: 10px;
  padding: 1px 6px;
  border-radius: 4px;
  border: 1px solid transparent;
}

.status-tag.visible {
  background: rgba(63, 185, 80, 0.15);
  color: #3fb950;
  border-color: rgba(63, 185, 80, 0.3);
}

.status-tag.hidden {
  background: rgba(110, 118, 129, 0.15);
  color: #8b949e;
  border-color: rgba(110, 118, 129, 0.3);
}

.status-tag.deleted {
  background: rgba(248, 81, 73, 0.15);
  color: #ff7b72;
  border-color: rgba(248, 81, 73, 0.3);
}

.status-tag.archived {
  background: rgba(251, 191, 36, 0.15);
  color: #fbbf24;
  border-color: rgba(251, 191, 36, 0.3);
}

.topmost-tag {
  display: inline-flex;
  align-items: center;
  gap: 3px;
  font-size: 10px;
  padding: 1px 5px;
  border-radius: 4px;
  background: rgba(210, 153, 34, 0.15);
  color: #d29922;
  border: 1px solid rgba(210, 153, 34, 0.3);
}

.footer-progress {
  display: flex;
  align-items: center;
  gap: 6px;
}

.progress-bar-wrap {
  width: 50px;
  height: 4px;
  background: rgba(255, 255, 255, 0.1);
  border-radius: 999px;
  overflow: hidden;
}

.progress-bar-fill {
  height: 100%;
  background: #3fb950;
  border-radius: 999px;
  transition: width 0.25s ease;
}

.progress-text {
  font-size: 10px;
  color: #8b949e;
}

:deep(.highlight-match) {
  background: rgba(245, 158, 11, 0.38) !important;
  color: #fef08a !important;
  font-weight: 600 !important;
  padding: 0 2px !important;
  border-radius: 2px !important;
  box-shadow: inset 0 0 0 1px rgba(245, 158, 11, 0.6) !important;
  border: none !important;
  display: inline !important;
  box-decoration-break: clone !important;
  -webkit-box-decoration-break: clone !important;
}

:deep(.is-light) .highlight-match {
  background: rgba(245, 158, 11, 0.3) !important;
  color: #b45309 !important;
  font-weight: 700 !important;
  box-shadow: inset 0 0 0 1px rgba(217, 119, 6, 0.6) !important;
  border: none !important;
}
</style>
