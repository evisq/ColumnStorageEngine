#!/bin/bash
set -e

apt-get update -qq
apt-get install -y --no-install-recommends cmake g++ libgtest-dev
