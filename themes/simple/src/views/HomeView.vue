<script setup lang="ts" name="Home">
import { computed, onMounted, ref, nextTick } from 'vue'
import draggable from 'vuedraggable'
import NoteItem from './NoteItem.vue'
import { Note, App } from '@/utils'
import { handleFormatKeydown } from '@/utils/format'
import { t } from '@/locales'
import { ElMessage } from 'element-plus'

const lock = ref(false)
const dataList = ref<Note[]>([])
const content = ref('')
const textareaRef = ref<HTMLTextAreaElement | null>(null)

onMounted(() => {
  App.on('data', (data) => {
    dataList.value = (data || []).map((it: any) => ({
      id: it.id,
      content: it.content,
      finish: it.finish ?? it.finished ?? false,
      editable: it.editable,
    }))
  })
  App.on('lock', (data) => {
    lock.value = data
  })
  App.init()
})

const sortData = computed(() => {
  const sortd = [...dataList.value]
  return sortd.sort((a, b) => {
    if (a.finish && !b.finish) return 1
    if (!a.finish && b.finish) return -1
    return 0
  })
})

const pendingCount = computed(() => dataList.value.filter((i) => !i.finish).length)

function adjustTextareaHeight() {
  if (textareaRef.value) {
    textareaRef.value.style.height = 'auto'
    textareaRef.value.style.height = `${textareaRef.value.scrollHeight}px`
  }
}

function add() {
  const text = content.value.trim()
  if (!text) return
  const data = new Note(text)
  dataList.value.unshift(data)
  content.value = ''
  if (textareaRef.value) {
    textareaRef.value.style.height = 'auto'
  }
  Note.add(data)
}

function onKeydown(e: KeyboardEvent) {
  if (handleFormatKeydown(e, textareaRef.value, content.value, (val) => {
    content.value = val
    nextTick(adjustTextareaHeight)
  })) {
    return
  }

  if (e.key === 'Enter' && !e.shiftKey) {
    e.preventDefault()
    add()
  } else if (e.key === 'Escape') {
    (e.target as HTMLElement)?.blur?.()
  } else {
    nextTick(adjustTextareaHeight)
  }
}

function onMainMouseDown(e: MouseEvent) {
  const target = e.target as HTMLElement | null
  if (target && !target.closest('.quick-add-box') && !target.closest('.note-item-card')) {
    (document.activeElement as HTMLElement)?.blur?.()
  }
}

function remove(item: Note) {
  const index = dataList.value.findIndex((a) => a.id === item.id)
  if (index > -1) {
    dataList.value.splice(index, 1)
    Note.remove(item)
  }
}

function hide() {
  App.hide()
}

function clearFinished() {
  const finishedItems = dataList.value.filter((i) => i.finish)
  if (finishedItems.length === 0) {
    ElMessage({
      message: t('noFinished'),
      type: 'info',
      duration: 1500,
      offset: 36,
    })
    return
  }
  dataList.value = dataList.value.filter((i) => !i.finish)
  Note.updateAll(dataList.value)
  ElMessage({
    message: t('cleanedFinished', { n: finishedItems.length }),
    type: 'success',
    duration: 1500,
    offset: 36,
  })
}

function onSort({ moved }: any) {
  let { oldIndex, newIndex } = moved
  oldIndex = dataList.value.findIndex((a) => a.id === sortData.value[oldIndex].id)
  newIndex = dataList.value.findIndex((a) => a.id === sortData.value[newIndex].id)
  const data = dataList.value[oldIndex]
  dataList.value.splice(oldIndex, 1)
  dataList.value.splice(newIndex, 0, data)
  Note.updateAll(dataList.value)
}
</script>

<template>
  <main class="flex-1 flex flex-col min-h-0 overflow-hidden" @mousedown="onMainMouseDown">
    <!-- 事项列表区（支持拖拽排序） -->
    <div class="flex-1 overflow-y-auto px-0 py-0">
      <draggable
        :model-value="sortData"
        group="note"
        handle=".drag-handle"
        tag="div"
        itemKey="id"
        animation="150"
        ghostClass="drag-ghost"
        @change="onSort"
      >
        <template #item="{ element: row }">
          <NoteItem
            :model-value="row"
            :lock="lock"
            @update:model-value="(d: Note) => (dataList[dataList.findIndex((a) => d.id === a.id)] = d)"
            @remove="remove(row)"
          />
        </template>
      </draggable>

      <div
        v-if="dataList.length === 0 && !lock"
        class="flex flex-col items-center justify-center py-8 text-gray-500 text-xs select-none opacity-50"
      >
        <i class="ri-sticky-note-line text-xl mb-1.5"></i>
        <span>{{ t('empty') }}</span>
      </div>
    </div>

    <!-- 快捷输入栏 -->
    <div v-if="!lock" class="quick-add-box" @mousedown.stop>
      <textarea
        ref="textareaRef"
        rows="1"
        v-model="content"
        :placeholder="t('addPlaceholder')"
        @input="adjustTextareaHeight"
        @keydown="onKeydown"
        @mousedown.stop
      />
      <el-tooltip placement="top" :content="t('addBtn')" :show-after="300">
        <button class="add-btn" @click="add" @mousedown.stop>
          <i class="ri-add-line text-sm"></i>
        </button>
      </el-tooltip>
    </div>

    <footer
      v-if="!lock && dataList.length > 0"
      class="px-2.5 py-1 flex items-center justify-between border-t border-[var(--card-border)] select-none text-[11px]"
    >
      <span class="text-[var(--text-muted)]">
        {{ pendingCount > 0 ? t('pending', { n: pendingCount }) : t('allDone') }}
      </span>

      <el-tooltip placement="top" :content="t('clearFinished')" :show-after="300">
        <button
          class="clear-btn text-[10px] text-gray-500 hover:text-blue-500 active:text-blue-600 transition-colors px-1 py-0.5 rounded cursor-pointer select-none"
          @mousedown.stop
          @click.stop="clearFinished"
        >
          {{ t('clearFinished') }}
        </button>
      </el-tooltip>
    </footer>
  </main>
</template>
