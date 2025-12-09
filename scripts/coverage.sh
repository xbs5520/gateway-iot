#!/bin/bash
# 代码覆盖率报告生成脚本

set -e  # 遇到错误立即退出

echo "========================================"
echo "  代码覆盖率分析"
echo "========================================"

# 检查 lcov 是否安装
if ! command -v lcov &> /dev/null; then
    echo "❌ 未安装 lcov"
    echo "安装命令: sudo apt install lcov"
    exit 1
fi

# 切换到项目根目录
cd "$(dirname "$0")/.."
PROJECT_ROOT=$(pwd)
echo "项目目录: $PROJECT_ROOT"

# 创建假证书（编译需要）
mkdir -p cert
touch cert/imx_ca.pem
touch cert/imx_cert.pem
touch cert/imx_key.pem

# 清理旧的构建
BUILD_DIR="build-coverage"
rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
cd $BUILD_DIR

echo ""
echo "[1/4] 编译 (启用覆盖率)..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage -O0" \
    -DCMAKE_EXE_LINKER_FLAGS="--coverage"

make -j$(nproc)

echo ""
echo "[2/4] 运行测试..."
ctest --output-on-failure

# 检查是否有 gcda 文件
if [ -z "$(find . -name '*.gcda')" ]; then
    echo "❌ 警告: 没有生成覆盖率数据文件 (.gcda)"
    exit 1
fi

echo ""
echo "[3/4] 收集覆盖率数据..."
lcov --gcov-tool gcov-10 \
    --capture \
    --directory . \
    --output-file coverage.info \
    --base-directory "$PROJECT_ROOT" \
    --ignore-errors mismatch

# 过滤掉不需要的文件
lcov --gcov-tool gcov-10 \
    --remove coverage.info \
    "*/tests/*" \
    "*/external/*" \
    "*/build-coverage/*" \
    "*/usr/*" \
    --output-file coverage_filtered.info \
    --ignore-errors unused

echo ""
echo "[4/4] 生成 HTML 报告..."
genhtml coverage_filtered.info \
    --output-directory coverage_html \
    --title "Gateway Coverage Report" \
    --prefix "$PROJECT_ROOT" \
    --ignore-errors source

echo ""
echo "========================================"
echo "✅ 完成！"
echo "========================================"
echo "报告位置: $PROJECT_ROOT/$BUILD_DIR/coverage_html/index.html"
echo ""

# 显示覆盖率摘要
lcov --gcov-tool gcov-10 --summary coverage_filtered.info