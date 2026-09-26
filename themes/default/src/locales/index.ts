// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
import { ref } from 'vue';

export const currentLanguage = ref<'zh-CN' | 'en-US'>('zh-CN');

export function setLanguage(lang?: string) {
  if (lang === 'en-US' || lang === 'en') {
    currentLanguage.value = 'en-US';
  } else {
    currentLanguage.value = 'zh-CN';
  }
}

export const zhDict = {
  addPlaceholder: '添加待办 (Enter 添加, Shift+Enter 换行)...',
  addBtn: '添加事项 (Enter)',
  empty: '暂无待办事项',
  pending: '{n} 项待办',
  allDone: '全部已完成',
  clearFinished: '清空已完成',
  noFinished: '暂无已完成事项',
  cleanedFinished: '已清理 {n} 项已完成事项',
  doubleClickTitle: '双击输入标题',
  titleHint: '双击编辑标题，单击拖拽便签',
  titlePlaceholder: '便签标题...',
  settingsTitle: '便签主题与设置',
  presetColors: '预设色彩',
  customColor: '自定义颜色',
  topmost: '窗口置顶',
  opacityable: '失焦半透明',
  enabled: '已开启',
  disabled: '已关闭',
  lockEdge: '锁定便签 (禁止拖动且不自动隐入)',
  unlockEdge: '解除贴边锁定 (允许拖动与自动隐入)',
  penetrate: '开启穿透挂件模式',
  closeNote: '关闭便签 (管理中心可恢复)',
  deleteItem: '删除事项',
};

export const enDict: typeof zhDict = {
  addPlaceholder: 'Add todo (Enter to add, Shift+Enter for newline)...',
  addBtn: 'Add item (Enter)',
  empty: 'No todo items',
  pending: '{n} pending',
  allDone: 'All completed',
  clearFinished: 'Clear completed',
  noFinished: 'No completed items',
  cleanedFinished: 'Cleared {n} completed items',
  doubleClickTitle: 'Double click to enter title',
  titleHint: 'Double click to edit title, drag to move',
  titlePlaceholder: 'Note title...',
  settingsTitle: 'Theme & Settings',
  presetColors: 'Preset Colors',
  customColor: 'Custom Color',
  topmost: 'Always on Top',
  opacityable: 'Blur Transparency',
  enabled: 'ON',
  disabled: 'OFF',
  lockEdge: 'Lock edge dock (Disable drag & auto-hide)',
  unlockEdge: 'Unlock edge dock',
  penetrate: 'Pin as desktop widget',
  closeNote: 'Close note (Restore via Manager)',
  deleteItem: 'Delete item',
};

export function t(key: keyof typeof zhDict, params?: Record<string, string | number>): string {
  const dict = currentLanguage.value === 'en-US' ? enDict : zhDict;
  let text = dict[key] || zhDict[key] || (key as string);
  if (params) {
    text = text.replace(/\{(\w+)\}/g, (_, k) => {
      return params[k] !== undefined ? String(params[k]) : `{${k}}`;
    });
  }
  return text;
}
