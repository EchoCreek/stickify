<!--
  Copyright (c) Sticky Notes Refactoring Team (2026)
  Part of Sticky Notes Refactoring Project
  Licensed under the Apache License, Version 2.0
-->
<template>
  <div class="w-full overflow-x-auto bg-[#161b22] rounded-lg border border-[#30363d] select-none">
    <table class="w-full text-left text-xs text-[#c9d1d9]">
      <thead class="bg-[#0d1117] text-[10px] font-semibold text-[#8b949e] uppercase tracking-wider border-b border-[#30363d]">
        <tr>
          <th class="p-2.5 w-10 text-center">
            <input
              type="checkbox"
              :checked="isAllSelected"
              @change="$emit('toggle-all')"
              class="w-3.5 h-3.5 rounded border-gray-600 text-blue-600 focus:ring-blue-500 cursor-pointer accent-blue-600"
            />
          </th>
          <th class="p-2.5 w-12 text-center">{{ t('noteTable.colColor') }}</th>
          <th class="p-2.5 font-medium">{{ t('noteTable.colTitle') }}</th>
          <th class="p-2.5 w-28">{{ t('noteTable.colItems') }}</th>
          <th v-if="isTrash" class="p-2.5 w-36 text-center">{{ t('noteTable.statusTrash') }}</th>
          <th v-else-if="isArchive" class="p-2.5 w-36 text-center">{{ t('noteTable.statusArchived') }}</th>
          <template v-else>
            <th class="p-2.5 w-28 text-center">{{ t('noteTable.colStatus') }}</th>
            <th class="p-2.5 w-16 text-center">{{ t('noteCard.topmost') }}</th>
          </template>
          <th class="p-2.5 w-36 text-right">{{ t('noteTable.colActions') }}</th>
        </tr>
      </thead>
      <tbody class="divide-y divide-[#21262d]">
        <tr
          v-for="note in notes"
          :key="note.name"
          @click="handleRowClick(note)"
          class="hover:bg-[#21262d] cursor-pointer transition-colors"
          :class="isSelected(note.name) ? 'bg-blue-600/10' : ''"
        >
          <!-- Checkbox -->
          <td class="p-2.5 text-center" @click.stop>
            <input
              type="checkbox"
              :checked="isSelected(note.name)"
              @change="$emit('toggle-select', note.name)"
              class="w-3.5 h-3.5 rounded border-gray-600 text-blue-600 focus:ring-blue-500 cursor-pointer accent-blue-600"
            />
          </td>

          <!-- Color Pill -->
          <td class="p-2.5 text-center">
            <span
              class="inline-block w-3.5 h-3.5 rounded border border-[#30363d]"
              :style="{ backgroundColor: note.bgColor || '#0d1117' }"
            ></span>
          </td>

          <!-- Title -->
          <td class="p-2.5 font-medium text-[#f0f6fc]">
            <div class="flex items-center gap-2 truncate">
              <span v-html="highlightKeyword(note.title || t('titleBar.newNote'), searchKeyword)"></span>
            </div>
          </td>

          <!-- Task Progress -->
          <td class="p-2.5">
            <div class="flex items-center gap-2">
              <div class="w-16 h-1 bg-[#21262d] rounded-full overflow-hidden shrink-0">
                <div
                  class="h-full bg-emerald-500 rounded-full"
                  :style="{ width: `${getProgress(note)}%` }"
                ></div>
              </div>
              <span class="text-[10px] text-[#8b949e] shrink-0">{{ note.completedCount }}/{{ note.itemCount }}</span>
            </div>
          </td>

          <!-- Trash Deleted Time vs Archive Time vs Normal Status/Topmost -->
          <td v-if="isTrash || note.isDeleted" class="p-2.5 text-center">
            <span class="inline-flex items-center gap-1 px-2 py-0.5 rounded text-[10px] font-semibold border bg-rose-500/15 text-rose-300 border-rose-500/30">
              <i class="ri-delete-bin-line text-[11px]"></i>
              {{ formatDeletedTime(note.deletedAt) }}
            </span>
          </td>
          <td v-else-if="isArchive || note.isArchived" class="p-2.5 text-center">
            <span class="inline-flex items-center gap-1 px-2 py-0.5 rounded text-[10px] font-semibold border bg-amber-500/15 text-amber-300 border-amber-500/30">
              <i class="ri-inbox-archive-line text-[11px]"></i>
              {{ formatArchivedTime(note.archivedAt) }}
            </span>
          </td>
          <template v-else>
            <!-- Status Badge -->
            <td class="p-2.5 text-center">
              <span
                class="inline-flex items-center gap-1 px-2 py-0.5 rounded text-[10px] font-semibold border"
                :class="note.visible ? 'bg-emerald-500/20 text-emerald-300 border-emerald-500/30' : 'bg-gray-800 text-gray-400 border-gray-700'"
              >
                <i :class="note.visible ? 'ri-computer-line' : 'ri-eye-off-line'" class="text-[11px]"></i>
                {{ note.visible ? t('noteCard.visible') : t('noteCard.hidden') }}
              </span>
            </td>

            <!-- Topmost -->
            <td class="p-2.5 text-center text-[#8b949e]">
              <i v-if="note.topMost" class="ri-pushpin-2-fill text-amber-400 text-sm" :title="t('noteCard.topmost')"></i>
              <span v-else class="text-gray-600">-</span>
            </td>
          </template>

          <!-- Actions -->
          <td class="p-2.5 text-right space-x-1" @click.stop>
            <template v-if="isTrash || note.isDeleted">
              <button
                @click="$emit('restore', note.name)"
                class="px-2 py-0.5 bg-[#0d1117] hover:bg-blue-500/20 text-[#c9d1d9] hover:text-blue-400 rounded text-[10px] border border-[#30363d] transition-colors"
                :title="t('noteCard.restore')"
              >
                <i class="ri-restart-line mr-0.5"></i>{{ t('common.reset') }}
              </button>
              <button
                @click="$emit('permanent-delete', note.name)"
                class="px-2 py-0.5 bg-[#0d1117] hover:bg-rose-500/20 text-gray-400 hover:text-rose-400 rounded text-[10px] border border-[#30363d] transition-colors"
                :title="t('noteCard.permanentDelete')"
              >
                <i class="ri-delete-bin-2-line mr-0.5"></i>{{ t('common.delete') }}
              </button>
            </template>
            <template v-else-if="isArchive || note.isArchived">
              <button
                @click="$emit('unarchive', note.name)"
                class="px-2 py-0.5 bg-[#0d1117] hover:bg-amber-500/20 text-[#c9d1d9] hover:text-amber-400 rounded text-[10px] border border-[#30363d] transition-colors"
                :title="t('noteCard.unarchive')"
              >
                <i class="ri-inbox-unarchive-line mr-0.5"></i>{{ t('noteCard.unarchive') }}
              </button>
              <button
                @click="$emit('delete', note.name)"
                class="px-2 py-0.5 bg-[#0d1117] hover:bg-rose-500/20 text-gray-400 hover:text-rose-400 rounded text-[10px] border border-[#30363d] transition-colors"
                :title="t('noteCard.moveToTrash')"
              >
                <i class="ri-delete-bin-line"></i>
              </button>
            </template>
            <template v-else>
              <button
                @click="$emit('locate', note.name)"
                class="px-2 py-0.5 bg-[#0d1117] hover:bg-[#21262d] text-[#c9d1d9] hover:text-white rounded text-[10px] border border-[#30363d] transition-colors"
                :title="t('noteCard.locate')"
              >
                <i class="ri-focus-3-line mr-0.5"></i>{{ t('detail.locateDesktop') }}
              </button>
              <button
                @click="$emit('toggle-visible', note)"
                class="px-2 py-0.5 bg-[#0d1117] hover:bg-[#21262d] text-[#c9d1d9] hover:text-white rounded text-[10px] border border-[#30363d] transition-colors"
              >
                <i :class="note.visible ? 'ri-eye-off-line' : 'ri-eye-line'" class="mr-0.5"></i>
                {{ note.visible ? t('noteCard.hidden') : t('noteCard.visible') }}
              </button>
              <button
                @click="$emit('archive', note.name)"
                class="px-2 py-0.5 bg-[#0d1117] hover:bg-amber-500/20 text-[#c9d1d9] hover:text-amber-400 rounded text-[10px] border border-[#30363d] transition-colors"
                :title="t('noteCard.archive')"
              >
                <i class="ri-inbox-archive-line mr-0.5"></i>{{ t('noteCard.archive') }}
              </button>
              <button
                @click="$emit('delete', note.name)"
                class="px-2 py-0.5 bg-[#0d1117] hover:bg-rose-500/20 text-gray-400 hover:text-rose-400 rounded text-[10px] border border-[#30363d] transition-colors"
                :title="t('noteCard.moveToTrash')"
              >
                <i class="ri-delete-bin-line"></i>
              </button>
            </template>
          </td>
        </tr>
      </tbody>
    </table>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import type { NoteSummary } from '../../api/managerApi';
import { formatDeletedTime, formatArchivedTime, highlightKeyword } from '../../utils';
import { useI18n } from '../../locales';

const props = defineProps<{
  notes: NoteSummary[];
  selectedNames: string[];
  searchKeyword?: string;
  isTrash?: boolean;
  isArchive?: boolean;
}>();

const { t } = useI18n();

const emit = defineEmits<{
  (e: 'select', note: NoteSummary): void;
  (e: 'toggle-select', name: string): void;
  (e: 'toggle-all'): void;
  (e: 'locate', name: string): void;
  (e: 'toggle-visible', note: NoteSummary): void;
  (e: 'toggle-topmost', note: NoteSummary): void;
  (e: 'archive', name: string): void;
  (e: 'unarchive', name: string): void;
  (e: 'delete', name: string): void;
  (e: 'restore', name: string): void;
  (e: 'permanent-delete', name: string): void;
}>();

function handleRowClick(note: NoteSummary) {
  if (props.isTrash || note.isDeleted || props.isArchive || note.isArchived) {
    emit('toggle-select', note.name);
  } else {
    emit('select', note);
  }
}

const isAllSelected = computed(() => {
  return props.notes.length > 0 && props.selectedNames.length === props.notes.length;
});

function isSelected(name: string): boolean {
  return props.selectedNames.includes(name);
}

function getProgress(note: NoteSummary): number {
  if (!note.itemCount || note.itemCount === 0) return 0;
  return Math.round((note.completedCount / note.itemCount) * 100);
}
</script>

<style scoped>
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
</style>
