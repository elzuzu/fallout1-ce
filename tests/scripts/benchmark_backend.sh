#!/bin/bash
# Placeholder benchmark script
BACKEND=${1:-SDL}
export FALLOUT_RENDER_BACKEND=$BACKEND
TIMEFILE=$(mktemp)
{ time ../fallout-ce --version >/dev/null; } 2> $TIMEFILE
cat $TIMEFILE
rm $TIMEFILE
