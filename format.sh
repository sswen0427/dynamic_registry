#!/bin/bash -ex

cd "$(dirname "$0")"

find csrc/ -iname '*.h' -print0 -o -iname '*.cpp' -print0 | xargs -0 clang-format -i --style=Google
find extension/ -iname '*.h' -print0 -o -iname '*.cpp' -print0 | xargs -0 clang-format -i --style=Google
