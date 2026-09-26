<script setup lang="ts">
import { onMounted, ref, watch, nextTick } from 'vue'
import { Config, App } from '@/utils'
import { t, setLanguage } from '@/locales'
import HomeView from './views/HomeView.vue'

const lock = ref(false)

const defaultColor = '#0d1117'
const setting = ref({
  bgcolor: defaultColor,
  topmost: true,
  opacityable: false,
  opacity: 50,
  title: '',
  language: 'zh-CN',
})

// 现代扁平极简预设色盘
const presetColors = [
  { name: '曜石黑', hex: '#0d1117', border: '#30363d' },
  { name: '石墨灰', hex: '#161b22', border: '#30363d' },
  { name: '极简白', hex: '#f6f8fa', border: '#d0d7de' },
  { name: '暖阳米', hex: '#fef3c7', border: '#fde68a' },
  { name: '抹茶绿', hex: '#dcfce7', border: '#bbf7d0' },
  { name: '冰川蓝', hex: '#e0f2fe', border: '#bae6fd' },
  { name: '丁香紫', hex: '#f3e8ff', border: '#e9d5ff' },
  { name: '柔粉桃', hex: '#ffe4e6', border: '#fecdd3' },
]

const colorPopoverVisible = ref(false)
const isEditingTitle = ref(false)
const tempTitle = ref('')
const titleInputRef = ref<HTMLInputElement | null>(null)

function startEditTitle() {
  if (lock.value) return
  tempTitle.value = setting.value.title || ''
  isEditingTitle.value = true
  nextTick(() => {
    titleInputRef.value?.focus()
    titleInputRef.value?.select()
  })
}

function saveEditTitle() {
  const newTitle = tempTitle.value.trim()
  setting.value.title = newTitle
  Config.title(newTitle)
  isEditingTitle.value = false
}

function cancelEditTitle() {
  tempTitle.value = setting.value.title || ''
  isEditingTitle.value = false
}

function isLightColor(bgcolor: string) {
  if (!bgcolor || bgcolor.length < 7) return false
  const R = parseInt(bgcolor.slice(1, 3), 16)
  const G = parseInt(bgcolor.slice(3, 5), 16)
  const B = parseInt(bgcolor.slice(5, 7), 16)
  const luminance = 0.299 * R + 0.587 * G + 0.114 * B
  return luminance > 140
}

function updateThemeMode(color: string) {
  const isLight = isLightColor(color)
  document.documentElement.classList.toggle('light-theme', isLight)
  document.documentElement.classList.toggle('dark', !isLight)
  document.documentElement.style.setProperty('--background-color', color || '#0d1117')
}

const edgeHidden = ref(false)
const edgeSide = ref<'left' | 'right' | 'top' | 'none'>('none')
const edgeLocked = ref(false)
const isLocating = ref(false)
let locateTimer: number | null = null

function onTabHover() {
  App.restoreDock()
}

function toggleEdgeLock() {
  edgeLocked.value = !edgeLocked.value
  App.lockEdge(edgeLocked.value)
}

onMounted(() => {
  App.on('setting', (data) => {
    setting.value = data
    if (data.bgcolor) {
      updateThemeMode(data.bgcolor)
    }
    if (data.language) {
      setLanguage(data.language)
    }
  })
  App.on('lock', (data) => {
    lock.value = data
  })
  App.on('edge_state', (data) => {
    if (data) {
      edgeHidden.value = !!data.hidden
      edgeSide.value = data.edge || 'none'
      if (typeof data.locked === 'boolean') {
        edgeLocked.value = data.locked
      }
    }
  })
  App.on('locate_pulse', () => {
    isLocating.value = true
    if (locateTimer) clearTimeout(locateTimer)
    locateTimer = window.setTimeout(() => {
      isLocating.value = false
    }, 1400)
  })
})

function selectPresetColor(hex: string) {
  setting.value.bgcolor = hex
  Config.bgcolor(hex)
  updateThemeMode(hex)
  colorPopoverVisible.value = false
}

function onCustomColorChange(val: string) {
  if (val) {
    setting.value.bgcolor = val
    Config.bgcolor(val)
    updateThemeMode(val)
  }
}

function startMove(e: MouseEvent) {
  if (e.button !== 0 || lock.value || edgeLocked.value) return

  const target = e.target as HTMLElement
  if (
    target &&
    target.closest(
      'button, input, textarea, a, .el-popover, .el-popper, .drag-handle, .item-text, .markdown-body, .el-popconfirm, .el-color-picker'
    )
  ) {
    return
  }

  e.preventDefault()
  App.move(true)

  const stopMove = () => {
    App.move(false)
    window.removeEventListener('mouseup', stopMove)
    window.removeEventListener('blur', stopMove)
  }

  window.addEventListener('mouseup', stopMove)
  window.addEventListener('blur', stopMove)
}

function onContainerMouseDown(e: MouseEvent) {
  (document.activeElement as HTMLElement)?.blur?.()
  startMove(e)
}

function onResizeStart(direction: string, e: MouseEvent) {
  if (e.button !== 0 || lock.value) return
  e.preventDefault()
  e.stopPropagation()
  App.resize(direction)
}

watch(
  () => setting.value.bgcolor,
  (val) => {
    if (val) {
      updateThemeMode(val)
    }
  }
)
</script>

<template>
  <!-- 桌面单个便签窗口容器 -->
  <div class="note-window-wrapper w-full h-full relative overflow-hidden">
    <!-- 便签主体容器：贴边时 100% 完全滑出桌面，零像素残留 -->
    <div
      class="note-container select-none"
      :class="{
        'mouse-lock': lock,
        'is-edge-hidden-left': edgeHidden && edgeSide === 'left',
        'is-edge-hidden-right': edgeHidden && edgeSide === 'right',
        'is-edge-hidden-top': edgeHidden && edgeSide === 'top',
        'is-locating': isLocating,
      }"
      @mousedown="onContainerMouseDown"
    >
      <!-- 八方向边缘与四角缩放手柄（穿透模式下自动禁用） -->
      <template v-if="!lock">
        <div class="resize-handle resize-top" @mousedown.prevent.stop="onResizeStart('top', $event)" />
        <div class="resize-handle resize-bottom" @mousedown.prevent.stop="onResizeStart('bottom', $event)" />
        <div class="resize-handle resize-left" @mousedown.prevent.stop="onResizeStart('left', $event)" />
        <div class="resize-handle resize-right" @mousedown.prevent.stop="onResizeStart('right', $event)" />
        <div class="resize-handle resize-top-left" @mousedown.prevent.stop="onResizeStart('top-left', $event)" />
        <div class="resize-handle resize-top-right" @mousedown.prevent.stop="onResizeStart('top-right', $event)" />
        <div class="resize-handle resize-bottom-left" @mousedown.prevent.stop="onResizeStart('bottom-left', $event)" />
        <div class="resize-handle resize-bottom-right" @mousedown.prevent.stop="onResizeStart('bottom-right', $event)" />
      </template>

      <!-- 头部平铺导航栏：当处于锁定模式且无标题时隐藏，有标题时展示并以实线分割 -->
      <header
        v-if="!lock || setting.title"
        class="note-header"
        :style="{ backgroundColor: lock ? 'transparent' : setting.bgcolor }"
      >
        <!-- 正常模式：标题区域（单击按住拖拽便签，双击进入就地编辑） -->
        <div v-if="!lock" class="flex items-center flex-1 min-w-0 mr-1.5 overflow-hidden">
          <input
            v-if="isEditingTitle"
            ref="titleInputRef"
            class="title-input"
            v-model="tempTitle"
            :placeholder="t('titlePlaceholder')"
            @mousedown.stop
            @keydown.enter="saveEditTitle"
            @keydown.esc="cancelEditTitle"
            @blur="saveEditTitle"
          />
          <div
            v-else
            class="title-display flex-1 min-w-0 cursor-default truncate"
            :class="{ 'opacity-40 italic font-normal': !setting.title }"
            @dblclick.stop="startEditTitle"
            :title="t('titleHint')"
          >
            {{ setting.title || t('doubleClickTitle') }}
          </div>
        </div>

        <!-- 锁定穿透模式：展示纯文本标题（与输入框完全重合无跳动） -->
        <div v-else-if="setting.title" class="title-display flex-1 min-w-0 truncate">
          {{ setting.title }}
        </div>

        <!-- 正常模式：操作工具组（仅保留 3 个核心动作：主题与设置、穿透、关闭） -->
        <div v-if="!lock" class="flex items-center space-x-1" @mousedown.stop>
          <!-- 调色盘与偏好设置气泡弹窗 -->
          <el-popover
            v-model:visible="colorPopoverVisible"
            trigger="click"
            placement="bottom-end"
            :width="200"
            popper-class="color-picker-popover"
          >
            <template #reference>
              <el-tooltip placement="bottom" :content="t('settingsTitle')" :show-after="300" :disabled="colorPopoverVisible">
                <button class="icon-btn" @mousedown.stop>
                  <span
                    class="w-3.5 h-3.5 rounded-sm border inline-block"
                    :style="{ backgroundColor: setting.bgcolor, borderColor: isLightColor(setting.bgcolor) ? 'rgba(0,0,0,0.15)' : 'rgba(255,255,255,0.2)' }"
                  ></span>
                </button>
              </el-tooltip>
            </template>
            <div class="p-1" @mousedown.stop>
              <div class="text-[11px] font-medium text-gray-400 mb-1.5 px-0.5">{{ t('presetColors') }}</div>
              <div class="grid grid-cols-4 gap-1.5 mb-2.5">
                <button
                  v-for="item in presetColors"
                  :key="item.hex"
                  @click="selectPresetColor(item.hex)"
                  class="w-7 h-7 rounded border flex items-center justify-center transition-all hover:scale-105"
                  :style="{ backgroundColor: item.hex, borderColor: item.border }"
                  :title="item.name"
                >
                  <i
                    v-if="setting.bgcolor.toLowerCase() === item.hex.toLowerCase()"
                    class="ri-check-line text-[11px]"
                    :class="isLightColor(item.hex) ? 'text-gray-800' : 'text-white'"
                  ></i>
                </button>
              </div>

              <div class="flex items-center justify-between py-1.5 border-t border-white/10 px-0.5">
                <span class="text-[11px] text-gray-300">{{ t('customColor') }}</span>
                <el-color-picker
                  size="small"
                  v-model="setting.bgcolor"
                  @change="onCustomColorChange"
                />
              </div>

              <!-- 便签偏好设置：置顶与半透明 -->
              <div class="pt-1.5 border-t border-white/10 flex flex-col gap-1 px-0.5">
                <div class="flex items-center justify-between text-[11px] text-gray-300">
                  <span>{{ t('topmost') }}</span>
                  <button
                    class="px-2 py-0.5 rounded text-[10px] border transition-colors"
                    :class="setting.topmost ? 'bg-blue-600 border-blue-500 text-white' : 'border-gray-600 text-gray-400'"
                    @click="Config.top((setting.topmost = !setting.topmost))"
                  >
                    {{ setting.topmost ? t('enabled') : t('disabled') }}
                  </button>
                </div>
                <div class="flex items-center justify-between text-[11px] text-gray-300">
                  <span>{{ t('opacityable') }}</span>
                  <button
                    class="px-2 py-0.5 rounded text-[10px] border transition-colors"
                    :class="setting.opacityable ? 'bg-blue-600 border-blue-500 text-white' : 'border-gray-600 text-gray-400'"
                    @click="Config.opacityable((setting.opacityable = !setting.opacityable))"
                  >
                    {{ setting.opacityable ? t('enabled') : t('disabled') }}
                  </button>
                </div>
              </div>
            </div>
          </el-popover>

          <!-- 贴边状态下的锁定按钮 VS 正常状态下的穿透模式按钮 -->
          <template v-if="edgeSide !== 'none'">
            <el-tooltip
              placement="bottom"
              :content="edgeLocked ? t('unlockEdge') : t('lockEdge')"
              :show-after="300"
            >
              <button
                class="icon-btn"
                :class="{ 'text-blue-400 bg-blue-500/20 border-blue-400/40': edgeLocked }"
                @mousedown.stop
                @click="toggleEdgeLock"
              >
                <i :class="edgeLocked ? 'ri-lock-fill' : 'ri-lock-unlock-line'" class="text-[12px]"></i>
              </button>
            </el-tooltip>
          </template>
          <template v-else>
            <!-- 开启鼠标穿透模式 -->
            <el-tooltip placement="bottom" :content="t('penetrate')" :show-after="300">
              <button class="icon-btn" @mousedown.stop @click="Config.lock(true); lock = true">
                <i class="ri-lock-line text-[12px]"></i>
              </button>
            </el-tooltip>
          </template>

          <!-- 关闭便签 -->
          <el-tooltip placement="bottom" :content="t('closeNote')" :show-after="300">
            <button class="icon-btn danger" @mousedown.stop @click="App.close()">
              <i class="ri-close-line text-sm"></i>
            </button>
          </el-tooltip>
        </div>
      </header>

      <!-- 单个便签待办主体视图 -->
      <HomeView class="flex-1 overflow-y-auto" />
    </div>

    <!-- 独立的边缘指向性胶囊拉手（完全脱离 note-container，纯净无重叠） -->
    <div
      class="edge-dock-tab"
      :class="[`edge-${edgeSide}`, { 'is-visible': edgeHidden, 'is-locating': isLocating }]"
      @mouseenter="onTabHover"
      @click="onTabHover"
      title="触碰还原便签"
    >
      <div class="edge-dock-content">
        <i class="ri-sticky-note-line tab-note-icon text-sm"></i>
        <i
          :class="edgeSide === 'left' ? 'ri-arrow-right-s-line' : (edgeSide === 'right' ? 'ri-arrow-left-s-line' : 'ri-arrow-down-s-line')"
          class="tab-chevron-icon text-xs"
        ></i>
      </div>
    </div>
  </div>
</template>

<style lang="scss">
.color-picker-popover {
  background: #161b22 !important;
  border: 1px solid #30363d !important;
  border-radius: 6px !important;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3) !important;
  padding: 8px !important;
}

.light-theme .color-picker-popover {
  background: #ffffff !important;
  border-color: #d0d7de !important;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.08) !important;

  .text-gray-400 {
    color: #656d76 !important;
  }

  .border-white\/10 {
    border-color: #e1e4e8 !important;
  }
}
</style>
