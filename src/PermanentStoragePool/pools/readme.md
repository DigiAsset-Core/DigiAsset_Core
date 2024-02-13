To add a new PSP to the pools you must do the following:

1) apply to become a PSP
2) extend PermanentStoragePool and PermanentStoragePoolMetaProcessor classes and save the .cpp and .h files under your
   pool name in this folder
3) add your files to the pools constant in /src/CMakeLists.txt
4) add your pool to /src/PermanentStoragePool/PermanentStoragePoolList.cpp
5) set up your server back end that monitors those subscribed to your pool and pays them
6) submit your code back to the repo for review