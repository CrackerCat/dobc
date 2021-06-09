# dobc

1. dobc now only work success in Windows10 and VS2017.
2. dobc now only work success in thumb mode so.

## Build And Compile

```
mkdir build
cd build
cmake ..
msbuild dobc.sln
```

## Usage

### show help
```
dobc -help
```
### de-ollvm one function in one so library
```
dobc -d [funcaddr] [soname] -o output.so 
```

### de-ollvm one so library
```
dobc soname -o output.so
```

## APK be de-obfusation

Some APK test by manual, becuase auto-test hard to work close-source so。

| APK-cn       | ollvm-so      | so-md5  | decode-so | manual | 
| ------------ |:-------------:| -----:|-----:| -----:| 
| 斗鱼         | libmakeurl    | 2f0a73a9509eb0599aca9c298016ee9 | |
| 抖音         | libcms        | 1d8f011a1a53b2b7bd73aed5ebe62142 | |
| 快手         | libqsmain     | 2f0a73a9509eb0599aca9c298016ee9 | |
| liblazarus  | [liblazarus](https://github.com/baikaishiuc/dobc/blob/main/data/liblazarus/liblazarus.so)     |       |  [liblazarus.so](https://github.com/baikaishiuc/dobc/blob/main/data/liblazarus/test.so) ||

## Wiki
