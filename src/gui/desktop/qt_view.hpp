#pragma once

extern "C" {
#include "../common/view.h"   // ViewInterface, ViewHandle_t, ElementData_t, InputEvent_t
}

/**
 * @brief Экземпляр Qt-интерфейса для BrickGame.
 *
 * Реализует контракт ViewInterface, используя Qt-виджеты.
 */
extern const ViewInterface qt_view;