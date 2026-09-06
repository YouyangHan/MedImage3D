#include "AppController.h"

AppController::AppController(QObject* parent)
    : QObject(parent)
{
}

void AppController::transitionTo(WorkflowState s)
{
    if (m_state == s)
        return;
    m_state = s;
    emit stateChanged(s);
}
