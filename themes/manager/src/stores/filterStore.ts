// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
import { defineStore } from 'pinia';
import { ref, watch } from 'vue';

export const useFilterStore = defineStore('filter', () => {
  const activeFilter = ref('all');
  const colorFilter = ref('');
  const searchQuery = ref('');
  const debouncedSearchQuery = ref('');
  const viewMode = ref<'grid' | 'table'>('grid');
  const sortBy = ref('name_desc');

  let debounceTimer: ReturnType<typeof setTimeout> | null = null;

  watch(searchQuery, (newVal) => {
    if (debounceTimer) {
      clearTimeout(debounceTimer);
      debounceTimer = null;
    }
    if (!newVal) {
      debouncedSearchQuery.value = '';
      return;
    }
    debounceTimer = setTimeout(() => {
      debouncedSearchQuery.value = newVal;
    }, 150);
  });

  function resetFilters() {
    activeFilter.value = 'all';
    colorFilter.value = '';
    searchQuery.value = '';
    debouncedSearchQuery.value = '';
  }

  return {
    activeFilter,
    colorFilter,
    searchQuery,
    debouncedSearchQuery,
    viewMode,
    sortBy,
    resetFilters,
  };
});
