#ifndef __RENDERAREA_H
#define __RENDERAREA_H

class RenderArea : public QT_QPAINTER_RENDERAREA_PARENT
{
     Q_OBJECT
 public:
     RenderArea(struct graphics_priv *priv, QT_QPAINTER_RENDERAREA_PARENT *parent = 0, int w=800, int h=800, int overlay=0);
     void do_resize(QSize size);
     QPixmap *pixmap;
     struct callback_list *cbl;
     struct graphics_priv *gra;

#if QT_QPAINTER_USE_EVENT_QT
     GHashTable *timer_type;
     GHashTable *timer_callback;
     GHashTable *watches;
#endif
 protected:
     int is_overlay;
     QSize sizeHint() const;
     void paintEvent(QPaintEvent *event);
     void resizeEvent(QResizeEvent *event);
     void mouseEvent(int pressed, QMouseEvent *event);
     void mousePressEvent(QMouseEvent *event);
     void mouseReleaseEvent(QMouseEvent *event);
     void mouseMoveEvent(QMouseEvent *event);
     void wheelEvent(QWheelEvent *event);
     void keyPressEvent(QKeyEvent *event);
     void closeEvent(QCloseEvent *event);
     bool event(QEvent *event);
#if QT_QPAINTER_USE_EVENT_QT
     void timerEvent(QTimerEvent *event);
#endif
 protected slots:
     void watchEvent(int fd);
 };

#endif /* __RENDERAREA_H */
