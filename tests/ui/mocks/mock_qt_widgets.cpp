// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mock_qt_widgets.h"

namespace Sudachi::Multiplayer::Testing {

// Static member definitions for MockMessageBox
MockMessageBox::MessageBoxCall MockMessageBox::last_critical_call;
MockMessageBox::MessageBoxCall MockMessageBox::last_warning_call;
MockMessageBox::MessageBoxCall MockMessageBox::last_info_call;
int MockMessageBox::mock_return_value = QMessageBox::Ok;

// Static member definitions for QtTestUtilities
QtTestUtilities::KeyPressEvent QtTestUtilities::last_key_press;

} // namespace Sudachi::Multiplayer::Testing

// Include MOC files for Qt objects
#include "mock_qt_widgets.moc"