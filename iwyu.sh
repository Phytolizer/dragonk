#!/bin/sh
set -e

fd -e c -e h -X iwyu-tool -p out/build/dev > /tmp/iwyu.out
iwyu-fix-includes --reorder < /tmp/iwyu.out
