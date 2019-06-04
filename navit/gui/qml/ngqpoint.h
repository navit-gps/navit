#ifndef NAVIT_GUI_QML_POINT_H
#define NAVIT_GUI_QML_POINT_H

static void
get_direction(char *buffer, int angle, int mode)
{
	angle=angle%360;
	switch (mode) {
	case 0:
		sprintf(buffer,"%d",angle);
		break;
	case 1:
		if (angle < 69 || angle > 291)
			*buffer++='N';
		if (angle > 111 && angle < 249)
			*buffer++='S';
		if (angle > 22 && angle < 158)
			*buffer++='E';
		if (angle > 202 && angle < 338)
			*buffer++='W';
		*buffer++='\0';
		break;
	case 2:
		angle=(angle+15)/30;
		if (! angle)
			angle=12;
		sprintf(buffer,"%d H", angle);
		break;
	}
}

enum NGQPointTypes {MapPoint,Bookmark,Position,Destination,PointOfInterest};

class NGQPoint : public QObject {
	Q_OBJECT;

    Q_PROPERTY(QString coordString READ coordString CONSTANT);
    Q_PROPERTY(QString pointName READ pointName CONSTANT);
    Q_PROPERTY(QString pointType READ pointType CONSTANT);
    Q_PROPERTY(QUrl pointUrl READ pointUrl CONSTANT);
public:
    NGQPoint(struct gui_priv* this_,struct point* p,NGQPointTypes type=MapPoint,QObject *parent=NULL) : QObject(parent) {
        this->object=this_;
        this->item.map=0;
        transform_reverse(navit_get_trans(this->object->nav), p, &co);
        transform_to_geo(transform_get_projection(navit_get_trans(this->object->nav)), &co, &g);
        c.pro = transform_get_projection(navit_get_trans(this->object->nav));
        c.x = co.x;
        c.y = co.y;
        this->p.x=p->x;
        this->p.y=p->y;
        this->type=type;

        this->name=this->_coordName();
        this->coord=this->_coordString();
    }
    NGQPoint(struct gui_priv* this_,struct coord* c,NGQPointTypes type=Bookmark,QObject *parent=NULL) : QObject(parent) {
        this->object=this_;
        this->item.map=0;
        this->co.x=c->x;
        this->co.y=c->y;
        transform_to_geo(transform_get_projection(navit_get_trans(this->object->nav)), &co, &g);
        this->c.pro = transform_get_projection(navit_get_trans(this->object->nav));
        this->c.x = c->x;
        this->c.y = c->y;
        this->type=type;

        this->name=this->_coordName();
        this->coord=this->_coordString();
    }

    NGQPoint(struct gui_priv* this_,struct pcoord* pc,NGQPointTypes type=Bookmark,QObject *parent=NULL) : QObject(parent) {
        this->object=this_;
        this->item.map=0;
        this->c.pro = pc->pro;
        this->c.x = pc->x;
        this->c.y = pc->y;
        this->co.x=pc->x;
        this->co.y=pc->y;
        transform_to_geo(this->c.pro, &co, &g);
        this->type=type;

        this->name=this->_coordName();
        this->coord=this->_coordString();
    }

    NGQPoint(struct gui_priv* this_,struct coord* c,QString name,NGQPointTypes type=Bookmark,QObject *parent=NULL) : QObject(parent) {
        this->object=this_;
        this->item.map=0;
        this->co.x=c->x;
        this->co.y=c->y;
        transform_to_geo(transform_get_projection(navit_get_trans(this->object->nav)), &co, &g);
        this->c.pro = transform_get_projection(navit_get_trans(this->object->nav));
        this->c.x = c->x;
        this->c.y = c->y;
        this->type=type;

        this->name=name;
        this->coord=this->_coordString();
    }

    struct pcoord* pc() { return &c; }
public slots:
    void setNewPoint(QString coord,NGQPointTypes type=PointOfInterest) {
            this->item.map=0;
            QStringList coordSplit=coord.split(" ",QString::SkipEmptyParts);
            this->co.x=coordSplit[0].toInt();
            this->co.y=coordSplit[1].toInt();
            transform_to_geo(transform_get_projection(navit_get_trans(this->object->nav)), &co, &g);
            this->c.pro = transform_get_projection(navit_get_trans(this->object->nav));
            this->c.x = coordSplit[0].toInt();
            this->c.y = coordSplit[1].toInt();
            this->type=type;

            this->name=this->_coordName();
            this->coord=this->_coordString();
    }
    QString pointName() {
            return this->name;
    }
    QString coordString() {
            return this->coord;
    }
    QString pointType() {
            switch(this->type) {
            case MapPoint:
                    return QString("MapPoint");
            case Bookmark:
                    return QString("Bookmark");
            case Position:
                    return QString("Position");
            case Destination:
                    return QString("Destination");
            case PointOfInterest:
                    return QString("PointOfInterest");
            }
            return QString("");
    }
    QUrl pointUrl() {
            return this->url;
    }
    QString getInformation() {
            struct map_rect *mr;
            struct item* item;
            struct attr attr;

            QDomDocument retDoc;
            QDomElement entries;
            entries=retDoc.createElement("point");
            retDoc.appendChild(entries);

            if (this->type!=Bookmark and this->item.map) {
                    mr=map_rect_new(this->item.map, NULL);
                    item = map_rect_get_item_byid(mr, this->item.id_hi, this->item.id_lo);
                    if (item) {
                            while(item_attr_get(item, attr_any, &attr)) {
                                     entries.appendChild(this->_fieldValueHelper(retDoc,QString::fromLocal8Bit(attr_to_name(attr.type)), QString::fromLocal8Bit(attr_to_text(&attr,this->item.map, 1))));
                            }
                    }
                    map_rect_destroy(mr);
            }
            return retDoc.toString();
    }
    QString getPOI(const QString &attr_name) {
            struct attr attr;
            struct item* item;
            struct mapset_handle *h;
            struct map_selection *sel,*selm;
            struct map_rect *mr;
            struct map *m;
            int idist,dist;
            struct coord center;
            QDomDocument retDoc(attr_name);
            QDomElement entries;
            char dirbuf[32];

            if (!gui_get_attr(this->object->gui,attr_radius,&attr,NULL)) {
                    return QString();
            }

            dist=attr.u.num*1000;

            sel=map_selection_rect_new(&this->c, dist, 18);
            center.x=this->c.x;
            center.y=this->c.y;
            h=mapset_open(navit_get_mapset(this->object->nav));

            entries=retDoc.createElement(attr_name);
            retDoc.appendChild(entries);

            while ((m=mapset_next(h, 1))) {
                    selm=map_selection_dup_pro(sel, this->c.pro, map_projection(m));
                    mr=map_rect_new(m, selm);
                    if (mr) {
                            while ((item=map_rect_get_item(mr))) {
                                    struct coord c;
                                    if ( item_coord_get_pro(item, &c, 1, this->c.pro) && coord_rect_contains(&sel->u.c_rect, &c) && (idist=transform_distance(this->c.pro, &center, &c)) < dist && item->type<type_line) {
                                            char* label;
                                            QString rs;
                                            if (item_attr_get(item, attr_label, &attr)) {
                                                    label=map_convert_string(m, attr.u.str);
                                                     if (QString(item_to_name(item->type)).startsWith(QString("poi_"))) {
                                                             rs=QString::fromLocal8Bit(item_to_name(item->type));
                                                             rs=rs.remove(QString("poi_"));
                                                             rs+=QString(" ")+QString::fromLocal8Bit(label);

                                                     } else if (QString(item_to_name(item->type)).startsWith(QString("poly_"))) {
                                                             rs=QString::fromLocal8Bit(item_to_name(item->type));
                                                             rs=rs.remove(QString("poly_"));
                                                             rs+=QString(" ")+QString::fromLocal8Bit(label);

                                                     } else if (QString(item_to_name(item->type)).startsWith(QString("street_"))) {
                                                             rs="Street ";
                                                             rs+=QString::fromLocal8Bit(label);
                                                     }
                                                     map_convert_free(label);
                                            } else
                                                    rs=item_to_name(item->type);
                                            get_direction(dirbuf, transform_get_angle_delta(&center, &c, 0), 1);
                                            if (rs.length()>0) {
                                                    QDomElement entry=retDoc.createElement("point");
                                                    QDomElement nameTag=retDoc.createElement("name");
                                                    QDomElement typeTag=retDoc.createElement("type");
                                                    QDomElement distTag=retDoc.createElement("distance");
                                                    QDomElement directTag=retDoc.createElement("direction");
                                                    QDomElement coordsTag=retDoc.createElement("coords");
                                                    QDomText nameT=retDoc.createTextNode(rs);
                                                    QDomText typeT=retDoc.createTextNode(QString(item_to_name(item->type)));
                                                    QDomText distT=retDoc.createTextNode(QString::number(idist/1000));
                                                    QDomText directT=retDoc.createTextNode(dirbuf);
                                                    QDomText coordsT=retDoc.createTextNode(QString("%1 %2").arg(c.x).arg(c.y));
                                                    nameTag.appendChild(nameT);
                                                    typeTag.appendChild(typeT);
                                                    distTag.appendChild(distT);
                                                    directTag.appendChild(directT);
                                                    coordsTag.appendChild(coordsT);
                                                    entry.appendChild(nameTag);
                                                    entry.appendChild(typeTag);
                                                    entry.appendChild(distTag);
                                                    entry.appendChild(directTag);
                                                    entry.appendChild(coordsTag);
                                                    entries.appendChild(entry);
                                            }
                                    }
                            }
                    }
                    map_selection_destroy(selm);
            }
            map_selection_destroy(sel);
            mapset_close(h);
            dbg(lvl_info,"%s",retDoc.toString().toLocal8Bit().constData());
            return retDoc.toString();
    }
protected:
        QDomElement _fieldValueHelper(QDomDocument doc, QString field,QString value) {
                QDomElement fieldTag=doc.createElement(field);
                QDomText valueText=doc.createTextNode(value);
                fieldTag.appendChild(valueText);
                return fieldTag;
        }
        QString _coordString() {
                char latc='N',lngc='E';
                int lat_deg,lat_min,lat_sec;
                int lng_deg,lng_min,lng_sec;
                struct coord_geo g=this->g;

                if (g.lat < 0) {
                        g.lat=-g.lat;
                        latc='S';
                }
                if (g.lng < 0) {
                        g.lng=-g.lng;
                        lngc='W';
                }
                lat_deg=g.lat;
                lat_min=fmod(g.lat*60,60);
                lat_sec=fmod(g.lat*3600,60);
                lng_deg=g.lng;
                lng_min=fmod(g.lng*60,60);
                lng_sec=fmod(g.lng*3600,60);
                return QString(QString::fromLocal8Bit("%1°%2'%3\" %4%5%6°%7'%8\" %9")).arg(lat_deg).arg(lat_min).arg(lat_sec).arg(latc).arg(' ').arg(lng_deg).arg(lng_min).arg(lng_sec).arg(lngc);
        }
        QString _coordName() {
                int dist=10;
                struct mapset *ms;
                struct mapset_handle *h;
                struct map_rect *mr;
                struct map *m;
                struct item *item;
                struct street_data *data;
                struct map_selection sel;
                struct transformation *trans;
                enum projection pro;
                struct attr attr;
                char *label;
                QString ret;

                trans=navit_get_trans(this->object->nav);
                pro=transform_get_projection(trans);
                transform_from_geo(pro, &g, &co);
                ms=navit_get_mapset(this->object->nav);
                sel.next=NULL;
                sel.u.c_rect.lu.x=c.x-dist;
                sel.u.c_rect.lu.y=c.y+dist;
                sel.u.c_rect.rl.x=c.x+dist;
                sel.u.c_rect.rl.y=c.y-dist;
                sel.order=18;
                sel.range=item_range_all;
                h=mapset_open(ms);
                while ((m=mapset_next(h,1))) {
                        mr=map_rect_new(m, &sel);
                        if (! mr)
                                continue;
                        while ((item=map_rect_get_item(mr))) {
                                data=street_get_data(item);
                                if (transform_within_dist_item(&co, item->type, data->c, data->count, dist)) {
                                        if (item_attr_get(item, attr_label, &attr)) {
                                                label=map_convert_string(m, attr.u.str);
                                                this->item=*item;
                                                this->_setUrl(item);
                                                 if (QString(item_to_name(item->type)).startsWith(QString("poi_"))) {
                                                         ret=QString::fromLocal8Bit(item_to_name(item->type));
                                                         ret=ret.remove(QString("poi_"));
                                                         ret+=QString(" ")+QString::fromLocal8Bit(label);
                                                 }
                                                 if (QString(item_to_name(item->type)).startsWith(QString("poly_"))) {
                                                         ret=QString::fromLocal8Bit(item_to_name(item->type));
                                                         ret=ret.remove(QString("poly_"));
                                                         ret+=QString(" ")+QString::fromLocal8Bit(label);
                                                 }
                                                 if (QString(item_to_name(item->type)).startsWith(QString("street_"))) {
                                                         ret="Street ";
                                                         ret+=QString::fromLocal8Bit(label);
                                                 }
                                                 map_convert_free(label);
                                                 street_data_free(data);
                                                 map_rect_destroy(mr);
                                                 mapset_close(h);
                                                 return ret;
                                        } else
                                                this->item=*item;
                                                this->_setUrl(item);
                                                ret=item_to_name(item->type);
                                }
                                street_data_free(data);
                        }
                        map_rect_destroy(mr);
                }
                mapset_close(h);
                return ret;
        }
        void _setUrl(struct item *item) {
                struct attr attr;
                if (item_attr_get(item,attr_osm_nodeid,&attr)) {
                        url.setUrl(QString("http://www.openstreetmap.org/browse/node/%1").arg(*attr.u.num64));
                } else if (item_attr_get(item,attr_osm_wayid,&attr)) {
                        url.setUrl(QString("http://www.openstreetmap.org/browse/way/%1").arg(*attr.u.num64));
                } else if (item_attr_get(item,attr_osm_relationid,&attr)) {
                        url.setUrl(QString("http://www.openstreetmap.org/browse/relation/%1").arg(*attr.u.num64));
                } else {
                        url.clear();
                }
        }
private:
    struct gui_priv* object;

    NGQPointTypes type;
    struct coord_geo g;
    struct coord co;
    struct pcoord c;
    struct point p;

    struct item item;

    QString name;
    QString coord;
    QUrl url;
};

#include "ngqpoint.moc"

#endif /* NAVIT_GUI_QML_POINT_H */
