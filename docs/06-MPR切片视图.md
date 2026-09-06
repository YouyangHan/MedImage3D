# 06 - MPR 切片视图

> 步骤 4 / 8：实现 MPR 切片视图组件（矢状/冠状/横断三视图基础）。
> 状态：✅ 编译通过（2026-09-06）

---

## 1. 本步目标

实现 MPR 三视图的基础组件：`ResliceViewer`(VTK 查看器子类) + `SliceView`(QWidget 包装)，
并在主窗口展示矢状/冠状/横断三个切片视图（共享十字线光标）。

| 层 | 新增文件 | 职责 |
|---|---|---|
| `view/` | `ResliceViewer.h/.cpp` | vtkResliceImageViewer 子类（Adapter） |
| `view/` | `SliceView.h/.cpp` | QWidget 包装 QVTKOpenGLNativeWidget + viewer |

> 三视图的交互联动（十字线拖动同步、滚轮翻层）在步骤 6 由 ViewManager 统一管理。

---

## 2. MPR 原理

**MPR（Multi-Planar Reconstruction，多平面重建）**：把三维体数据沿三个正交平面
重采样出二维切片：

```
vtkResliceImageViewer(内部 vtkImageReslice 斜切重采样)
  ├─ WindowLevel = vtkImageMapToWindowLevelColors   (窗宽窗位)
  └─ ResliceCursor = vtkResliceCursor               (十字线光标)
        └─ 三个视图共享同一光标 -> 十字线自动同步
```

方向约定（vtkImageViewer2 常量）：

| 常量 | 值 | 切面 |
|---|---|---|
| `SLICE_ORIENTATION_YZ` | 0 | 矢状面(Sagittal) |
| `SLICE_ORIENTATION_XZ` | 1 | 冠状面(Coronal) |
| `SLICE_ORIENTATION_XY` | 2 | 横断面(Axial) |

---

## 3. ResliceViewer（Adapter）

[view/ResliceViewer.cpp](../src/view/ResliceViewer.cpp) —— 把 VTK 复杂 pipeline 适配成三个语义化方法：

```cpp
void ResliceViewer::setVolume(vtkImageData* image)
{
    this->SetInputData(image);                       // 内部设置 reslice cursor 的 image
    double range[2];
    image->GetScalarRange(range);
    this->SetColorWindow(range[1] - range[0]);       // 默认窗宽窗位(全范围)
    this->SetColorLevel((range[0] + range[1]) / 2.0);
    // 重采样背景色设为体素最小值(切出体外的区域用最小值填充)
    ...
    this->Reset();                                   // 重置相机到体数据
}
```

构造函数（[ResliceViewer.cpp](../src/view/ResliceViewer.cpp)）：
- `SetResliceModeToAxisAligned()`：轴对齐(正交切片)模式
- `SetSliceScrollOnMouseWheel(1)`：滚轮翻层

`setSharedCursor()` 调用 `SetResliceCursor()`，是**三视图联动的基础**——三个 viewer
指向同一 `vtkResliceCursor`，光标中心/轴向全局唯一。

---

## 4. SliceView（QWidget 包装）

[view/SliceView.cpp](../src/view/SliceView.cpp) 封装 `QVTKOpenGLNativeWidget` + viewer：

```cpp
SliceView::SliceView(QWidget* parent)
{
    m_vtkWidget = new QVTKOpenGLNativeWidget(this);
    m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    m_viewer = vtkSmartPointer<ResliceViewer>::New();
    m_viewer->SetRenderWindow(m_renderWindow);
    m_vtkWidget->setRenderWindow(m_renderWindow);
    m_viewer->SetupInteractor(m_vtkWidget->interactor());   // 十字线 widget 需 interactor
}
```

> 注意：`vtkImageViewer2::SetupInteractor` 只接受 1 个参数(interactor)，
> render window 已由 `SetRenderWindow` 设置。

---

## 5. 三视图搭建（主窗口）

[MainWindow.cpp](../src/MainWindow.cpp) `setupMprViews()`：

```cpp
// 共享十字线光标
m_cursor->SetImage(volume.imageData);
m_cursor->SetCenter(volume.imageData->GetCenter());

// 三个方向各一个 SliceView，2x2 网格(右下留待步骤 5 的体绘制)
const int orientations[3] = {
    vtkImageViewer2::SLICE_ORIENTATION_XY,   // 横断
    vtkImageViewer2::SLICE_ORIENTATION_YZ,   // 矢状
    vtkImageViewer2::SLICE_ORIENTATION_XZ,   // 冠状
};
for (int i = 0; i < 3; ++i) {
    auto* sv = new SliceView(m_viewContainer);
    sv->setVolume(volume.imageData);
    sv->setOrientation(orientations[i]);
    sv->setSharedCursor(m_cursor);           // 三个视图共享同一光标
    ...
}
```

工作流更新：加载完成 `onLoadFinished` → `setupMprViews` 切换到三视图；
重新扫描时 `onScanFinished` 切回序列面板。

---

## 6. 设计模式小结

| 模式 | 应用 |
|---|---|
| 适配器(Adapter) | `ResliceViewer` 适配 vtkResliceImageViewer 复杂 pipeline |
| 封装(Wrapper) | `SliceView` 封装 QVTKOpenGLNativeWidget + viewer |

**下一步**：步骤 5 —— `view/VolumeView` + `TransferFunctionFactory`（三维体绘制 + 传递函数预设），
对应 [07-体绘制视图.md](07-体绘制视图.md)。
