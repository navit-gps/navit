//
//  DownloaderAppDelegate.h
//  Downloader
//
//  Created by Nick Geoghegan on 09/08/2011.
//  Copyright 2011 Navit Project. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface DownloaderAppDelegate : NSObject <UIApplicationDelegate> {

    UIWindow *window;
    UINavigationController *navigationController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet UINavigationController *navigationController;

@end

