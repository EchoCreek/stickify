<!--
  Copyright (c) Sticky Notes Refactoring Team (2026)
  Part of Sticky Notes Refactoring Project
  Licensed under the Apache License, Version 2.0
-->
<template>
  <div
    v-if="visible"
    class="modal-backdrop select-none"
    @click.self="$emit('close')"
  >
    <div class="modal-card">
      <div class="modal-header">
        <div class="modal-title-wrap">
          <i :class="format === 'json' ? 'ri-code-s-slash-line' : 'ri-markdown-line'" class="text-blue-400 text-lg"></i>
          <h3 class="modal-title">{{ t('exportModal.preview') }} ({{ format.toUpperCase() }})</h3>
        </div>
        <button
          @click="$emit('close')"
          class="close-btn"
          :title="t('common.close')"
        >
          <i class="ri-close-line text-base"></i>
        </button>
      </div>

      <div class="modal-body">
        <textarea
          readonly
          :value="content"
          class="content-textarea"
          :placeholder="t('exportModal.emptyWarning')"
        ></textarea>
      </div>

      <div class="modal-footer">
        <button
          @click="copyToClipboard"
          class="action-btn copy-btn"
        >
          <i :class="copied ? 'ri-check-line text-emerald-400' : 'ri-file-copy-line'"></i>
          <span>{{ copied ? t('common.copied') : t('exportModal.copyContent') }}</span>
        </button>

        <button
          @click="downloadFile"
          class="action-btn download-btn"
        >
          <i class="ri-download-2-line"></i>
          <span>{{ t('exportModal.downloadFile') }} (.{{ format }})</span>
        </button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue';
import { useI18n } from '../../locales';

const props = defineProps<{
  visible: boolean;
  content: string;
  format: string;
}>();

defineEmits<{
  (e: 'close'): void;
}>();

const { t } = useI18n();

const copied = ref(false);

function copyToClipboard() {
  navigator.clipboard.writeText(props.content).then(() => {
    copied.value = true;
    setTimeout(() => {
      copied.value = false;
    }, 2000);
  });
}

function downloadFile() {
  const blob = new Blob([props.content], { type: 'text/plain;charset=utf-8' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `sticky_notes_export_${new Date().toISOString().slice(0, 10)}.${props.format}`;
  a.click();
  URL.revokeObjectURL(url);
}
</script>

<style scoped>
.modal-backdrop {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  width: 100%;
  height: 100%;
  background: rgba(0, 0, 0, 0.65);
  backdrop-filter: blur(8px);
  -webkit-backdrop-filter: blur(8px);
  z-index: 8000;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 16px;
  box-sizing: border-box;
  border-bottom-left-radius: 10px;
  border-bottom-right-radius: 10px;
  animation: modalFadeIn 0.18s ease-out;
}

@keyframes modalFadeIn {
  from {
    opacity: 0;
    transform: scale(0.98);
  }
  to {
    opacity: 1;
    transform: scale(1);
  }
}

.modal-card {
  width: 580px;
  max-width: 90vw;
  max-height: calc(100vh - 80px);
  background: #161b22;
  border: 1px solid #30363d;
  border-radius: 10px;
  box-shadow: 0 20px 48px rgba(0, 0, 0, 0.7), 0 0 0 1px rgba(255, 255, 255, 0.08);
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.modal-header {
  height: 44px;
  padding: 0 16px;
  border-bottom: 1px solid #30363d;
  display: flex;
  align-items: center;
  justify-content: space-between;
  background: #1c2333;
}

.modal-title-wrap {
  display: flex;
  align-items: center;
  gap: 8px;
}

.modal-title {
  font-size: 13px;
  font-weight: 700;
  color: #f0f6fc;
  margin: 0;
}

.close-btn {
  width: 26px;
  height: 26px;
  border-radius: 4px;
  background: transparent;
  border: none;
  color: #8b949e;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  transition: all 0.15s;
}

.close-btn:hover {
  background: #252d3d;
  color: #f0f6fc;
}

.modal-body {
  padding: 14px 16px;
  background: #0d1117;
}

.content-textarea {
  width: 100%;
  height: 280px;
  background: #161b22;
  border: 1px solid #30363d;
  border-radius: 6px;
  padding: 10px 12px;
  font-family: monospace;
  font-size: 11px;
  color: #c9d1d9;
  outline: none;
  resize: none;
  line-height: 1.5;
  box-sizing: border-box;
}

.modal-footer {
  padding: 10px 16px;
  border-top: 1px solid #30363d;
  background: #1c2333;
  display: flex;
  align-items: center;
  justify-content: flex-end;
  gap: 8px;
}

.action-btn {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 6px 14px;
  font-size: 11px;
  font-weight: 600;
  border-radius: 6px;
  cursor: pointer;
  transition: all 0.15s;
}

.copy-btn {
  background: #21262d;
  border: 1px solid #30363d;
  color: #f0f6fc;
}

.copy-btn:hover {
  background: #30363d;
}

.download-btn {
  background: #238636;
  border: 1px solid rgba(240, 246, 252, 0.1);
  color: #ffffff;
}

.download-btn:hover {
  background: #2ea043;
}
</style>
