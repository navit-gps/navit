#ifndef NAVIT_GUI_QML_POINT_H
#define NAVIT_GUI_QML_POINT_H

enum NGQPointTypes {MapPoint,Bookmark};

class NGQPoint : public QObject {
	Q_OBJECT;

    Q_PROPERTY(QString coordString READ coordString CONSTANT);
    Q_PROPERTY(QString pointName READ pointName CONSTANT);
    Q_PROPERTY(QString pointType READ pointType CONSTANT);
public:
    NGQPoint(struct gui_priv* this_,struct point* p,NGQPointTypes type=MapPoint,QObject *parent=NULL) : QObject(parent) {
        this->object=this_;
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

    NGQPoint(struct gui_priv* this_,struct coord* c,QString name,NGQPointTypes type=Bookmark,QObject *parent=NULL) : QObject(parent) {
        this->object=this_;
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
            }
            return QString("");
    }
    QString getAttrList(const QString &attr_name) {
            struct attr attr;
            struct item* item;
            struct mapset_handle *h;
            struct map_selection *sel,*selm;
            struct map_rect *mr;
            struct map *m;
            int idist,dist;
            struct coord center;
            QString retXml;

            if (!gui_get_attr(this->object->gui,attr_radius,&attr,NULL)) {
                    return QString();
            }

            dist=attr.u.num*1000;

            sel=map_selection_rect_new(&this->c, dist, 18);
            center.x=this->c.x;
            center.y=this->c.y;
            h=mapset_open(navit_get_mapset(this->object->nav));

            retXml=QString("<%1>").arg(attr_name);

            while ((m=mapset_next(h, 1))) {
                    selm=map_selection_dup_pro(sel, this->c.pro, map_projection(m));
                    mr=map_rect_new(m, selm);
                    if (mr) {
                            while ((item=map_rect_get_item(mr))) {
                                    struct coord c;
                                    if ( item_coord_get_pro(item, &c, 1, this->c.pro) && coord_rect_contains(&sel->u.c_rect, &c) && (idist=transform_distance(this->c.pro, &center, &c)) < dist ) {
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
                                            if (rs.length()>0) {
                                                    QString pointXml="<point>";
                                                    pointXml+="<name>"+rs+"</name>";
                                                    pointXml+="<type>"+QString(item_to_name(item->type))+"</type>";
                                                    pointXml+="</point>";
        
                                                    retXml+=pointXml;
                                            }
                                    }
                            }
                    }
                    map_selection_destroy(selm);
            }
            map_selection_destroy(sel);
            mapset_close(h);
           
            retXml+=QString("</%1>").arg(attr_name);
            dbg(0,"Reulsting xml: %s\n",retXml.toLocal8Bit().constData());
            return retXml;
    }
protected:
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
                                                 if (QString(item_to_name(item->type)).startsWith(QString("poi_"))) {
                                                         ret=QString::fromLocal8Bit(item_to_name(item->type));
                                                         ret=ret.remove(QString("poi_"));
                                                         ret+=QString(" ")+QString::fromLocal8Bit(label);
                                                         map_convert_free(label);
                                                         street_data_free(data);
                                                         map_rect_destroy(mr);
                                                         mapset_close(h);
                                                         return ret;
                                                 }
                                                 if (QString(item_to_name(item->type)).startsWith(QString("poly_"))) {
                                                         ret=QString::fromLocal8Bit(item_to_name(item->type));
                                                         ret=ret.remove(QString("poly_"));
                                                         ret+=QString(" ")+QString::fromLocal8Bit(label);
                                                         map_convert_free(label);
                                                         street_data_free(data);
                                                         map_rect_destroy(mr);
                                                         mapset_close(h);
                                                         return ret;                                                     
                                                 }
                                                 if (QString(item_to_name(item->type)).startsWith(QString("street_"))) {
                                                         ret="Street ";
                                                         ret+=QString::fromLocal8Bit(label);
                                                         map_convert_free(label);
                                                         street_data_free(data);
                                                         map_rect_destroy(mr);
                                                         mapset_close(h);
                                                         return ret;                                                      
                                                 }
                                                 map_convert_free(label);
                                        } else
                                                ret=item_to_name(item->type);
                                }
                                street_data_free(data);
                        }
                        map_rect_destroy(mr);
                }
                mapset_close(h);
                return ret;
        }
private:
    struct gui_priv* object;

    NGQPointTypes type;
    struct coord_geo g;
    struct coord co;
    struct pcoord c;
    struct point p;

    QString name;
    QString coord;
};

#include "ngqpoint.moc"

#endif /* NAVIT_GUI_QML_POINT_H */
