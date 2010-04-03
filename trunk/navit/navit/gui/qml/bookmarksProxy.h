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
	QString getAttrList(const QString &attr_name) {
		NGQStandardItemModel* ret=new NGQStandardItemModel(this);
		struct attr attr;
		struct item* item;
		struct map_rect *mr=NULL;
		QHash<QString,QString> seenMap;

		navit_get_attr(this->object->nav, attr_bookmarks, &attr, NULL);
		mr=map_rect_new(bookmarks_get_map(attr.u.bookmarks), NULL);
		if (!mr) {
			return QString();
		}

		if (!this->current_path.isEmpty()) {
			QStandardItem* curItem=new QStandardItem();	
			QStringList returnPath=this->current_path.split("/",QString::SkipEmptyParts);

			curItem->setData("..",NGQStandardItemModel::ItemName);
			curItem->setData("yes",NGQStandardItemModel::ItemIcon);

			if (returnPath.size()>1) {
				returnPath.removeLast();
				curItem->setData(returnPath.join("/"),NGQStandardItemModel::ItemPath);
			} else {
				//Fast way
				curItem->setData(QString(),NGQStandardItemModel::ItemPath);
			}

			ret->appendRow(curItem);
		}
		while ((item=map_rect_get_item(mr))) {
			QStandardItem* curItem=new QStandardItem();
			QString label;
			QStringList labelList;
			
			if (item->type != type_bookmark) continue;
			if (!item_attr_get(item, attr_label, &attr)) continue;
			//We need to treeize output
			label=QString::fromLocal8Bit(attr.u.str);
			curItem->setData(label,NGQStandardItemModel::ItemId);
			if (!label.startsWith(this->current_path)) continue;
			label=label.right(label.length()-this->current_path.length());
			labelList=label.split("/",QString::SkipEmptyParts);
			if (seenMap[labelList[0]]==labelList[0]) continue;
			seenMap[labelList[0]]=labelList[0];
			curItem->setData(labelList[0],NGQStandardItemModel::ItemName);
			curItem->setData(labelList[0],NGQStandardItemModel::ItemValue);
			curItem->setData(QString(this->current_path).append(labelList[0]).append("/"),NGQStandardItemModel::ItemPath);
			if (labelList.size()>1) {
				curItem->setData("yes",NGQStandardItemModel::ItemIcon);
			} else {
				curItem->setData("no",NGQStandardItemModel::ItemIcon);
			}

			ret->appendRow(curItem);
		}

		this->object->guiWidget->rootContext()->setContextProperty("listModel",static_cast<QObject*>(ret));

		return QString();
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
	QString Paste(QString location) {
		struct attr attr;
		navit_get_attr(this->object->nav, attr_bookmarks, &attr, NULL);
		if (!bookmarks_paste_bookmark(attr.u.bookmarks, location.toLocal8Bit().constData()) ) {
			return "Failed!";
		} else {
			return "Success";
		}
	}
	QString Delete(QString bookmark) {
		struct attr attr;
		navit_get_attr(this->object->nav, attr_bookmarks, &attr, NULL);
		if (!bookmarks_del_bookmark(attr.u.bookmarks, bookmark.toLocal8Bit().constData()) ) {
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
