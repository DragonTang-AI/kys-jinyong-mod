#!/bin/bash
# 运行金庸群侠传复刻版
# 从 build 目录运行，使 ../game/ 指向项目根目录下的 game/
cd "$(dirname "$0")/build"
exec ./src/kys
