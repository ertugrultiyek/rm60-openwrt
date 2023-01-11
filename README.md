在新安装的ubuntu18.04 64位下的编译 需要安装的工具：
 
cd scripts/config/; make clean
 
sudo apt-get install make quilt git libncurses5-dev build-essential gawk zlib1g-dev
编译方法：
cd hlk_build
编译RM60固件
./build.sh RM60 all
编译GD11 固件
./build.sh GD11 all