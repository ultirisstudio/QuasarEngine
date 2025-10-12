#pragma once

#include "UIDebugOverlay.h"
#include <sstream>

#define UI_DIAG_INFO(msg)  do { QuasarEngine::UIDebugOverlay::Instance().Post(QuasarEngine::UIDebugOverlay::Severity::Info,  (msg)); } while(0)
#define UI_DIAG_WARN(msg)  do { QuasarEngine::UIDebugOverlay::Instance().Post(QuasarEngine::UIDebugOverlay::Severity::Warn,  (msg)); } while(0)
#define UI_DIAG_ERROR(msg) do { QuasarEngine::UIDebugOverlay::Instance().Post(QuasarEngine::UIDebugOverlay::Severity::Error, (msg)); } while(0)

#define UI_DIAG_CHECK(cond, msg) do { if(!(cond)) { UI_DIAG_ERROR(std::string("CHECK FAILED: ") + (msg)); } } while(0)
