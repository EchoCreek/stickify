<script setup lang="ts" name="Note">
import { ref, watch, nextTick, onUnmounted } from 'vue'
import { marked } from 'marked'
import { Note } from '@/utils'
import { handleFormatKeydown } from '@/utils/format'
import { t } from '@/locales'
import { ElMessage } from 'element-plus'

const props = defineProps<{
  modelValue: Note
  lock?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [val: Note]
  'remove': []
}>()

const data = ref<Note>(props.modelValue || new Note())
const isEditing = ref(false)
const editText = ref('')
const textareaRef = ref<HTMLTextAreaElement | null>(null)
const editBoxRef = ref<HTMLElement | null>(null)

function handleGlobalClick(e: MouseEvent) {
  if (!isEditing.value) return
  const target = e.target as HTMLElement | null
  if (editBoxRef.value && !editBoxRef.value.contains(target)) {
    saveEdit()
  }
}

watch(isEditing, (val) => {
  if (val) {
    setTimeout(() => {
      window.addEventListener('mousedown', handleGlobalClick)
    }, 0)
  } else {
    window.removeEventListener('mousedown', handleGlobalClick)
  }
})

onUnmounted(() => {
  window.removeEventListener('mousedown', handleGlobalClick)
})

watch(() => props.modelValue, (val) => {
  data.value = val
})

watch(() => data.value, (val) => {
  emit('update:modelValue', val)
}, { deep: true })

marked.use({
  silent: true,
  gfm: true,
  breaks: true,
})

function renderMarkdown(content: string) {
  if (!content) return ''
  const sanitized = content
    .replace(/<script\b[^<]*(?:(?!<\/script>)<[^<]*)*<\/script>/gi, '')
    .replace(/href\s*=\s*["']?javascript:[^"'>]*/gi, 'href="#"')
  return marked(sanitized)
}

function toggleFinish() {
  if (props.lock || isEditing.value) return
  data.value.finish = !data.value.finish
  Note.update(data.value)
}

function adjustTextareaHeight() {
  if (textareaRef.value) {
    textareaRef.value.style.height = 'auto'
    textareaRef.value.style.height = `${textareaRef.value.scrollHeight}px`
  }
}

function startEdit() {
  if (props.lock) return
  editText.value = data.value.content
  isEditing.value = true
  nextTick(() => {
    if (textareaRef.value) {
      adjustTextareaHeight()
      textareaRef.value.focus()
      const len = textareaRef.value.value.length
      textareaRef.value.setSelectionRange(len, len)
    }
  })
}

function saveEdit() {
  const text = editText.value.trim()
  if (text) {
    data.value.content = text
    Note.update(data.value)
  }
  isEditing.value = false
}

function cancelEdit() {
  isEditing.value = false
  editText.value = ''
}

function onEditKeydown(e: KeyboardEvent) {
  if (handleFormatKeydown(e, textareaRef.value, editText.value, (val) => {
    editText.value = val
    nextTick(adjustTextareaHeight)
  })) {
    return
  }

  if (e.key === 'Enter' && !e.shiftKey) {
    e.preventDefault()
    saveEdit()
  } else if (e.key === 'Escape') {
    cancelEdit()
  } else {
    nextTick(adjustTextareaHeight)
  }
}

function exportToCalendar() {
  Note.makeTask(data.value)
  ElMessage({
    message: 'Task Exported',
    type: 'success',
    duration: 2000,
    offset: 36,
  })
}
</script>

<template>
  <div
    class="note-item-card group"
    :class="{
      'is-finished': data.finish,
      'cursor-pointer': !props.lock && !isEditing,
    }"
    @click="toggleFinish"
  >
    <div class="flex items-start gap-1.5 min-w-0 flex-1 pr-12">
      <!-- 极简拖拽把手（位于复选框左侧，鼠标悬停时优雅淡入） -->
      <button
        v-if="!props.lock"
        type="button"
        class="drag-handle mt-0.5 w-3 h-3.5 flex items-center justify-center text-[var(--text-muted)] opacity-0 group-hover:opacity-60 hover:!opacity-100 transition-opacity cursor-grab active:cursor-grabbing shrink-0"
        title="Drag to sort"
      >
        <i class="ri-draggable text-xs"></i>
      </button>

      <button
        type="button"
        class="mt-0.5 w-3.5 h-3.5 rounded-full border flex items-center justify-center transition-all shrink-0"
        :class="[
          data.finish
            ? 'bg-blue-500 border-blue-500 text-white'
            : 'border-[var(--text-muted)] hover:border-blue-400 bg-transparent',
          props.lock ? 'pointer-events-none' : 'cursor-pointer',
        ]"
        @mousedown.stop
        @click.stop="toggleFinish"
      >
        <i
          v-if="data.finish"
          class="ri-check-line text-[10px]"
        ></i>
      </button>

      <div
        class="flex-1 min-w-0 text-[13px] leading-relaxed break-words item-text"
        :class="{ 'cursor-text': !props.lock && !isEditing }"
        @mousedown.stop
      >
        <div
          v-if="!isEditing"
          class="markdown-body inline"
          @click.stop="startEdit"
          v-html="renderMarkdown(data.content || '')"
        ></div>

        <div v-else ref="editBoxRef" class="w-full">
          <textarea
            ref="textareaRef"
            v-model="editText"
            rows="1"
            class="inline-edit-textarea"
            :placeholder="t('titlePlaceholder')"
            @input="adjustTextareaHeight"
            @mousedown.stop
            @keydown="onEditKeydown"
            @blur="saveEdit"
          />
        </div>
      </div>
    </div>

    <div v-if="!props.lock && !isEditing" class="item-actions" @mousedown.stop @click.stop="">
      <el-tooltip v-if="!data.finish" placement="top" content="Calendar" :show-after="300">
        <button
          type="button"
          class="item-action-btn"
          @mousedown.stop
          @click="exportToCalendar"
        >
          <i class="ri-calendar-event-line"></i>
        </button>
      </el-tooltip>

      <el-tooltip placement="top" :content="t('deleteItem')" :show-after="300">
        <button
          type="button"
          class="item-action-btn danger"
          @mousedown.stop
          @click="emit('remove')"
        >
          <i class="ri-close-line"></i>
        </button>
      </el-tooltip>
    </div>
  </div>
</template>

<style lang="scss" scoped>
.markdown-body {
  background: transparent !important;
  font-size: 13px !important;
  color: inherit !important;

  :deep(p) {
    margin-bottom: 0 !important;
    color: inherit !important;
  }

  :deep(a) {
    color: var(--accent-color) !important;
  }

  :deep(code) {
    background: var(--item-hover-bg) !important;
    border-radius: 3px;
    padding: 1px 3px;
    font-size: 11.5px;
  }
}
</style>
