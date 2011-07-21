//
//  HelloWorldAppDelegate.m
//  HelloWorld
//
//  Created by Nick Geoghegan on 21/07/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "cocoa_if.h"

@implementation HelloWorldAppDelegate

@synthesize window;

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	// Insert code here to initialize your application 
}

@end

int cocoa_if_main(void)
{
	int  argc=1;
	char *argv[]={"navit",NULL};
	return NSApplicationMain(argc,  (const char **) argv);
}

