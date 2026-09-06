#include "Logging.h"

// ============================================================================
// 日志类别的定义(每个类别必须在此用 Q_LOGGING_CATEGORY 定义一次)
//
// 类别名(第二个参数)可在运行时通过环境变量过滤, 例如只输出 dicom 日志：
//   QT_LOGGING_RULES="dicom.debug=true;*.debug=false"
// ============================================================================

Q_LOGGING_CATEGORY(lcApp, "app")
Q_LOGGING_CATEGORY(lcDicom, "dicom")
Q_LOGGING_CATEGORY(lcRender, "render")
