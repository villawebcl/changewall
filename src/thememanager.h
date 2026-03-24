#pragma once

#include <QObject>
#include <QString>

class ThemeManager : public QObject {
    Q_OBJECT

public:
    explicit ThemeManager(QObject *parent = nullptr);

    void applyTheme(const QString &colorScheme, const QString &lookAndFeel);

signals:
    void themeApplied();
    void themeFailed(const QString &errorMessage);

private:
    void applyLookAndFeel(const QString &lookAndFeel);
    void applyColorScheme(const QString &colorScheme);

    int m_pendingProcesses = 0;
    bool m_hadFailure = false;
};
