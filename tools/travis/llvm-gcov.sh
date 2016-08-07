#!/bin/bash

set -e

exec llvm-cov gcov "$@"
