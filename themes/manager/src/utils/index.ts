// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
import { isWebview, sendToNative, onNativeMessage, type NoteManagerMessageType, type NoteManagerMessage } from '../ipc';
import { t } from '../locales';

export { isWebview, sendToNative, onNativeMessage };
export type { NoteManagerMessageType, NoteManagerMessage };

// ------------------------------------------------------------------
// 现代极简统一色盘与跨主题智能色系对齐
// ------------------------------------------------------------------
export interface PresetColor {
  name: string;
  hex: string;
  border: string;
  key?: string;
}

export const PRESET_COLORS: PresetColor[] = [
  { name: '曜石黑', hex: '#0d1117', border: '#30363d', key: 'obsidianBlack' },
  { name: '石墨灰', hex: '#161b22', border: '#30363d', key: 'graphiteGray' },
  { name: '极简白', hex: '#f6f8fa', border: '#d0d7de', key: 'minimalWhite' },
  { name: '暖阳米', hex: '#fef3c7', border: '#fde68a', key: 'warmBeige' },
  { name: '抹茶绿', hex: '#dcfce7', border: '#bbf7d0', key: 'matchaGreen' },
  { name: '冰川蓝', hex: '#e0f2fe', border: '#bae6fd', key: 'glacierBlue' },
  { name: '丁香紫', hex: '#f3e8ff', border: '#e9d5ff', key: 'lilacPurple' },
  { name: '柔粉桃', hex: '#ffe4e6', border: '#fecdd3', key: 'softPeach' },
];

export function getColorName(hexOrName: string): string {
  const item = PRESET_COLORS.find(
    (c) => c.hex.toLowerCase() === hexOrName.toLowerCase() || c.name === hexOrName
  );
  if (item && item.key) {
    return t(`colors.${item.key}`);
  }
  return hexOrName || t('colors.customColor');
}

export function hexToRgb(hex: string): { r: number; g: number; b: number } | null {
  if (!hex) return null;
  let h = hex.replace('#', '').trim();
  if (h.length === 3) {
    h = h.split('').map((c) => c + c).join('');
  }
  if (h.length !== 6) return null;
  const num = parseInt(h, 16);
  if (isNaN(num)) return null;
  return {
    r: (num >> 16) & 255,
    g: (num >> 8) & 255,
    b: num & 255,
  };
}

export function isLightColor(bgcolor: string): boolean {
  const rgb = hexToRgb(bgcolor);
  if (!rgb) return false;
  const luminance = 0.299 * rgb.r + 0.587 * rgb.g + 0.114 * rgb.b;
  return luminance > 140;
}

/**
 * 跨主题同系色对齐字典（解决 Default 与 Simple 皮肤历史色值 #0d1117/#0b0f14 与 #f6f8fa/#fafaf9 差异）
 */
export function isColorMatch(c1: string | undefined | null, c2: string | undefined | null): boolean {
  if (!c1 || !c2) return false;
  const norm1 = c1.trim().toLowerCase();
  const norm2 = c2.trim().toLowerCase();
  if (norm1 === norm2) return true;

  const themeAliases: Record<string, string> = {
    '#0b0f14': '#0d1117',
    '#0d1117': '#0b0f14',
    '#fafaf9': '#f6f8fa',
    '#f6f8fa': '#fafaf9',
  };

  return themeAliases[norm1] === norm2;
}

export function escapeHtml(text: string): string {
  return text
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#039;');
}

export function escapeRegex(text: string): string {
  return text.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

export function highlightKeyword(text?: string, keyword?: string): string {
  if (!text) return '';
  if (!keyword || !keyword.trim()) return escapeHtml(text);
  const escaped = escapeRegex(keyword.trim());
  const reg = new RegExp(`(${escaped})`, 'gi');
  return escapeHtml(text).replace(reg, '<span class="highlight-match">$1</span>');
}

export function formatRelativeTime(ts?: number, defaultText?: string, justNowPrefix?: string): string {
  if (!ts || ts === 0) return defaultText || t('time.updated');
  const now = Date.now();
  const diff = now - ts;
  if (diff < 0) return justNowPrefix || t('time.justNow');
  if (diff < 60 * 1000) return justNowPrefix || t('time.justNow');
  if (diff < 3600 * 1000) return t('time.minutesAgo', { n: Math.floor(diff / 60000) });
  if (diff < 86400 * 1000) return t('time.hoursAgo', { n: Math.floor(diff / 3600000) });
  if (diff < 7 * 86400 * 1000) return t('time.daysAgo', { n: Math.floor(diff / 86400000) });
  const d = new Date(ts);
  const month = (d.getMonth() + 1).toString().padStart(2, '0');
  const day = d.getDate().toString().padStart(2, '0');
  const hours = d.getHours().toString().padStart(2, '0');
  const minutes = d.getMinutes().toString().padStart(2, '0');
  return `${month}-${day} ${hours}:${minutes}`;
}

export function formatDeletedTime(ts?: number): string {
  return formatRelativeTime(ts, t('time.inTrash'), t('time.justDeleted'));
}

export function formatArchivedTime(ts?: number): string {
  return formatRelativeTime(ts, t('time.archived'), t('time.justArchived'));
}
