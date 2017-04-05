#ifndef QNAVITQUICK_H
#define QNAVITQUICK_H
class QNavitQuick;
#include <QtQuick/QQuickPaintedItem>
#include <QColor>

#include "graphics_qt5.h"

class QNavitQuick : public QQuickPaintedItem
{
    Q_OBJECT
public:
    QNavitQuick(QQuickItem *parent = 0);

    void paint(QPainter *painter);

    Q_INVOKABLE void setGraphicContext(GraphicsPriv *gp);

protected:
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);
    virtual void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
    virtual void mouseEvent(int pressed, QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void wheelEvent(QWheelEvent *event);


private:
    struct graphics_priv* graphics_priv;
};

#endif
