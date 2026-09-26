<!--
  Copyright (c) Sticky Notes Refactoring Team (2026)
  Part of Sticky Notes Refactoring Project
  Licensed under the Apache License, Version 2.0
-->
<template>
  <div class="settings-panel select-none">
    <div class="settings-container">
      <!-- Section Header -->
      <div class="panel-header">
        <div class="header-title">
          <i class="ri-settings-3-line text-blue-400 mr-2 text-xl"></i>
          <span>{{ t('settings.headerTitle') }}</span>
        </div>
        <div class="header-desc">
          {{ t('settings.headerDesc') }}
        </div>
      </div>

      <div class="settings-content">
        <!-- 1. 基础常规设置 -->
        <div class="settings-card">
          <div class="card-title">
            <i class="ri-toggle-line card-icon"></i>
            <span>{{ t('settings.generalTitle') }}</span>
          </div>

          <!-- 开机自启动 -->
          <div class="setting-item">
            <div class="item-info">
              <div class="item-label">{{ t('settings.autoRunLabel') }}</div>
              <div class="item-help">{{ t('settings.autoRunHelp') }}</div>
            </div>
            <div class="item-control">
              <div
                class="glass-switch"
                :class="{ active: form.autoRun }"
                role="switch"
                :aria-checked="form.autoRun"
                tabindex="0"
                @click="form.autoRun = !form.autoRun"
                @keydown.space.prevent="form.autoRun = !form.autoRun"
                @keydown.enter.prevent="form.autoRun = !form.autoRun"
              >
                <div class="glass-switch-knob"></div>
              </div>
            </div>
          </div>

          <!-- 界面显示语言 (Language) -->
          <div class="setting-item">
            <div class="item-info">
              <div class="item-label">{{ t('settings.languageLabel') }}</div>
              <div class="item-help">{{ t('settings.languageHelp') }}</div>
            </div>
            <div class="item-control">
              <el-popover
                v-model:visible="isLangOpen"
                trigger="click"
                placement="bottom-end"
                width="auto"
                :show-arrow="false"
                :offset="6"
                transition="dropdown-slide"
                popper-class="custom-theme-popover"
                :teleported="true"
              >
                <template #reference>
                  <button
                    type="button"
                    class="theme-trigger-btn"
                    :class="{ active: isLangOpen }"
                    :title="t('settings.languageLabel')"
                  >
                    <i class="ri-translate-2 theme-prefix-icon"></i>
                    <span class="theme-current-label">{{ form.language === 'en-US' ? t('settings.languageEn') : t('settings.languageZh') }}</span>
                    <i class="ri-arrow-down-s-line theme-caret" :class="{ 'is-open': isLangOpen }"></i>
                  </button>
                </template>

                <div class="theme-popover-menu">
                  <div class="theme-menu-title">{{ t('settings.languageLabel') }}</div>
                  <div
                    class="theme-option-row"
                    :class="{ 'is-active': form.language === 'zh-CN' }"
                    @click="selectLanguage('zh-CN')"
                  >
                    <i class="ri-global-line option-icon"></i>
                    <span class="option-text">{{ t('settings.languageZh') }}</span>
                    <i v-if="form.language === 'zh-CN'" class="ri-check-line option-check"></i>
                  </div>
                  <div
                    class="theme-option-row"
                    :class="{ 'is-active': form.language === 'en-US' }"
                    @click="selectLanguage('en-US')"
                  >
                    <i class="ri-earth-line option-icon"></i>
                    <span class="option-text">{{ t('settings.languageEn') }}</span>
                    <i v-if="form.language === 'en-US'" class="ri-check-line option-check"></i>
                  </div>
                </div>
              </el-popover>
            </div>
          </div>

          <!-- 新建便签默认背景色 -->
          <div class="setting-item">
            <div class="item-info">
              <div class="item-label">{{ t('settings.defaultColorLabel') }}</div>
              <div class="item-help">{{ t('settings.defaultColorHelp') }}</div>
            </div>
            <div class="item-control">
              <div class="color-palette">
                <button
                  v-for="c in presetColors"
                  :key="c.hex"
                  type="button"
                  class="color-chip"
                  :class="{ active: form.defaultBgColor?.toLowerCase() === c.hex.toLowerCase() }"
                  :style="{ backgroundColor: c.hex, borderColor: c.border }"
                  :title="getColorName(c.hex)"
                  @click="form.defaultBgColor = c.hex"
                >
                  <i
                    v-if="form.defaultBgColor?.toLowerCase() === c.hex.toLowerCase()"
                    class="ri-check-line check-mark"
                    :class="isLightColor(c.hex) ? 'text-gray-800' : 'text-white'"
                  ></i>
                </button>
              </div>
            </div>
          </div>
        </div>

        <!-- 2. 全局系统快捷键 -->
        <div class="settings-card">
          <div class="card-title">
            <i class="ri-keyboard-line card-icon"></i>
            <span>{{ t('settings.hotkeysTitle') }}</span>
          </div>
          <div class="card-hint">
            {{ t('settings.hotkeysHint') }}
          </div>

          <div class="grid grid-cols-1 md:grid-cols-2 gap-4 mt-3">
            <div class="hotkey-field">
              <div class="hotkey-label">
                <i class="ri-add-circle-line mr-1 text-green-400"></i>{{ t('settings.newNoteLabel') }}
              </div>
              <HotkeyRecorder
                v-model="form.hotkeys.newNote.value"
                v-model:text="form.hotkeys.newNote.text"
              />
            </div>

            <div class="hotkey-field">
              <div class="hotkey-label">
                <i class="ri-edit-box-line mr-1 text-blue-400"></i>{{ t('settings.editNoteLabel') }}
              </div>
              <HotkeyRecorder
                v-model="form.hotkeys.editNote.value"
                v-model:text="form.hotkeys.editNote.text"
              />
            </div>

            <div class="hotkey-field">
              <div class="hotkey-label">
                <i class="ri-eye-line mr-1 text-purple-400"></i>{{ t('settings.showAllLabel') }}
              </div>
              <HotkeyRecorder
                v-model="form.hotkeys.showAll.value"
                v-model:text="form.hotkeys.showAll.text"
              />
            </div>

            <div class="hotkey-field">
              <div class="hotkey-label">
                <i class="ri-eye-off-line mr-1 text-amber-400"></i>{{ t('settings.hideAllLabel') }}
              </div>
              <HotkeyRecorder
                v-model="form.hotkeys.hideAll.value"
                v-model:text="form.hotkeys.hideAll.text"
              />
            </div>
          </div>
        </div>

        <!-- 3. 数据与存储 -->
        <div class="settings-card">
          <div class="card-title">
            <i class="ri-database-2-line card-icon"></i>
            <span>{{ t('settings.storageTitle') }}</span>
          </div>

          <div class="setting-item storage-item">
            <div class="item-info storage-info">
              <div class="item-label">{{ t('settings.storageLabel') }}</div>
              <div class="item-help">{{ t('settings.storageHelp') }}</div>
            </div>
            <div class="path-box-row w-full flex items-center gap-2 mt-2">
              <div class="glass-input-wrapper flex-1">
                <i class="ri-folder-3-line input-icon"></i>
                <input
                  type="text"
                  v-model="form.noteDir"
                  readonly
                  :placeholder="t('settings.folderPlaceholder')"
                  class="glass-input"
                />
              </div>
              <button
                type="button"
                class="btn-secondary"
                @click="browseFolder"
              >
                <i class="ri-folder-open-line mr-1"></i>{{ t('settings.selectFolder') }}
              </button>
              <button
                type="button"
                class="btn-secondary"
                @click="openFolder"
              >
                <i class="ri-external-link-line mr-1"></i>{{ t('settings.openFolder') }}
              </button>
            </div>
          </div>

          <!-- 数据库全量备份与覆盖导入（独立即时执行） -->
          <div class="setting-item storage-item border-t border-white/10 pt-4 mt-3">
            <div class="item-info storage-info">
              <div class="item-label flex items-center gap-1.5">
                <i class="ri-shield-keyhole-line text-blue-400"></i>
                <span>{{ t('settings.dbBackupTitle') }}</span>
              </div>
              <div class="item-help">{{ t('settings.dbBackupHelp') }}</div>
              <div class="item-notice text-xs text-amber-300/80 mt-1 flex items-center gap-1">
                <i class="ri-information-line"></i>
                <span>{{ t('settings.dbBackupInstantNotice') }}</span>
              </div>
            </div>
            <div class="flex items-center gap-3 mt-3">
              <button
                type="button"
                class="btn-secondary"
                :disabled="exportingDb"
                @click="handleExportDb"
              >
                <i v-if="exportingDb" class="ri-loader-4-line animate-spin mr-1.5 text-blue-400"></i>
                <i v-else class="ri-download-2-line mr-1.5 text-blue-400"></i>
                {{ t('settings.exportDbBtn') }}
              </button>

              <button
                type="button"
                class="btn-secondary danger-action"
                :disabled="importingDb"
                @click="handleImportDb"
              >
                <i v-if="importingDb" class="ri-loader-4-line animate-spin mr-1.5 text-amber-400"></i>
                <i v-else class="ri-upload-2-line mr-1.5 text-amber-400"></i>
                {{ t('settings.importDbBtn') }}
              </button>
            </div>
          </div>
        </div>

        <!-- 4. 运行环境与主题 -->
        <div class="settings-card">
          <div class="card-title">
            <i class="ri-palette-line card-icon"></i>
            <span>{{ t('settings.themeTitle') }}</span>
          </div>

          <div class="setting-item">
            <div class="item-info">
              <div class="item-label">{{ t('settings.themeLabel') }}</div>
              <div class="item-help">{{ t('settings.themeHelp') }}</div>
            </div>
            <div class="item-control">
              <el-popover
                v-model:visible="isThemeOpen"
                trigger="click"
                placement="bottom-end"
                width="auto"
                :show-arrow="false"
                :offset="6"
                transition="dropdown-slide"
                popper-class="custom-theme-popover"
                :teleported="true"
              >
                <template #reference>
                  <button
                    type="button"
                    class="theme-trigger-btn"
                    :class="{ active: isThemeOpen }"
                    :title="t('settings.selectTheme')"
                  >
                    <i class="ri-palette-line theme-prefix-icon"></i>
                    <span class="theme-current-label">{{ form.theme || 'Default' }}</span>
                    <i class="ri-arrow-down-s-line theme-caret" :class="{ 'is-open': isThemeOpen }"></i>
                  </button>
                </template>

                <div class="theme-popover-menu">
                  <div class="theme-menu-title">{{ t('settings.themeLabel') }}</div>
                  <div
                    v-for="themeItem in availableThemes"
                    :key="themeItem"
                    class="theme-option-row"
                    :class="{ 'is-active': form.theme === themeItem }"
                    @click="selectTheme(themeItem)"
                  >
                    <i :class="getThemeIcon(themeItem)" class="option-icon"></i>
                    <span class="option-text">{{ themeItem }}</span>
                    <i v-if="form.theme === themeItem" class="ri-check-line option-check"></i>
                  </div>
                </div>
              </el-popover>
            </div>
          </div>

          <div class="setting-item">
            <div class="item-info">
              <div class="item-label">{{ t('settings.webviewStatusLabel') }}</div>
              <div class="item-help">{{ t('settings.webviewStatusHelp') }}</div>
            </div>
            <div class="item-control">
              <span class="runtime-badge">
                <i class="ri-checkbox-circle-fill text-green-400 mr-1"></i>
                {{ t('settings.webviewReady') }}
              </span>
            </div>
          </div>

          <div class="setting-item">
            <div class="item-info">
              <div class="item-label">{{ t('settings.customWebviewLabel') }}</div>
              <div class="item-help">{{ t('settings.customWebviewHelp') }}</div>
            </div>
            <div class="item-control">
              <div
                class="glass-switch"
                :class="{ active: form.customWebview2 }"
                role="switch"
                :aria-checked="form.customWebview2"
                tabindex="0"
                @click="form.customWebview2 = !form.customWebview2"
                @keydown.space.prevent="form.customWebview2 = !form.customWebview2"
                @keydown.enter.prevent="form.customWebview2 = !form.customWebview2"
              >
                <div class="glass-switch-knob"></div>
              </div>
            </div>
          </div>

          <div v-if="form.customWebview2" class="path-box-row w-full flex items-center gap-2 mt-2">
            <div class="glass-input-wrapper flex-1">
              <i class="ri-code-line input-icon"></i>
              <input
                type="text"
                v-model="form.webview2Path"
                :placeholder="t('settings.webviewPathPlaceholder')"
                class="glass-input"
              />
            </div>
          </div>
        </div>

        <!-- 5. 软件关于与许可 -->
        <div class="settings-card">
          <div class="card-title">
            <i class="ri-information-line card-icon"></i>
            <span>{{ t('settings.aboutTitle') }}</span>
          </div>

          <div class="about-section">
            <div class="about-header">
              <div class="about-logo">
                <img src="/logo.svg" class="about-logo-img" alt="Stickify Logo" />
              </div>
              <div class="about-meta">
                <div class="about-name">{{ t('settings.appName') }}</div>
                <div class="about-ver">{{ t('settings.appVersionLabel', { version: appVersion || 'V1.0.0' }) }}</div>
              </div>
            </div>
            <div class="about-desc">
              {{ t('settings.appDesc') }}
            </div>
            <div class="about-license">
              <span>{{ t('settings.appLicense') }}</span>
              <span class="mx-2">•</span>
              <span>{{ t('settings.copyright') }}</span>
            </div>
          </div>
        </div>
      </div>

      <!-- 底部固定保存动作条 -->
      <div class="settings-footer">
        <div class="footer-hint">
          <i class="ri-shield-check-line text-green-400 mr-1"></i>
          {{ t('settings.footerHint') }}
        </div>
        <div class="footer-buttons">
          <button
            type="button"
            class="btn-reset"
            :disabled="saving"
            @click="resetForm"
          >
            {{ t('settings.resetBtn') }}
          </button>
          <button
            type="button"
            class="btn-save"
            :disabled="saving"
            @click="saveSettings"
          >
            <i v-if="saving" class="ri-loader-4-line animate-spin mr-1"></i>
            <i v-else class="ri-save-3-line mr-1"></i>
            {{ saving ? t('settings.savingBtn') : t('settings.saveBtn') }}
          </button>
        </div>
      </div>
    </div>

    <!-- 数据库覆盖导入二次确认高危弹窗 -->
    <ConfirmModal
      :visible="importConfirmVisible"
      :title="t('settings.importConfirmTitle')"
      :message="t('settings.importConfirmMsg')"
      :confirmText="t('settings.importConfirmBtn')"
      :cancelText="t('settings.importCancelBtn')"
      :danger="true"
      @confirm="onImportConfirmed"
      @cancel="importConfirmVisible = false"
    />
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted, onUnmounted } from 'vue';
import { ElMessage } from 'element-plus';
import { managerApi, type AppSettingsDto } from '../../api/managerApi';
import { PRESET_COLORS, isLightColor, getColorName } from '../../utils';
import { useI18n, type SupportedLanguage } from '../../locales';
import HotkeyRecorder from './HotkeyRecorder.vue';
import ConfirmModal from './ConfirmModal.vue';

const { t, setLanguage, lang } = useI18n();

const saving = ref(false);
const exportingDb = ref(false);
const importingDb = ref(false);
const importConfirmVisible = ref(false);
const appVersion = ref('V1.0.0');
const availableThemes = ref<string[]>(['Default', 'Simple']);
const isThemeOpen = ref(false);
const isLangOpen = ref(false);

function selectTheme(tName: string) {
  form.theme = tName;
  isThemeOpen.value = false;
}

function selectLanguage(newLang: SupportedLanguage) {
  form.language = newLang;
  // 语言选择仅记录在当前表单中，待点击“保存设置”后统一生效
  isLangOpen.value = false;
}

function getThemeIcon(themeName: string) {
  if (themeName.toLowerCase().includes('simple')) return 'ri-layout-masonry-line';
  return 'ri-layout-grid-line';
}

const form = reactive({
  autoRun: true,
  customWebview2: false,
  webview2Path: '',
  noteDir: '',
  theme: 'Default',
  defaultBgColor: '#0d1117',
  language: lang.value as string,
  hotkeys: {
    newNote: { value: 0x0C48, text: 'Win + Alt + F8' },
    editNote: { value: 0x0C47, text: 'Win + Alt + F7' },
    hideAll: { value: 0x0C49, text: 'Win + Alt + F9' },
    showAll: { value: 0x0C4A, text: 'Win + Alt + F10' },
  }
});

let originalSettings: AppSettingsDto | null = null;
const presetColors = PRESET_COLORS;

function applyIncomingSettings(settings: AppSettingsDto) {
  originalSettings = JSON.parse(JSON.stringify(settings));
  form.autoRun = settings.autoRun ?? true;
  form.customWebview2 = settings.customWebview2 ?? false;
  form.webview2Path = settings.webview2Path || '';
  form.noteDir = settings.noteDir || '';
  form.theme = settings.theme || 'Default';
  form.defaultBgColor = settings.defaultBgColor || '#0d1117';
  if (settings.language) {
    form.language = settings.language;
    setLanguage(settings.language);
  }

  if (settings.appVersion) {
    appVersion.value = settings.appVersion;
  }

  if (settings.themes && settings.themes.length > 0) {
    availableThemes.value = settings.themes;
  }

  if (settings.hotkeys) {
    if (settings.hotkeys.newNote) {
      form.hotkeys.newNote.value = settings.hotkeys.newNote.value;
      form.hotkeys.newNote.text = settings.hotkeys.newNote.text;
    }
    if (settings.hotkeys.editNote) {
      form.hotkeys.editNote.value = settings.hotkeys.editNote.value;
      form.hotkeys.editNote.text = settings.hotkeys.editNote.text;
    }
    if (settings.hotkeys.hideAll) {
      form.hotkeys.hideAll.value = settings.hotkeys.hideAll.value;
      form.hotkeys.hideAll.text = settings.hotkeys.hideAll.text;
    }
    if (settings.hotkeys.showAll) {
      form.hotkeys.showAll.value = settings.hotkeys.showAll.value;
      form.hotkeys.showAll.text = settings.hotkeys.showAll.text;
    }
  }
}

function resetForm() {
  if (originalSettings) {
    applyIncomingSettings(originalSettings);
    ElMessage({
      message: t('settings.resetSuccess'),
      type: 'info',
      duration: 1800,
    });
  }
}

function browseFolder() {
  managerApi.browseFolder(form.noteDir);
}

function openFolder() {
  managerApi.openFolder(form.noteDir);
}

function handleExportDb() {
  exportingDb.value = true;
  managerApi.exportDb();
}

function handleImportDb() {
  importConfirmVisible.value = true;
}

function onImportConfirmed() {
  importConfirmVisible.value = false;
  importingDb.value = true;
  managerApi.importDb();
}

function saveSettings() {
  saving.value = true;
  const payload = {
    autoRun: form.autoRun,
    customWebview2: form.customWebview2,
    webview2Path: form.webview2Path,
    noteDir: form.noteDir,
    theme: form.theme,
    defaultBgColor: form.defaultBgColor,
    language: form.language,
    newHotKey: form.hotkeys.newNote.value,
    editHotKey: form.hotkeys.editNote.value,
    unActiveHotKey: form.hotkeys.hideAll.value,
    activeAllHotKey: form.hotkeys.showAll.value,
  };

  managerApi.saveAppSettings(payload);
}

let unbindSettings: (() => void) | null = null;
let unbindFolder: (() => void) | null = null;
let unbindResult: (() => void) | null = null;
let unbindExportDb: (() => void) | null = null;
let unbindImportDb: (() => void) | null = null;

onMounted(() => {
  unbindSettings = managerApi.onAppSettings((settings) => {
    applyIncomingSettings(settings);
  });

  unbindFolder = managerApi.onFolderSelected((data) => {
    if (data && data.folder) {
      form.noteDir = data.folder;
      ElMessage({
        message: `${t('settings.storageLabel')}: ${data.folder}`,
        type: 'success',
        duration: 2200,
      });
    }
  });

  unbindResult = managerApi.onSaveSettingsResult((res) => {
    saving.value = false;
    if (res.success) {
      if (form.language) {
        setLanguage(form.language as SupportedLanguage);
      }
      ElMessage({
        message: res.message || t('settings.saveSuccess'),
        type: 'success',
        duration: 2500,
      });
    } else {
      ElMessage({
        message: res.message || t('common.error'),
        type: 'error',
        duration: 3000,
      });
    }
  });

  unbindExportDb = managerApi.onExportDbResult((res) => {
    exportingDb.value = false;
    if (res.success && res.path) {
      ElMessage({
        message: t('settings.exportDbSuccess', { path: res.path }),
        type: 'success',
        duration: 3500,
      });
    }
  });

  unbindImportDb = managerApi.onImportDbResult((res) => {
    importingDb.value = false;
    if (res.success) {
      ElMessage({
        message: t('settings.importDbSuccess', { n: res.count ?? 0 }),
        type: 'success',
        duration: 3500,
      });
      managerApi.listAll();
    }
  });

  managerApi.getAppSettings();
});

onUnmounted(() => {
  if (unbindSettings) unbindSettings();
  if (unbindFolder) unbindFolder();
  if (unbindResult) unbindResult();
  if (unbindExportDb) unbindExportDb();
  if (unbindImportDb) unbindImportDb();
});
</script>

<style scoped>
.settings-panel {
  width: 100%;
  height: 100%;
  overflow-y: auto;
  padding: 20px 24px 80px 24px;
  box-sizing: border-box;
  background: #1c2333;
}

.settings-container {
  max-width: 820px;
  margin: 0 auto;
  display: flex;
  flex-direction: column;
  gap: 18px;
}

.panel-header {
  margin-bottom: 4px;
}

.header-title {
  display: flex;
  align-items: center;
  font-size: 16px;
  font-weight: 700;
  color: #f0f6fc;
}

.header-desc {
  font-size: 11px;
  color: #8b949e;
  margin-top: 4px;
}

.settings-content {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.settings-card {
  background: rgba(13, 17, 23, 0.7);
  border: 1px solid #30363d;
  border-radius: 8px;
  padding: 16px 18px;
  backdrop-filter: blur(12px);
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.2);
}

.card-title {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 13px;
  font-weight: 700;
  color: #f0f6fc;
  margin-bottom: 12px;
  padding-bottom: 8px;
  border-bottom: 1px solid rgba(48, 54, 61, 0.6);
}

.card-icon {
  font-size: 16px;
  color: #58a6ff;
}

.card-hint {
  font-size: 11px;
  color: #8b949e;
  margin-bottom: 10px;
}

.setting-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 10px 0;
  border-bottom: 1px solid rgba(48, 54, 61, 0.35);
}

.setting-item:last-child {
  border-bottom: none;
  padding-bottom: 0;
}

.setting-item.storage-item {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  text-align: left;
}

.item-info {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  text-align: left;
  gap: 2px;
}

.item-label {
  font-size: 12px;
  font-weight: 600;
  color: #c9d1d9;
  text-align: left;
}

.item-help {
  font-size: 10px;
  color: #8b949e;
  text-align: left;
  line-height: 1.4;
}

/* ─── Modern iOS/Mac-Style Glass Switch ───────────────────────────────────── */
.glass-switch {
  position: relative;
  width: 44px;
  height: 24px;
  background: #30363d;
  border: 1px solid rgba(240, 246, 252, 0.1);
  border-radius: 999px;
  cursor: pointer;
  outline: none;
  transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1);
  box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.3);
}

.glass-switch:hover {
  border-color: #58a6ff;
}

.glass-switch:focus-visible {
  box-shadow: 0 0 0 2px rgba(88, 166, 255, 0.4);
}

.glass-switch.active {
  background: #238636;
  border-color: #2ea043;
}

.glass-switch-knob {
  position: absolute;
  top: 2px;
  left: 2px;
  width: 18px;
  height: 18px;
  background: #ffffff;
  border-radius: 50%;
  transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1);
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.35);
}

.glass-switch.active .glass-switch-knob {
  left: 22px;
}

/* ─── Inputs & Textboxes ─────────────────────────────────────────────────── */
.path-box-row {
  display: flex;
  align-items: center;
  gap: 8px;
}

.glass-input-wrapper {
  position: relative;
  display: flex;
  align-items: center;
}

.input-icon {
  position: absolute;
  left: 10px;
  color: #8b949e;
  font-size: 14px;
  pointer-events: none;
}

.glass-input {
  width: 100%;
  background: #0d1117;
  border: 1px solid #30363d;
  border-radius: 6px;
  padding: 6px 10px 6px 30px;
  color: #f0f6fc;
  font-size: 11px;
  font-family: ui-monospace, SFMono-Regular, "SF Mono", Menlo, Consolas, monospace;
  outline: none;
  transition: all 0.15s ease;
  box-sizing: border-box;
}

.glass-input:hover:not(:read-only),
.glass-input:focus:not(:read-only) {
  border-color: #58a6ff;
  box-shadow: 0 0 0 2px rgba(88, 166, 255, 0.2);
}

.glass-input:read-only {
  background: #161b22;
  color: #f0f6fc;
}

.glass-input::placeholder {
  color: #6e7681;
}

/* ─── UI Theme Popover Trigger & Menu (Matching TitleBar Sort Dropdown) ──── */
.theme-trigger-btn {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  height: 28px;
  padding: 0 10px;
  background: #0d1117;
  border: 1px solid #30363d;
  border-radius: 6px;
  font-size: 11px;
  font-weight: 600;
  color: #c9d1d9;
  cursor: pointer;
  outline: none;
  transition: all 0.15s ease;
  user-select: none;
  min-width: 120px;
  box-sizing: border-box;
}

.theme-trigger-btn:hover {
  background: #161b22;
  border-color: #58a6ff;
  color: #f0f6fc;
}

.theme-trigger-btn.active {
  border-color: #60a5fa;
  background: #1c2333;
  color: #60a5fa;
  box-shadow: 0 0 0 1px rgba(96, 165, 250, 0.4);
}

.theme-prefix-icon {
  font-size: 13px;
  color: #60a5fa;
  flex-shrink: 0;
}

.theme-current-label {
  line-height: 1;
  flex: 1;
  text-align: left;
  margin: 0 2px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.theme-caret {
  font-size: 12px;
  color: #8b949e;
  transition: transform 0.2s ease;
  flex-shrink: 0;
}

.theme-caret.is-open {
  transform: rotate(180deg);
  color: #60a5fa;
}

.theme-popover-menu {
  min-width: 170px;
  width: 100%;
  box-sizing: border-box;
  padding: 2px;
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.theme-menu-title {
  font-size: 11px;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.05em;
  color: #8b949e;
  padding: 4px 8px 3px 8px;
  margin-bottom: 2px;
  user-select: none;
}

.theme-option-row {
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

.theme-option-row:hover {
  background: #21283b;
  color: #f0f6fc;
}

.theme-option-row.is-active {
  background: rgba(96, 165, 250, 0.14);
  color: #60a5fa;
  font-weight: 500;
}

.theme-option-row .option-icon {
  font-size: 14px;
  color: #8b949e;
  flex-shrink: 0;
  width: 16px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
}

.theme-option-row.is-active .option-icon {
  color: #60a5fa;
}

.theme-option-row .option-text {
  flex: 1;
  white-space: nowrap;
  padding-right: 4px;
}

.theme-option-row .option-check {
  font-size: 14px;
  color: #60a5fa;
  flex-shrink: 0;
  margin-left: auto;
}

/* ─── Color Palette ───────────────────────────────────────────────────────── */
.color-palette {
  display: flex;
  align-items: center;
  gap: 6px;
}

.color-chip {
  width: 24px;
  height: 24px;
  border-radius: 6px;
  border: 1px solid #30363d;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: all 0.15s ease;
}

.color-chip:hover {
  transform: scale(1.1);
}

.color-chip.active {
  box-shadow: 0 0 0 2px #58a6ff;
}

.check-mark {
  font-size: 12px;
}

/* ─── Hotkey Fields ───────────────────────────────────────────────────────── */
.hotkey-field {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.hotkey-label {
  display: flex;
  align-items: center;
  font-size: 11px;
  font-weight: 600;
  color: #c9d1d9;
}

/* ─── Buttons & Badges ────────────────────────────────────────────────────── */
.btn-secondary {
  display: inline-flex;
  align-items: center;
  padding: 6px 12px;
  background: #21262d;
  border: 1px solid #30363d;
  border-radius: 6px;
  color: #c9d1d9;
  font-size: 11px;
  font-weight: 600;
  cursor: pointer;
  white-space: nowrap;
  transition: all 0.15s;
}

.btn-secondary:hover {
  background: #30363d;
  color: #f0f6fc;
  border-color: #8b949e;
}

.btn-secondary.danger-action:hover {
  background: rgba(248, 81, 73, 0.15);
  border-color: #f85149;
  color: #f85149;
}

.runtime-badge {
  display: inline-flex;
  align-items: center;
  padding: 3px 10px;
  background: rgba(35, 134, 54, 0.15);
  border: 1px solid rgba(46, 160, 67, 0.4);
  border-radius: 999px;
  font-size: 11px;
  color: #3fb950;
  font-weight: 600;
}

/* ─── About Section ───────────────────────────────────────────────────────── */
.about-section {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.about-header {
  display: flex;
  align-items: center;
  gap: 12px;
}

.about-logo {
  width: 44px;
  height: 44px;
  border-radius: 10px;
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.12);
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 6px;
  box-sizing: border-box;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
}

.about-logo-img {
  width: 100%;
  height: 100%;
  object-fit: contain;
  filter: drop-shadow(0 2px 4px rgba(0, 0, 0, 0.2));
}

.about-name {
  font-size: 13px;
  font-weight: 700;
  color: #f0f6fc;
}

.about-ver {
  font-size: 11px;
  color: #8b949e;
}

.about-desc {
  font-size: 11px;
  line-height: 1.6;
  color: #8b949e;
}

.about-license {
  font-size: 10px;
  color: #6e7681;
  display: flex;
  align-items: center;
}

/* ─── Footer Action Bar ───────────────────────────────────────────────────── */
.settings-footer {
  margin-top: 10px;
  padding: 12px 18px;
  background: rgba(13, 17, 23, 0.85);
  border: 1px solid #30363d;
  border-radius: 8px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  backdrop-filter: blur(10px);
}

.footer-hint {
  font-size: 11px;
  color: #8b949e;
  display: flex;
  align-items: center;
}

.footer-buttons {
  display: flex;
  align-items: center;
  gap: 8px;
}

.btn-reset {
  padding: 6px 14px;
  background: transparent;
  border: 1px solid #30363d;
  border-radius: 6px;
  color: #8b949e;
  font-size: 11px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.15s;
}

.btn-reset:hover:not(:disabled) {
  color: #f0f6fc;
  border-color: #8b949e;
  background: #21262d;
}

.btn-save {
  display: inline-flex;
  align-items: center;
  padding: 6px 18px;
  background: #238636;
  border: 1px solid rgba(240, 246, 252, 0.1);
  border-radius: 6px;
  color: #ffffff;
  font-size: 11px;
  font-weight: 600;
  cursor: pointer;
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.3);
  transition: all 0.15s;
}

.btn-save:hover:not(:disabled) {
  background: #2ea043;
}

.btn-save:disabled,
.btn-reset:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}
</style>

<style>
/* Global Popover styling for teleported theme dropdown (matching custom-sort-popover) */
.custom-theme-popover.el-popper {
  background: #161b22 !important;
  border: 1px solid #30363d !important;
  border-radius: 8px !important;
  box-shadow: 0 12px 32px rgba(0, 0, 0, 0.65), 0 0 0 1px rgba(255, 255, 255, 0.06) !important;
  padding: 4px !important;
  width: auto !important;
  min-width: 170px !important;
  box-sizing: border-box !important;
  transform-origin: top right !important;
}

.custom-theme-popover.el-popper .el-popper__arrow {
  display: none !important;
}
</style>
