#include "hintsettings.h"

#include <QDebug>
#include <QDir>
#include <QString>
#include <QFileInfo>
#include <QToolBar>
#include <QPalette>
#include <QToolButton>
#include <QMainWindow>
#include <QApplication>
#include <QGuiApplication>
#include <QDialogButtonBox>
#include <QScreen>
#include <QStandardPaths>
#include <QTemporaryFile>

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>

static const QByteArray s_systemFontName = QByteArrayLiteral("Font");
static const QByteArray s_systemFixedFontName = QByteArrayLiteral("FixedFont");
static const QByteArray s_systemPointFontSize = QByteArrayLiteral("FontSize");
static const QByteArray s_darkModeName = QByteArrayLiteral("DarkMode");
static const QByteArray s_lightIconName = QByteArrayLiteral("heera-light");
static const QByteArray s_darkIconName = QByteArrayLiteral("heera-dark");

HintsSettings::HintsSettings(QObject *parent)
    : QObject(parent),
      m_settings(new QSettings(QSettings::UserScope, "heeraos", "theme"))
{
    m_hints[QPlatformTheme::SystemIconThemeName] = darkMode() ? s_darkIconName : s_lightIconName;
    m_hints[QPlatformTheme::StyleNames] = "heera";
    m_hints[QPlatformTheme::SystemIconFallbackThemeName] = QStringLiteral("Breez");
    m_hints[QPlatformTheme::IconThemeSearchPaths] = xdgIconThemePaths();
    m_hints[QPlatformTheme::UseFullScreenForPopupMenu] = false;

    m_settingsFile = m_settings->fileName();

    QMetaObject::invokeMethod(this, "lazyInit", Qt::QueuedConnection);
}

HintsSettings::~HintsSettings()
{
}

void HintsSettings::lazyInit()
{
    m_fileWatcher = new QFileSystemWatcher();
    m_fileWatcher->addPath(m_settingsFile);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &HintsSettings::onFileChanged);
}

QStringList HintsSettings::xdgIconThemePaths() const
{
    QStringList paths;

    // make sure we have ~/.local/share/icons in paths if it exists
    paths << QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("icons"), QStandardPaths::LocateDirectory);

    const QFileInfo homeIconDir(QDir::homePath() + QStringLiteral("/.icons"));
    if (homeIconDir.isDir()) {
        paths << homeIconDir.absoluteFilePath();
    }

    return paths;
}

QString HintsSettings::systemFont() const
{
    return m_settings->value(s_systemFontName, "Noto Sans").toString();
}

QString HintsSettings::systemFixedFont() const
{
    return m_settings->value(s_systemFixedFontName, "Monospace").toString();
}

qreal HintsSettings::systemFontPointSize() const
{
    return m_settings->value(s_systemPointFontSize, 11).toDouble();
}

bool HintsSettings::darkMode()
{
    return m_settings->value(s_darkModeName, false).toBool();
}

void HintsSettings::onFileChanged(const QString &path)
{
    Q_UNUSED(path);

    QVariantMap map;
    for (const QString &value : m_settings->allKeys()) {
        map[value] = m_settings->value(value);
    }

    m_settings->sync();

    for (const QString &value : m_settings->allKeys()) {
        const QVariant &oldValue = map.value(value);
        const QVariant &newValue = m_settings->value(value);

        if (oldValue != newValue) {
            if (value == s_systemFontName)
                emit systemFontChanged(newValue.toString());
            else if (value == s_systemFixedFontName)
                emit systemFixedFontChanged(newValue.toString());
            else if (value == s_systemPointFontSize)
                emit systemFontPointSizeChanged(newValue.toDouble());
            else if (value == s_darkModeName) {
                emit darkModeChanged(newValue.toBool());
                // Need to update the icon to dark
                m_hints[QPlatformTheme::SystemIconThemeName] = darkMode() ? s_darkIconName : s_lightIconName;
                emit iconThemeChanged();
            }
        }
    }

    bool fileDeleted = !m_fileWatcher->files().contains(m_settingsFile);
    if (fileDeleted)
        m_fileWatcher->addPath(m_settingsFile);
}
