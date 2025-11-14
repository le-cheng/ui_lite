mingw32-make.exe clean

# 遇到错误立刻终止
set -e

qmake libui.pro

mingw32-make.exe -j$(nproc) #2>/dev/null

# cd libs && ./UITest.exe

# cd ./test && mingw32-make clean && mingw32-make -j8