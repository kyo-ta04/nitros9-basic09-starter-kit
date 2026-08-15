#!/bin/sh
# Compatibility wrapper -> nitros9-runtime
exec "$(dirname "$0")/nitros9-runtime/run.sh" "$@"
