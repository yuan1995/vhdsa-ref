# 项目结构 Project Structure

## 项目文件 Project Files

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

- [附加文件 Additional Files](additional-files.md)

[返回目录](../README.md#目录-table-of-contents)
