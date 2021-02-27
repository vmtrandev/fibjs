/*
 * CefWebView.cpp
 *
 *  Created on: Feb 24, 2021
 *      Author: lion
 */

#include "CefWebView.h"
#include "gui_handler.h"
#include "include/cef_parser.h"
#include "include/views/cef_window.h"
#include "include/views/cef_browser_view.h"
#include "../os_gui.h"

namespace fibjs {

class GuiWindowDelegate : public CefWindowDelegate {
public:
    explicit GuiWindowDelegate(CefRefPtr<CefBrowserView> browser_view, CefWebView* webview = NULL)
        : browser_view_(browser_view)
        , m_webview(webview)
    {
    }

    void OnWindowCreated(CefRefPtr<CefWindow> window) OVERRIDE
    {
        window->AddChildView(browser_view_);

        if (m_webview) {
            m_webview->m_browser = browser_view_->GetBrowser();
            m_webview->config_window();
        }

        window->Show();
        browser_view_->RequestFocus();
    }

    void OnWindowDestroyed(CefRefPtr<CefWindow> window) OVERRIDE
    {
        browser_view_ = nullptr;
        m_webview.Release();
    }

    bool CanClose(CefRefPtr<CefWindow> window) OVERRIDE
    {
        CefRefPtr<CefBrowser> browser = browser_view_->GetBrowser();
        if (browser)
            return browser->GetHost()->TryCloseBrowser();
        return true;
    }

    CefSize GetPreferredSize(CefRefPtr<CefView> view) OVERRIDE
    {
        return CefSize(800, 600);
    }

private:
    CefRefPtr<CefBrowserView> browser_view_;
    obj_ptr<CefWebView> m_webview;

    IMPLEMENT_REFCOUNTING(GuiWindowDelegate);
};

class GuiBrowserViewDelegate : public CefBrowserViewDelegate {
public:
    GuiBrowserViewDelegate() {}

    bool OnPopupBrowserViewCreated(CefRefPtr<CefBrowserView> browser_view,
        CefRefPtr<CefBrowserView> popup_browser_view, bool is_devtools) OVERRIDE
    {
        CefWindow::CreateTopLevelWindow(new GuiWindowDelegate(popup_browser_view));
        return true;
    }

private:
    IMPLEMENT_REFCOUNTING(GuiBrowserViewDelegate);
};

CefWebView::CefWebView(exlib::string url, NObject* opt)
    : m_opt(opt)
    , m_url(url)
    , m_bDebug(true)
    , m_bPopup(true)
    , m_bHeadless(false)
{
    holder()->Ref();

    m_opt = opt;
    m_url = url;

    if (m_opt) {
        Variant v;

        if (m_opt->get("debug", v) == 0)
            m_bDebug = v.boolVal();

        if (m_opt->get("popup", v) == 0)
            m_bPopup = v.boolVal();

        if (m_opt->get("headless", v) == 0)
            m_bHeadless = v.boolVal();
    }
}

CefWebView::~CefWebView()
{
}

void CefWebView::open()
{
    static CefRefPtr<GuiHandler> gui_handler;

    if (gui_handler == NULL)
        gui_handler = new GuiHandler();

#ifdef Darwin
    bool use_view = false;
#else
    bool use_view = !m_bHeadless;
#endif

    gui_handler->browser_list_.push_back(this);

    CefBrowserSettings browser_settings;
    if (use_view) {
        static CefRefPtr<GuiBrowserViewDelegate> view_delegate;
        if (view_delegate == NULL)
            view_delegate = new GuiBrowserViewDelegate();

        CefRefPtr<CefBrowserView> browser_view = CefBrowserView::CreateBrowserView(gui_handler, m_url.c_str(), browser_settings,
            nullptr, nullptr, view_delegate);
        CefWindow::CreateTopLevelWindow(new GuiWindowDelegate(browser_view, this));
    } else {
        CefWindowInfo window_info;

        if (m_bHeadless)
            window_info.windowless_rendering_enabled = true;

        m_browser = CefBrowserHost::CreateBrowserSync(window_info, gui_handler, m_url.c_str(), browser_settings,
            nullptr, nullptr);

        if (!m_bHeadless)
            config_window();
        else
            _emit("open");
    }
}

#ifndef Darwin
void CefWebView::config_window()
{
    os_config_window(this, (void*)m_browser->GetHost()->GetWindowHandle(), m_opt);
}
#endif

result_t CefWebView::loadUrl(exlib::string url, AsyncEvent* ac)
{
    if (ac->isSync())
        return CHECK_ERROR(CALL_E_GUICALL);

    if (m_browser)
        m_browser->GetMainFrame()->LoadURL(url.c_str());

    return 0;
}

inline std::string GetDataURI(const exlib::string& data, const std::string& mime_type)
{
    return "data:" + mime_type + ";base64," + CefURIEncode(CefBase64Encode(data.c_str(), data.length()), false).ToString();
}

result_t CefWebView::setHtml(exlib::string html, AsyncEvent* ac)
{
    if (ac->isSync())
        return CHECK_ERROR(CALL_E_GUICALL);

    if (m_browser)
        m_browser->GetMainFrame()->LoadURL(GetDataURI(html, "text/html"));

    return 0;
}

result_t CefWebView::reload(AsyncEvent* ac)
{
    if (ac->isSync())
        return CHECK_ERROR(CALL_E_GUICALL);

    if (m_browser)
        m_browser->Reload();

    return 0;
}

result_t CefWebView::goBack(AsyncEvent* ac)
{
    if (ac->isSync())
        return CHECK_ERROR(CALL_E_GUICALL);

    if (m_browser)
        m_browser->GoBack();

    return 0;
}

result_t CefWebView::goForward(AsyncEvent* ac)
{
    if (ac->isSync())
        return CHECK_ERROR(CALL_E_GUICALL);

    if (m_browser)
        m_browser->GoForward();

    return 0;
}

result_t CefWebView::print(int32_t mode, AsyncEvent* ac)
{
    if (ac->isSync())
        return CHECK_ERROR(CALL_E_GUICALL);

    if (m_browser)
        m_browser->GetHost()->Print();

    return 0;
}

result_t CefWebView::printToPDF(exlib::string file, AsyncEvent* ac)
{
    class GuiPdfPrintCallback : public CefPdfPrintCallback {
    public:
        GuiPdfPrintCallback(AsyncEvent* ac)
            : m_ac(ac)
        {
        }

    public:
        virtual void OnPdfPrintFinished(const CefString& path, bool ok)
        {
            m_ac->apost(ok ? 0 : CALL_E_INTERNAL);
        }

    private:
        AsyncEvent* m_ac;

    private:
        IMPLEMENT_REFCOUNTING(GuiPdfPrintCallback);
    };

    if (ac->isSync())
        return CHECK_ERROR(CALL_E_GUICALL);

    if (!m_browser)
        return 0;

    CefPdfPrintSettings settings;
    m_browser->GetHost()->PrintToPDF(file.c_str(), settings, new GuiPdfPrintCallback(ac));

    return CALL_E_PENDDING;
}

result_t CefWebView::executeJavaScript(exlib::string code, AsyncEvent* ac)
{
    if (ac->isSync())
        return CHECK_ERROR(CALL_E_GUICALL);

    if (!m_browser)
        return 0;

    CefRefPtr<CefFrame> frame = m_browser->GetMainFrame();
    frame->ExecuteJavaScript(code.c_str(), frame->GetURL(), 0);

    return 0;
}

result_t CefWebView::close(AsyncEvent* ac)
{
    if (ac && ac->isSync())
        return CHECK_ERROR(CALL_E_GUICALL);

    if (!m_browser)
        return 0;

    return m_browser->GetHost()->TryCloseBrowser();
}

result_t CefWebView::postMessage(exlib::string msg, AsyncEvent* ac)
{
    return 0;
}
}