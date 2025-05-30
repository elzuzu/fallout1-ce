#!/bin/bash
# Placeholder stress test
BACKEND=${1:-SDL}
export FALLOUT_RENDER_BACKEND=$BACKEND
# run for 5 loops opening and closing the engine
for i in $(seq 1 5); do
  echo "Run $i"
  ../fallout-ce --version >/dev/null || exit 1
  sleep 1
done
