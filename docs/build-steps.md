# 编译步骤 Build Steps

在支持 Meson/Ninja 的环境中编译项目：

```bash
mkdir build
cd build
meson .. --buildtype release -Dmarch-native=enabled
ninja
```

[返回使用方法](getting-started.md)
