#!/bin/bash
FILE=$1
if [ -z "$FILE" ]; then
    echo "Usage: ./check.sh path/to/file.c"
    exit 1
fi
echo ""
echo "=== Checking: $FILE ==="
echo ""
echo -n "[1] Dynamic allocation check... "
RESULT=$(grep -n "malloc\|calloc\|realloc\|free" "$FILE")
if [ -z "$RESULT" ]; then
    echo "PASS"
else
    echo "FAIL"
    echo "$RESULT"
fi
echo -n "[2] Direct OS call check... "
RESULT=$(grep -n "pthread_\|vTaskDelay\|k_sleep\|osDelay" "$FILE")
if [ -z "$RESULT" ]; then
    echo "PASS"
else
    echo "WARN - only ok if this is a platform file"
    echo "$RESULT"
fi
echo ""
echo "[3] Cppcheck..."
cppcheck --std=c99 --enable=warning,style \
         --suppress=missingIncludeSystem \
         -I./dem -I./platform \
         "$FILE" 2>&1
echo ""
echo "=== Done ==="
