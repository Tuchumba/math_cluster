#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/tests/bench"
mkdir -p "$OUT"

MANAGER="$ROOT/bin/manager"
WORKER="$ROOT/bin/worker"

HOST=127.0.0.1
BASE_PORT=5600

# Наборы конфигураций: число рабочих и ядер на рабочего
WORKERS_SET=(1 2 3 4)
CORES_SET=(1 2 4)

A=0
B=1
N=10000000
TIMEOUT=60

CSV="$OUT/results.csv"
echo "workers,cores,total_cores,time_sec,integral" > "$CSV"

run_case () {
  local workers=$1
  local cores=$2
  local port=$3

  local log="$OUT/w${workers}_c${cores}.txt"
  local err="$OUT/w${workers}_c${cores}.err"

  # Запускаем менеджера
  "$MANAGER" "$workers" "$HOST" "$port" --a "$A" --b "$B" --n "$N" --timeout "$TIMEOUT" >"$log" 2>"$err" &
  local mpid=$!

  # Даем менеджеру встать на accept
  sleep 0.02

  # Запускаем N воркеров
  for ((i=1; i<=workers; i++)); do
    "$WORKER" --host "$HOST" --port "$port" --cores "$cores" --timeout "$TIMEOUT" >"$OUT/worker_${workers}_${cores}_${i}.txt" 2>"$OUT/worker_${workers}_${cores}_${i}.err" &
  done

  # Ждем завершения менеджера
  wait $mpid || true

  # Парсим результаты
  local t=$(awk -F= '/^TOTAL_TIME_SEC=/{print $2; f=1} END{if(!f)print "NA"}' "$log")
  local val=$(awk -F= '/^INTEGRAL=/{print $2; f=1} END{if(!f)print "NA"}' "$log")
  local total_cores=$(( workers * cores ))
  echo "$workers,$cores,$total_cores,$t,$val" | tee -a "$CSV" >/dev/null
}

echo "[BENCH] writing CSV to $CSV"
p=$BASE_PORT
for w in "${WORKERS_SET[@]}"; do
  for c in "${CORES_SET[@]}"; do
    echo "[BENCH] workers=$w cores=$c (port $p)"
    run_case "$w" "$c" "$p"
    p=$((p+1))
  done
done

# Красивая табличка по завершении
echo
echo "=== Scalability summary ==="
column -t -s, "$CSV" || cat "$CSV"

# Небольшая эвристика для «масштабируется»
# Сравним базовую конфигурацию (1x1) с максимальной (max workers × max cores)
base_t=$(awk -F, 'NR==2 {print $4; f=1} END{if(!f)print "NA"}' "$CSV")
max_t=$(tail -n 1 "$CSV" | awk -F, '{print $4}')

BASE="$base_t" BEST="$max_t" python3 - <<'PY'
import os
b = os.environ.get('BASE','NA')
m = os.environ.get('BEST','NA')
if b == 'NA' or m == 'NA' or b == '' or m == '':
    print("Speedup: NA (missing timings)")
else:
    try:
        b = float(b); m = float(m)
        if m > 0:
            print(f"\nEstimated speedup (1x1 -> max): {b/m:.2f}x")
        else:
            print("\nEstimated speedup: INF (best==0)")
    except ValueError:
        print("Speedup: NA (bad numbers)")
PY

