//
//  HelloWorldAppDelegate.h
//  HelloWorld
//
//  Created by Nick Geoghegan on 21/07/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifdef __OBJC__


#import <Cocoa/Cocoa.h>

@interface HelloWorldAppDelegate : NSObject <NSApplicationDelegate> {
    NSWindow *window;
}

@property (assign) IBOutlet NSWindow *window;

@end

int coca_if_main(void);
#endif
