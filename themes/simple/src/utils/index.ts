// ------------------------------------------------------------------
// Original Work Copyright (c) imlinhanchao
// https://github.com/imlinhanchao/sticky_notes
// Modified by Sticky Notes Refactoring Team (2026): Delegate to typed ipc.ts contract
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
import { isWebview, sendToNative, onNativeMessage, type WebMessageType, type WebMessage } from '../ipc';

export { isWebview, sendToNative, onNativeMessage };
export type { WebMessageType, WebMessage };

/**
 * 发送事件到客户端
 * @param event 事件
 * @param data 数据
 */
export function send(event: WebMessageType, data: any = null) {
  sendToNative(event, data);
}

/**
 * 监听客户端返回数据
 * @param callback 监听函数
 * @returns 
 */
export function listen(callback: (event: string, data: any) => void) {
  return onNativeMessage((msg: WebMessage) => {
    if (msg && msg.event !== undefined) {
      callback(msg.event, msg.data);
    }
  });
}

/**
 * 便签操作类
 */
export class App {
  static listener: { [key: string]: ((data: any) => void)[] } = {};
  static state: { [key: string]: any } = {};

  /**
   * 初始化，进入便签时调用
   */
  static init () {
    sendToNative('listen');
  }

  /**
   * 隐藏便签
   */
  static hide () {
    sendToNative('hide');
  }

  /**
   * 触发/停止移动窗口
   * @param move 是否移动
   */
  static move (move: boolean) {
    sendToNative('move', move);
  }

  /**
   * 触发原生窗口缩放
   * @param direction 缩放方向: 'top' | 'bottom' | 'left' | 'right' | 'top-left' | 'top-right' | 'bottom-left' | 'bottom-right'
   */
  static resize (direction: string) {
    sendToNative('resize', direction);
  }

  /**
   * 关闭便签
   */
  static close () {
    sendToNative('close');
  }

  /**
   * 贴边自动隐藏时触碰拉手还原便签
   */
  static restoreDock () {
    sendToNative('restore_dock');
  }

  /**
   * 贴边锁定便签（禁止拖动且不自动隐入桌面外）
   * @param locked 是否锁定
   */
  static lockEdge (locked: boolean) {
    sendToNative('edge_lock', locked);
  }

  /**
   * 监听客户端事件（具备状态回放功能，若事件已提前到达则立即触发）
   * @param event 事件名，比如 setting：设置项目更新，lock：鼠标穿透开关，data：便签数据更新
   * @param callback 回调
   */
  static on(event: string, callback: (data: any) => void) {
    this.listener[event] = this.listener[event] || [];
    this.listener[event].push(callback);
    // 如果该事件之前已经有 Native 数据到达，立即回放
    if (this.state[event] !== undefined) {
      try {
        callback(this.state[event]);
      } catch (err) {
        console.error('Error in replayed handler for ' + event, err);
      }
    }
  }

  /**
   * 关闭客户端监听
   * @param event 事件名
   * @param callback 回调
   */
  static off(event: string, callback: (data: any) => void) {
    if (this.listener[event]) {
      const idx = this.listener[event].indexOf(callback);
      if (idx > -1) {
        this.listener[event].splice(idx, 1);
      }
    }
  }
}

/**
 * 设置操作类
 */
export class Config {
  /**
   * 设置背景色
   * @param color 背景颜色
   */
  static bgcolor(color: string) {
    sendToNative('bgcolor', color);
  }

  /**
   * 开启关闭半透明
   * @param value 是否可透明
   */
  static opacityable (value: boolean) {
    sendToNative('opacityable', value);
  }

  /**
   * 开启/关闭鼠标穿透
   * @param value 是否开启鼠标穿透
   */
  static lock (value: boolean) {
    sendToNative('lock', value);
  }

  /**
   * 开启/关闭窗口置顶
   * @param value 是否置顶
   */
  static top (value: boolean) {
    sendToNative('top', value);
  }

  /**
   * 设置便签标题
   * @param value 便签标题
   */
  static title(value: string) {
    sendToNative('title', value);
  }
}

/**
 * 便签项定义与操作类
 */
export class Note {
  /**
   * 便签id
   */
  id: number = 0;
  /**
   * 是否完成
   */
  finish: boolean = false;
  /**
   * 便签内容
   */
  content: string = '';
  /**
   * 是否可编辑
   */
  editable?: boolean;

  constructor(content = '') {
    this.id = Date.now() * 1000 + Math.floor(Math.random() * 1000);
    this.content = content;
  }

  /**
   * 添加便签项
   * @param data 便签内容
   */
  static add (data: Note) {
    sendToNative('add', data);
  }

  /**
   * 更新便签项
   * @param data 便签项
   */
  static update (data: Note) {
    sendToNative('update', data);
  }

  /**
   * 删除便签项
   * @param data 便签项
   */
  static remove (data: Note) {
    sendToNative('remove', data);
  }

  /**
   * 添加便签项目到日程
   * @param data 便签项
   */
  static makeTask (data: Note) {
    sendToNative('task', data);
  }

  /**
   * 清空便签项
   */
  static clear () {
    sendToNative('clear');
  }

  /**
   * 批量更新便签项，通常用于排序
   * @param notes 便签项列表
   */
  static updateAll (notes: Note[]) {
    sendToNative('update_all', notes);
  }
}

// 提前启动 Native 消息监听，并缓存最新状态（支持后续挂载组件自动回放）
listen((ev, data) => {
  App.state[ev] = data;
  if (App.listener[ev]) {
    App.listener[ev].forEach(fn => {
      try {
        fn(data);
      } catch (err) {
        console.error('Error in handler for ' + ev, err);
      }
    });
  }
});

