import { ElColorPicker, ElPopover, ElSwitch, ElInput, ElSelect, ElOption, ElButton } from "element-plus";
import "./themes/style.scss";
import "remixicon/fonts/remixicon.css";
import "element-plus/theme-chalk/base.css";
import "element-plus/theme-chalk/el-popper.css";
import "element-plus/theme-chalk/el-popover.css";
import "element-plus/theme-chalk/el-color-picker.css";
import "element-plus/theme-chalk/el-message.css";
import "element-plus/theme-chalk/el-switch.css";
import "element-plus/theme-chalk/el-input.css";
import "element-plus/theme-chalk/el-select.css";
import "element-plus/theme-chalk/el-option.css";
import "element-plus/theme-chalk/el-button.css";
import "element-plus/theme-chalk/dark/css-vars.css";

import { createApp } from 'vue'
import { createPinia } from 'pinia'

import App from './App.vue'
import router from './router'

const app = createApp(App)
app.use(createPinia())
app.use(ElColorPicker)
app.use(ElPopover)
app.use(ElSwitch)
app.use(ElInput)
app.use(ElSelect)
app.use(ElOption)
app.use(ElButton)
app.use(router)

router.isReady().then(() => {
  app.mount('#app')
})
