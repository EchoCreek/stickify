/// <reference types="vite/client" />
declare module '*.vue' {
  import { defineComponent } from 'vue'
  const Component: ReturnType<typeof defineComponent>
  export default Component
}

declare module 'element-plus/dist/locale/zh-cn.mjs' {
  const zhCn: any;
  export default zhCn;
}

declare module 'element-plus/dist/locale/en.mjs' {
  const en: any;
  export default en;
}