#!/bin/bash
# Trigger-SKRoot v1.1.0 编译脚本
# 需要: Android NDK 26d+

set -e

NDK_PATH="${ANDROID_NDK_HOME:-$ANDROID_NDK}"

if [ -z "$NDK_PATH" ]; then
    echo "❌ 请设置 ANDROID_NDK_HOME 环境变量"
    echo "   export ANDROID_NDK_HOME=/path/to/ndk"
    exit 1
fi

# 检查 SDK 是否已链接
if [ ! -d "skroot_sdk/include" ]; then
    echo "❌ SKRoot SDK 未找到，请确认 skroot_sdk 软链接存在"
    exit 1
fi

echo "🔨 编译 Trigger-SKRoot 模块..."
echo "   NDK: $NDK_PATH"

# ndk-build 编译
$NDK_PATH/ndk-build \
    NDK_PROJECT_PATH=. \
    APP_BUILD_SCRIPT=jni/Android.mk \
    APP_ABI=arm64-v8a \
    APP_PLATFORM=android-28 \
    -j$(nproc)

# 打包 ZIP
echo "📦 打包 SKRoot 模块 ZIP..."
mkdir -p dist

# 复制编译产物
cp libs/arm64-v8a/libtrigger_skroot.so dist/trigger_skroot.so
cp -r wwwroot dist/

# 复制温控配置文件
cp -r odm dist/

cd dist
cat > module.prop << 'EOF'
id=Trigger-SKRoot
name=Trigger-SKRoot
version=v1.1.0
versionCode=110
author=Trigger (Vorthas) -> SKRoot port
description=自启动 | 关闭USB调试 | 解温控 | 游戏清理 | WebUI管理
EOF

zip -r ../Trigger-SKRoot-v1.1.0.zip .
cd ..

echo "✅ 完成: Trigger-SKRoot-v1.1.0.zip"
echo "   大小: $(du -h Trigger-SKRoot-v1.1.0.zip | cut -f1)"
echo ""
echo "安装方法: 在 SKRoot 管理器中刷入此 ZIP"
