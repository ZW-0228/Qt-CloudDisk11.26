# ☁️ Qt 网盘系统（Qt Cloud Disk）

基于 **Qt + C++ + MySQL** 开发的 C/S 架构在线云存储系统，支持多用户注册登录、文件上传下载、MD5 秒传、断点续传、文件分享等完整网盘功能。

![Language](https://img.shields.io/badge/language-C%2B%2B17-red)
![Framework](https://img.shields.io/badge/framework-Qt%205.15.2-green)
![Database](https://img.shields.io/badge/database-MySQL-blue)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)

## ✨ 功能特性

- 👤 **用户系统** — 注册 / 登录（用户名 + 密码 + 手机号，唯一性校验）
- 📁 **文件管理** — 文件列表浏览、上传、下载、删除、关键字搜索
- ⚡ **MD5 秒传** — 上传前校验文件 MD5，服务器已有相同文件则直接建立映射，秒级完成
- 🔄 **断点续传** — 服务端记录传输偏移量，中断后可从未完成的偏移继续上传
- 📦 **分块传输** — 文件内容按 4 KB 分块传输
- 🔗 **文件分享** — 生成 4 位提取码，他人凭提取码把文件加入自己的网盘
- 🌐 **多客户端并发** — 服务端基于 `select()` I/O 多路复用

## 🏗️ 项目架构

C/S 三层架构，客户端与服务器通过 **TCP 自定义二进制协议**（4 字节长度前缀 + 结构体序列化）通信。

```
┌────────────────────────────────────────────┐
│              客户端 (3-29-client)           │
│   UI 层 (Qt Widgets: login/register/...)   │
│      ↕ 信号槽                                │
│   业务层 (Kernel : IKernel)                │
│      ↕                                      │
│   网络层 (tcpnet : Winsock2)                │
└──────────────────┬─────────────────────────┘
                   │ TCP :8899
┌──────────────────┴─────────────────────────┐
│              服务端 (3-21-server)           │
│   业务层 (Kernel : 饿汉单例)                │
│      ↕                                      │
│   网络层 (TCPNet : select 多路复用)         │
│   数据层 (CMySql : MySQL C API)             │
└──────────────────┬─────────────────────────┘
                   ↓
            MySQL 数据库 (3-22disk)
```

## 📂 目录结构

```
Qt-CloudDisk/
├── 3-21-server/              # 服务端（Qt Core 控制台程序）
│   ├── 3-21-server.pro
│   ├── main.cpp
│   ├── packdef.h             # 通信协议结构体定义
│   ├── Kernel/               # 业务层（lKernel.h + kernel.cpp 单例）
│   ├── netWork/              # 网络层（INet.h + tcpnet.cpp，select 模型）
│   └── CMySQL/               # 数据层（MySQL C API 封装）
│
├── 3-29-client/              # 客户端（Qt Widgets）
│   ├── 3-29-client.pro
│   ├── main.cpp
│   ├── login / register / mainwindow / dialog   # 界面层（.ui + .cpp）
│   ├── packdef.h             # 与服务端保持一致的协议定义
│   ├── image.qrc             # 图标与背景图资源
│   ├── Kernel/               # 业务层（IKernel.h + Kernel.cpp）
│   ├── TcpNet/               # 网络层（INet.h + tcpnet.cpp）
│   └── MD5/                  # MD5 校验工具
│
├── disk_file/                # 服务端文件存储根目录（用户文件按 u_id 分目录）
├── server.txt                # 数据库建表 SQL（见下方说明）
└── .gitignore
```

## 📡 通信协议

所有数据包继承自 `STRU_BASE`（首字节 `m_nType` 标识包类型），协议定义集中在两端的 `packdef.h`：

| 协议 | 类型值 | 说明 |
|------|:---:|------|
| `register_rq / rs` | 1 / 2 | 注册请求 / 响应 |
| `login_rq / rs` | 3 / 4 | 登录请求 / 响应（返回 u_id） |
| `getfilelist_rq / rs` | 5 / 6 | 获取文件列表（每包最多 15 条） |
| `uploadfileinfo_rq / rs` | 7 / 8 | 上传文件元数据（MD5 判定秒传/续传） |
| `uploadfilecontent_rq / rs` | 9 / 10 | 上传文件内容（4 KB 分块） |
| `deletefile_rq / rs` | 11 / 12 | 删除文件 |
| `downloadfileinfo_rq / rs` | 13 / 14 | 下载文件元数据 |
| `downloadfilecontent_rq / rs` | 15 / 16 | 下载文件内容 |
| `selectfile_rq / rs` | 17 / 18 | 关键字搜索文件 |
| `sharelink_rq / rs` | 19 / 20 | 生成分享提取码 |
| `getlink_rq / rs` | 21 / 22 | 凭提取码提取文件 |

传输格式：`[4 字节包长][包体]`，包体即对应结构体的原始内存布局。

## 🗄️ 数据库设计

库名 **`3-22disk`**。

| 表 / 视图 | 说明 |
|---------|------|
| `user` | 用户表（u_id, u_name, u_password, u_tel），用户名 / 手机号唯一 |
| `file` | 文件表（f_id, f_name, f_size, f_uploadtime, f_path, f_count, f_md5），`f_count` 为引用计数，支持秒传 |
| `user_file` | 用户 — 文件多对多关联表（u_id, f_id, time） |
| `user_shared` | 分享表（uid, fid, code） |
| `ufile` | 视图：`user_file` 关联 `file`，供列表 / 搜索 / 下载查询 |

> **关于 `server.txt`**：这是早期版本的建表脚本，使用时需要注意两点：
> 1. 视图 `ufile` 定义里写死了 schema 名 `server`，需要改成你的实际库名 `3-22disk`；
> 2. 其中**未包含 `user_shared` 表**，分享功能需要自行创建（字段：`uid`、`fid`、`code`）。

## 🛠️ 开发环境

| 项目 | 配置 |
|------|------|
| IDE | Qt Creator |
| Qt | 5.15.2（MinGW 64-bit） |
| 编译器 | MinGW 64-bit（GCC），C++17 |
| 网络 | Winsock2（`-lws2_32`） |
| 数据库 | MySQL 5.7 / 8.x（C API，需配置 `libmysql.lib`） |
| 平台 | Windows |

## 🚀 构建与运行

```bash
# 1. 初始化数据库：在 MySQL 中创建库 3-22disk，执行 server.txt 建表（注意上面提到的两点）

# 2. 编译服务端
cd 3-21-server
qmake 3-21-server.pro && mingw32-make

# 3. 编译客户端
cd ../3-29-client
qmake 3-29-client.pro && mingw32-make
```

运行顺序：先启动服务端（监听 `8899`，并连接 MySQL），再启动客户端（默认连 `127.0.0.1:8899`）。

## ⚙️ 本地配置

以下内容与机器强相关，克隆后需要按自己环境修改：

| 位置 | 需要修改的内容 |
|------|------|
| `3-21-server/Kernel/kernel.cpp` — `Kernel::open()` | MySQL 主机、账号、密码、库名 |
| `3-21-server/Kernel/kernel.cpp` — `Kernel` 构造函数 | 文件存储根路径 `m_szSystemPath` |
| `3-21-server/CMySQL/CMySQL.pri` | `libmysql.lib` 与 MySQL `include` 目录的绝对路径 |
| `3-29-client/TcpNet/tcpnet.cpp` | 服务端 IP 与端口（默认 `127.0.0.1:8899`） |

## 📝 说明

- 本项目为学习性项目，网络层基于 Winsock2，**仅支持 Windows 平台**
- 无自动化测试，需手动启动服务端 + 客户端联调
- 数据库密码等敏感信息以明文形式写在 `kernel.cpp` 中，仅用于本地学习，请勿用于生产环境
