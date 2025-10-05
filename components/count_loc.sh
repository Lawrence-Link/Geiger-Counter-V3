#!/usr/bin/env bash
#
# 递归统计某个目录下的代码行数 (LOC)
# 支持多扩展名过滤、排除目录、自动忽略隐藏文件夹
#
# 用法:
#   ./count_loc.sh [目录] [扩展名...] [--exclude dir1 dir2 ...]
#
# 示例:
#   ./count_loc.sh . cpp h --exclude build .git .pio
#

# ---------- 参数解析 ----------
if [ $# -lt 1 ]; then
    echo "用法: $0 <目录> [扩展名...] [--exclude dir1 dir2 ...]"
    exit 1
fi

DIR="$1"
shift

EXTS=()
EXCLUDES=()
EXCLUDE_MODE=0

while [ $# -gt 0 ]; do
    if [ "$1" == "--exclude" ]; then
        EXCLUDE_MODE=1
        shift
        continue
    fi

    if [ $EXCLUDE_MODE -eq 1 ]; then
        EXCLUDES+=("$1")
    else
        EXTS+=("$1")
    fi
    shift
done

# 如果未指定扩展名，默认统计常见 C/C++ 文件
if [ ${#EXTS[@]} -eq 0 ]; then
    EXTS=(c cpp h hpp cc S)
fi

# ---------- 构造 find 条件 ----------
FIND_EXPR=""
for ext in "${EXTS[@]}"; do
    FIND_EXPR="$FIND_EXPR -o -iname *.$ext"
done
FIND_EXPR="${FIND_EXPR# -o }"  # 去掉第一个 -o

# 构造排除目录条件
EXCLUDE_EXPR=""
for ex in "${EXCLUDES[@]}"; do
    EXCLUDE_EXPR="$EXCLUDE_EXPR -path */$ex -prune -o"
done

# ---------- 输出信息 ----------
echo "📂 统计目录: $DIR"
echo "📄 文件类型: ${EXTS[*]}"
if [ ${#EXCLUDES[@]} -gt 0 ]; then
    echo "🚫 排除目录: ${EXCLUDES[*]}"
fi
echo "--------------------------------------"

# ---------- 主逻辑 ----------
TOTAL_LINES=0
FILE_COUNT=0

while IFS= read -r file; do
    [[ "$file" == "$0" ]] && continue  # 排除脚本自身
    lines=$(wc -l < "$file" 2>/dev/null)
    (( TOTAL_LINES += lines ))
    (( FILE_COUNT++ ))
done < <(
    # 注意 find 的排除条件顺序：先 prune 再找文件
    find "$DIR" $EXCLUDE_EXPR \( $FIND_EXPR \) -type f -print
)

# ---------- 输出结果 ----------
echo "文件数量: $FILE_COUNT"
echo "总代码行数: $TOTAL_LINES"
if [ $FILE_COUNT -gt 0 ]; then
    echo "平均每文件: $(( TOTAL_LINES / FILE_COUNT )) 行"
fi
