#!/bin/bash
set -e

QUERY_NUM=$1
COLUMNAR=$2
OUTPUT=$3
LOGS=$4

mkdir -p "$(dirname "${OUTPUT}")" "$(dirname "${LOGS}")"
./build/src/execute/hits_bench -q "${QUERY_NUM}" "${COLUMNAR}" "${OUTPUT}" 2>"${LOGS}"
