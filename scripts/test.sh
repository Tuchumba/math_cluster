#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/tests/out"
mkdir -p "$OUT"

MANAGER="$ROOT/bin/manager"
WORKER="$ROOT/bin/worker"

STEPS=100000

PORT=5555
HOST=127.0.0.1

echo "[TEST] single worker run"
"$MANAGER" 1 "$HOST" "$PORT" --a 0 --b 1 --n $STEPS --timeout 20 >"$OUT/run1.txt" 2>"$OUT/run1.err" &
MPID=$!
sleep 1
"$WORKER" --host "$HOST" --port "$PORT" --cores 2 --timeout 20 >"$OUT/w1.txt" 2>"$OUT/w1.err" || true
wait $MPID || true

# Безопасный парсинг (если строки нет — подставим NA)
VAL1=$(awk -F= '/^INTEGRAL=/{print $2;f=1} END{if(!f)print "NA"}' "$OUT/run1.txt")
T1=$(awk -F= '/^TOTAL_TIME_SEC=/{print $2;f=1} END{if(!f)print "NA"}' "$OUT/run1.txt")

echo "[TEST] two workers run"
PORT=5556
"$MANAGER" 2 "$HOST" "$PORT" --a 0 --b 1 --n $STEPS --timeout 20 >"$OUT/run2.txt" 2>"$OUT/run2.err" &
MPID=$!
sleep 1
"$WORKER" --host "$HOST" --port "$PORT" --cores 2 --timeout 20 >"$OUT/w2a.txt" 2>"$OUT/w2a.err" || true &
"$WORKER" --host "$HOST" --port "$PORT" --cores 2 --timeout 20 >"$OUT/w2b.txt" 2>"$OUT/w2b.err" || true &
wait $MPID || true

VAL2=$(awk -F= '/^INTEGRAL=/{print $2;f=1} END{if(!f)print "NA"}' "$OUT/run2.txt")
T2=$(awk -F= '/^TOTAL_TIME_SEC=/{print $2;f=1} END{if(!f)print "NA"}' "$OUT/run2.txt")

# 1) корректность результата (близко к π). Если NA — выводим лог и падаем.
VAL1="$VAL1" VAL2="$VAL2" OUT="$OUT" python3 - <<'PY'
import math, os, sys, pathlib
v1=os.environ['VAL1']; v2=os.environ['VAL2']; out=os.environ['OUT']
def die(msg, file):
    print(msg)
    try:
        print("====", file, "====")
        print(pathlib.Path(file).read_text())
    except Exception: pass
    sys.exit(1)
if v1=="NA":
    die("[ASSERT] missing INTEGRAL in run1.txt", out+"/run1.txt")
if v2=="NA":
    die("[ASSERT] missing INTEGRAL in run2.txt", out+"/run2.txt")
val1=float(v1); val2=float(v2)
ok1 = abs(val1 - math.pi) < 1e-4
ok2 = abs(val2 - math.pi) < 1e-4
print("[ASSERT] correctness:", ok1 and ok2)
sys.exit(0 if (ok1 and ok2) else 1)
PY

# 2) ускорение — делаем мягкой проверкой (не валим CI из-за шума)
T1="$T1" T2="$T2" python3 - <<'PY'
import os
t1=os.environ['T1']; t2=os.environ['T2']
if t1=="NA" or t2=="NA":
    print("[CHECK] speedup: NA (no timings) -> WARN")
else:
    t1=float(t1); t2=float(t2)
    print(f"[CHECK] speedup: t1={t1:.3f}s, t2={t2:.3f}s -> {'OK' if t2<t1 else 'WARN'}")
PY

# 3) отказ — менеджер без воркера должен завершиться с ошибкой
echo "[TEST] failure detection"
PORT=5557
set +e
"$MANAGER" 1 "$HOST" "$PORT" --a 0 --b 1 --n $STEPS --timeout 2 >"$OUT/fail.txt" 2>"$OUT/fail.err" &
MPID=$!
sleep 0.5
wait $MPID; RC=$?
set -e
test $RC -ne 0 && echo "[ASSERT] manager failed as expected" || (echo "[ASSERT] expected failure"; exit 1)

echo "OK"
