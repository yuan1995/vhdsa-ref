# 基于 AES 对称加密的抗量子签名

本项目是 VHDSA 抗量子数字签名算法的 C 语言参考实现，提供密钥生成、签名、验签、参数集选择、功能测试和性能测试能力。代码采用 Meson/Ninja 构建，支持生成静态库、动态库和各参数集 API 测试程序。

## 目录 Table of Contents

- [功能特性 Features](#功能特性-features)
- [项目结构 Project Structure](#项目结构-project-structure)
  - [项目文件 Project Files](#项目文件-project-files)
  - [附加文件 Additional Files](#附加文件-additional-files)
- [使用方法 Getting Started](#使用方法-getting-started)
  - [安装方法 Installation](#安装方法-installation)
  - [编译步骤 Build Steps](#编译步骤-build-steps)
  - [测试步骤 Test Steps](#测试步骤-test-steps)
- [接口说明 API](#接口说明-api)
- [项目声明 Project Statement](#项目声明-project-statement)
- [友情链接 Related Links](#友情链接-related-links)
- [许可证 License](#许可证-license)

## 功能特性 Features

- 支持 VHDSA-128s、VHDSA-128f、VHDSA-192s、VHDSA-192f、VHDSA-256s、VHDSA-256f 参数集。
- 实现密钥生成、签名、验签和密钥清理等签名 API。
- 包含 AES/OWF、随机预言机、有限域运算、通用哈希、VOLE 协议、BAVC 向量承诺、SHAKE/Keccak 等基础模块。
- 提供 `tests/api_test.c` 和 `benchmark_test.sh`，用于功能正确性验证和运行时间测试。

## 项目结构 Project Structure

### 项目文件 Project Files

```text
vhdsa-ref/
|-- aes.c / aes.h                    AES 与伪随机生成基础实现
|-- bavc.c / bavc.h                  基于半树的向量承诺
|-- fields.c / fields.h              有限域运算
|-- instances.c / instances.h        VHDSA 参数集定义
|-- owf.c / owf.h                    单向函数实现
|-- random_oracle.c / random_oracle.h 随机预言机 H0-H4
|-- universal_hashing.c / .h         VOLE/ZK/Leaf 哈希
|-- vhdsa_impl.c / vhdsa_impl.h      签名与验签核心流程
|-- vhdsa_aes.c.in / vhdsa_param.*.in 参数化代码模板
|-- vhdsa_128f/ vhdsa_128s/          128 bit 安全强度参数集
|-- vhdsa_192f/ vhdsa_192s/          192 bit 安全强度参数集
|-- vhdsa_256f/ vhdsa_256s/          256 bit 安全强度参数集
|-- sha3/                            SHAKE/Keccak 实现
|-- tests/                           API 正确性测试
|-- benchmark_test.sh                性能测试脚本
`-- meson.build                      Meson 构建配置
```

### 附加文件 Additional Files

- `README.md`：项目说明、使用方法和目录索引。
- `PROJECT_STATEMENT.md`：项目作者、单位、指导教师和开源声明。
- `.clang-format`：C/C++ 代码格式配置。
- `.gitignore`：本地构建产物、IDE 配置和文档转换产物忽略规则。

## 使用方法 Getting Started

### 安装方法 Installation

安装构建依赖：

```bash
pip install meson ninja
```

### 编译步骤 Build Steps

在支持 Meson/Ninja 的环境中编译项目：

```bash
mkdir build
cd build
meson .. --buildtype release -Dmarch-native=enabled
ninja
```

### 测试步骤 Test Steps

运行示例测试程序：

```bash
./vhdsa_192f/vhdsa_192f_api_test
./vhdsa_256f/vhdsa_256f_api_test
```

也可以使用项目提供的性能测试脚本：

```bash
./benchmark_test.sh
```

## 接口说明 API

项目提供 NIST/SUPERCOP 风格接口：

```c
void CRYPT_setup_DSA(CRYPT_state* crypt_state);
int crypto_sign_keypair(unsigned char* pk, unsigned char* sk, CRYPT_state* crypt_state);
int crypto_sign(unsigned char* sm, unsigned long long* smlen,
                const unsigned char* m, unsigned long long mlen,
                const unsigned char* sk, CRYPT_state* crypt_state);
int crypto_sign_open(unsigned char* m, unsigned long long* mlen,
                     const unsigned char* sm, unsigned long long smlen,
                     const unsigned char* pk);
```

## 项目声明 Project Statement

本项目的作者及单位：

The author and affiliation of this project:

```text
项目名称（Project Name）: 基于 AES 对称加密的抗量子签名
英文名称（English Name）: vhdsa-ref
项目作者（Author）: 待补充
项目负责人（Maintainer）: 待补充
作者单位（Affiliation）: 暨南大学，具体学院/课题组信息待补充
指导教师（Supervisor）: 待补充
项目时间（Project Period）: 2025 年 5 月至今
开源地址（Repository URL）: https://github.com/yuan1995/vhdsa-ref
许可证（License）: 待项目负责人确认后补充
```

## 友情链接 Related Links

- [GitHub Repository](https://github.com/yuan1995/vhdsa-ref)
- [Project Statement](PROJECT_STATEMENT.md)

## 许可证 License

许可证类型待项目负责人确认后补充。
