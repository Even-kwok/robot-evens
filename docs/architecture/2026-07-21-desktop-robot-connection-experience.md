# Desktop Robot 用户连接体验

日期：2026-07-21
交付节点：`PB-20260721-0010-001`
发布固件：`V0.81`

## 目标

将原有调试入口收敛为面向日常使用者的 Desktop Robot 连接与控制体验：用户不必扫描 Wi-Fi 列表，也不必记住调试地址；机器人在需要恢复连接时始终能通过连续三下电源键给出清晰的本机信息。

## 连接约定

- 产品显示名称：`Desktop Robot`
- 默认 mDNS 主机名：`desktop-robot`
- 默认访问地址：`http://desktop-robot.local`
- 恢复入口：连续按三下电源键开启设置热点；再次连续按三下关闭热点并回到待机画面。

机器人设置屏会以大字号显示：热点状态、当前 Wi-Fi 状态、已连接网络名称、实际固件地址、V0.81、设置热点名称和设置页地址。设置热点的凭据只在机器人屏幕上显示，不写入说明书或项目记录。

## 固件变更

### 不再扫描 Wi-Fi

设置页面只提供 `SSID` 和密码输入框。连接请求直接使用用户输入的网络名称，避免扫描带来的等待、隐藏网络遗漏和认证类型推测错误。旧的扫描 API 仍保留，供历史调试工具兼容；用户页面与新 App 均不调用它。

### 状态协议

`GET /api/setup/status` 除原有状态外，新增：

- `deviceName`：固定为 `Desktop Robot`
- `firmwareUrl`：当前可达固件地址；已联网时为实际 IP，热点时为设置页地址，其他情况为 mDNS 地址
- `mdnsUrl`：稳定的 `http://desktop-robot.local`

App 先请求 `/api/ping` 确认设备，再读取 `/api/setup/status` 更新 Wi-Fi 名称和实际固件地址。

保存 Wi-Fi 成功时，设置热点不会立即关闭：API 会返回 `connected`、实际站点地址及 `setupPortalRetained`，让网页或 App 先展示确认结果。用户以三击电源键关闭热点。

## iPhone App

App 使用原生 SwiftUI 系统组件，首页为单任务连接屏：大标题、连接状态、固件地址、主连接按钮和“按机器人按钮设置”入口。用户可在需要时输入一个不同的固件地址。

连接成功后，日常功能按四个原生标签组织：

- Robot：连接详情与返回待机
- Expression：常用表情
- Actions：情绪动作与灯光
- Settings：Wi-Fi 恢复设置与断开连接

此前的创作、实时音频、动作编排等调试/制作界面保留在工程中以便回滚，但不再作为 App 启动入口，也不出现在新的用户导航中。

## 回滚

- App：将 `RobotEvensControlApp.swift` 的根视图改回 `ContentView()`，并从 Xcode 工程中移除 `DesktopRobotAppView.swift` 的引用即可恢复旧调试入口。
- 固件：恢复 `ROBOT_MDNS_NAME` 与 `ROBOT_AP_SSID` 的原配置，同时回退 `wifi_manager.*`、`firmware.ino`、`api.cpp` 的本次改动；刷写前保留当前设备固件包和网络设置。
- 本次只完成编译验证，未向实体机器人刷写固件。

## 验证

- `arduino-cli compile --fqbn esp32:esp32:m5stack-cores3 ... firmware/`：通过。
- `swift test`（RobotEvensCore）：91 项通过。
- 无签名 iPhone Simulator 构建：通过。
- iPhone 13 Pro Max 模拟器已安装并运行首屏；截图路径为 `/private/tmp/desktop-robot-welcome.png`。
- 针对 Wi-Fi 设置页的固件源契约测试：通过；完整历史源测试集仍含与本次无关的缺失资源/旧断言，未作为本次固件编译结果的替代。
