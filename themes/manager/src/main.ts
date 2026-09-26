import { createApp } from 'vue';
import { createPinia } from 'pinia';
import { ElPopover } from 'element-plus';
import App from './App.vue';

import "./themes/style.scss";
import "remixicon/fonts/remixicon.css";
import "element-plus/theme-chalk/base.css";
import "element-plus/theme-chalk/el-popper.css";
import "element-plus/theme-chalk/el-popover.css";
import "element-plus/theme-chalk/el-message.css";
import "element-plus/theme-chalk/dark/css-vars.css";

const app = createApp(App);
app.use(createPinia());
app.use(ElPopover);
app.mount('#app');
