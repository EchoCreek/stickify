import { ElColorPicker, ElPopover } from "element-plus";
import "./themes/style.scss";
import "remixicon/fonts/remixicon.css";
import "element-plus/theme-chalk/base.css";
import "element-plus/theme-chalk/el-popper.css";
import "element-plus/theme-chalk/el-popover.css";
import "element-plus/theme-chalk/el-color-picker.css";
import "element-plus/theme-chalk/el-message.css";
import "element-plus/theme-chalk/dark/css-vars.css";

import { createApp } from 'vue'
import { createPinia } from 'pinia'

import App from './App.vue'
import router from './router'

const app = createApp(App)
app.use(createPinia())
app.use(ElColorPicker)
app.use(ElPopover)
app.use(router)
app.mount('#app')
