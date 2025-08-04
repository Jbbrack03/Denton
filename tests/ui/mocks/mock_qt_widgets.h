// SPDX-FileCopyrightText: 2025 Sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QDialog>
#include <QMessageBox>
#include <QTimer>
#include <QSignalSpy>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace Sudachi::Multiplayer::Testing {

/**
 * Mock Qt Widget base class for UI testing
 * Provides controlled behavior for widget interactions
 */
class MockQWidget : public QWidget {
    Q_OBJECT

public:
    explicit MockQWidget(QWidget* parent = nullptr) : QWidget(parent) {}

    MOCK_METHOD(void, show, (), (override));
    MOCK_METHOD(void, hide, (), (override));
    MOCK_METHOD(void, setEnabled, (bool enabled), (override));
    MOCK_METHOD(void, setVisible, (bool visible), (override));
    MOCK_METHOD(void, update, (), (override));
    MOCK_METHOD(void, repaint, (), (override));
    MOCK_METHOD(QSize, sizeHint, (), (const, override));
    MOCK_METHOD(void, resize, (int w, int h), (override));
    MOCK_METHOD(void, move, (int x, int y), (override));

signals:
    void mockSignalEmitted();
    void stateChanged(int state);
};

/**
 * Mock Push Button for multiplayer mode toggle testing
 */
class MockPushButton : public QPushButton {
    Q_OBJECT

public:
    explicit MockPushButton(const QString& text, QWidget* parent = nullptr) 
        : QPushButton(text, parent) {}

    MOCK_METHOD(void, setText, (const QString& text), (override));
    MOCK_METHOD(QString, text, (), (const, override));
    MOCK_METHOD(void, setIcon, (const QIcon& icon), (override));
    MOCK_METHOD(void, setCheckable, (bool checkable), (override));
    MOCK_METHOD(void, setChecked, (bool checked), (override));
    MOCK_METHOD(bool, isChecked, (), (const, override));
    MOCK_METHOD(void, setStyleSheet, (const QString& styleSheet), (override));
    MOCK_METHOD(void, animateClick, (int msec), (override));

    // Mock click simulation
    void simulateClick() {
        emit clicked();
    }

    void simulateToggle(bool checked) {
        emit toggled(checked);
    }

signals:
    void clicked();
    void toggled(bool checked);
};

/**
 * Mock Label for status display testing
 */
class MockLabel : public QLabel {
    Q_OBJECT

public:
    explicit MockLabel(const QString& text, QWidget* parent = nullptr)
        : QLabel(text, parent) {}

    MOCK_METHOD(void, setText, (const QString& text), (override));
    MOCK_METHOD(QString, text, (), (const, override));
    MOCK_METHOD(void, setPixmap, (const QPixmap& pixmap), (override));
    MOCK_METHOD(void, setStyleSheet, (const QString& styleSheet), (override));
    MOCK_METHOD(void, setAlignment, (Qt::Alignment alignment), (override));
    MOCK_METHOD(void, setWordWrap, (bool on), (override));

    // Helper for testing text changes
    void simulateTextChange(const QString& newText) {
        setText(newText);
        emit textChanged(newText);
    }

signals:
    void textChanged(const QString& text);
};

/**
 * Mock Progress Bar for connection status testing
 */
class MockProgressBar : public QProgressBar {
    Q_OBJECT

public:
    explicit MockProgressBar(QWidget* parent = nullptr) : QProgressBar(parent) {}

    MOCK_METHOD(void, setValue, (int value), (override));
    MOCK_METHOD(int, value, (), (const, override));
    MOCK_METHOD(void, setRange, (int minimum, int maximum), (override));
    MOCK_METHOD(void, setMaximum, (int maximum), (override));
    MOCK_METHOD(void, setMinimum, (int minimum), (override));
    MOCK_METHOD(void, setFormat, (const QString& format), (override));
    MOCK_METHOD(void, setTextVisible, (bool visible), (override));

    // Mock progress simulation
    void simulateProgress(int value) {
        setValue(value);
        emit valueChanged(value);
    }

signals:
    void valueChanged(int value);
};

/**
 * Mock Dialog for error display testing
 */
class MockDialog : public QDialog {
    Q_OBJECT

public:
    explicit MockDialog(QWidget* parent = nullptr) : QDialog(parent) {}

    MOCK_METHOD(int, exec, (), (override));
    MOCK_METHOD(void, accept, (), (override));
    MOCK_METHOD(void, reject, (), (override));
    MOCK_METHOD(void, setModal, (bool modal), (override));
    MOCK_METHOD(void, setWindowTitle, (const QString& title), (override));
    MOCK_METHOD(QString, windowTitle, (), (const, override));

    // Mock dialog result simulation
    void simulateAccept() {
        accept();
        emit accepted();
    }

    void simulateReject() {
        reject();
        emit rejected();
    }

signals:
    void accepted();
    void rejected();
};

/**
 * Mock Message Box for error notifications
 */
class MockMessageBox : public QMessageBox {
    Q_OBJECT

public:
    explicit MockMessageBox(QWidget* parent = nullptr) : QMessageBox(parent) {}

    MOCK_METHOD(int, exec, (), (override));
    MOCK_METHOD(void, setText, (const QString& text), (override));
    MOCK_METHOD(void, setInformativeText, (const QString& text), (override));
    MOCK_METHOD(void, setIcon, (Icon icon), (override));
    MOCK_METHOD(void, setStandardButtons, (StandardButtons buttons), (override));
    MOCK_METHOD(void, setDefaultButton, (StandardButton button), (override));

    // Static mock methods for common message box operations
    static int mockCritical(QWidget* parent, const QString& title, const QString& text) {
        // Record the call for testing
        last_critical_call = {parent, title, text};
        return mock_return_value;
    }

    static int mockWarning(QWidget* parent, const QString& title, const QString& text) {
        last_warning_call = {parent, title, text};
        return mock_return_value;
    }

    static int mockInformation(QWidget* parent, const QString& title, const QString& text) {
        last_info_call = {parent, title, text};
        return mock_return_value;
    }

    // Test helpers
    struct MessageBoxCall {
        QWidget* parent = nullptr;
        QString title;
        QString text;
    };

    static MessageBoxCall last_critical_call;
    static MessageBoxCall last_warning_call;
    static MessageBoxCall last_info_call;
    static int mock_return_value;

    static void resetMockCalls() {
        last_critical_call = {};
        last_warning_call = {};
        last_info_call = {};
        mock_return_value = QMessageBox::Ok;
    }
};

/**
 * Mock Timer for animation and periodic updates
 */
class MockTimer : public QTimer {
    Q_OBJECT

public:
    explicit MockTimer(QObject* parent = nullptr) : QTimer(parent) {}

    MOCK_METHOD(void, start, (int msec), (override));
    MOCK_METHOD(void, start, (), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, setInterval, (int msec), (override));
    MOCK_METHOD(int, interval, (), (const, override));
    MOCK_METHOD(void, setSingleShot, (bool singleShot), (override));
    MOCK_METHOD(bool, isSingleShot, (), (const, override));
    MOCK_METHOD(bool, isActive, (), (const, override));

    // Mock timer simulation
    void simulateTimeout() {
        emit timeout();
    }

signals:
    void timeout();
};

/**
 * Qt Test Utilities for UI testing
 */
class QtTestUtilities {
public:
    /**
     * Create a signal spy for testing Qt signals
     */
    template<typename T>
    static std::unique_ptr<QSignalSpy> createSignalSpy(T* object, const char* signal) {
        return std::make_unique<QSignalSpy>(object, signal);
    }

    /**
     * Wait for a signal to be emitted with timeout
     */
    static bool waitForSignal(QSignalSpy& spy, int timeout_ms = 5000) {
        return spy.wait(timeout_ms);
    }

    /**
     * Simulate mouse click on widget
     */
    static void simulateClick(QWidget* widget, Qt::MouseButton button = Qt::LeftButton) {
        // In real implementation, would use QTest::mouseClick
        // For mock testing, we'll emit appropriate signals
        if (auto* button_widget = qobject_cast<MockPushButton*>(widget)) {
            button_widget->simulateClick();
        }
    }

    /**
     * Simulate key press on widget
     */
    static void simulateKeyPress(QWidget* widget, Qt::Key key) {
        // In real implementation, would use QTest::keyPress
        // For mock testing, record the key press
        last_key_press = {widget, key};
    }

    struct KeyPressEvent {
        QWidget* widget = nullptr;
        Qt::Key key = Qt::Key_Unknown;
    };

    static KeyPressEvent last_key_press;

    static void resetTestState() {
        last_key_press = {};
        MockMessageBox::resetMockCalls();
    }
};

} // namespace Sudachi::Multiplayer::Testing