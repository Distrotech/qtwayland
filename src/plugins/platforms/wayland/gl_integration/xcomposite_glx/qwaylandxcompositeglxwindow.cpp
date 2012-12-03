/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwaylandxcompositeglxwindow.h"
#include "qwaylandxcompositebuffer.h"

#include <QtCore/QDebug>

#include "wayland-xcomposite-client-protocol.h"
#include <QtGui/QRegion>

#include <X11/extensions/Xcomposite.h>

QWaylandXCompositeGLXWindow::QWaylandXCompositeGLXWindow(QWindow *window, QWaylandXCompositeGLXIntegration *glxIntegration)
    : QWaylandWindow(window)
    , m_glxIntegration(glxIntegration)
    , m_xWindow(0)
    , m_config(qglx_findConfig(glxIntegration->xDisplay(), glxIntegration->screen(), window->format(), GLX_WINDOW_BIT | GLX_PIXMAP_BIT))
    , m_syncCallback(0)
{
}

QWaylandWindow::WindowType QWaylandXCompositeGLXWindow::windowType() const
{
    //yeah. this type needs a new name
    return QWaylandWindow::Egl;
}

void QWaylandXCompositeGLXWindow::setGeometry(const QRect &rect)
{
    QWaylandWindow::setGeometry(rect);

    if (m_xWindow) {
        delete mBuffer;
        mBuffer = 0;
        XDestroyWindow(m_glxIntegration->xDisplay(), m_xWindow);
        m_xWindow = 0;
    }
}

Window QWaylandXCompositeGLXWindow::xWindow() const
{
    if (!m_xWindow)
        const_cast<QWaylandXCompositeGLXWindow *>(this)->createSurface();

    return m_xWindow;
}

void QWaylandXCompositeGLXWindow::waitForSync()
{
    if (!m_syncCallback) {
        struct wl_display *dpy = m_glxIntegration->waylandDisplay()->wl_display();
        m_syncCallback = wl_display_sync(dpy);
        wl_callback_add_listener(m_syncCallback, &sync_callback_listener, this);
    }
    m_glxIntegration->waylandDisplay()->flushRequests();
    while (m_syncCallback)
        m_glxIntegration->waylandDisplay()->readEvents();
}

const struct wl_callback_listener QWaylandXCompositeGLXWindow::sync_callback_listener = {
    QWaylandXCompositeGLXWindow::sync_function
};

void QWaylandXCompositeGLXWindow::createSurface()
{
    QSize size(geometry().size());
    if (size.isEmpty()) {
        //QGLWidget wants a context for a window without geometry
        size = QSize(1,1);
    }

    if (!m_glxIntegration->xDisplay()) {
        qWarning("XCompositeGLXWindow: X display still null?!");
        return;
    }

    XVisualInfo *visualInfo = glXGetVisualFromFBConfig(m_glxIntegration->xDisplay(), m_config);
    Colormap cmap = XCreateColormap(m_glxIntegration->xDisplay(), m_glxIntegration->rootWindow(),
            visualInfo->visual, AllocNone);

    XSetWindowAttributes a;
    a.background_pixel = WhitePixel(m_glxIntegration->xDisplay(), m_glxIntegration->screen());
    a.border_pixel = BlackPixel(m_glxIntegration->xDisplay(), m_glxIntegration->screen());
    a.colormap = cmap;
    m_xWindow = XCreateWindow(m_glxIntegration->xDisplay(), m_glxIntegration->rootWindow(),0, 0, size.width(), size.height(),
                              0, visualInfo->depth, InputOutput, visualInfo->visual,
                              CWBackPixel|CWBorderPixel|CWColormap, &a);

    XCompositeRedirectWindow(m_glxIntegration->xDisplay(), m_xWindow, CompositeRedirectManual);
    XMapWindow(m_glxIntegration->xDisplay(), m_xWindow);

    XSync(m_glxIntegration->xDisplay(), False);
    mBuffer = new QWaylandXCompositeBuffer(m_glxIntegration->waylandXComposite(),
                                            (uint32_t)m_xWindow,
                                            size);
    attach(mBuffer);
    waitForSync();
}

void QWaylandXCompositeGLXWindow::sync_function(void *data, struct wl_callback *callback, uint32_t time)
{
    Q_UNUSED(time);
    QWaylandXCompositeGLXWindow *that = static_cast<QWaylandXCompositeGLXWindow *>(data);
    if (that->m_syncCallback != callback)
        return;
    that->m_syncCallback = 0;
    wl_callback_destroy(callback);
}
