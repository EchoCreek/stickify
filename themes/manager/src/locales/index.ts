// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
import { ref, computed } from 'vue';
import zhCnLocale from 'element-plus/dist/locale/zh-cn.mjs';
import enLocale from 'element-plus/dist/locale/en.mjs';
import { zhCN } from './zh-CN';
import { enUS } from './en-US';

export type SupportedLanguage = 'zh-CN' | 'en-US';
export type LocaleSchema = typeof zhCN;

const dictionaries: Record<SupportedLanguage, LocaleSchema> = {
  'zh-CN': zhCN,
  'en-US': enUS,
};

const currentLanguage = ref<SupportedLanguage>('zh-CN');

export function setLanguage(lang: SupportedLanguage | string) {
  if (lang === 'en-US' || lang === 'en') {
    currentLanguage.value = 'en-US';
  } else {
    currentLanguage.value = 'zh-CN';
  }
}

export function getLanguage(): SupportedLanguage {
  return currentLanguage.value;
}

/**
 * 强类型/动态路径翻译函数
 * 例如: t('settings.headerTitle') 或 t('batch.selectedCount', { n: 5 })
 */
export function t(path: string, params?: Record<string, string | number>): string {
  const dict = dictionaries[currentLanguage.value] || zhCN;
  const keys = path.split('.');
  let current: any = dict;

  for (const k of keys) {
    if (current && typeof current === 'object' && k in current) {
      current = current[k];
    } else {
      // 回退到 zh-CN
      let fallback: any = zhCN;
      for (const fk of keys) {
        if (fallback && typeof fallback === 'object' && fk in fallback) {
          fallback = fallback[fk];
        } else {
          return path;
        }
      }
      current = fallback;
      break;
    }
  }

  if (typeof current !== 'string') {
    return path;
  }

  if (!params) {
    return current;
  }

  return current.replace(/\{(\w+)\}/g, (_, key) => {
    return params[key] !== undefined ? String(params[key]) : `{${key}}`;
  });
}

export const elementPlusLocale = computed(() => {
  return currentLanguage.value === 'en-US' ? enLocale : zhCnLocale;
});

export function useI18n() {
  return {
    t,
    lang: currentLanguage,
    setLanguage,
    getLanguage,
    elementPlusLocale,
  };
}

export { zhCN, enUS };
