#pragma once

// ============================================================================
// DicomTagReader —— 单个 DICOM 文件头读取器
//
// 设计模式：适配器(Adapter)
//   把 DCMTK 的 DcmFileFormat/DcmDataset 复杂接口，适配成
//   "open() + 一组语义化 getter" 的简单接口，上层无需了解 DCMTK 细节。
//
// 使用方式：
//   DicomTagReader reader;
//   if (reader.open(path)) { QString uid = reader.seriesInstanceUID(); ... }
//
// 线程安全：每个实例独立持有数据，多线程各用各的实例即可。
// ============================================================================

#include <QSize>
#include <QString>

#include <memory>

class DcmFileFormat;
class DcmDataset;

class DicomTagReader
{
public:
    DicomTagReader();
    // 构造/析构都在 .cpp 定义：内联版本会为异常安全生成 unique_ptr 的销毁代码，
    // 而 DcmFileFormat 在此仅前向声明，导致"delete 不完整类型"。
    ~DicomTagReader();

    // 打开文件并读取头部标签；非 DICOM 文件返回 false（不会产生异常）。
    // 只读取有限长度(不含像素数据)，大文件也很快。
    bool open(const QString& filePath);

    bool isOpen() const { return m_dataset != nullptr; }

    // ---- 语义化标签 getter（未打开或标签缺失时返回空串/0）----
    QString seriesInstanceUID() const;   // 序列实例 UID（分组键）
    QString seriesDescription() const;   // 序列描述
    int     seriesNumber() const;        // 序列号
    int     instanceNumber() const;      // 实例号（序列内排序用）
    QString modality() const;            // 设备类型
    QString patientName() const;         // 患者姓名
    QString studyDate() const;           // 检查日期
    QSize   imageSize() const;           // 图像宽高
    double  sliceLocation() const;       // 层面位置（排序兜底用）

private:
    // unique_ptr 自动释放 DCMTK 对象；拷贝语义无意义，直接删除
    DicomTagReader(const DicomTagReader&) = delete;
    DicomTagReader& operator=(const DicomTagReader&) = delete;

    std::unique_ptr<DcmFileFormat> m_fileFormat; // 拥有文件对象
    DcmDataset* m_dataset = nullptr;             // 指向其内部数据集（不拥有）
};
