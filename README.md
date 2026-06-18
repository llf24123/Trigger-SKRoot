# Trigger-SKRoot

基于 [Trigger](https://github.com/WolfLineage/Trigger) (GPL-3.0) 改写的 **SKRoot Pro 模块**。

## 对比原版

| 功能 | 原版 (Shell) | SKRoot 版 (C) |
|------|-------------|---------------|
| 关闭 ADB | `settings put` | 内核态直接操作 |
| 解温控 | shell 循环 + mount bind | 内核级守护 + 属性注入 |
| 游戏清理 | shell `rm -rf` | C 递归删除（更高效） |
| WebUI | 无 | 内置 Web 管理页面 |
| App 触发 | `while true + pidof` | 支持（保留原配置格式） |
| 隐藏性 | 一般 | 内核态执行，更难检测 |

## 功能

1. **开机自动关闭 ADB** — 保护隐私
2. **内核级解温控** — ColorOS/OPPO 温控解除，0 性能损耗
3. **游戏缓存清理** — 三角洲行动 / 瓦罗兰特一键清理
4. **内置 WebUI** — 浏览器管理模块状态
5. **App 触发执行** — 保留原版配置格式

## 编译

```bash
# 需要:
# - Android NDK 26d+
# - SKRoot SDK

export ANDROID_NDK_HOME=/path/to/ndk
export SKROOT_SDK_PATH=/path/to/skroot_sdk

chmod +x build.sh
./build.sh
```

## 安装

在 SKRoot 管理器中刷入 `Trigger-SKRoot-v1.0.0.zip`。

## WebUI

安装后在 SKRoot 管理器中打开模块的 WebUI 页面，或通过分配的端口在浏览器访问。

## 配置

配置文件: `/data/adb/.Magic/Trigger/kernel_config.txt`

格式:
```
$APP_PKG=$PATH=$INPUT

示例:
com.tencent.tmgp.dfm=/data/adb/kernel.sh=1\n2\n
```

## 许可证

GPL-3.0（继承自 Trigger）
