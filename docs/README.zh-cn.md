<div align="center">

# PlantsVsZombies AndroidTV

**[English](./README.md)** | **简体中文**

[![license](https://img.shields.io/github/license/ZombieYetis/PlantsVsZombies-AndroidTV)][GPL-3.0]
[![Android CI](https://github.com/ZombieYetis/PlantsVsZombies-AndroidTV/actions/workflows/android.yml/badge.svg)](https://github.com/ZombieYetis/PlantsVsZombies-AndroidTV/actions/workflows/android.yml "Android CI")

一个基于植物大战僵尸 TV 版的改版.

</div>

## 构建

- 确保已安装下列组件:
    * Android SDK Platform 35
    * NDK v27.3.13750724 (r27d)
    * CMake v3.20+

- 克隆仓库.
    ```sh
    git clone https://github.com/ZombieYetis/PlantsVsZombies-AndroidTV.git
    cd PlantsVsZombies-AndroidTV
    ```

- 复制 assets 文件到路径 `PlantsVsZombies-AndroidTV/app/src/main/assets/` 下.
    > 需要资源文件请联系仓库作者.

- 构建方式:
    * Android Studio: 点击构建按钮.
    * 命令行: 运行以下命令:
        ```sh
        ./gradlew assembleDebugV115
        ```

- 如果要发布, 先在位于项目根目录的 `keystore.properties` 文件 (需要自行创建) 中配置签名. 文件内容样式如下:
    ```properties
    storePassword=myStorePassword
    keyPassword=mykeyPassword
    keyAlias=myKeyAlias
    storeFile=myStoreFileLocation
    ```

## 参与贡献

### 编码风格 (C++)

#### 命名约定

- 函数/类型/概念: `PascalCase`
- 变量: `camelCase`
- 命名空间: `snake_case`
- 宏/常量/枚举成员/非类型模板参数: `UPPER_CASE`

#### 格式

见 [`.clang-format`](/.clang-format).

> 建议在每次提交前先用 IDE 对代码进行格式化.

### 提交

参考[约定式提交](https://www.conventionalcommits.org/zh-hans/v1.0.0/).

## 许可协议

本项目的源代码使用 [GPL-3.0][GPL-3.0] 许可进行授权.

本项目与渡维科技、宝开或艺电无关, 也未获得他们的认可.

[GPL-3.0]: https://www.gnu.org/licenses/gpl-3.0.html "GPL-3.0"
