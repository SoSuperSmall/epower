

1、根据目标程序运行环境，需要简单修改stdenv
如果目标程序需要再X86_64环境运行
export ARCH=$(arch)
或export ARCH=x86_64
如果在OK1043上运行，对应aarch64环境，则：
xport ARCH=aarch64

2、正常情况下diag和driver目录下的文件不需要修改，以lib库方式放在一级目录lib_x86_64或lib_aarch64中
   用户程序代码放在app目录中

3、用户根据app中用户代码情况，参考app/makefile.am 确认需要编译的源码文件和附加编译选项

4、一级目录下执行 make，如果需要确认编译过程则增加选项V=99，即make V=99

5、对应生成可执行程序在 bin_x86_64 目录下，为 epower_cpp
