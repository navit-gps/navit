//
//  RootViewController.h
//  Downloader
//
//  Created by Nick Geoghegan on 09/08/2011.
//  Copyright 2011 Navit Project. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface RootViewController : UITableViewController
{
	NSMutableArray* locations_;

}

@property (nonatomic, retain) NSMutableArray* locations;

@end
