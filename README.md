# MedImage3D

基于 Qt6 + VTK + ITK + DCMTK 的医学图像三维重建软件（C++17 / CMake）。

读取 CT 序列（DICOM）→ 序列选择 → 三维重建 → 矢状面 / 冠状面 / 横断面 / 三维体绘制 四视图联动显示。

> 参考工程：三维重建方法源自 `3d-xmake`，辅助类功能参考 `3dpre`。

---

## 功能特性

- **DICOM 扫描与序列分组**：DCMTK 解析目录，按序列分组并生成缩略图
- **序列选择界面**：左侧序列列表（每个序列显示首图），右侧大窗口预览所选序列
- **MPR 多平面重建**：基于 `vtkResliceImageViewer` 的矢状面 / 冠状面 / 横断面三视图，十字线联动
- **三维体绘制**：`vtkSmartVolumeMapper` GPU 体渲染，骨骼 / 软组织 / 肌肉等窗宽窗位预设
- **四视图联动**：三个切片视图 + 一个三维视图共享同一光标（Mediator + Observer）
- **中文界面**：按钮带图标 + 中文 ToolTip，文案 / 路径资源与代码分离

## 技术栈

| 组件 | 版本 | 说明 |
|---|---|---|
| C++ 标准 | C++17 | MSVC 2022 (v143) |
| Qt | 6.12.0 | msvc2022_64，GUI 框架 |
| VTK | 9.6.2 | 可视化（MPR / 体绘制），含 Qt 支持模块 |
| ITK | 5.4.7 | 图像处理，GDCMImageIO 读取 DICOM 像素，ITKVtkGlue 桥接 VTK |
| DCMTK | 3.7.0 | DICOM 头解析 / 序列分组 |
| CMake | ≥ 3.25 | Qt 6.12 要求；便携版随附于 `ThirdParty/_tools` |
| 构建器 | Ninja | VS2022 自带 |

## 目录结构

```
MedImage3D/
├── CMakeLists.txt          主工程(相对路径, 无绝对路径)
├── CMakePresets.json       配置预设(Qt6_ROOT 取 $env{QTDIR})
├── cmake/                  FindVTK/FindITK/FindDCMTK + DeployRuntime
│   ├── FindVTK.cmake       查找 ThirdParty/VTK 并转发 CONFIG 包
│   ├── FindITK.cmake
│   ├── FindDCMTK.cmake
│   └── DeployRuntime.cmake 构建后自动拷贝 DLL + windeployqt
├── src/                    源码
│   ├── main.cpp            入口
│   ├── resources/          资源常量(中文文案 / 路径), 与代码分离
│   ├── common/             基础设施(Singleton/Logging/Settings/MathUtils)
│   ├── dicom/              DICOM 扫描与读取(规划中)
│   ├── core/               体数据与仓库(规划中)
│   ├── view/               MPR / 体绘制视图(规划中)
│   └── ui/                 序列选择等界面(规划中)
├── ThirdParty/
│   └── scripts/            VTK/ITK/DCMTK 及主程序的一键构建脚本
│   (VTK/ITK/DCMTK/_build/_src/_tools 为本地构建产物, 不入库)
└── docs/                   开发文档(每步一篇, 含关键代码与位置)
    ├── 01-环境搭建.md
    ├── 02-架构设计.md
    ├── 03-基础设施与资源.md
    └── …
```

## 环境要求与构建

**前置条件**（环境变量方式定位，CMake 全程相对路径，不含绝对路径）：

- Windows 10/11 x64 + Visual Studio 2022（v143 工具集）
- Qt 6.12.0 (msvc2022_64)，并设置环境变量 **`QTDIR`** 指向 Qt 根目录
  （如 `...\Qt\6.12.0\msvc2022_64`），脚本经 `%QTDIR%` / `$env{QTDIR}` 引用

**构建步骤**：

```bat
:: 1. 编译第三方库(一次性, 耗时较长, 输出至 ThirdParty/ 下)
ThirdParty\scripts\build_dcmtk.bat
ThirdParty\scripts\build_vtk.bat
ThirdParty\scripts\build_itk.bat

:: 2. 配置并构建主程序(输出至 build/, 自动拷贝全部依赖 DLL)
ThirdParty\scripts\configure_app.bat
ThirdParty\scripts\build_app.bat
```

构建完成后运行 `build\MedImage3D.exe`。

## 开发进度（八步计划）

| 步骤 | 内容 | 状态 | 文档 |
|---|---|---|---|
| 1 | 基础设施 + 资源（Singleton/Logging/Settings/MathUtils） | ✅ | [03](docs/03-基础设施与资源.md) |
| 2 | DicomScanner + DicomTagReader（目录扫描 + 序列分组） | ✅ | [04](docs/04-DICOM扫描与序列分组.md) |
| 3 | VolumeLoader + LoadThread + Volume + DataRepository | ✅ | [05](docs/05-体数据加载.md) |
| 4 | ResliceViewer + SliceView（MPR 切片视图） | ✅ | [06](docs/06-MPR切片视图.md) |
| 5 | VolumeView + TransferFunctionFactory（体绘制） | ✅ | [07](docs/07-体绘制视图.md) |
| 6 | ViewManager 四视图联动 | ✅ | [08](docs/08-四视图联动.md) |
| 7 | SeriesSelectDialog 序列选择界面 | ✅ | [04](docs/04-DICOM扫描与序列分组.md) |
| 8 | MainWindow + AppController 工作流状态机 | ⬜ | 10 |

## 开发文档

- [01 - 环境搭建](docs/01-环境搭建.md)：版本选型分析、第三方库编译、相对路径约定、FAQ
- [02 - 架构设计](docs/02-架构设计.md)：分层架构、工作流状态机、重建原理、设计模式清单
- [03 - 基础设施与资源](docs/03-基础设施与资源.md)：common/resources 两层的关键代码与位置
- [04 - DICOM 扫描与序列分组](docs/04-DICOM扫描与序列分组.md)：DicomScanner/DicomTagReader 数据导入与序列选择界面
- [05 - 体数据加载](docs/05-体数据加载.md)：VolumeLoader(ITK GDCM)/LoadThread/Volume/DataRepository
- [06 - MPR 切片视图](docs/06-MPR切片视图.md)：ResliceViewer/SliceView 三视图基础
- [07 - 体绘制视图](docs/07-体绘制视图.md)：VolumeView/TransferFunctionFactory 三维体绘制与传递函数预设
- [08 - 四视图联动](docs/08-四视图联动.md)：ViewManager 四视图编排与十字线/翻层联动

## 许可证

仅供学习与研究使用。
