# HTStream
A toolset for high throughput sequence analysis using a streaming approach facilitated by Linux pipes.

https://ibest.github.io/HTStream/

## build
```
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make all
```

## debug build
```
mkdir debug
cd debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make all
```

## clang build

note if you change the exports on an existing build dir, you must delete the CMakeCache.txt

```
mkdir clang-debug
cd clang-debug
cmake -DCXX=clang++ -DCC=clang -DCMAKE_BUILD_TYPE=Debug ..
make all
```

## gprof
```
mkdir gprof
cd gprof
cmake -DCMAKE_BUILD_TYPE=Relese -DCMAKE_CXX_FLAGS=-pg -DCMAKE_EXE_LINKER_FLAGS=-pg -DCMAKE_SHARED_LINKER_FLAGS=-pg ..
make all

```

## verbose
```
make VERBOSE=1

## test

```
make testSD
make test
```

## install (defaults to /usr/local/bin)

override by `cmake -DCMAKE_INSTALL_PREFIX=/foo`

```
make install
```


## Trouble shooting the build
If you use a module system, or have alternative versions of gcc or boost installed cmake may fail to correctly detect the required g++ or boost versions. Try a varaition on the the following commands, where paths are modified to match the approriate paths for your environment.:
```

```
export CC=`which gcc`
export CXX=`which g++`
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INCLUDE_PATH=/opt/modules/devel/boost/1.61.0/include -DCMAKE_LIBRARY_PATH=/opt/modules/devel/boost/1.61.0/lib ..
```
