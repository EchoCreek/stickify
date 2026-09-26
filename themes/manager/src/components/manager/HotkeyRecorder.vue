<!--
  Copyright (c) Sticky Notes Refactoring Team (2026)
  Part of Sticky Notes Refactoring Project
  Licensed under the Apache License, Version 2.0
-->
<template>
  <div class="hotkey-recorder-container">
    <div
      class="hotkey-box"
      :class="{ recording: isRecording }"
      tabindex="0"
      @click="startRecording"
      @keydown.prevent.stop="handleKeyDown"
      @blur="stopRecording"
    >
      <div v-if="isRecording" class="recording-prompt">
        <i class="ri-record-circle-line animate-pulse text-blue-400 mr-1.5"></i>
        <span>{{ recordingPreview || t('settings.recorderRecording') }}</span>
      </div>

      <div v-else class="hotkey-display">
        <template v-if="keyChips.length > 0">
          <span
            v-for="(k, idx) in keyChips"
            :key="idx"
            class="key-chip"
          >
            {{ k }}
          </span>
        </template>
        <span v-else class="text-gray-500 text-xs">{{ t('common.none') }}</span>
      </div>

      <div class="hotkey-actions">
        <button
          v-if="!isRecording"
          type="button"
          class="action-btn record-btn"
          :title="t('settings.recorderPlaceholder')"
          @click.stop="startRecording"
        >
          <i class="ri-edit-line"></i>
        </button>
        <button
          v-if="modelValue !== 0 && !isRecording"
          type="button"
          class="action-btn clear-btn"
          :title="t('settings.recorderClear')"
          @click.stop="clearHotkey"
        >
          <i class="ri-close-line"></i>
        </button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue';
import { useI18n } from '../../locales';

const props = defineProps<{
  modelValue: number;
  text?: string;
}>();

const emit = defineEmits<{
  (e: 'update:modelValue', val: number): void;
  (e: 'update:text', val: string): void;
}>();

const { t } = useI18n();

const isRecording = ref(false);
const recordingPreview = ref('');

const keyChips = computed(() => {
  if (props.text && props.text.trim()) {
    return props.text.split('+').map((s) => s.trim()).filter(Boolean);
  }
  if (!props.modelValue) return [];
  // Parse DWORD to chips
  const val = props.modelValue;
  const mod = (val >> 8) & 0xff;
  const vk = val & 0xff;
  const chips: string[] = [];
  if (mod & 0x08) chips.push('Win');
  if (mod & 0x02) chips.push('Ctrl');
  if (mod & 0x01) chips.push('Shift');
  if (mod & 0x04) chips.push('Alt');

  let keyName = '';
  if (vk >= 0x70 && vk <= 0x7b) {
    keyName = `F${vk - 0x70 + 1}`;
  } else if (vk >= 0x41 && vk <= 0x5a) {
    keyName = String.fromCharCode(vk);
  } else if (vk >= 0x30 && vk <= 0x39) {
    keyName = String.fromCharCode(vk);
  } else if (vk === 0x20) keyName = 'Space';
  else if (vk === 0x0d) keyName = 'Enter';
  else if (vk === 0x09) keyName = 'Tab';
  else if (vk === 0x1b) keyName = 'Esc';
  else keyName = `VK_${vk.toString(16).toUpperCase()}`;

  if (keyName) chips.push(keyName);
  return chips;
});

function startRecording() {
  isRecording.value = true;
  recordingPreview.value = '';
}

function stopRecording() {
  isRecording.value = false;
  recordingPreview.value = '';
}

function clearHotkey() {
  emit('update:modelValue', 0);
  emit('update:text', '');
}

function handleKeyDown(e: KeyboardEvent) {
  e.preventDefault();
  e.stopPropagation();
  if (!isRecording.value) return;

  if (e.key === 'Escape') {
    stopRecording();
    return;
  }

  const isCtrl = e.ctrlKey;
  const isAlt = e.altKey;
  const isShift = e.shiftKey;
  const isWin = e.metaKey;

  // If user only pressed modifier keys, show interim preview
  const isOnlyModifier = ['Control', 'Alt', 'Shift', 'Meta'].includes(e.key);
  if (isOnlyModifier) {
    const parts: string[] = [];
    if (isWin) parts.push('Win');
    if (isCtrl) parts.push('Ctrl');
    if (isAlt) parts.push('Alt');
    if (isShift) parts.push('Shift');
    recordingPreview.value = parts.join(' + ') + ' + ...';
    return;
  }

  // Calculate Win32 Hotkey Modifiers
  // HOTKEYF_SHIFT = 0x01, HOTKEYF_CONTROL = 0x02, HOTKEYF_ALT = 0x04, HOTKEYF_EXT = 0x08
  let modFlags = 0;
  if (isShift) modFlags |= 0x01;
  if (isCtrl) modFlags |= 0x02;
  if (isAlt) modFlags |= 0x04;
  if (isWin) modFlags |= 0x08;

  let vkCode = e.keyCode;
  let keyLabel = e.key.toUpperCase();

  // Normalize key labels and VK codes
  if (e.code && e.code.startsWith('Key')) {
    keyLabel = e.code.slice(3).toUpperCase();
    vkCode = keyLabel.charCodeAt(0);
  } else if (e.code && e.code.startsWith('Digit')) {
    keyLabel = e.code.slice(5);
    vkCode = keyLabel.charCodeAt(0);
  } else if (e.code && e.code.startsWith('F') && !isNaN(Number(e.code.slice(1)))) {
    const fn = Number(e.code.slice(1));
    if (fn >= 1 && fn <= 12) {
      keyLabel = `F${fn}`;
      vkCode = 0x70 + (fn - 1);
    }
  } else if (e.key === ' ') {
    keyLabel = 'Space';
    vkCode = 0x20;
  } else if (e.key === 'Enter') {
    keyLabel = 'Enter';
    vkCode = 0x0d;
  } else if (e.key === 'Tab') {
    keyLabel = 'Tab';
    vkCode = 0x09;
  }

  const dwordValue = (modFlags << 8) | (vkCode & 0xff);

  const parts: string[] = [];
  if (isWin) parts.push('Win');
  if (isCtrl) parts.push('Ctrl');
  if (isShift) parts.push('Shift');
  if (isAlt) parts.push('Alt');
  parts.push(keyLabel);

  const formattedText = parts.join(' + ');

  emit('update:modelValue', dwordValue);
  emit('update:text', formattedText);

  stopRecording();
}
</script>

<style scoped>
.hotkey-recorder-container {
  width: 100%;
}

.hotkey-box {
  display: flex;
  align-items: center;
  justify-content: space-between;
  min-height: 34px;
  padding: 4px 10px;
  background: rgba(22, 27, 34, 0.75);
  border: 1px solid #30363d;
  border-radius: 6px;
  cursor: pointer;
  outline: none;
  transition: all 0.15s ease;
  user-select: none;
}

.hotkey-box:hover {
  border-color: #58a6ff;
  background: rgba(22, 27, 34, 0.95);
}

.hotkey-box:focus,
.hotkey-box.recording {
  border-color: #58a6ff;
  box-shadow: 0 0 0 2px rgba(88, 166, 255, 0.2);
  background: rgba(13, 17, 23, 0.95);
}

.hotkey-display {
  display: flex;
  align-items: center;
  flex-wrap: wrap;
  gap: 5px;
}

.key-chip {
  display: inline-flex;
  align-items: center;
  padding: 2px 7px;
  font-size: 11px;
  font-family: ui-monospace, SFMono-Regular, "SF Mono", Menlo, Consolas, monospace;
  font-weight: 600;
  color: #c9d1d9;
  background: #21262d;
  border: 1px solid #30363d;
  border-bottom-width: 2px;
  border-radius: 4px;
  box-shadow: 0 1px 2px rgba(0, 0, 0, 0.2);
}

.recording-prompt {
  display: flex;
  align-items: center;
  font-size: 11px;
  color: #58a6ff;
  font-weight: 500;
}

.hotkey-actions {
  display: flex;
  align-items: center;
  gap: 4px;
  margin-left: 8px;
}

.action-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 22px;
  height: 22px;
  border-radius: 4px;
  border: none;
  background: transparent;
  color: #8b949e;
  cursor: pointer;
  font-size: 13px;
  transition: all 0.15s;
}

.action-btn:hover {
  color: #f0f6fc;
  background: #30363d;
}

.clear-btn:hover {
  color: #f85149;
}
</style>
