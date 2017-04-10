#ifndef __QNavitWidget_h
#define __QNavitWidget_h
class QNavitWidget;
#include "graphics_qt5.h"
#include <QPixmap>
#include <QWidget>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QEvent>

class QNavitWidget : public QWidget
{
        Q_OBJECT
public:
        QNavitWidget(struct graphics_priv *my_graphics_priv,
                             QWidget * parent,
                             Qt::WindowFlags flags);
protected:
        virtual bool event(QEvent *event);
        virtual void keyPressEvent(QKeyEvent *event);
        virtual void paintEvent(QPaintEvent * event);
        virtual void resizeEvent(QResizeEvent * event);
        virtual void mouseEvent(int pressed, QMouseEvent *event);
        virtual void mousePressEvent(QMouseEvent *event);
        virtual void mouseReleaseEvent(QMouseEvent *event);
        virtual void mouseMoveEvent(QMouseEvent *event);
        virtual void wheelEvent(QWheelEvent * event);


private:
        struct graphics_priv *graphics_priv;   
};
#endif

