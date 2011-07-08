#include "graphics_qt_qpainter.h"
#include "RenderArea.h"

#include "RenderArea.moc"

//##############################################################################################################
//# Description: Constructor
//# Comment: Using a QPixmap for rendering the graphics
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
RenderArea::RenderArea(struct graphics_priv *priv, QT_QPAINTER_RENDERAREA_PARENT *parent, int w, int h, int overlay)
	: QT_QPAINTER_RENDERAREA_PARENT(parent)
{
	pixmap = new QPixmap(w, h);
#ifndef QT_QPAINTER_NO_WIDGET
	if (!overlay) {
#if QT_VERSION >= 0x040700                                                 
		grabGesture(Qt::PinchGesture);
		grabGesture(Qt::SwipeGesture);
		grabGesture(Qt::PanGesture);
#endif
#if QT_VERSION >= 0x040000
		setWindowTitle("Navit");
#else
		setCaption("Navit");
#endif
	}
#endif
	is_overlay=overlay;
	gra=priv;
#if QT_QPAINTER_USE_EVENT_QT
	timer_type=g_hash_table_new(NULL, NULL);
	timer_callback=g_hash_table_new(NULL, NULL);
	watches=g_hash_table_new(NULL, NULL);
#ifndef QT_QPAINTER_NO_WIDGET
	setBackgroundMode(QWidget::NoBackground);
#endif
#endif
}

//##############################################################################################################
//# Description: QWidget:closeEvent
//# Comment: Deletes navit object and stops event loop on graphics shutdown
//##############################################################################################################
void RenderArea::closeEvent(QCloseEvent* event) 
{
	callback_list_call_attr_0(this->cbl, attr_window_closed);
}

bool RenderArea::event(QEvent *event)
{
#if QT_VERSION >= 0x040700                                                 
	if (event->type() == QEvent::Gesture) {
		dbg(0,"gesture\n");
		return true;
	}
#endif
	return QWidget::event(event);
}
//##############################################################################################################
//# Description: QWidget:sizeHint
//# Comment: This property holds the recommended size for the widget
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
QSize RenderArea::sizeHint() const
{
	return QSize(gra->w, gra->h);
}

//##############################################################################################################
//# Description: QWidget:paintEvent
//# Comment: A paint event is a request to repaint all or part of the widget.
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
void RenderArea::paintEvent(QPaintEvent * event)
{
	qt_qpainter_draw(gra, &event->rect(), 1);
#if 0
	QPainter painter(this);
	painter.drawPixmap(0, 0, *pixmap);
#endif
}

void RenderArea::do_resize(QSize size)
{
	delete pixmap;
	pixmap=new QPixmap(size);
	pixmap->fill();
	QPainter painter(pixmap);
	QBrush brush;
	painter.fillRect(0, 0, size.width(), size.height(), brush);
	dbg(0,"size %dx%d\n", size.width(), size.height());
	dbg(0,"pixmap %p %dx%d\n", pixmap, pixmap->width(), pixmap->height());
	callback_list_call_attr_2(this->cbl, attr_resize, (void *)size.width(), (void *)size.height());
}

//##############################################################################################################
//# Description: QWidget::resizeEvent()
//# Comment: When resizeEvent() is called, the widget already has its new geometry.
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
void RenderArea::resizeEvent(QResizeEvent * event)
{
	if (!this->is_overlay) {
		RenderArea::do_resize(event->size());
	}
}

//##############################################################################################################
//# Description: Method to handle mouse clicks
//# Comment: Delegate of QWidget::mousePressEvent and QWidget::mouseReleaseEvent (see below)
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
void RenderArea::mouseEvent(int pressed, QMouseEvent *event)
{
	struct point p;
	p.x=event->x();
	p.y=event->y();
	switch (event->button()) {
	case Qt::LeftButton:
		callback_list_call_attr_3(this->cbl, attr_button, (void *)pressed, (void *)1, (void *)&p);
		break;
	case Qt::MidButton:
		callback_list_call_attr_3(this->cbl, attr_button, (void *)pressed, (void *)2, (void *)&p);
		break;
	case Qt::RightButton:
		callback_list_call_attr_3(this->cbl, attr_button, (void *)pressed, (void *)3, (void *)&p);
		break;
	default:
		break;
	}
}

void RenderArea::mousePressEvent(QMouseEvent *event)
{
	mouseEvent(1, event);
}

void RenderArea::mouseReleaseEvent(QMouseEvent *event)
{
	mouseEvent(0, event);
}

//##############################################################################################################
//# Description: QWidget::mouseMoveEvent
//# Comment: If mouse tracking is switched on, mouse move events occur even if no mouse button is pressed.
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
void RenderArea::mouseMoveEvent(QMouseEvent *event)
{
	struct point p;
	p.x=event->x();
	p.y=event->y();
	callback_list_call_attr_1(this->cbl, attr_motion, (void *)&p);
}


//##############################################################################################################
//# Description: Qt Event :: Zoom in/out with the mouse's scrollwheel
//# Comment:
//# Authors: Stefan Klumpp (04/2008)
//##############################################################################################################
void RenderArea::wheelEvent(QWheelEvent *event)
{
	struct point p;
	int button;
	
	p.x=event->x();	// xy-coordinates of the mouse pointer
	p.y=event->y();
	
	if (event->delta() > 0)	// wheel movement away from the person
		button=4;
	else if (event->delta() < 0) // wheel movement towards the person
		button=5;
	else
		button=-1;
	
	if (button != -1) {
		callback_list_call_attr_3(this->cbl, attr_button, (void *)1, (void *)button, (void *)&p);
		callback_list_call_attr_3(this->cbl, attr_button, (void *)0, (void *)button, (void *)&p);
	}
	
	event->accept();
}

void RenderArea::keyPressEvent(QKeyEvent *event)
{
#if QT_VERSION < 0x040000
	QString str=event->text();
	QCString cstr=str.utf8();
	const char *text=cstr;
	dbg(0,"enter text='%s' 0x%x (%d) key=%d\n", text, text[0], strlen(text), event->key());
	if (!text || !text[0] || text[0] == 0x7f) {
		dbg(0,"special key\n");
		switch (event->key()) {
		case 4099:
			{
				text=(char []){NAVIT_KEY_BACKSPACE,'\0'};
			}
			break;
		case 4101:
			{
				text=(char []){NAVIT_KEY_RETURN,'\0'};
			}
			break;
		case 4114:
			{
				text=(char []){NAVIT_KEY_LEFT,'\0'};
			}
			break;
		case 4115:
			{
				text=(char []){NAVIT_KEY_UP,'\0'};
			}
			break;
		case 4116:
			{
				text=(char []){NAVIT_KEY_RIGHT,'\0'};
			}
			break;
		case 4117:
			{
				text=(char []){NAVIT_KEY_DOWN,'\0'};
			}
			break;
		}
	}
	callback_list_call_attr_1(this->cbl, attr_keypress, (void *)text);
	event->accept();
#endif
}

void RenderArea::watchEvent(int fd)
{
#if QT_QPAINTER_USE_EVENT_QT
	struct event_watch *ev=(struct event_watch *)g_hash_table_lookup(watches, (void *)fd);
	dbg(1,"fd=%d ev=%p cb=%p\n", fd, ev, ev->cb);
	callback_call_0(ev->cb);
#endif
}

#ifdef QT_QPAINTER_USE_EVENT_QT
void RenderArea::timerEvent(QTimerEvent *event)
{
	int id=event->timerId();
	struct callback *cb=(struct callback *)g_hash_table_lookup(timer_callback, (void *)id);
	if (cb) 
		callback_call_0(cb);
	if (!g_hash_table_lookup(timer_type, (void *)id))
		event_qt_remove_timeout((struct event_timeout *)id);
}
#endif /* QT_QPAINTER_USE_EVENT_QT */

