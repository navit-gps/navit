/**
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
#ifndef VEHICLE_IPHONE_CORELOCATION_H
#define VEHICLE_IPHONE_CORELOCATION_H

typedef void(*FT_LOCATION_CB)(void *, double , double , double , double, char *, double);
void corelocation_update(double lat,
		double lng,
		double dir,
		double spd,
		char * time,
		double radius);
void corelocation_init(void * pv_arg, FT_LOCATION_CB pf_cb);
void corelocation_exit(void);

#ifdef VEHICLE_IPHONE_OBJC
#import <CoreLocation/CoreLocation.h>


@interface corelocation : NSObject <CLLocationManagerDelegate> {
	CLLocationManager *locationManager;
	NSDateFormatter *dateFormatter;
	NSDate* eventDate;
@public
	FT_LOCATION_CB pf_cb;
	void * pv_arg;
}

@property (nonatomic, retain) CLLocationManager *locationManager;
@property (nonatomic, retain) NSDateFormatter *dateFormatter;
@property (nonatomic, retain) NSDate* eventDate;
@property (nonatomic) int first;
@property (nonatomic) void * pv_arg;
@property (nonatomic) FT_LOCATION_CB pf_cb;

- (void)locationManager:(CLLocationManager *)manager
	didUpdateToLocation:(CLLocation *)newLocation
		   fromLocation:(CLLocation *)oldLocation;

- (void)locationManager:(CLLocationManager *)manager
	   didFailWithError:(NSError *)error;

@end

#endif
#endif
