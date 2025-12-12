Simulator build (Windows):

cd foundation/arkui/ui_lite/tools/qt/simulator
qmake simulator.pro
mingw32-make.exe -j4
cd libs
./UITest.exe
cd ..

