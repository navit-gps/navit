/*
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "glib.h"
#include <stdio.h>
#include <libintl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//  FIXME temporary fix for enum
#include "projection.h"

#include "item.h"
#include "navit.h"
#include "vehicle.h"	
#include "profile.h"
#include "transform.h"
#include "gui.h"
#include "coord.h"
#include "config.h"
#include "plugin.h"
#include "callback.h"
#include "point.h"
#include "graphics.h"
#include "gui_directfb.h"
#include "navigation.h"
#include "debug.h"
#include "attr.h"
#include "track.h"
#include "menu.h"
#include "map.h"


#include <directfb.h>



IDirectFBSurface 	*DFB_primary_layer_surface__i;
IDirectFBSurface 	*DFB_gui_surface__i;
IDirectFBSurface 	*DFB_graphics_surface__i;
IDirectFBSurface	*DFB_gui_buttons_surface__i;

IDirectFBFont		*DFB_primary_layer_surface_font__i;
IDirectFBFont		*DFB_gui_surface_font__i;

IDirectFBWindow  	*DFB_gui_window__i;
IDirectFBWindow  	*DFB_graphics_window__i;

static int screen_width  = 0;
static int screen_height = 0;

struct navit *dfb_gui_navit;




static int run_main_loop(struct gui_priv *this_)
{
	int frames;

	fprintf(stderr,"saucïsse !! \n");		

	
	for(frames=0;frames<100;frames++)
	{
		sleep(3);
		fprintf(stderr,"debug >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>< frame n° %d",frames);			
	}

	return 0;
}

static void vehicle_callback_handler( struct navit *nav, struct vehicle *v){

	fprintf(stderr,"debut vehicle callback\n");	

/*	
	char buffer [50];
	struct attr attr;
	int sats=0, sats_used=0;
	int err;
	
	if (vehicle_get_attr(v, attr_position_speed, &attr))
		sprintf (buffer, "Speed : %02.02f km/h", *attr.u.numd);
	else
		strcpy (buffer, "Speed : N/A");
	fprintf(stderr,"debug vehicle_callback : %s",buffer);
  	primary->DrawString(primary,buffer,-1,0,50,DSTF_LEFT|DSTF_TOP);

	if (vehicle_get_attr(v, attr_position_height, &attr))
		sprintf (buffer, "Altimeter : %.f m", *attr.u.numd);
	else
		strcpy (buffer, "Altimeter : N/A");
	fprintf(stderr,"debug vehicle_callback : %s",buffer);
	primary->DrawString(primary,buffer,-1,0,70,DSTF_LEFT|DSTF_TOP);

	if (vehicle_get_attr(v, attr_position_sats, &attr))
		sats=attr.u.num;
	if (vehicle_get_attr(v, attr_position_sats_used, &attr))
		sats_used=attr.u.num;
	
 	sprintf(buffer," sats : %i, used %i: \n",sats,sats_used);
	fprintf(stderr,"debug vehicle_callback : %s",buffer);
  	primary->DrawString(dfbSG,buffer,-1,0,90,DSTF_LEFT|DSTF_TOP);
	primary->Flip (primary, NULL, DSFLIP_WAITFORSYNC);

*/

}

struct gui_methods gui_dfb_methods = {
	run_main_loop,
};

static int
gui_directfb_set_graphics(struct gui_priv *this, struct graphics *gra)
{
	fprintf(stderr,"gui_directfb_set_graphics !!!\n");
	return 0;
}

static struct gui_priv *
gui_directfb_new(struct navit *nav, struct gui_methods *meth, struct attr **attrs) 
{
	dbg(1,"Begin DirectFB init\n");
	

	struct gui_priv *this_;
	dfb_gui_navit=nav;
	
	/* DirectFB specifics */
	
	DFBResult              		DFB_ret;
	IDirectFB 			*DFB_main__i;
	IDirectFBDisplayLayer 		*DFB_primary_layer__i;

	IDirectFBImageProvider		*DFB_zoomin__img;
	DFBRectangle			DFB_zoomin_img__zone;
	
	DFBDisplayLayerConfig  		*DFB_primary_layer__c;
	DFBWindowDescription		*DFB_graphics_window__d;
	DFBFontDescription		*DFB_primary_layer_surface_font__d;
	DFBFontDescription		*DFB_gui_surface_font__d;
	DFBSurfaceDescription		DFB_gui_buttons_surface__d;

	int				fontheight;

	//char	*	DFB_argc[];

	/*******DirectFB*******/


	if(dfb_gui_navit)
	{	
		dbg(1,"VALID navit instance in gui\n");
	}
	else
	{
		dbg(1,"Invalid navit instance in gui\n");
	}
	if(nav)
	{	
		dbg(1,"VALID source navit instance in gui\n");
	}
	else 
	{
		dbg(1,"Invalid source navit instance in gui\n");
	}
	
	*meth=gui_dfb_methods;

	this_=g_new0(struct gui_priv, 1);
    
	/* INITIALISATION INTERFACE PRIMAIRE */
	DFB_ret = DirectFBInit(NULL,NULL);
	if (DFB_ret != DFB_OK) 
	{
		DirectFBError( "DirectFBInit() failed", DFB_ret );
		return NULL;
	}

	DFB_ret = DirectFBCreate( &DFB_main__i );
	if (DFB_ret != DFB_OK) 
	{
		DirectFBError( "DirectFBCreate() failed", DFB_ret );
		return NULL;
	}
	
	
	DFB_ret = DFB_main__i->GetDisplayLayer( DFB_main__i, DLID_PRIMARY, &DFB_primary_layer__i );
	if (DFB_ret != DFB_OK) 
	{
		D_DERROR( DFB_ret, "IDirectFB::GetDisplayLayer() failed!\n" );
		DFB_main__i->Release(DFB_main__i);
		return NULL;
	}
	
	DFB_ret = DFB_primary_layer__i->SetCooperativeLevel(DFB_primary_layer__i,DLSCL_ADMINISTRATIVE);
	if (DFB_ret != DFB_OK) 
	{
		D_DERROR( DFB_ret, "IDirectFB::SetCooperativeLevel() failed!\n" );
		DFB_main__i->Release(DFB_main__i);
		return NULL;
	}

	/* FIN INITIALISATION INTERFACE PRIMAIRE */

	/* CREATION FENETRE GUI SUR INTERFACE PRIMAIRE */
	DFB_ret = DFB_primary_layer__i->GetSurface(DFB_primary_layer__i,&DFB_gui_surface__i);
	if (DFB_ret != DFB_OK)
	{
		D_DERROR( DFB_ret, "IDirectFBDisplayLayer::GetSurface failed!\n" );
		DFB_main__i->Release(DFB_main__i);
		return NULL;
	}
	
	DFB_primary_layer__c = g_new0(DFBDisplayLayerConfig, 1);
	DFB_ret = DFB_primary_layer__i->GetConfiguration(DFB_primary_layer__i,DFB_primary_layer__c);
	if (DFB_ret != DFB_OK)
	{
		D_DERROR( DFB_ret, "IDirectFBDisplayLayer::GetConfiguration failed!\n" );
		DFB_main__i->Release(DFB_main__i);
		return NULL;
	}
	DFB_primary_layer_surface_font__d = g_new0(DFBFontDescription, 1);
	DFB_primary_layer_surface_font__d->flags = DFDESC_HEIGHT;
	DFB_primary_layer_surface_font__d->height = DFB_primary_layer__c->width/50;
	fprintf(stderr,"debug                                    =============       %d\n",DFB_primary_layer__c->width);

	DFB_ret = DFB_main__i->CreateFont( DFB_main__i, "/usr/share/fonts/truetype/DejaVuSans.ttf", DFB_primary_layer_surface_font__d, &DFB_primary_layer_surface_font__i );
	if (DFB_ret != DFB_OK)
	{
		D_DERROR( DFB_ret, "IDirectFB : CreateFont failed!\n" );
		DFB_main__i->Release(DFB_main__i);
		return NULL;
	}
	
      	DFB_ret = DFB_main__i->CreateImageProvider(DFB_main__i,"/usr/share/navit/xpm/gui_zoom_in_96_96.png",&DFB_zoomin__img);
	if (DFB_ret != DFB_OK)
	{
		D_DERROR( DFB_ret, "IDirectFB : CreateImageProvider failed!\n" );
		DFB_main__i->Release(DFB_main__i);
		return NULL;
	}

	//DFB_gui_surface__i->SetBlittingFlags(DFB_gui_surface__i,DSBLIT_SRC_COLORKEY);
	//DFB_gui_surface__i->SetSrcColorKey(DFB_gui_surface__i,0x00,0x00,0x00);
	DFB_gui_buttons_surface__d.flags = DSDESC_WIDTH|DSDESC_HEIGHT;
	DFB_gui_buttons_surface__d.width = 48;
	DFB_gui_buttons_surface__d.height = 48;
	
	DFB_ret = DFB_main__i->CreateSurface(DFB_main__i,&DFB_gui_buttons_surface__d,&DFB_gui_buttons_surface__i);
	if (DFB_ret != DFB_OK)
	{
		D_DERROR( DFB_ret, "IDirectFB : CreateSurface failed!\n" );
		DFB_main__i->Release(DFB_main__i);
		return NULL;
	}

	//DFB_primary_layer_surface_font__i->GetHeight( DFB_primary_layer_surface_font__i, &fontheight );
	
	DFB_gui_surface__i->SetFont (DFB_gui_surface__i, DFB_primary_layer_surface_font__i);
	DFB_gui_surface__i->SetColor(DFB_gui_surface__i,0xff,0xff,0xff,0xff);
	DFB_gui_surface__i->DrawString(DFB_gui_surface__i,"Navit on DirectFB",-1,0,0,DSTF_LEFT|DSTF_TOP);
	
	DFB_zoomin_img__zone.x = 0;
	DFB_zoomin_img__zone.y = 0;
	DFB_zoomin_img__zone.w = 48;
	DFB_zoomin_img__zone.h = 48;
	DFB_zoomin__img->RenderTo(DFB_zoomin__img,DFB_gui_buttons_surface__i,NULL);
	DFB_gui_surface__i->Blit(DFB_gui_surface__i,DFB_gui_buttons_surface__i,NULL,20,20);
	
	
	DFB_gui_surface__i->Flip(DFB_gui_surface__i,NULL,DSFLIP_NONE);
	
	/* FIN CREATION FENETRE GUI SUR INTERFACE PRIMAIRE */


	DFB_graphics_window__d = g_new0(DFBWindowDescription, 1);
	//DFB_graphics_window__d->caps = DWCAPS_NONE;
	//DFB_graphics_window__d->width = 100;
	//DFB_graphics_window__d->height = 100;
	//DFB_graphics_window__d->posy = 10;
	//DFB_graphics_window__d->posx = 10;
	DFB_graphics_window__d->stacking = DWSC_UPPER;

	//DFB_graphics_window__d->pixelformat = DSPF_RGB32;
	//	DFB_graphics_window__d->options = DWOP_KEEP_POSITION;

	DFB_ret = DFB_primary_layer__i->CreateWindow(DFB_primary_layer__i,DFB_graphics_window__d,&DFB_graphics_window__i);
	 if (DFB_ret != DFB_OK)
	{
		D_DERROR( DFB_ret, "IDirectFBDisplayLayer::CreateWindow failed!\n" );
		DFB_main__i->Release(DFB_main__i);
		return NULL;
	}

	DFB_ret = DFB_graphics_window__i->Resize(DFB_graphics_window__i,600,400);
	if (DFB_ret != DFB_OK)
	{
		D_DERROR( DFB_ret, "IDirectFBWindow::Resize failed!\n" );
		DFB_main__i->Release(DFB_main__i);
		return NULL;
	}
	DFB_ret = DFB_graphics_window__i->MoveTo(DFB_graphics_window__i,20,78);
	if (DFB_ret != DFB_OK)
	{
		D_DERROR( DFB_ret, "IDirectFBWindow::MoveTo failed!\n" );
		DFB_main__i->Release(DFB_main__i);
		return NULL;
	}
	
	DFB_ret = DFB_graphics_window__i->SetOpacity(DFB_graphics_window__i, 0xff );
	if (DFB_ret != DFB_OK)
	{
		D_DERROR( DFB_ret, "IDirectFBWindow::SetOpacity failed!\n" );
		DFB_main__i->Release(DFB_main__i);
		return NULL;
	}

	DFB_ret = DFB_graphics_window__i->GetSurface(DFB_graphics_window__i,&DFB_graphics_surface__i);
	if (DFB_ret != DFB_OK)
	{
		D_DERROR( DFB_ret, "IDirectFBWindow::GetSurface failed!\n" );
		DFB_main__i->Release(DFB_main__i);
		return NULL;
	}
	
	fprintf(stderr,"fin new gui\n");	
	
	
	/* add callback for position updates */
	//struct callback *cb=callback_new_attr_0(callback_cast(vehicle_callback_handler), attr_position_coord_geo);
	
	//navit_add_callback(nav,cb);
	//this_->nav=nav;
	//return this_;
	return NULL;
}


void
plugin_init(void)
{
	dbg(1,"registering directfb plugin\n");
	fprintf(stderr,"registering gui directfb\n");	
	plugin_register_gui_type("directfb", gui_directfb_new);
}




