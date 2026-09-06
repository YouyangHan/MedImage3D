#pragma once

// ============================================================================
// AppController —— 工作流控制器
//
// 设计模式：状态机(State)
//   以枚举状态 + transitionTo() 驱动 §1.2 的工作流，状态变化经 Qt 信号广播，
//   MainWindow 等 UI 层据此启用/禁用按钮、切换页面。
//
// 工作流(见 docs/02 §1.2)：
//   Idle -> Scanning -> SeriesSelect -> Loading -> FourView
//   (取消可回退 Idle / SeriesSelect)
// ============================================================================

#include <QObject>

// 工作流状态
enum class WorkflowState {
    Idle,          // 空闲(启动后 / 取消后)
    Scanning,      // 正在扫描 DICOM 目录
    SeriesSelect,  // 序列选择(序列面板显示中)
    Loading,       // 正在加载体数据
    FourView,      // 四视图显示(重建完成)
};

class AppController : public QObject
{
    Q_OBJECT

public:
    explicit AppController(QObject* parent = nullptr);

    // 当前状态
    WorkflowState state() const { return m_state; }

    // 状态切换(相同状态直接忽略)，并广播 stateChanged
    void transitionTo(WorkflowState s);

signals:
    // 状态变化(供 UI 层响应)
    void stateChanged(WorkflowState state);

private:
    WorkflowState m_state = WorkflowState::Idle;
};
