/*
 * webview.cpp - QWebView handling links and copying text
 * Copyright (C) 2010-2016  senu, Sergey Ilinykh
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "webview.h"

#include "textutil.h"
#include "urlobject.h"
#include "xmpp_vcard.h"

#include <QApplication>
#include <QDebug>
#include <QDrag>
#include <QFile>
#include <QMimeData>
#include <QStyle>
#ifdef WEBENGINE
#include <QWebEngineSettings>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QWebEngineContextMenuRequest>
#elif QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
#include <QWebEngineContextMenuData>
#endif
#else
#include <QNetworkRequest>
#include <QWebFrame>
#include <QWebSecurityOrigin>
#endif

WebView::WebView(QWidget *parent) :
#ifdef WEBENGINE
    QWebEngineView(parent),
#else
    QWebView(parent),
#endif
    possibleDragging(false), isLoading_(false)
{
    setAcceptDrops(false);

#ifdef WEBENGINE
    setAttribute(Qt::WA_NativeWindow);
    settings()->setAttribute(QWebEngineSettings::PluginsEnabled, false);
    settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, false);
    // TODO cache cotrol
    // TODO network request interception
    // Review ink delegation (in other words all local links on page should work)
#else
    settings()->setAttribute(QWebSettings::JavaEnabled, false);
    settings()->setAttribute(QWebSettings::PluginsEnabled, false);
    settings()->setAttribute(QWebSettings::LocalStorageEnabled, false);
    settings()->setMaximumPagesInCache(0);
    settings()->setObjectCacheCapacities(0, 0, 0);
    settings()->clearMemoryCaches();
#endif
    connectPageActions();
}

WebView::~WebView() { qDebug("WebView::~WebView"); }

void WebView::linkClickedEvent(const QUrl &url)
{
    // qDebug()<<"clicked link: "<<url.toString();
    URLObject::getInstance()->popupAction(url.toEncoded());
}

void WebView::loadStartedEvent()
{
    // qDebug("page load started");
    isLoading_ = true;
}

void WebView::loadFinishedEvent(bool success)
{
    // qDebug("page load finished");
    if (!success) {
        qDebug("webview page load failed");
    }
    isLoading_ = false;
}

void WebView::contextMenuEvent(QContextMenuEvent *event)
{
    if (isLoading_)
        return;
#ifdef WEBENGINE
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QWebEngineContextMenuRequest *r       = lastContextMenuRequest();
    QUrl                          linkUrl = r->linkUrl();
#elif QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QWebEngineContextMenuData r       = page()->contextMenuData();
    QUrl                      linkUrl = r.linkUrl();
#endif
#else
    QWebHitTestResult r       = page()->mainFrame()->hitTestContent(event->pos());
    QUrl              linkUrl = r.linkUrl();
#endif
    QMenu *menu;

    if (!linkUrl.isEmpty()) {
        if (linkUrl.scheme() == "addnick") {
            event->ignore();
            return;
        }
        menu = URLObject::getInstance()->createPopupMenu(linkUrl.toEncoded());
        // menu->addAction(pageAction(QWebPage::CopyLinkToClipboard));
    } else {
        menu = new QMenu(this);
        if (!page()->selectedText().isEmpty()) {
#ifdef WEBENGINE
            menu->addAction(pageAction(QWebEnginePage::Copy));
            for (auto act : std::as_const(contextMenuActions_)) {
                menu->addAction(act);
            }
        } else {
            if (!menu->isEmpty()) {
                menu->addSeparator();
            }
            menu->addAction(pageAction(QWebEnginePage::SelectAll));
        }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        menu->addAction(pageAction(QWebEnginePage::InspectElement));
#endif
    }
    // menu->addAction(pageAction(QWebEnginePage::Reload));
#else
            menu->addAction(pageAction(QWebPage::Copy));
            for (auto act : contextMenuActions_) {
                menu->addAction(act);
            }
        } else {
            if (!menu->isEmpty()) {
                menu->addSeparator();
            }
            menu->addAction(pageAction(QWebPage::SelectAll));
        }
        if (settings()->testAttribute(QWebSettings::DeveloperExtrasEnabled)) {
            menu->addAction(pageAction(QWebPage::InspectElement));
        }
    }
    // menu->addAction(pageAction(QWebPage::Reload));
#endif
    menu->exec(mapToGlobal(event->pos()));
    event->accept();
    delete menu;
}

#ifndef WEBENGINE
void WebView::mousePressEvent(QMouseEvent *event)
{
    if (isLoading_)
        return;
    QWebView::mousePressEvent(event);
    if (event->buttons() & Qt::LeftButton) {
        QWebHitTestResult r  = page()->mainFrame()->hitTestContent(event->pos());
        QSize             cs = page()->mainFrame()->contentsSize();
        QSize             vs = page()->viewportSize();
        possibleDragging     = r.isContentSelected()
            && QRect(QPoint(0, 0),
                     cs
                         - QSize(cs.width() > vs.width() ? 1 : 0, cs.height() > vs.height() ? 1 : 0)
                             * style()->pixelMetric(QStyle::PM_ScrollBarExtent))
                   .contains(event->pos());
        dragStartPosition = event->pos();
    } else {
        possibleDragging = false;
    }
}

void WebView::mouseReleaseEvent(QMouseEvent *event)
{
    QWebView::mouseReleaseEvent(event);
    possibleDragging = false;
#ifdef HAVE_X11
    if (!page()->selectedHtml().isEmpty()) {
        convertClipboardHtmlImages(QClipboard::Selection);
    }
#endif
}

void WebView::mouseMoveEvent(QMouseEvent *event)
{
    // QWebView::mouseMoveEvent(event);
    if (!possibleDragging || !(event->buttons() & Qt::LeftButton)) {
        QWebView::mouseMoveEvent(event);
        return;
    }
    if ((event->pos() - dragStartPosition).manhattanLength() < QApplication::startDragDistance())
        return;

    QDrag     *drag     = new QDrag(this);
    QMimeData *mimeData = new QMimeData;

    QString html = TextUtil::img2title(selectedHtml());
    mimeData->setHtml(html);
    mimeData->setText(TextUtil::rich2plain(html));

    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction);
}

void WebView::convertClipboardHtmlImages(QClipboard::Mode mode)
{
    QClipboard *cb   = QApplication::clipboard();
    QString     html = TextUtil::img2title(selectedHtml());
    QMimeData  *data = new QMimeData;
    data->setHtml(html);
    data->setText(TextUtil::rich2plain(html, false));
    cb->setMimeData(data, mode);
}
#endif

void WebView::evaluateJS(const QString &scriptSource)
{
    // qDebug()<< "EVALUATE: " << (scriptSource.size()>200?scriptSource.mid(0,200)+"...":scriptSource);
#ifdef WEBENGINE
    page()->runJavaScript(scriptSource);
#else
    page()->mainFrame()->evaluateJavaScript(scriptSource);
#endif
}

void WebView::addContextMenuAction(QAction *act) { contextMenuActions_.append(act); }

void WebView::connectPageActions()
{
#ifdef WEBENGINE
    connect(page()->action(QWebEnginePage::Copy), SIGNAL(triggered()), SLOT(textCopiedEvent()));
    connect(page()->action(QWebEnginePage::Cut), SIGNAL(triggered()), SLOT(textCopiedEvent()));
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(page()->action(QWebEnginePage::InspectElement), &QAction::triggered, this, [this](bool) {
        auto devView = new QWebEngineView();
        devView->setAttribute(Qt::WA_DeleteOnClose);
        devView->setWindowIcon(QIcon(IconsetFactory::iconPtr("psi/logo_128")->icon()));
        devView->setWindowTitle("Psi WebView DevTools");
        page()->setDevToolsPage(devView->page());
        devView->show();
    });
#endif
#else
    page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);

    connect(page()->action(QWebPage::Copy), SIGNAL(triggered()), SLOT(textCopiedEvent()));
    connect(page()->action(QWebPage::Cut), SIGNAL(triggered()), SLOT(textCopiedEvent()));
    connect(page(), SIGNAL(linkClicked(const QUrl &)), this,
            SLOT(linkClickedEvent(const QUrl &))); // most likely we don't need this at all
#endif
    connect(page(), SIGNAL(loadStarted()), this, SLOT(loadStartedEvent()));
    connect(page(), SIGNAL(loadFinished(bool)), this, SLOT(loadFinishedEvent(bool)));
}

#ifndef WEBENGINE
QString WebView::selectedText() { return TextUtil::rich2plain(TextUtil::img2title(selectedHtml())); }
#endif

void WebView::copySelected()
{
    // use native selectedText w/o clipboard hacks.
    if (page()->hasSelection() && !page()->selectedText().isEmpty()) {
#ifdef WEBENGINE
        page()->triggerAction(QWebEnginePage::Copy);
#else
        page()->triggerAction(QWebPage::Copy);
        textCopiedEvent();
#endif
    }
}

void WebView::textCopiedEvent()
{
#ifdef WEBENGINE
    qWarning("Fixme: convert clipboard html images");
#else
    convertClipboardHtmlImages(QClipboard::Clipboard);
#endif
}

#ifndef WEBENGINE

#endif
