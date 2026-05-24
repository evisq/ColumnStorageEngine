#!/bin/bash
set -e

INPUT_CSV=$1
COLUMNAR=$2

mkdir -p "$(dirname "${COLUMNAR}")"
./build/src/execute/hits_bench -c "${INPUT_CSV}" src/execute/scheme.csv "${COLUMNAR}"
