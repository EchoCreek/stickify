// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------

export type FormatType = 'bold' | 'italic' | 'underline' | 'strike'

/**
 * 针对 textarea 应用富文本格式（加粗、倾斜、下划线、删除线）
 * 支持：选中文本包裹、无选区光标居中插入、已包裹选区智能解除 (Toggle)
 */
export function applyFormat(
  textarea: HTMLTextAreaElement,
  type: FormatType,
  currentValue: string,
  updateValue: (val: string) => void
): boolean {
  const start = textarea.selectionStart
  const end = textarea.selectionEnd
  const selected = currentValue.substring(start, end)

  let prefix = ''
  let suffix = ''

  switch (type) {
    case 'bold':
      prefix = '**'
      suffix = '**'
      break
    case 'italic':
      prefix = '*'
      suffix = '*'
      break
    case 'underline':
      prefix = '<u>'
      suffix = '</u>'
      break
    case 'strike':
      prefix = '~~'
      suffix = '~~'
      break
  }

  // 1. 如果选中内容本身已经被该格式完全包裹，例如选中的是 "**hello**" 或 "<u>hello</u>" -> 剥离解开
  if (
    selected.length >= prefix.length + suffix.length &&
    selected.startsWith(prefix) &&
    selected.endsWith(suffix)
  ) {
    const unformatted = selected.substring(prefix.length, selected.length - suffix.length)
    const newVal = currentValue.substring(0, start) + unformatted + currentValue.substring(end)
    updateValue(newVal)
    setTimeout(() => {
      textarea.focus()
      textarea.setSelectionRange(start, start + unformatted.length)
    }, 0)
    return true
  }

  // 2. 如果选区外侧刚好是该格式的前后包裹符，例如 "**|hello|**" -> 消除外部包裹符
  if (
    start >= prefix.length &&
    end + suffix.length <= currentValue.length &&
    currentValue.substring(start - prefix.length, start) === prefix &&
    currentValue.substring(end, end + suffix.length) === suffix
  ) {
    const newVal =
      currentValue.substring(0, start - prefix.length) +
      selected +
      currentValue.substring(end + suffix.length)
    updateValue(newVal)
    setTimeout(() => {
      textarea.focus()
      textarea.setSelectionRange(start - prefix.length, end - prefix.length)
    }, 0)
    return true
  }

  // 3. 正常包裹格式
  if (selected.length === 0) {
    // 未选中文字：插入 prefix + suffix 并将光标置于中间，方便用户直接键入
    const newVal = currentValue.substring(0, start) + prefix + suffix + currentValue.substring(end)
    updateValue(newVal)
    setTimeout(() => {
      textarea.focus()
      textarea.setSelectionRange(start + prefix.length, start + prefix.length)
    }, 0)
  } else {
    // 选中了文字：包裹选中文本并保持选中状态
    const formatted = prefix + selected + suffix
    const newVal = currentValue.substring(0, start) + formatted + currentValue.substring(end)
    updateValue(newVal)
    setTimeout(() => {
      textarea.focus()
      textarea.setSelectionRange(start, start + formatted.length)
    }, 0)
  }

  return true
}

/**
 * 监听并处理键盘快捷键：
 * - 加粗 (Bold): Ctrl + B / Cmd + B
 * - 倾斜 (Italic): Ctrl + I / Cmd + I
 * - 下划线 (Underline): Ctrl +扩大 U / Cmd + U
 * - 删除线 (Strikethrough): Ctrl + Shift + S / Ctrl + Shift + X / Alt + Shift + 5
 */
export function handleFormatKeydown(
  e: KeyboardEvent,
  textarea: HTMLTextAreaElement | null,
  currentValue: string,
  updateValue: (val: string) => void
): boolean {
  if (!textarea) return false

  const isCtrlOrCmd = e.ctrlKey || e.metaKey
  const key = e.key.toLowerCase()

  // 1. 加粗: Ctrl + B
  if (isCtrlOrCmd && !e.shiftKey && !e.altKey && key === 'b') {
    e.preventDefault()
    e.stopPropagation()
    return applyFormat(textarea, 'bold', currentValue, updateValue)
  }

  // 2. 倾斜: Ctrl + I
  if (isCtrlOrCmd && !e.shiftKey && !e.altKey && key === 'i') {
    e.preventDefault()
    e.stopPropagation()
    return applyFormat(textarea, 'italic', currentValue, updateValue)
  }

  // 3. 下划线: Ctrl + U
  if (isCtrlOrCmd && !e.shiftKey && !e.altKey && key === 'u') {
    e.preventDefault()
    e.stopPropagation()
    return applyFormat(textarea, 'underline', currentValue, updateValue)
  }

  // 4. 删除线: Ctrl + Shift + S 或 Ctrl + Shift + X 或 Alt + Shift + 5
  if (
    (isCtrlOrCmd && e.shiftKey && (key === 's' || key === 'x')) ||
    (e.altKey && e.shiftKey && e.key === '5')
  ) {
    e.preventDefault()
    e.stopPropagation()
    return applyFormat(textarea, 'strike', currentValue, updateValue)
  }

  return false
}
