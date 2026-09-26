<!--
  Copyright (c) Sticky Notes Refactoring Team (2026)
  Part of Sticky Notes Refactoring Project
  Licensed under the Apache License, Version 2.0
-->
<template>
  <div
    v-if="visible && localNote"
    class="detail-drawer select-none"
  >
    <!-- Header Card -->
    <div class="drawer-header">
      <div class="header-info">
        <div class="drawer-logo-icon">
          <i class="ri-file-settings-line"></i>
        </div>
        <h2 class="drawer-title">{{ t('detail.title') }}</h2>
      </div>
      <button
        type="button"
        class="close-btn"
        @click="$emit('close')"
        :title="t('detail.closeDrawer')"
      >
        <i class="ri-close-line text-base"></i>
      </button>
    </div>

    <!-- Drawer Body -->
    <div class="drawer-body">
      <!-- Title Input Section -->
      <div class="setting-section">
        <label class="section-label">{{ t('noteTable.colTitle') }}</label>
        <input
          type="text"
          v-model="localNote.title"
          @change="saveChanges"
          :placeholder="t('detail.titlePlaceholder')"
          class="title-input"
        />
      </div>

      <!-- Theme & Window Appearance -->
      <div class="setting-card">
        <div class="section-label">{{ t('detail.colorPalette') }}</div>
        <div class="color-grid">
          <button
            v-for="c in presetColors"
            :key="c.hex"
            @click="setColor(c.hex)"
            class="color-btn"
            :class="{ active: isColorMatch(localNote.bgColor, c.hex) }"
            :style="{ backgroundColor: c.hex, borderColor: c.border }"
            :title="getColorName(c.hex)"
          >
            <i
              v-if="isColorMatch(localNote.bgColor, c.hex)"
              class="ri-check-line check-icon"
              :class="isLightColor(c.hex) ? 'text-gray-800' : 'text-white'"
            ></i>
          </button>
        </div>

        <div class="switches-grid">
          <label class="switch-item">
            <div class="flex items-center gap-1.5">
              <i class="ri-pushpin-2-line text-xs"></i>
              <span>{{ t('detail.topmostState') }}</span>
            </div>
            <input
              type="checkbox"
              v-model="localNote.topMost"
              @change="saveChanges"
              class="checkbox"
            />
          </label>

          <label class="switch-item">
            <div class="flex items-center gap-1.5">
              <i class="ri-eye-line text-xs"></i>
              <span>{{ t('detail.desktopVisibility') }}</span>
            </div>
            <input
              type="checkbox"
              v-model="localNote.visible"
              @change="saveChanges"
              class="checkbox"
            />
          </label>
        </div>

        <div class="opacity-section">
          <div class="opacity-header">
            <label class="opacity-label">
              <input
                type="checkbox"
                v-model="localNote.opacityEnabled"
                @change="saveChanges"
                class="checkbox"
              />
              <span class="flex items-center gap-1">
                <i class="ri-contrast-2-line text-xs"></i>
                {{ t('detail.opacityState') }}
              </span>
            </label>
            <span class="opacity-val">{{ localNote.opacity }}%</span>
          </div>
          <input
            type="range"
            min="20"
            max="100"
            v-model.number="localNote.opacity"
            @input="saveChanges"
            :disabled="!localNote.opacityEnabled"
            class="range-slider"
          />
        </div>
      </div>

      <!-- Checklist Section -->
      <div class="tasks-section">
        <div class="tasks-header">
          <span class="section-label">{{ t('detail.todoProgress') }} ({{ localNote.items?.length || 0 }})</span>
          <span class="tasks-rate">{{ t('detail.itemsCount', { completed: completedCount, total: localNote.items?.length || 0 }) }}</span>
        </div>

        <!-- Quick Add Box -->
        <div class="quick-add">
          <input
            type="text"
            v-model="newItemText"
            @keyup.enter="addItem"
            :placeholder="t('detail.addItemPlaceholder')"
            class="quick-input"
          />
          <button
            @click="addItem"
            class="add-btn"
          >
            <i class="ri-add-line mr-0.5"></i>{{ t('detail.addItemBtn') }}
          </button>
        </div>

        <!-- Items list -->
        <div class="items-list">
          <div
            v-for="(item, idx) in localNote.items"
            :key="item.id"
            class="task-row"
          >
            <input
              type="checkbox"
              v-model="item.finished"
              @change="saveChanges"
              class="task-checkbox"
            />
            <input
              type="text"
              v-model="item.content"
              @change="saveChanges"
              class="task-text"
              :class="{ finished: item.finished }"
            />
            <button
              @click="removeItem(idx)"
              class="task-del-btn"
              :title="t('common.delete')"
            >
              <i class="ri-delete-bin-line"></i>
            </button>
          </div>

          <div
            v-if="!localNote.items || localNote.items.length === 0"
            class="empty-tasks"
          >
            {{ t('detail.noItemsHint') }}
          </div>
        </div>
      </div>
    </div>

    <!-- Footer Action Bar -->
    <div class="drawer-footer">
      <button
        @click="$emit('locate', localNote.name)"
        class="locate-btn"
      >
        <i class="ri-focus-3-line mr-1 text-xs"></i>
        <span>{{ t('detail.locateDesktop') }}</span>
      </button>
      <button
        @click="$emit('export', localNote.name)"
        class="export-btn"
        :title="t('detail.exportNote')"
      >
        <i class="ri-download-2-line mr-1 text-xs"></i>
        <span>{{ t('common.download') }}</span>
      </button>
      <button
        @click="$emit('delete', localNote.name)"
        class="delete-btn"
        :title="t('detail.deleteNote')"
      >
        <i class="ri-delete-bin-line"></i>
      </button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, computed } from 'vue';
import type { NoteDetail, NoteItemDto } from '../../api/managerApi';
import { PRESET_COLORS, isColorMatch, isLightColor, getColorName } from '../../utils';
import { useI18n } from '../../locales';

const props = defineProps<{
  visible: boolean;
  note: NoteDetail | null;
}>();

const emit = defineEmits<{
  (e: 'close'): void;
  (e: 'update', note: NoteDetail): void;
  (e: 'locate', name: string): void;
  (e: 'delete', name: string): void;
  (e: 'export', name: string): void;
}>();

const { t } = useI18n();

const localNote = ref<NoteDetail | null>(null);
const newItemText = ref('');

const presetColors = PRESET_COLORS;

watch(
  () => props.note,
  (val) => {
    if (val) {
      localNote.value = JSON.parse(JSON.stringify(val));
    } else {
      localNote.value = null;
    }
  },
  { immediate: true }
);

const completedCount = computed(() => {
  if (!localNote.value?.items) return 0;
  return localNote.value.items.filter((it: NoteItemDto) => it.finished).length;
});

function setColor(c: string) {
  if (localNote.value) {
    localNote.value.bgColor = c;
    saveChanges();
  }
}

function addItem() {
  if (!newItemText.value.trim() || !localNote.value) return;
  if (!localNote.value.items) localNote.value.items = [];
  localNote.value.items.push({
    id: Date.now(),
    content: newItemText.value.trim(),
    finished: false,
  });
  newItemText.value = '';
  saveChanges();
}

function removeItem(idx: number) {
  if (!localNote.value?.items) return;
  localNote.value.items.splice(idx, 1);
  saveChanges();
}

function saveChanges() {
  if (localNote.value) {
    emit('update', JSON.parse(JSON.stringify(localNote.value)));
  }
}
</script>

<style scoped>
.detail-drawer {
  position: absolute;
  top: 0;
  right: 0;
  bottom: 0;
  width: 360px;
  background: #182030;
  border-left: 1px solid #30363d;
  box-shadow: -6px 0 24px rgba(0, 0, 0, 0.45);
  z-index: 50;
  display: flex;
  flex-direction: column;
  animation: drawerSlideIn 0.22s cubic-bezier(0.16, 1, 0.3, 1);
  box-sizing: border-box;
}

@keyframes drawerSlideIn {
  0% {
    transform: translateX(100%);
    opacity: 0.4;
  }
  100% {
    transform: translateX(0);
    opacity: 1;
  }
}

.drawer-header {
  height: 42px;
  padding: 0 14px;
  border-bottom: 1px solid #30363d;
  display: flex;
  align-items: center;
  justify-content: space-between;
  background: #1c2333;
  flex-shrink: 0;
}

.header-info {
  display: flex;
  align-items: center;
  gap: 8px;
}

.drawer-logo-icon {
  width: 22px;
  height: 22px;
  border-radius: 6px;
  background: rgba(96, 165, 250, 0.15);
  border: 1px solid rgba(96, 165, 250, 0.3);
  display: flex;
  align-items: center;
  justify-content: center;
  color: #60a5fa;
  font-size: 13px;
  flex-shrink: 0;
}

.drawer-title {
  font-size: 12px;
  font-weight: 600;
  color: #f0f6fc;
  margin: 0;
}

.close-btn {
  width: 24px;
  height: 24px;
  border-radius: 4px;
  background: transparent;
  border: none;
  color: #8b949e;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  transition: all 0.15s ease;
}

.close-btn:hover {
  background: #252d3d;
  color: #f0f6fc;
}

.drawer-body {
  flex: 1;
  overflow-y: auto;
  padding: 14px;
  display: flex;
  flex-direction: column;
  gap: 14px;
}

.section-label {
  font-size: 10px;
  font-weight: 600;
  color: #8b949e;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  margin-bottom: 6px;
}

.title-input {
  width: 100%;
  padding: 6px 10px;
  background: #161b22;
  border: 1px solid #30363d;
  border-radius: 6px;
  font-size: 12px;
  font-weight: 600;
  color: #f0f6fc;
  outline: none;
  transition: border-color 0.2s;
  box-sizing: border-box;
}

.title-input:focus {
  border-color: #60a5fa;
  background: #1c2333;
}

.setting-card {
  background: #161b22;
  border: 1px solid #30363d;
  border-radius: 8px;
  padding: 12px;
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.color-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 6px;
}

.color-btn {
  width: 100%;
  height: 26px;
  border-radius: 5px;
  border: 1px solid #30363d;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: transform 0.15s;
}

.color-btn:hover {
  transform: scale(1.05);
}

.color-btn.active {
  box-shadow: 0 0 0 2px #60a5fa;
}

.check-icon {
  font-size: 11px;
}

.switches-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 8px;
}

.switch-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 6px 8px;
  background: #1c2333;
  border: 1px solid #30363d;
  border-radius: 6px;
  font-size: 11px;
  color: #c9d1d9;
  cursor: pointer;
}

.checkbox {
  width: 13px;
  height: 13px;
  accent-color: #60a5fa;
  cursor: pointer;
}

.opacity-section {
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding-top: 4px;
}

.opacity-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 11px;
  color: #c9d1d9;
}

.opacity-label {
  display: flex;
  align-items: center;
  gap: 6px;
  cursor: pointer;
}

.opacity-val {
  font-size: 10px;
  color: #8b949e;
  font-family: monospace;
}

.range-slider {
  -webkit-appearance: none;
  appearance: none;
  width: 100%;
  height: 5px;
  background: rgba(255, 255, 255, 0.12);
  border-radius: 9999px;
  outline: none;
  cursor: pointer;
  transition: background 0.2s ease;
  margin: 4px 0 2px 0;
}

.range-slider:hover:not(:disabled) {
  background: rgba(255, 255, 255, 0.2);
}

.range-slider::-webkit-slider-runnable-track {
  width: 100%;
  height: 5px;
  background: transparent;
  border-radius: 9999px;
}

.range-slider::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 14px;
  height: 14px;
  border-radius: 50%;
  background: #3b82f6;
  border: 2px solid #161b22;
  box-shadow: 0 0 6px rgba(59, 130, 246, 0.6), 0 2px 4px rgba(0, 0, 0, 0.4);
  cursor: pointer;
  margin-top: -4.5px;
  transition: transform 0.15s ease, background-color 0.15s ease, box-shadow 0.15s ease;
}

.range-slider:hover:not(:disabled)::-webkit-slider-thumb {
  background: #60a5fa;
  transform: scale(1.18);
  box-shadow: 0 0 10px rgba(96, 165, 250, 0.8), 0 2px 6px rgba(0, 0, 0, 0.5);
}

.range-slider:active:not(:disabled)::-webkit-slider-thumb {
  background: #2563eb;
  transform: scale(1.25);
}

.range-slider:disabled {
  opacity: 0.35;
  cursor: not-allowed;
}

.range-slider:disabled::-webkit-slider-thumb {
  cursor: not-allowed;
  background: #64748b;
  box-shadow: none;
}

.tasks-section {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.tasks-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.tasks-rate {
  font-size: 10px;
  color: #3fb950;
  font-weight: 500;
}

.quick-add {
  display: flex;
  align-items: center;
  gap: 6px;
  background: #161b22;
  border: 1px solid #30363d;
  border-radius: 6px;
  padding: 3px 4px;
}

.quick-input {
  flex: 1;
  background: transparent;
  border: none;
  outline: none;
  font-size: 11px;
  color: #f0f6fc;
  padding: 3px 6px;
}

.quick-input::placeholder {
  color: #6e7681;
}

.add-btn {
  background: #238636;
  border: none;
  color: #ffffff;
  font-size: 11px;
  font-weight: 600;
  padding: 3px 8px;
  border-radius: 4px;
  cursor: pointer;
  display: flex;
  align-items: center;
}

.add-btn:hover {
  background: #2ea043;
}

.items-list {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.task-row {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 6px 8px;
  background: #161b22;
  border: 1px solid #30363d;
  border-radius: 6px;
  transition: background 0.15s;
}

.task-row:hover {
  background: #1c2333;
}

.task-checkbox {
  width: 13px;
  height: 13px;
  accent-color: #3fb950;
  cursor: pointer;
}

.task-text {
  flex: 1;
  background: transparent;
  border: none;
  outline: none;
  font-size: 11px;
  color: #f0f6fc;
}

.task-text.finished {
  text-decoration: line-through;
  opacity: 0.5;
}

.task-del-btn {
  background: transparent;
  border: none;
  color: #6e7681;
  cursor: pointer;
  font-size: 12px;
  padding: 2px 4px;
  opacity: 0;
  transition: opacity 0.15s;
}

.task-row:hover .task-del-btn {
  opacity: 1;
}

.task-del-btn:hover {
  color: #f85149;
}

.empty-tasks {
  font-size: 11px;
  color: #6e7681;
  text-align: center;
  padding: 16px;
  border: 1px dashed #30363d;
  border-radius: 6px;
}

.drawer-footer {
  padding: 10px 14px;
  border-top: 1px solid #30363d;
  background: #1c2333;
  display: flex;
  align-items: center;
  gap: 8px;
  flex-shrink: 0;
}

.locate-btn {
  flex: 1;
  padding: 6px 12px;
  background: #21262d;
  border: 1px solid #30363d;
  border-radius: 6px;
  color: #f0f6fc;
  font-size: 11px;
  font-weight: 600;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  transition: all 0.15s;
}

.locate-btn:hover {
  background: #30363d;
}

.export-btn {
  padding: 6px 10px;
  background: #21262d;
  border: 1px solid #30363d;
  border-radius: 6px;
  color: #f0f6fc;
  font-size: 11px;
  font-weight: 600;
  cursor: pointer;
  display: flex;
  align-items: center;
}

.export-btn:hover {
  background: #30363d;
}

.delete-btn {
  padding: 6px 10px;
  background: rgba(248, 81, 73, 0.1);
  border: 1px solid rgba(248, 81, 73, 0.3);
  border-radius: 6px;
  color: #f85149;
  font-size: 12px;
  cursor: pointer;
}

.delete-btn:hover {
  background: rgba(248, 81, 73, 0.25);
}
</style>
