#!/bin/bash
# 静态代码分析脚本

set -e

echo "========================================"
echo "  静态代码分析 (cppcheck)"
echo "========================================"

# 检查工具
if ! command -v cppcheck &> /dev/null; then
    echo "❌ 未安装 cppcheck"
    echo "安装命令: sudo apt install cppcheck"
    exit 1
fi

# 切换到项目根目录
cd "$(dirname "$0")/.."
PROJECT_ROOT=$(pwd)
echo "项目目录: $PROJECT_ROOT"

echo ""
echo "开始分析..."
cppcheck \
    --enable=all \
    --suppress=missingIncludeSystem \
    --suppress=unmatchedSuppression \
    --std=c++17 \
    --inline-suppr \
    --error-exitcode=0 \
    -I include \
    -I src \
    --quiet \
    src/ \
    2>&1 | tee static_analysis.log

echo ""
echo "========================================"
echo "✅ 完成！"
echo "========================================"
echo "报告保存: $PROJECT_ROOT/static_analysis.log"
echo ""

# 统计问题数量
ERROR_COUNT=$(grep -c "error:" static_analysis.log || true)
WARN_COUNT=$(grep -c "warning:" static_analysis.log || true)

echo "发现问题:"
echo "  错误: $ERROR_COUNT"
echo "  警告: $WARN_COUNT"