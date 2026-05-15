# 使用方法 Getting Started

## 安装方法 Installation

安装构建依赖：

```bash
pip install meson ninja
```

## 编译步骤 Build Steps

在支持 Meson/Ninja 的环境中编译项目：

```bash
mkdir build
cd build
meson .. --buildtype release -Dmarch-native=enabled
ninja
```

## 测试步骤 Test Steps

运行示例测试程序：

```bash
./vhdsa_192f/vhdsa_192f_api_test
./vhdsa_256f/vhdsa_256f_api_test
```

也可以使用项目提供的性能测试脚本：

```bash
./benchmark_test.sh
```

[返回目录](../README.md#目录-table-of-contents)

