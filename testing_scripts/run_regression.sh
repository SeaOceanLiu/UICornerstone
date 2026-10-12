#!/bin/bash
# 全量回归扫描：遍历 build/test/Debug/test_*.exe（跳过 temp_* 探针），带 auto=5 自动关闭。
# 用法：bash testing_scripts/run_regression.sh [repo_root]
# 输出：每个用例一行（ok / FAILED），结尾 === done ===；grep FAILED 查看失败项。
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="${1:-$(cd "$SCRIPT_DIR/.." && pwd)}"
cd "$REPO_ROOT/build/test/Debug" || exit 1
echo "=== regression scan ==="
for exe in test_*.exe; do
  case "$exe" in temp_*) continue;; esac
  out=$(timeout 90 "./$exe" auto=5 2>&1)
  rc=$?
  fails=$(printf '%s\n' "$out" | grep -c "^FAIL")
  passes=$(printf '%s\n' "$out" | grep -c "^PASS")
  if [ $rc -ne 0 ] || [ "$fails" != "0" ]; then
    echo "FAILED $exe rc=$rc fails=$fails"
    printf '%s\n' "$out" | grep "^FAIL" | head -3
  else
    echo "ok $exe (pass=$passes rc=0)"
  fi
done
echo "=== done ==="
