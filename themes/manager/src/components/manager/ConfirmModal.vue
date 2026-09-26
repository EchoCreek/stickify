<!--
  Copyright (c) Sticky Notes Refactoring Team (2026)
  Part of Sticky Notes Refactoring Project
  Licensed under the Apache License, Version 2.0
-->
<template>
  <div
    v-if="visible"
    class="confirm-backdrop select-none"
    @click.self="$emit('cancel')"
  >
    <div class="confirm-card" role="dialog" aria-modal="true">
      <div class="confirm-header">
        <div class="confirm-icon-wrap" :class="danger ? 'is-danger' : 'is-warning'">
          <i :class="danger ? 'ri-delete-bin-fill' : 'ri-error-warning-fill'" class="confirm-icon"></i>
        </div>
        <div class="confirm-title-wrap">
          <h3 class="confirm-title">{{ title || '确认操作' }}</h3>
          <p class="confirm-desc">{{ message }}</p>
        </div>
      </div>

      <div class="confirm-footer">
        <button
          type="button"
          class="btn-cancel"
          @click="$emit('cancel')"
        >
          {{ cancelText || t('confirm.cancelBtn') }}
        </button>
        <button
          type="button"
          class="btn-confirm"
          :class="{ 'is-danger': danger }"
          @click="$emit('confirm')"
        >
          <i v-if="danger" class="ri-delete-bin-line mr-1"></i>
          <span>{{ confirmText || t('confirm.confirmBtn') }}</span>
        </button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { useI18n } from '../../locales';

defineProps<{
  visible: boolean;
  title?: string;
  message: string;
  confirmText?: string;
  cancelText?: string;
  danger?: boolean;
}>();

defineEmits<{
  (e: 'confirm'): void;
  (e: 'cancel'): void;
}>();

const { t } = useI18n();
</script>

<style scoped>
.confirm-backdrop {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  width: 100vw;
  height: 100vh;
  background: rgba(0, 0, 0, 0.7);
  backdrop-filter: blur(8px);
  -webkit-backdrop-filter: blur(8px);
  z-index: 99999;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 16px;
  box-sizing: border-box;
  animation: confirmFadeIn 0.16s ease-out;
}

@keyframes confirmFadeIn {
  from {
    opacity: 0;
    transform: scale(0.96);
  }
  to {
    opacity: 1;
    transform: scale(1);
  }
}

.confirm-card {
  width: 380px;
  max-width: 90vw;
  background: #161b22;
  border: 1px solid #30363d;
  border-radius: 10px;
  box-shadow: 0 20px 48px rgba(0, 0, 0, 0.75), 0 0 0 1px rgba(255, 255, 255, 0.08);
  display: flex;
  flex-direction: column;
  overflow: hidden;
  padding: 20px 20px 16px 20px;
  box-sizing: border-box;
}

.confirm-header {
  display: flex;
  align-items: flex-start;
  gap: 14px;
}

.confirm-icon-wrap {
  width: 36px;
  height: 36px;
  border-radius: 8px;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.confirm-icon-wrap.is-danger {
  background: rgba(248, 81, 73, 0.15);
  border: 1px solid rgba(248, 81, 73, 0.3);
  color: #f85149;
}

.confirm-icon-wrap.is-warning {
  background: rgba(210, 153, 34, 0.15);
  border: 1px solid rgba(210, 153, 34, 0.3);
  color: #d29922;
}

.confirm-icon {
  font-size: 18px;
}

.confirm-title-wrap {
  flex: 1;
}

.confirm-title {
  font-size: 13px;
  font-weight: 700;
  color: #f0f6fc;
  margin: 0 0 6px 0;
}

.confirm-desc {
  font-size: 11px;
  line-height: 1.5;
  color: #8b949e;
  margin: 0;
  word-break: break-word;
}

.confirm-footer {
  display: flex;
  align-items: center;
  justify-content: flex-end;
  gap: 8px;
  margin-top: 18px;
}

.btn-cancel {
  padding: 5px 12px;
  background: #21262d;
  border: 1px solid #30363d;
  border-radius: 6px;
  color: #c9d1d9;
  font-size: 11px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.15s ease;
}

.btn-cancel:hover {
  background: #30363d;
  color: #f0f6fc;
  border-color: #8b949e;
}

.btn-confirm {
  padding: 5px 14px;
  background: #238636;
  border: 1px solid rgba(240, 246, 252, 0.1);
  border-radius: 6px;
  color: #ffffff;
  font-size: 11px;
  font-weight: 600;
  cursor: pointer;
  display: inline-flex;
  align-items: center;
  transition: all 0.15s ease;
}

.btn-confirm:hover {
  background: #2ea043;
}

.btn-confirm.is-danger {
  background: #da3633;
  border-color: rgba(240, 246, 252, 0.1);
}

.btn-confirm.is-danger:hover {
  background: #f85149;
}
</style>
