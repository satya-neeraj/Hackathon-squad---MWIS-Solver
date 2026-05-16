#!/usr/bin/env bash
# Run the MWIS solver on input.in (or a file you specify) with a 5-min budget.
#
# Usage:
#   bash run.sh                       # reads input.in, 300s budget
#   bash run.sh examples/star.in      # reads chosen file
#   bash run.sh input.in 60           # custom time limit (seconds)
set -e
cd "$(dirname "$0")"

# Pick whichever binary exists
SOLVE=""
if   [[ -x build/solve.exe ]]; then SOLVE=build/solve.exe
elif [[ -x build/solve ]];     then SOLVE=build/solve
fi

if [[ -z "$SOLVE" ]]; then
    echo "Solver not built yet — running build.sh now..."
    bash build.sh
    if   [[ -x build/solve.exe ]]; then SOLVE=build/solve.exe
    elif [[ -x build/solve ]];     then SOLVE=build/solve
    else
        echo "Build failed — could not find build/solve"
        exit 1
    fi
fi

INPUT="${1:-input.in}"
TIMELIMIT="${2:-300}"

if [[ ! -f "$INPUT" ]]; then
    echo "Input file '$INPUT' not found."
    echo
    echo "Usage:  bash run.sh [INPUT_FILE] [TIME_LIMIT_SECONDS]"
    echo "  Default INPUT_FILE:   input.in"
    echo "  Default TIME_LIMIT:   300 seconds (5 minutes)"
    echo
    echo "Examples available in ./examples/:"
    ls examples/ 2>/dev/null
    exit 1
fi

"$SOLVE" -i "$INPUT" -t "$TIMELIMIT"
