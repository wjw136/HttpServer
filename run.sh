set -x

mkdir build
rm -rf build/*
cd build
cmake ..
make -j2

# 可执行文件 不能使用bash和sh
./bin/main.o


# 101.43.240.169