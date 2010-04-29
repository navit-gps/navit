#ifndef NAVIT_GUI_QML_BOOKMARKSPROXY_H
#define NAVIT_GUI_QML_BOOKMARKSPROXY_H

class NGQProxyBookmarks : public NGQProxy {
    Q_OBJECT;


	Q_PROPERTY(QString currentPath READ currentPath WRITE setCurrentPath);
public:
	NGQProxyBookmarks(struct gui_priv* object, QObject* parent) : NGQProxy(object,parent) { };

public slots:
	QString currentPath() {
		return this->current_path;
	}
	void setCurrentPath(QString currentPath) {
		this->current_path=currentPath;
	}
	void moveRoot() {
		struct attr mattr;
		navit_get_attr(this->object->nav, attr_bookmarks, &mattr, NULL);
		bookmarks_move_root(mattr.u.bookmarks);
	}
	void moveUp() {
		struct attr mattr;
		navit_get_attr(this->object->nav, attr_bookmarks, &mattr, NULL);
		bookmarks_move_up(mattr.u.bookmarks);
	}
	void moveDown(QString path) {
		struct attr mattr;
		navit_get_attr(this->object->nav, attr_bookmarks, &mattr, NULL);
		bookmarks_move_down(mattr.u.bookmarks,path.toLocal8Bit().constData());
	}

	QString getBookmarks(const QString &attr_name) {
		struct attr attr,mattr;
		struct item* item;
		struct coord c;
		QDomDocument retDoc(attr_name);
		QDomElement entries;

		entries=retDoc.createElement(attr_name);
		retDoc.appendChild(entries);

		navit_get_attr(this->object->nav, attr_bookmarks, &mattr, NULL);

		bookmarks_item_rewind(mattr.u.bookmarks);
		while ((item=bookmarks_get_item(mattr.u.bookmarks))) {
			QString label;
			QString path;
			
			if (item->type != type_bookmark && item->type != type_bookmark_folder) continue;
			if (!item_attr_get(item, attr_label, &attr)) continue;
			label=QString::fromLocal8Bit(attr.u.str);
			if (!item_attr_get(item, attr_path, &attr)) {
				path="";
			}
			path=QString::fromLocal8Bit(attr.u.str);
			item_coord_get(item, &c, 1);
			QDomElement entry=retDoc.createElement("bookmark");
			QDomElement nameTag=retDoc.createElement("label");
			QDomElement pathTag=retDoc.createElement("path");
			QDomElement typeTag=retDoc.createElement("type");
			QDomElement distTag=retDoc.createElement("distance");
			QDomElement directTag=retDoc.createElement("direction");
			QDomElement coordsTag=retDoc.createElement("coords");
			QDomText nameT=retDoc.createTextNode(label);
			QDomText pathT=retDoc.createTextNode(path);
			QDomText typeT=retDoc.createTextNode(QString(item_to_name(item->type)));
			//QDomText distT=retDoc.createTextNode(QString::number(idist/1000));
			//QDomText directT=retDoc.createTextNode(dirbuf);
			QDomText distT=retDoc.createTextNode("100500");
			QDomText directT=retDoc.createTextNode("nahuy");
			QDomText coordsT=retDoc.createTextNode(QString("%1 %2").arg(c.x).arg(c.y));
			nameTag.appendChild(nameT);
			pathTag.appendChild(pathT);
			typeTag.appendChild(typeT);
			distTag.appendChild(distT);
			directTag.appendChild(directT);
			coordsTag.appendChild(coordsT);
			entry.appendChild(nameTag);
			entry.appendChild(pathTag);
			entry.appendChild(typeTag);
			entry.appendChild(distTag);
			entry.appendChild(directTag);
			entry.appendChild(coordsTag);
			entries.appendChild(entry);

		}

		dbg(0,"%s\n",retDoc.toString().toLocal8Bit().constData());
		return retDoc.toString();
	}
	QString AddBookmark(QString description) {
		struct attr attr;
		navit_get_attr(this->object->nav, attr_bookmarks, &attr, NULL);
		if (!bookmarks_add_bookmark(attr.u.bookmarks, this->object->currentPoint->pc(), description.toLocal8Bit().constData()) ) {
			return "Failed!";
		} else {
			return "Success";
		}
	}
	QString Cut(QString description) {
		struct attr attr;
		navit_get_attr(this->object->nav, attr_bookmarks, &attr, NULL);
		if (!bookmarks_cut_bookmark(attr.u.bookmarks, description.toLocal8Bit().constData()) ) {
			return "Failed!";
		} else {
			return "Success";
		}
	}
	QString Copy(QString description) {
		struct attr attr;
		navit_get_attr(this->object->nav, attr_bookmarks, &attr, NULL);
		if (!bookmarks_copy_bookmark(attr.u.bookmarks, description.toLocal8Bit().constData()) ) {
			return "Failed!";
		} else {
			return "Success";
		}
	}
	QString Paste() {
		struct attr attr;
		navit_get_attr(this->object->nav, attr_bookmarks, &attr, NULL);
		if (!bookmarks_paste_bookmark(attr.u.bookmarks) ) {
			return "Failed!";
		} else {
			return "Success";
		}
	}
	QString Delete(QString bookmark) {
		struct attr attr;
		navit_get_attr(this->object->nav, attr_bookmarks, &attr, NULL);
		if (!bookmarks_delete_bookmark(attr.u.bookmarks, bookmark.toLocal8Bit().constData()) ) {
			return "Failed!";
		} else {
			return "Success";
		}
	}
	void setPoint(QString bookmark) {
		struct attr attr;
		struct item* item;
		struct coord c;
		struct map_rect *mr=NULL;

		navit_get_attr(this->object->nav, attr_bookmarks, &attr, NULL);
		mr=map_rect_new(bookmarks_get_map(attr.u.bookmarks), NULL);
		if (!mr) {
			return;
		}

		while ((item=map_rect_get_item(mr))) {
			QString label;

			if (item->type != type_bookmark) continue;
			if (!item_attr_get(item, attr_label, &attr)) continue;
			
			label=QString::fromLocal8Bit(attr.u.str);
			dbg(0,"Bookmark is %s\n",bookmark.toStdString().c_str());
			if (label.compare(bookmark)) continue;
			item_coord_get(item, &c, 1);
			if (this->object->currentPoint!=NULL) {
				delete this->object->currentPoint;
			}
			this->object->currentPoint=new NGQPoint(this->object,&c,bookmark,Bookmark,NULL);
			this->object->guiWidget->rootContext()->setContextProperty("point",this->object->currentPoint);
		}

		return;
	}

protected:
	int getAttrFunc(enum attr_type type, struct attr* attr, struct attr_iter* iter) { return 0; }
	int setAttrFunc(struct attr* attr) {return 0; }
	struct attr_iter* getIterFunc() { return NULL; };
	void dropIterFunc(struct attr_iter* iter) { return; };

private:
	QString current_path;
};

#include "bookmarksProxy.moc"

#endif /* NAVIT_GUI_QML_BOOKMARKSPROXY_H */
