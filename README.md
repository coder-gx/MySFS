本程序采用VSCode进行开发，测试环境分为Linux，利用CMake方式编译，确保程序的可移植性。

Linux:

Ubuntu-22.04环境，编译器采用gcc/g++ 11.4.0，cmake 3.27.9

先将文件上传到Linux服务器，文件包括源码、磁盘文件myDisk.img和CMakeList.txt，CMakeList.txt已经写好

配置好cmke和gcc/g++编译环境后，在CMakeList同一目录下执行：

'''
mkdir build
cd build 
cmake ..
make clean
make
./MySFS
'''

就可以看到程序运行成功了


如果要直接运行可执行程序，则在./MySFS同一文件夹， chmod +x  ./MySFS，然后运行./MySFS



