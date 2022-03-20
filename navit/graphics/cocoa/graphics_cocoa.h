//
//  graphics_cocoa.h
//  graphics_cocoa
//
//  Created by olf on 12.06.21.
//

#ifndef graphics_cocoa_h
#define graphics_cocoa_h

#if USE_UIKIT
#import <UIKIT/UIKIT.h>
#else
#import <Cocoa/Cocoa.h>
#endif

CGContextRef current_context(void);

#endif /* graphics_cocoa_h */
