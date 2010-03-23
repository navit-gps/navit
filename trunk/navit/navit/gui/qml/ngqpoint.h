#ifndef NAVIT_GUI_QML_POINT_H
#define NAVIT_GUI_QML_POINT_H

enum NGQPointTypes {MapPoint};

class NGQPoint : public QObject {
	Q_OBJECT;

    /*Q_PROPERTY(QString type READ type CONSTANT);*/

    Q_PROPERTY(QString coordString READ coordString CONSTANT);
    Q_PROPERTY(QString pointName READ pointName CONSTANT);
    Q_PROPERTY(QString py READ py CONSTANT);
    /*Q_PROPERTY(QString cx READ cx CONSTANT);
    Q_PROPERTY(QString cy READ cy CONSTANT);*/
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
    }

    struct pcoord* pc() { return &c; }
public slots:
     QString coordString() {
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
     QString pointName() {
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
    QString px() { 
            char buffer[1024];
            coord_format(this->g.lat,this->g.lng,DEGREES_MINUTES_SECONDS,buffer,sizeof(buffer));
            return QString(buffer);
    }
    QString py() { return QString().setNum(p.y);    }
protected:
private:
    struct gui_priv* object;

    NGQPointTypes type;
	struct coord_geo g;
	struct coord co;
    struct pcoord c;
    struct point p;
};

#include "ngqpoint.moc"

#endif /* NAVIT_GUI_QML_POINT_H */
