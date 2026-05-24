#!/bin/bash
set -e

apt-get update -qq
apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ca-certificates
