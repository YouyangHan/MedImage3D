# 04 - DICOM 扫描与序列分组

> 步骤 2 / 8：实现数据导入（目录扫描 + DCMTK 序列分组）与序列选择界面。
> 状态：✅ 编译 + 运行自检通过（2026-09-06）

---

## 1. 本步目标

完成工作流的前两环：**选目录 → 扫描分组 → 序列选择**。

| 层 | 新增文件 | 职责 |
|---|---|---|
| `dicom/` | `SeriesInfo.h` | 序列数据结构（纯数据载体） |
| `dicom/` | `DicomTagReader.h/.cpp` | 单文件 DICOM 头读取（Adapter） |
| `dicom/` | `DicomScanner.h/.cpp` | 目录扫描 + 分组（Facade） |
| `dicom/` | `ScanThread.h` | 后台扫描线程 |
| `dicom/` | `SliceImageLoader.h/.cpp` | 单张切片 → QImage 缩略图 |
| `dicom/` | `TestDataGenerator.h/.cpp` | 合成测试数据（自检用） |
| `ui/` | `SeriesListModel.h/.cpp` | 序列列表数据模型（Model-View） |
| `ui/` | `SeriesThumbDelegate.h/.cpp` | 缩略图列表项绘制代理 |
| `ui/` | `SeriesSelectDialog.h/.cpp` | 序列选择对话框 |

---

## 2. 数据流

```
目录 ──> DicomScanner.scanDirectory()                     (后台 ScanThread)
          │  collectCandidateFiles()  递归枚举候选文件
          │  DicomTagReader.open()    逐个读头标签
          │  absorbFile()             按 SeriesInstanceUID 分组
          ▼
        QList<SeriesInfo> ──信号──> MainWindow::onScanFinished
                                    │  SeriesSelectDialog.exec()
                                    ▼
                                  用户选中序列 -> (步骤3 体数据加载)
```

---

## 3. 数据结构 SeriesInfo

[dicom/SeriesInfo.h](../src/dicom/SeriesInfo.h) —— 纯数据（POD 风格），无业务逻辑。

```cpp
struct SeriesInfo
{
    QString seriesUID;         // 分组键 SeriesInstanceUID
    QString seriesDescription; // 序列描述
    int     seriesNumber = 0;
    QString modality;
    QString patientName;
    QString studyDate;
    QSize   imageSize;
    QStringList filePaths;     // 已按实例号升序
    QImage   thumbnail;        // 首图缩略图
    int imageCount() const { return filePaths.size(); }
};
Q_DECLARE_METATYPE(SeriesInfo)   // 允许跨线程信号传递 QList<SeriesInfo>
```

**关键点**：`Q_DECLARE_METATYPE` + 在 [main.cpp](../src/main.cpp) 里
`qRegisterMetaType<QList<SeriesInfo>>()` 是**必须的**——`ScanThread`(后台) 的
`scanFinished` 信号经**队列连接**发给主线程，Qt 要在两线程间拷贝该参数，
必须登记元类型。

---

## 4. 单文件头读取 DicomTagReader（Adapter）

[dicom/DicomTagReader.h](../src/dicom/DicomTagReader.h) 把 DCMTK 的
`DcmFileFormat/DcmDataset` 适配成"open + 语义化 getter"：

```cpp
bool DicomTagReader::open(const QString& filePath)
{
    auto file = std::make_unique<DcmFileFormat>();
    const OFCondition status = file->loadFile(
        filePath.toUtf8().constData(), EXS_Unknown, EGL_withoutGL,
        kMaxHeaderReadBytes /*64KB*/, ERM_autoDetect);   // 只读头, 快
    ...
}
```

**设计要点**（两处易错，见 [DicomTagReader.h:26-30](../src/dicom/DicomTagReader.h#L26-L30)）：

1. `DcmFileFormat` 仅前向声明，成员用 `std::unique_ptr<DcmFileFormat>`；
2. **构造与析构都定义在 .cpp**（`= default` 内联版会为异常安全生成
   `unique_ptr` 的销毁代码，而此处类型不完整 → 触发 "delete 不完整类型" 编译错）。

标签读取集中在匿名命名空间的 `tagString/tagInt/tagDouble`（[DicomTagReader.cpp](../src/dicom/DicomTagReader.cpp)），
通过 `DCM_SeriesInstanceUID` 等常量（`<dcmtk/dcmdata/dcdeftag.h>`）取字段。

---

## 5. 目录扫描 DicomScanner（Facade）

[dicom/DicomScanner.cpp](../src/dicom/DicomScanner.cpp) 封装整条流水线：

```cpp
QList<SeriesInfo> DicomScanner::scanDirectory(const QString& dirPath)
{
    // 1. collectCandidateFiles(): QDirIterator 递归, 跳过 <256B 和已知非 DICOM 扩展名
    // 2. 逐文件 reader.open() -> absorbFile() 按 SeriesInstanceUID 分组
    // 3. 组内排序: 用 "实例号|层面位置|路径" 定宽编码后 sort, 再剥前缀
    // 4. 首图缩略图: SliceImageLoader::load(first, 96)
    // 5. 序列间排序: 序列号升序
}
```

**排序编码技巧**（[DicomScanner.cpp:79-90](../src/dicom/DicomScanner.cpp#L79-L90)）：
把排序键和路径拼成 `QString`，定宽补零后字符串字典序 == 数值序，避免平行数组：

```cpp
const QString sortKey = QStringLiteral("%1|%2|%3")
    .arg(reader.instanceNumber(), 8, 10, QLatin1Char('0'))
    .arg(reader.sliceLocation(), 12, 'f', 4, QLatin1Char('0'))
    .arg(filePath);
group.filePaths << sortKey;   // 之后统一 sort() 再 section('|') 取第2段
```

取消用 `std::atomic<bool>`（[DicomScanner.h](../src/dicom/DicomScanner.h)），
`scanDirectory()` 循环内 `m_cancelRequested.load()` 检查，协作式退出。

---

## 6. 缩略图 SliceImageLoader

[dicom/SliceImageLoader.cpp](../src/dicom/SliceImageLoader.cpp) 用 DCMTK `DicomImage`：

```cpp
DicomImage dicomImage(path, 0, 0, 1);        // 只取第1帧
const void* raw = dicomImage.getOutputData(8); // 应用窗宽窗位后输出 8bit 灰度
QImage image(w, h, QImage::Format_Grayscale8);
for (int y = 0; y < h; ++y)
    memcpy(image.scanLine(y), src + y*w, w);  // QImage 行对齐, 逐行拷贝
```

---

## 7. 后台扫描 ScanThread

[dicom/ScanThread.h](../src/dicom/ScanThread.h)：继承 QThread，进度与结果经信号
发回 GUI 线程（自动队列连接）：

```cpp
void run() override {
    m_scanner.setProgressCallback([this](int d, int t){ emit progressChanged(d, t); });
    emit scanFinished(m_scanner.scanDirectory(m_dirPath));
}
```

---

## 8. 序列选择界面（Model-View）

- [SeriesListModel](../src/ui/SeriesListModel.cpp)：`QAbstractListModel` 提供
  `DisplayRole`(两行文字) / `DecorationRole`(缩略图) / `ToolTipRole`(完整信息)。
- [SeriesThumbDelegate](../src/ui/SeriesThumbDelegate.cpp)：自定义绘制"缩略图+两行文字+选中高亮"的卡片式条目。
- [SeriesSelectDialog](../src/ui/SeriesSelectDialog.cpp)：左列表 + 右大预览 + 确定/取消。

```cpp
// 选中变化 -> 重新加载大图预览(清晰度高于 96px 缩略图)
void SeriesSelectDialog::updatePreview(const SeriesInfo& s) {
    QImage image = SliceImageLoader::load(s.filePaths.first(), kPreviewSize/*512*/);
    m_preview->setPixmap(QPixmap::fromImage(image));
}
```

---

## 9. 主窗口接线（工作流前两环）

[MainWindow.cpp](../src/MainWindow.cpp)：

1. `onOpenCtDirectory()`：`QFileDialog` 选目录（优先 `Settings::lastDirectory()`）→ `startScan()`；
2. `startScan()`：弹 `QProgressDialog`，`new ScanThread`，连接进度/完成信号；
3. `onScanFinished()`：关进度框，空结果弹警告，否则 `SeriesSelectDialog.exec()`。

---

## 10. 自检

`main.cpp` 内置两个命令行开关（无界面，便于 CI/冒烟测试）：

```bat
MedImage3D.exe --gen-test-data <目录>     :: 生成两套假 CT 序列(12+8张)
MedImage3D.exe --scan-smoke-test <目录>   :: 生成数据 + 扫描并打印结果
```

实测输出：

```
扫描到序列数: 2
  [序列 1] Head CT  张数=12  尺寸=64x64  缩略图=有
  [序列 2] Chest CT  张数=8   尺寸=64x64  缩略图=有
```

`TestDataGenerator`（[dicom/TestDataGenerator.cpp](../src/dicom/TestDataGenerator.cpp)）
写合法 CT DICOM：`UID_CTImageStorage` SOP、`RescaleIntercept=-1024`、`WindowCenter/Width`、
16bit MONOCHROME2 像素，每层像素为随层号右移的亮圆（肉眼可验证排序）。

---

## 11. 设计模式小结

| 模式 | 应用 |
|---|---|
| Facade | `DicomScanner` 封装扫描→分组→缩略图整条流水线 |
| Adapter | `DicomTagReader` 适配 DCMTK 复杂接口 |
| Model-View | `SeriesListModel` + `QListView` + `SeriesThumbDelegate` |
| 线程模型 | `ScanThread`(QThread) + 信号槽队列连接 |

**下一步**：步骤 3 —— `VolumeLoader` + `LoadThread` + `core/Volume` + `DataRepository`（体数据加载），
对应 [05-体数据加载.md](05-体数据加载.md)。
