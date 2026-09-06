#include "SliceImageLoader.h"

#include "common/Logging.h"

#include <dcmtk/dcmimgle/dcmimage.h>   // DicomImage

#include <cstring>

namespace SliceImageLoader {

QImage load(const QString& filePath, int maxSize)
{
    // 只取第 1 帧(fstart=0, fcount=1)，避免多帧文件占用过多内存
    DicomImage dicomImage(filePath.toUtf8().constData(), /*flags=*/0, /*fstart=*/0, /*fcount=*/1);

    if (dicomImage.getStatus() != EIS_Normal) {
        LOG_DEBUG(lcDicom, "DicomImage 打开失败: " << filePath);
        return {};
    }
    // CT 序列均为单帧灰度；彩色(如超声截图)不在本软件范围内
    if (!dicomImage.isMonochrome()) {
        LOG_DEBUG(lcDicom, "跳过彩色图像: " << filePath);
        return {};
    }

    const int width  = static_cast<int>(dicomImage.getWidth());
    const int height = static_cast<int>(dicomImage.getHeight());
    if (width <= 0 || height <= 0)
        return {};

    // getOutputData(8): 按当前窗宽窗位渲染出 8 位灰度缓冲(每像素 1 字节)
    const void* raw = dicomImage.getOutputData(8);
    if (!raw)
        return {};

    QImage image(width, height, QImage::Format_Grayscale8);
    const uchar* src = static_cast<const uchar*>(raw);
    // QImage 每行按 32 位对齐，行宽可能与图像宽不同，逐行拷贝
    for (int y = 0; y < height; ++y)
        std::memcpy(image.scanLine(y), src + static_cast<size_t>(y) * width,
                    static_cast<size_t>(width));

    if (maxSize > 0 && (width > maxSize || height > maxSize))
        image = image.scaled(maxSize, maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    return image;
}

} // namespace SliceImageLoader
