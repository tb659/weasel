# 小狼毫 (Weasel) - Windows Rime 输入法

## 构建命令

**全量构建**（boost + librime + 小狼毫 + 安装包）：
```
build.bat all
```

**仅构建小狼毫**（需要预编译的 librime 在 `lib/`、`lib64/`、`output/`）：
```
build.bat weasel
```

**快速重编译**（重编所有 x64 项目）：
```
build_all.bat
```

**xmake 方式**：
```
xbuild.bat weasel
```

**构建前置条件**：
- Visual Studio，C++ 桌面开发（ATL、MFC）
- `env.bat`（从 `env.bat.template` 复制），设置 `BOOST_ROOT`
- Git 子模块已初始化：`git submodule update --init --recursive`
- 构建前必须关闭 WeaselServer（`build.bat` 自动处理）

**平台工具集**：v143（VS 2022），可通过 `env.bat` 配置

## 代码格式化

从仓库根目录执行（需要 clang-format >= 17）：
```
./clang-format.sh -i     # 原地格式化
./clang-format.sh -i     # 检查（CI 使用）
```
PowerShell 等效命令：`./clang-format.ps1 -n`（格式化）或 `-i`（检查）

风格：基于 Chromium（`BasedOnStyle: Chromium, SortIncludes: false`）

## 架构

基于命名管道的客户端-服务器 IPC，多进程架构：

- **WeaselTSF**（TSF DLL，加载到应用进程）——客户端。`WeaselTSF/`
- **WeaselIPC**——客户端库。`WeaselIPC/`
- **WeaselServer**——独立进程，托管 librime。`WeaselServer/`
- **WeaselIPCServer**——服务端 IPC 处理。`WeaselIPCServer/`
- **RimeWithWeasel**——librime 集成层（桥接 Weasel IPC 与 librime API）。`RimeWithWeasel/`
- **WeaselUI**——候选窗口渲染。`WeaselUI/`
- **WeaselDeployer**——配置/部署工具（独立进程）。`WeaselDeployer/`
- **WeaselSetup**——安装程序。`WeaselSetup/`
- **librime**——核心 Rime 引擎（git 子模块）。`librime/`
- **plum**——Rime 包管理器（git 子模块）。`plum/`

**数据流**：TSF 应用 → `WeaselTSF` → IPC → `WeaselIPCServer` → `RimeWithWeasel` → `librime` → 响应 → IPC → `WeaselTSF` → 更新 UI

**IPC 命令**定义在 `include/WeaselIPC.h`（枚举 `WEASEL_IPC_COMMAND`）

**UIStyle** 通过 Boost 序列化在 `include/WeaselIPCData.h`——修改 `UIStyle` 结构体需同步更新 `serialize` 模板

## 测试

无正式测试框架，仅有 IPC 集成测试（`test/TestWeaselIPC`、`test/TestResponseParser`）。CI 仅运行 clang-format 检查。测试主要靠手动：构建、安装、在应用中输入。

## CI

GitHub Actions（`.github/workflows/ci.yml`）：
1. **lint** 任务：ubuntu 上运行 clang-format 检查
2. **build** 任务：在 windows-2022 上用 msbuild 和 xmake 两种方式构建

## 关键陷阱

- `RimeWithWeaselHandler` 中的 `m_disabled` 控制按键事件是否被处理。`Initialize()` 中 `join_maintenance_thread()` 完成后**必须**将其重置为 `false`，否则所有输入都会被静默丢弃
- `SetOption()` **不检查** `m_disabled`——这是故意的（允许在维护期间更新托盘图标）
- `AddSession()` 在 `m_disabled` 为 true 时调用 `EndMaintenance()` → `Initialize()` 作为恢复路径
- `UIStyle` 序列化对版本敏感——添加字段需同步更新 `serialize` 模板
- ARM64 构建使用 `arm64x_wrapper/`——独立构建步骤
- 子模块 `librime` 自行构建依赖（leveldb、opencc 等）——clean 构建会缓存 librime 构建目录

##

- 使用中文回复用户
