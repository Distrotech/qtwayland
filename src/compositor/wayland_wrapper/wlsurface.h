/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Compositor.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef WL_SURFACE_H
#define WL_SURFACE_H

#include "waylandexport.h"

#include "wlsurfacebuffer.h"
#include "waylandsurface.h"

#include "waylandobject.h"

#include <QtCore/QRect>
#include <QtGui/QImage>

#include <QtCore/QTextStream>
#include <QtCore/QMetaType>

#ifdef QT_COMPOSITOR_WAYLAND_GL
#include <QtGui/QOpenGLContext>
#include <QtGui/qopengl.h>
#endif

#include <wayland-util.h>

class QTouchEvent;

namespace Wayland {

class Compositor;
class Buffer;
class ExtendedSurface;
class SubSurface;
class ShellSurface;

class Q_COMPOSITOR_EXPORT Surface : public Object<struct wl_surface>
{
public:
    Surface(struct wl_client *client, uint32_t id, Compositor *compositor);
    ~Surface();

    WaylandSurface::Type type() const;
    bool isYInverted() const;

    bool visible() const;

    uint id() const { return base()->resource.object.id; }

    QPointF pos() const;
    QPointF nonAdjustedPos() const;
    void setPos(const QPointF  &pos);

    QSize size() const;
    void setSize(const QSize &size);

    QRegion inputRegion() const;
    QRegion opaqueRegion() const;

    QImage image() const;

#ifdef QT_COMPOSITOR_WAYLAND_GL
    GLuint textureId(QOpenGLContext *context) const;
#endif

    void sendFrameCallback();

    void frameFinished();

    WaylandSurface *waylandSurface() const;

    QPoint lastMousePos() const;

    void setExtendedSurface(ExtendedSurface *extendedSurface);
    ExtendedSurface *extendedSurface() const;

    void setSubSurface(SubSurface *subSurface);
    SubSurface *subSurface() const;

    void setShellSurface(ShellSurface *shellSurface);
    ShellSurface *shellSurface() const;

    Compositor *compositor() const;

    void damage(const QRect &rect);

    static const struct wl_surface_interface surface_interface;

    QString title() const { return m_title; }
    void setTitle(const QString &title);

    bool transientInactive() const { return m_transientInactive; }
    void setTransientInactive(bool v) { m_transientInactive = v; }

private:
    Q_DISABLE_COPY(Surface)

    Compositor *m_compositor;
    WaylandSurface *m_waylandSurface;

    SurfaceBuffer *m_backBuffer;
    SurfaceBuffer *m_frontBuffer;
    QList<SurfaceBuffer *> m_bufferQueue;
    bool m_surfaceMapped;

    QPoint m_lastLocalMousePos;
    QPoint m_lastGlobalMousePos;

    struct wl_list m_frame_callback_list;

    ExtendedSurface *m_extendedSurface;
    SubSurface *m_subSurface;
    ShellSurface *m_shellSurface;

    QRegion m_inputRegion;
    QRegion m_opaqueRegion;

    static const int buffer_pool_size = 3;
    SurfaceBuffer *m_bufferPool[buffer_pool_size];

    QPointF m_position;
    QSize m_size;
    QString m_title;
    bool m_transientInactive;

    inline SurfaceBuffer *currentSurfaceBuffer() const;
    bool advanceBufferQueue();
    void doUpdate();
    SurfaceBuffer *createSurfaceBuffer(struct wl_buffer *buffer);
    void frameFinishedInternal();
    bool postBuffer();
    void commit();

    void attach(struct wl_buffer *buffer);

    static void surface_destroy(struct wl_client *client, struct wl_resource *_surface);
    static void surface_attach(struct wl_client *client, struct wl_resource *surface,
                        struct wl_resource *buffer, int x, int y);
    static void surface_damage(struct wl_client *client, struct wl_resource *_surface,
                        int32_t x, int32_t y, int32_t width, int32_t height);
    static void surface_frame(struct wl_client *client, struct wl_resource *resource,
                       uint32_t callback);
    static void surface_set_opaque_region(struct wl_client *client, struct wl_resource *resource,
                                          struct wl_resource *region);
    static void surface_set_input_region(struct wl_client *client, struct wl_resource *resource,
                                         struct wl_resource *region);
    static void surface_commit(struct wl_client *client, struct wl_resource *resource);

};

inline SurfaceBuffer *Surface::currentSurfaceBuffer() const {
    return m_backBuffer? m_backBuffer : m_frontBuffer;
}

}

#endif //WL_SURFACE_H
