#include "thememanager.h"

#include <QProcess>

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
{
}

void ThemeManager::applyTheme(const QString &colorScheme, const QString &lookAndFeel)
{
    m_pendingProcesses = 0;
    m_hadFailure = false;

    if (!lookAndFeel.trimmed().isEmpty()) {
        applyLookAndFeel(lookAndFeel);
    }

    if (!colorScheme.trimmed().isEmpty()) {
        applyColorScheme(colorScheme);
    }

    if (m_pendingProcesses == 0) {
        emit themeApplied();
    }
}

void ThemeManager::applyLookAndFeel(const QString &lookAndFeel)
{
    auto *process = new QProcess(this);
    ++m_pendingProcesses;

    connect(process, &QProcess::finished, this, [this, process, lookAndFeel](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            m_hadFailure = true;
            emit themeFailed(tr("lookandfeeltool failed for %1: %2")
                                 .arg(lookAndFeel, QString::fromUtf8(process->readAllStandardError()).trimmed()));
        }

        if (--m_pendingProcesses == 0 && !m_hadFailure) {
            emit themeApplied();
        }

        process->deleteLater();
    });

    process->start(QStringLiteral("lookandfeeltool"), {QStringLiteral("-a"), lookAndFeel});
}

void ThemeManager::applyColorScheme(const QString &colorScheme)
{
    auto *process = new QProcess(this);
    ++m_pendingProcesses;

    connect(process, &QProcess::finished, this, [this, process, colorScheme](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            m_hadFailure = true;
            emit themeFailed(tr("plasma-apply-colorscheme failed for %1: %2")
                                 .arg(colorScheme, QString::fromUtf8(process->readAllStandardError()).trimmed()));
        }

        if (--m_pendingProcesses == 0 && !m_hadFailure) {
            emit themeApplied();
        }

        process->deleteLater();
    });

    process->start(QStringLiteral("plasma-apply-colorscheme"), {QStringLiteral("-a"), colorScheme});
}
