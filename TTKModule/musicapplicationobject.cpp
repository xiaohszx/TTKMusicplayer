#include "musicapplicationobject.h"
#include "musicmobiledeviceswidget.h"
#include "musictimerwidget.h"
#include "musicspectrumwidget.h"
#include "musictimerautoobject.h"
#include "musicmessagebox.h"
#include "musicequalizerdialog.h"
#include "musicsettingmanager.h"
#include "musicwindowsmanager.h"
#include "musicsourceupdatewidget.h"
#include "musicsoundeffectswidget.h"
#include "musicmessageaboutdialog.h"
#include "musicnumberdefine.h"
#include "musicapplication.h"
#include "musictopareawidget.h"
#include "musicwidgetutils.h"
#include "musicgiflabelwidget.h"
#include "musicotherdefine.h"
#include "musicurlutils.h"
#include "musicfileutils.h"
#include "musicalgorithmutils.h"
#include "musicdownloadcounterpvthread.h"
#include "musicsinglemanager.h"
#include "musicdesktopsaverwidget.h"

#include "qdevicewatcher.h"

#include <QPropertyAnimation>

#include "qoss/ossconf.h"

#define MARGIN_SIDE     5
#define MARGIN_SIDE_BY  1
#define OSS_HOST_URL    "VDVnYUdYMW9xNnVWSnd6L0J6NHI2MFZ5d0R3R2NiRVF4VW5WckpNcUhnUT0="

MusicApplicationObject *MusicApplicationObject::m_instance = nullptr;

MusicApplicationObject::MusicApplicationObject(QObject *parent)
    : QObject(parent)
{
    Q_INIT_RESOURCE(MusicPlayer);
    m_instance = this;

    musicResetWindow();

    m_quitAnimation = new QPropertyAnimation(this);
    m_sideAnimation = new QPropertyAnimation(parent, "geometry", this);
    m_quitAnimation->setTargetObject(parent);
    m_sideAnimation->setDuration(250*MT_MS);

    m_musicTimerAutoObject = new MusicTimerAutoObject(this);
    m_desktopSaverWidget = nullptr;
    m_setWindowToTop = false;
    m_mobileDeviceWidget = nullptr;
    m_quitContainer = nullptr;

    m_deviceWatcher = new QDeviceWatcher(this);
    connect(m_deviceWatcher, SIGNAL(deviceChanged(bool)), SLOT(musicDeviceChanged(bool)));
    connect(m_deviceWatcher, SIGNAL(deviceAdded(QString)), SLOT(musicDeviceNameChanged(QString)));
    m_deviceWatcher->appendEventReceiver(this);
    m_deviceWatcher->start();

    m_counterPVThread = new MusicDownloadCounterPVThread(this);

    musicToolSetsParameter();
}

MusicApplicationObject::~MusicApplicationObject()
{
    cleanUp();
    Q_CLEANUP_RESOURCE(MusicPlayer);

    delete m_musicTimerAutoObject;
    delete m_desktopSaverWidget;
    delete m_quitAnimation;
    delete m_sideAnimation;
    delete m_deviceWatcher;
    delete m_mobileDeviceWidget;
    delete m_quitContainer;
    delete m_counterPVThread;
}

MusicApplicationObject *MusicApplicationObject::instance()
{
    return m_instance;
}

void MusicApplicationObject::loadNetWorkSetting()
{
#ifndef QT_NO_SSL
    // ssl support check
    M_LOGGER_INFO(QString("App Support ssl: %1").arg(QSslSocket::supportsSsl() ? "true" : "false"));
#endif
    //oss host init
    OSSConf::OSS_HOST = MusicUtils::Algorithm::mdII(OSS_HOST_URL, false);
    m_counterPVThread->startToDownload();
}

void MusicApplicationObject::applySettingParameter()
{
#ifdef Q_OS_WIN
    if(M_SETTING_PTR->value(MusicSettingManager::FileAssociation).toInt())
    {
        MusicWindowsManager windows;
        windows.setMusicRegeditAssociateFileIcon();
    }
#endif
    if(M_SETTING_PTR->value(MusicSettingManager::OtherDesktopSaverEnable).toInt())
    {
        if(!m_desktopSaverWidget)
        {
            m_desktopSaverWidget = new MusicDesktopSaverWidget;
        }
        m_desktopSaverWidget->applySettingParameter();
    }
}

void MusicApplicationObject::windowCloseAnimation()
{
    if(M_SETTING_PTR->value(MusicSettingManager::WindowQuitMode).toBool())
    {
        MusicTopAreaWidget::instance()->setTimerStop();
        MusicApplication *w = MusicApplication::instance();
        w->setMinimumSize(0, 0); ///remove fixed size

        m_quitAnimation->stop();
        m_quitAnimation->setPropertyName("geometry");
        m_quitAnimation->setDuration(250*MT_MS);
        m_quitAnimation->setStartValue(w->geometry());
        m_quitAnimation->setEndValue(QRect(w->x(), w->geometry().center().y(), w->width(), 0));
        m_quitAnimation->start();
        QTimer::singleShot(m_quitAnimation->duration(), this, SLOT(windowCloseAnimationFinished()));
    }
    else
    {
        float v = M_SETTING_PTR->value(MusicSettingManager::BackgroundTransparent).toInt();
              v = MusicUtils::Widget::reRenderValue<float>(1, 0.35, v);
        m_quitAnimation->stop();
        m_quitAnimation->setPropertyName("windowOpacity");
        m_quitAnimation->setDuration(MT_S2MS);
        m_quitAnimation->setStartValue(v);
        m_quitAnimation->setEndValue(0);
        m_quitAnimation->start();
    }
    QTimer::singleShot(MT_S2MS, qApp, SLOT(quit()));
}

void MusicApplicationObject::soureUpdateCheck()
{
    MusicSourceUpdateNotifyWidget *w = new MusicSourceUpdateNotifyWidget;
    w->show();
    w->start();
}

void MusicApplicationObject::sideAnimationByOn()
{
    if(!M_SETTING_PTR->value(MusicSettingManager::OtherSideBy).toBool())
    {
        return;
    }

    MusicApplication *w = MusicApplication::instance();
    if(w->isFullScreen() || w->isMaximized())
    {
        return;
    }

    QPoint pt = w->mapToGlobal(w->rect().topLeft());
    if(-MARGIN_SIDE <= pt.x() && pt.x() <= MARGIN_SIDE)
    {
        m_leftSideByOn = true;
        m_sideAnimation->stop();
        m_sideAnimation->setStartValue(w->geometry());
        m_sideAnimation->setEndValue(QRect(-w->width() + MARGIN_SIDE_BY, w->y(), w->width(), w->height()));
        m_sideAnimation->start();
        M_SETTING_PTR->setValue(MusicSettingManager::OtherSideByIn, true);
    }

    const QRect &rect = MusicUtils::Widget::windowScreenGeometry();
    pt = w->mapToGlobal(rect.topRight());
    if(-MARGIN_SIDE + rect.width() <= pt.x() && pt.x() <= MARGIN_SIDE + rect.width())
    {
        m_rightSideByOn = true;
        m_sideAnimation->stop();
        m_sideAnimation->setStartValue(w->geometry());
        m_sideAnimation->setEndValue(QRect(rect.width() - MARGIN_SIDE_BY, w->y(), w->width(), w->height()));
        m_sideAnimation->start();
        M_SETTING_PTR->setValue(MusicSettingManager::OtherSideByIn, true);
    }
}

void MusicApplicationObject::sideAnimationByOff()
{
    if(!M_SETTING_PTR->value(MusicSettingManager::OtherSideBy).toBool())
    {
        return;
    }

    MusicApplication *w = MusicApplication::instance();
    if(m_leftSideByOn)
    {
        m_leftSideByOn = false;
        m_sideAnimation->stop();
        m_sideAnimation->setStartValue(w->geometry());
        m_sideAnimation->setEndValue(QRect(MARGIN_SIDE_BY, w->y(), w->width(), w->height()));
        m_sideAnimation->start();
        M_SETTING_PTR->setValue(MusicSettingManager::OtherSideByIn, false);
    }
    else if(m_rightSideByOn)
    {
        const QRect &rect = MusicUtils::Widget::windowScreenGeometry();
        m_rightSideByOn = false;
        m_sideAnimation->stop();
        m_sideAnimation->setStartValue(w->geometry());
        m_sideAnimation->setEndValue(QRect(rect.width() - w->width() - MARGIN_SIDE_BY, w->y(), w->width(), w->height()));
        m_sideAnimation->start();
        M_SETTING_PTR->setValue(MusicSettingManager::OtherSideByIn, false);
    }
}

void MusicApplicationObject::sideAnimationReset()
{
    if(!M_SETTING_PTR->value(MusicSettingManager::OtherSideBy).toBool())
    {
        return;
    }

    if(m_leftSideByOn)
    {
        MusicApplication *w = MusicApplication::instance();
        w->move(MARGIN_SIDE_BY, w->y());
    }
    else if(m_rightSideByOn)
    {
        MusicApplication *w = MusicApplication::instance();
        const QRect &rect = MusicUtils::Widget::windowScreenGeometry();
        w->move(rect.width() - w->width() - MARGIN_SIDE_BY, w->y());
    }
}

void MusicApplicationObject::windowCloseAnimationFinished()
{
    if(!m_quitContainer)
    {
        MusicApplication *w = MusicApplication::instance();
        w->hide();

        m_quitContainer = new QWidget;
        m_quitContainer->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
        m_quitContainer->setAttribute(Qt::WA_TranslucentBackground);

        MusicGifLabelWidget *gifWidget = new MusicGifLabelWidget(m_quitContainer);
        gifWidget->setType(MusicGifLabelWidget::Gif_Close_White);
        gifWidget->setInterval(25*MT_MS);
        gifWidget->setInfinited(false);
        m_quitContainer->resize(gifWidget->size());

        const QPoint &center = w->geometry().center();
        m_quitContainer->move(QPoint(center.x() - m_quitContainer->width()/2, center.y() - m_quitContainer->height()/2));

        m_quitContainer->raise();
        m_quitContainer->show();
        gifWidget->start();
        gifWidget->show();
    }
}

void MusicApplicationObject::musicAboutUs()
{
    MusicMessageAboutDialog().exec();
}

void MusicApplicationObject::musicBugReportView()
{
    MusicUtils::Url::openUrl(MusicUtils::Algorithm::mdII(REPORT_URL, false), false);
}

void MusicApplicationObject::musicVersionUpdate()
{
    MusicSourceUpdateWidget(MusicApplication::instance()).exec();
}

void MusicApplicationObject::musicTimerWidget()
{
    MusicTimerWidget timer;
    QStringList list;
    MusicApplication::instance()->getCurrentPlaylist(list);
    timer.setSongStringList(list);
    timer.exec();
}

void MusicApplicationObject::musicSpectrumWidget()
{
    M_SINGLE_MANAGER_WIDGET_CLASS(MusicSpectrumWidget);
}

void MusicApplicationObject::musicSetWindowToTop()
{
    m_setWindowToTop = !m_setWindowToTop;
    Qt::WindowFlags flags = MusicApplication::instance()->windowFlags();
    MusicApplication::instance()->setWindowFlags(m_setWindowToTop ? (flags | Qt::WindowStaysOnTopHint) : (flags & ~Qt::WindowStaysOnTopHint));
    MusicApplication::instance()->show();
}

void MusicApplicationObject::musicResetWindow()
{
    m_leftSideByOn = false;
    m_rightSideByOn = false;

    const QRect &rect = MusicUtils::Widget::windowScreenGeometry();
    M_SETTING_PTR->setValue(MusicSettingManager::ScreenSize, rect.size());
    M_SETTING_PTR->setValue(MusicSettingManager::WidgetSize, QSize(WINDOW_WIDTH_MIN, WINDOW_HEIGHT_MIN));

    QWidget *w = MusicApplication::instance();
    if(w->isMaximized() || w->isMinimized() || w->isFullScreen())
    {
        w->showNormal();
    }
    w->setGeometry((rect.width() - WINDOW_WIDTH_MIN)/2, (rect.height() - WINDOW_HEIGHT_MIN)/2, WINDOW_WIDTH_MIN, WINDOW_HEIGHT_MIN);
}

void MusicApplicationObject::musicToolSetsParameter()
{
    m_musicTimerAutoObject->runTimerAutoConfig();
#ifdef Q_OS_WIN
    MusicWindowsManager windows;
    const int version = windows.getLocalIEVersion();
    if(version == -1 || version < 8)
    {
        MusicMessageBox message;
        message.setText(version == -1 ? QObject::tr("No IE Core Detected!") : QObject::tr("IE Version Less Than 8!"));
        message.exec();
    }
#endif
}

void MusicApplicationObject::musicDeviceNameChanged(const QString &name)
{
    M_SETTING_PTR->setValue(MusicSettingManager::ExtraDevicePath, name);
}

void MusicApplicationObject::musicDeviceChanged(bool state)
{
    delete m_mobileDeviceWidget;
    m_mobileDeviceWidget = nullptr;

    if(state)
    {
        m_mobileDeviceWidget = new MusicMobileDevicesWidget;
        m_mobileDeviceWidget->show();
    }
    else
    {
        M_SETTING_PTR->setValue(MusicSettingManager::ExtraDevicePath, QString());
    }
}

void MusicApplicationObject::musicSetEqualizer()
{
    if(!closeCurrentEqualizer())
    {
        return;
    }

    MusicEqualizerDialog equalizer;
    equalizer.exec();
}

void MusicApplicationObject::musicSetSoundEffect()
{
    if(!closeCurrentEqualizer())
    {
        return;
    }

    MusicSoundEffectsWidget sound;
    sound.setParentConnect(this);
    sound.exec();
}

void MusicApplicationObject::musicEffectChanged()
{
    if(M_SETTING_PTR->value(MusicSettingManager::EnhancedBS2B).toInt() == 1)
    {
        MusicSoundEffectsItemWidget::soundEffectChanged(MusicSoundEffectsItemWidget::BS2B, true);
    }

    if(M_SETTING_PTR->value(MusicSettingManager::EnhancedCrossfade).toInt() == 1)
    {
        MusicSoundEffectsItemWidget::soundEffectChanged(MusicSoundEffectsItemWidget::Crossfade, true);
    }

    if(M_SETTING_PTR->value(MusicSettingManager::EnhancedStereo).toInt() == 1)
    {
        MusicSoundEffectsItemWidget::soundEffectChanged(MusicSoundEffectsItemWidget::Stereo, true);
    }

    if(M_SETTING_PTR->value(MusicSettingManager::EnhancedSOX).toInt() == 1)
    {
        MusicSoundEffectsItemWidget::soundEffectChanged(MusicSoundEffectsItemWidget::Soxr, true);
    }

    if(M_SETTING_PTR->value(MusicSettingManager::EnhancedSRC).toInt() == 1)
    {
        MusicSoundEffectsItemWidget::soundEffectChanged(MusicSoundEffectsItemWidget::SrcConverter, true);
    }

    if(M_SETTING_PTR->value(MusicSettingManager::EnhancedMonoStereo).toInt() == 1)
    {
        MusicSoundEffectsItemWidget::soundEffectChanged(MusicSoundEffectsItemWidget::MonoStereo, true);
    }

    if(M_SETTING_PTR->value(MusicSettingManager::EnhancedMono).toInt() == 1)
    {
        MusicSoundEffectsItemWidget::soundEffectChanged(MusicSoundEffectsItemWidget::Mono, true);
    }

#ifdef Q_OS_UNIX
    if(M_SETTING_PTR->value(MusicSettingManager::EnhancedLADSPA).toInt() == 1)
    {
        MusicSoundEffectsItemWidget::soundEffectChanged(MusicSoundEffectsItemWidget::LADSPA, true);
    }
#endif

}

bool MusicApplicationObject::closeCurrentEqualizer()
{
    if(M_SETTING_PTR->value(MusicSettingManager::EnhancedMusic).toInt() != 0)
    {
        MusicMessageBox message;
        message.setText(tr("we are opening the magic sound, if you want to close?"));
        if(!message.exec())
        {
            return false;
        }

        Q_EMIT enhancedMusicChanged(0);
    }
    return true;
}

void MusicApplicationObject::cleanUp()
{
    QFile::remove(TEMPPATH);
    QFile::remove(MUSIC_COLOR_FILE);
    QFile::remove(MUSIC_IMAGE_FILE);
    QFile::remove(MUSIC_RECORD_FILE);
    QFile::remove(MUSIC_RECORD_IN_FILE);
    QFile::remove(MUSIC_RECORD_OUT_FILE);
    QFile::remove(MUSIC_NETWORK_TEST_FILE);

    ///remove daily pic theme
    MusicUtils::File::removeRecursively(STRCAT(CACHE_DIR_FULL, MUSIC_DAILY_DIR));
}
