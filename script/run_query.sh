#!/bin/bash

QUERY_NUM=$1
COLUMNAR=$2
OUTPUT=$3
LOGS=$4

./build/src/execute/hits_bench -q "${QUERY_NUM}" "${COLUMNAR}" "${OUTPUT}" 2>"${LOGS}"
