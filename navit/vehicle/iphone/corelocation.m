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

#define VEHICLE_IPHONE_OBJC
#import "corelocation.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

/* global location structure */
corelocation *locationcontroller = NULL;

/** C update procedure */
void corelocation_update(double lat, double lng, double dir, double spd, char * str_time, double radius) {
	FT_LOCATION_CB pf_cb = locationcontroller->pf_cb;
	void * pv_arg = locationcontroller->pv_arg;
	if(pf_cb) pf_cb(pv_arg, lat, lng, dir, spd, str_time, radius);
}

/** C init procedure */
void corelocation_init(void * pv_arg, FT_LOCATION_CB pf_cb) {
	locationcontroller = [[corelocation alloc] init];

	/** Save callbacks */
	locationcontroller->pv_arg = pv_arg;
	locationcontroller->pf_cb = pf_cb;

	/** Start location process */
	[locationcontroller.locationManager startUpdatingLocation];
}

/** C exit procedure */
void corelocation_exit(void) {
	[locationcontroller dealloc];
}

/** Core location implementation */
@implementation corelocation

@synthesize locationManager;
@synthesize dateFormatter;
@synthesize eventDate;
@synthesize pv_arg;
@synthesize pf_cb;

/** Init corelocation */
- (id) init {
	self = [super init];
	if (self != nil) {
		self.locationManager = [[[CLLocationManager alloc] init] autorelease];
		self.locationManager.distanceFilter = kCLDistanceFilterNone;
		self.locationManager.delegate = self; // send loc updates to myself
		self.pf_cb = NULL;
		self.pv_arg = NULL;
		self.eventDate = [NSDate date];
		self.dateFormatter = [[NSDateFormatter alloc] init];
		[self.dateFormatter setDateFormat:@"yyyy-MM-dd'T'HH:mm:ss'Z'"];
	}
	return self;
}

/** New Data EVENT */
- (void)locationManager:(CLLocationManager *)manager
didUpdateToLocation:(CLLocation *)newLocation
fromLocation:(CLLocation *)oldLocation
{
	NSLog(@"New Location: %@", [newLocation description]);
	NSString *newDateString = [self.dateFormatter stringFromDate:newLocation.timestamp];
	const char* cString = [newDateString cStringUsingEncoding:NSASCIIStringEncoding];

	if(self.pf_cb) {
		self.pf_cb(
				self.pv_arg,
				(double)newLocation.coordinate.latitude,
				(double) newLocation.coordinate.longitude,
				(double) newLocation.course,
				(double) newLocation.speed,
				cString,
				(double) newLocation.horizontalAccuracy
			  );
	}
}

/** Error EVENT */
- (void)locationManager:(CLLocationManager *)manager
didFailWithError:(NSError *)error
{
	NSLog(@"Error: %@", [error description]);
}

/** Destructor */
- (void)dealloc {
	[self.locationManager release];
	[super dealloc];
}

@end

