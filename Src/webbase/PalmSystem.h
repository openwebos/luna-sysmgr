
#ifndef PALMSYSTEM_H_
#define PALMSYSTEM_H_

#include <QObject>
#include <QVariantMap>
#include <QString>

#include "Event.h"

class QWebFrame;
class SysMgrWebBridge;

class PalmSystem : public QObject
{
    Q_OBJECT

    // properties exposed to javascript
    Q_PROPERTY(QString launchParams READ launchParams)
    Q_PROPERTY(bool hasAlphaHole READ hasAlphaHole WRITE setHasAlphaHole)
    Q_PROPERTY(QString locale READ locale)
    Q_PROPERTY(QString localeRegion READ localeRegion)
    Q_PROPERTY(QString timeFormat READ timeFormat)
    Q_PROPERTY(QString timeZone READ timeZone)
    Q_PROPERTY(bool isMinimal READ isMinimal)
    Q_PROPERTY(QString identifier READ identifier)
    Q_PROPERTY(QString version READ version)
    Q_PROPERTY(QString screenOrientation READ screenOrientation)
    Q_PROPERTY(QString windowOrientation READ windowOrientation WRITE setPropWindowOrientation)
    Q_PROPERTY(QString specifiedWindowOrientation READ specifiedWindowOrientation)
    Q_PROPERTY(QString videoOrientation READ videoOrientation)
    Q_PROPERTY(QString deviceInfo READ deviceInfo)
    Q_PROPERTY(bool isActivated READ isActivated)
    Q_PROPERTY(int activityId READ activityId)
    Q_PROPERTY(QString phoneRegion READ phoneRegion)

public:
    PalmSystem(SysMgrWebBridge* bridge = 0);

    void setLaunchParams(const QString& params);

public:
    // methods exposed to javascript
    Q_INVOKABLE QString getIdentifier();
    Q_INVOKABLE QString addBannerMessage(const QString& msg, const QString& params, 
                                      const QString& icon = "", const QString& soundClass = "", 
                                      const QString& soundFile = "", int duration = -1, bool doNotSuppress = false);
    Q_INVOKABLE void removeBannerMessage(const QString& msgId);
    Q_INVOKABLE void clearBannerMessages();
    Q_INVOKABLE void playSoundNotification(const QString& soundClass, const QString& soundFile = "",
                                        int duration = -1, bool wakeUpScreen = false);
    Q_INVOKABLE void simulateMouseClick(int pageX, int pageY, bool pressed);
    Q_INVOKABLE void paste();
    Q_INVOKABLE void copiedToClipboard();
    Q_INVOKABLE void pastedFromClipboard();
    Q_INVOKABLE void setWindowOrientation(const QString& orientation);
//    Q_INVOKABLE QString runTextIndexer(const QString& src, const QVariantMap& fields);
    Q_INVOKABLE QString encrypt(const QString& key, const QString& str);
    Q_INVOKABLE QString decrypt(const QString& key, const QString& str);
    Q_INVOKABLE void shutdown();
    Q_INVOKABLE void markFirstUseDone();
    Q_INVOKABLE void enableFullScreenMode(bool enable);
    Q_INVOKABLE void activate();
    Q_INVOKABLE void deactivate();
    Q_INVOKABLE void stagePreparing();
    Q_INVOKABLE void stageReady();
    Q_INVOKABLE void setAlertSound(const QString& soundClass, const QString& soundFile = "");
    Q_INVOKABLE void receivePageUpDownInLandscape(bool enable);
    Q_INVOKABLE void show();
    Q_INVOKABLE void hide();
    Q_INVOKABLE void enableDockMode(bool enable);
    Q_INVOKABLE QString getLocalizedString(const QString& str);
    Q_INVOKABLE QString addNewContentIndicator();
    Q_INVOKABLE void removeNewContentIndicator(const QString& requestId);
    Q_INVOKABLE void runAnimationLoop(const QVariantMap& domObj, const QString& onStep, 
                                    const QString& onComplete, const QString& curve, qreal duration, 
                                    qreal start, qreal end);
    Q_INVOKABLE void setActiveBannerWindowWidth();
    Q_INVOKABLE void cancelVibrations();
    Q_INVOKABLE void setWindowProperties(const QVariantMap& properties);
    Q_INVOKABLE bool addActiveCallBanner(const QString& icon, const QString& message, quint32 timeStart);
    Q_INVOKABLE void removeActiveCallBanner();
    Q_INVOKABLE bool updateActiveCallBanner(const QString& icon, const QString& message, quint32 timeStart);
    Q_INVOKABLE void applyLaunchFeedback(int offsetX, int offsetY);
    Q_INVOKABLE void launcherReady();
    Q_INVOKABLE QString getDeviceKeys(int key);
    Q_INVOKABLE void repaint();
    Q_INVOKABLE void hideSpellingWidget();
    Q_INVOKABLE void printFrame(const QString& frameName, int lpsJobId, int widthPx, 
                                int heightPx, int printDpi, bool landscape, bool reverseOrder = false);
    Q_INVOKABLE void editorFocused(bool focused, int fieldType, int fieldActions);
    Q_INVOKABLE void allowResizeOnPositiveSpaceChange(bool allowResize);
    Q_INVOKABLE void keepAlive(bool keep);
    Q_INVOKABLE void useSimulatedMouseClicks(bool uses);
    Q_INVOKABLE void handleTapAndHoldEvent(int pageX, int pageY);
    Q_INVOKABLE void setManualKeyboardEnabled(bool enabled);
    Q_INVOKABLE void keyboardShow(int fieldType);
    Q_INVOKABLE void keyboardHide();
    Q_INVOKABLE QVariant getResource(QVariant a, QVariant b);
protected:
    // read properties
    QString launchParams() const;
    bool hasAlphaHole() const;
    QString locale() const;
    QString localeRegion() const;
    QString timeFormat() const;
    QString timeZone() const;
    bool isMinimal() const;
    QString identifier() const;
    QString version() const;
    QString screenOrientation() const;
    QString windowOrientation() const;
    QString specifiedWindowOrientation() const;
    QString videoOrientation() const;
    QString deviceInfo() const;
    bool isActivated() const;
    int activityId() const;
    QString phoneRegion() const;

protected:
    // write properties
    void setHasAlphaHole(bool newAlpha);
    void setPropWindowOrientation(QString windowOrientation);

private:
    static QString nameForOrientation(Event::Orientation o);

private:
    SysMgrWebBridge* m_bridge;
    QString m_launchParams;
    bool m_hasAlphaHole; // TODO: is this property obsolete?
    QString m_windowOrientation; // TODO: remove once implemented
    QString m_specifiedWindowOrientation;
};

#endif
