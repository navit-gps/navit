#ifndef NAVIT_GUI_QML_BOOKMARKSPROXY_H
#define NAVIT_GUI_QML_BOOKMARKSPROXY_H

class NGQProxyBookmarks : public NGQProxy {
    Q_OBJECT;

public:
	NGQProxyBookmarks(struct gui_priv* object, QObject* parent) : NGQProxy(object,parent) { };

public slots:
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

	QString getBookmarks() {
		struct attr attr,mattr;
		struct item* item;
		struct coord c;
		QDomDocument retDoc("bookmarks");
		QDomElement entries;

		entries=retDoc.createElement("bookmarks");
		retDoc.appendChild(entries);

		navit_get_attr(this->object->nav, attr_bookmarks, &mattr, NULL);

        if (bookmarks_item_cwd(mattr.u.bookmarks)) {
			QDomElement entry=retDoc.createElement("bookmark");
			entry.appendChild(this->_fieldValueHelper(retDoc,"label",".."));
			entry.appendChild(this->_fieldValueHelper(retDoc,"path",".."));
			entry.appendChild(this->_fieldValueHelper(retDoc,"type",QString(item_to_name(type_bookmark_folder))));
			entry.appendChild(this->_fieldValueHelper(retDoc,"distance",""));
			entry.appendChild(this->_fieldValueHelper(retDoc,"direction",""));
			entry.appendChild(this->_fieldValueHelper(retDoc,"coords",QString("%1 %2").arg(0).arg(0)));
			entries.appendChild(entry);
		}

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
			entry.appendChild(this->_fieldValueHelper(retDoc,"label",label));
			entry.appendChild(this->_fieldValueHelper(retDoc,"path",path));
			entry.appendChild(this->_fieldValueHelper(retDoc,"type",QString(item_to_name(item->type))));
			//entry.appendChild(this->_fieldValueHelper(retDoc,"distance",QString::number(idist/1000)));
			entry.appendChild(this->_fieldValueHelper(retDoc,"distance","100500"));
			//entry.appendChild(this->_fieldValueHelper(retDoc,"direction",dirbuf));
			entry.appendChild(this->_fieldValueHelper(retDoc,"direction","nahut"));
			entry.appendChild(this->_fieldValueHelper(retDoc,"coords",QString("%1 %2").arg(c.x).arg(c.y)));
			entries.appendChild(entry);
		}

		dbg(lvl_info,"%s",retDoc.toString().toLocal8Bit().constData());
		return retDoc.toString();
	}
	QString AddFolder(QString description) {
		struct attr attr;
		navit_get_attr(this->object->nav, attr_bookmarks, &attr, NULL);
		if (!bookmarks_add_bookmark(attr.u.bookmarks, NULL, description.toLocal8Bit().constData()) ) {
			return "Failed!";
		} else {
			return "Success";
		}
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
		struct attr attr, mattr;
		struct item* item;
		struct coord c;

		navit_get_attr(this->object->nav, attr_bookmarks, &mattr, NULL);

		bookmarks_item_rewind(mattr.u.bookmarks);
		while ((item=bookmarks_get_item(mattr.u.bookmarks))) {
			QString label;

			if (item->type != type_bookmark) continue;
			if (!item_attr_get(item, attr_label, &attr)) continue;

			label=QString::fromLocal8Bit(attr.u.str);
			dbg(lvl_debug,"Bookmark is %s",bookmark.toStdString().c_str());
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
};

#include "bookmarksProxy.moc"

#endif /* NAVIT_GUI_QML_BOOKMARKSPROXY_H */
