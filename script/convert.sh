#!/bin/bash
set -e

INPUT_CSV=$1
COLUMNAR=$2
SCHEME="src/execute/scheme.csv"

./build/src/execute/hits_bench convert "${INPUT_CSV}" "${SCHEME}" "${COLUMNAR}"
