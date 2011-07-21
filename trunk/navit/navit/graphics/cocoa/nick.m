//
//  nick.m
//  HelloWorld
//
//  Created by Nick Geoghegan on 21/07/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "nick.h"


@implementation nick

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect {
    // Drawing code here.
	NSString*               hws = @"Hello Navit!";
	NSPoint                 p;
	NSMutableDictionary*    attribs;
	NSColor*                c;
	NSFont*                 fnt;
	
	p = NSMakePoint( 10, 100 );
	
	attribs = [[[NSMutableDictionary alloc] init] autorelease];
	
	c = [NSColor redColor];
	fnt = [NSFont fontWithName:@"Times Roman" size:48];
	
	[attribs setObject:c forKey:NSForegroundColorAttributeName];
	[attribs setObject:fnt forKey:NSFontAttributeName];
	
	[hws drawAtPoint:p withAttributes:attribs];
}

@end
