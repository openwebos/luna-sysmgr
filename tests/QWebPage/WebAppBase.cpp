#include "WebAppBase.h"

WebAppBase::WebAppBase() : m_page(0),
                           m_inCache(false),
                           m_keepAlive(false),
{
#if defined(HAS_NYX)
    m_OrientationAngle = INVALID_ANGLE
#endif
}

WebAppBase::~WebAppBase()
{
}

void WebAppBase::attach(SysMgrWebBridge* bridge)
{
    // connect to the signals of the WebBridge
    // parse up the ApplicationDescription
}

void WebAppBase::detach(void)
{
    // disconnect signals
}

void WebAppBase::stagePreparing()
{
    // just has some perf testing
}

void WebAppBase::stageReady()
{
    // just has perf testing
}

void WebAppBase::setAppDescription(ApplicationDescription* appDesc)
{
}

void WebAppBase::setManualEditorFocusEnabled(bool enable)
{
}

void WebAppBase::setManualEditorFocus(bool focused, const PalmIME::EditorState& editorState)
{
}

void WebAppBase::close()
{
}

void WebAppBase::screenSize(int& width, int& height)
{
    // TODO: get sizes from WAM
    width = 0;
    height = 0;
}

void WebAppBase::resizedContents(int contentsWidth, int contentsHeight)
{
}

void WebAppBase::zoomedContents(double scaleFactor, int contentsWidth, int contentsHeight, int newScrollOffsetX, int newScrollOffsetY)
{
}

void WebAppBase::scrolledContents(int newContentsX, int newContentsY)
{
}

void WebAppBase::uriChanged(const char* uri)
{
}

void WebAppBase::titleChanged(const char* title)
{
}

void WebAppBase::statusMessage(const char* msg)
{
}

void WebAppBase::dispatchFailedLoad(const char* domain, int errorCode, const char* failingURL, const char* localizedDescription)
{
}

void WebAppBase::createActivity()
{
}

void WebAppBase::destroyActivity()
{
}

void WebAppBase::focusActivity()
{
}

void WebAppBase::blurActivity()
{
}

void WebAppBase::cleanResources()
{
}

void WebAppBase::destroyAllSensors()
{
}
