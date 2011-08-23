#include "config.h"
#include "debug.h"
#include "plugin.h"
#include "point.h"
#include "window.h"
#include "graphics.h"
#include "event.h"
#include <glib.h>


#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED
#define USE_UIKIT 1
#else
#define USE_UIKIT 0
#endif

#if USE_UIKIT
#import <UIKit/UIKit.h>
#define NSRect CGRect

CGContextRef
current_context(void)
{
	return UIGraphicsGetCurrentContext();
}

#else
#import <Cocoa/Cocoa.h>
#define UIView NSView
#define UIViewController NSViewController
#define UIApplicationDelegate NSApplicationDelegate
#define UIWindow NSWindow
#define UIApplication NSApplication
#define UIApplicationMain(a,b,c,d) NSApplicationMain(a,b)
#define UIScreen NSScreen
#define applicationFrame frame

CGContextRef
current_context(void)
{
	return [[NSGraphicsContext currentContext] graphicsPort];
}

#endif

@interface NavitView : UIView {
@public
	CGLayerRef layer;
}

@end

@implementation NavitView

- (void)drawRect:(NSRect)rect
{
	NSLog(@"NavitView:drawRect...");

	CGContextRef X = current_context();

	CGRect bounds = CGContextGetClipBoundingBox(X);
	CGPoint center = CGPointMake((bounds.size.width / 2), (bounds.size.height / 2));

	// fill background rect dark blue
	CGContextSetRGBFillColor(X, 0,0,0.3, 1.0);
	CGContextFillRect(X, bounds);

	// circle
	CGContextSetRGBFillColor(X, 0,0,0.6, 1.0);
	CGContextFillEllipseInRect(X, bounds);

	// fat rounded-cap line from origin to center of view
	CGContextSetRGBStrokeColor(X, 0,0,1, 1.0);
	CGContextSetLineWidth(X, 30);
	CGContextSetLineCap(X, kCGLineCapRound);
	CGContextBeginPath(X);
	CGContextMoveToPoint(X, 0,0);
	CGContextAddLineToPoint(X, center.x, center.y);
	CGContextStrokePath(X);

	// Draw the text Navit in red
	char* text = "Hello World!";
	CGContextSelectFont(X, "Helvetica Bold", 24.0f, kCGEncodingMacRoman);
	CGContextSetTextDrawingMode(X, kCGTextFill);
	CGContextSetRGBFillColor(X, 0.8f, 0.3f, 0.1f, 1.0f);
	CGAffineTransform xform = CGAffineTransformMake( 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	CGContextSetTextMatrix(X, xform);
	CGContextShowTextAtPoint(X, center.x, center.y, text, strlen(text));
	CGContextDrawLayerAtPoint(X, CGPointZero, layer);
}


- (void)dealloc {
	[super dealloc];
}


@end

@interface NavitViewController : UIViewController
{
	NSRect frame;
	CGLayerRef layer;
}

@property (nonatomic) NSRect frame;

- (id) init_withFrame : (NSRect) _frame;

@end



@implementation NavitViewController

@synthesize frame;

- (id) init_withFrame : (NSRect) _frame
{
	NSLog(@"init with frame\n");
	frame = _frame;
	return [self init];
}

- (void)loadView
{
	NSLog(@"loadView");
	NavitView* myV = [NavitView alloc];

	CGContextRef X = current_context();
	CGRect lr=CGRectMake(0, 0, 64, 64);
	myV->layer=CGLayerCreateWithContext(X, lr.size, NULL);
	CGContextRef lc=CGLayerGetContext(myV->layer);
	CGContextSetRGBFillColor(lc, 1, 0, 0, 1);
	CGContextFillRect(lc, lr);

	[myV initWithFrame: frame];

	[self setView: myV];

	[myV release];
}

- (void)didReceiveMemoryWarning {
	// Releases the view if it doesn't have a superview.
	[super didReceiveMemoryWarning];

	// Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
	// Release any retained subviews of the main view.
	// e.g. self.myOutlet = nil;
}


- (void)dealloc {
	[super dealloc];
}

@end

@class NavitViewController;

@interface NavitAppDelegate : NSObject <UIApplicationDelegate> {
	UIWindow *window;
	NavitViewController *viewController;
}

@property (nonatomic, retain) /*IBOutlet*/ UIWindow *window;
@property (nonatomic, retain) /*IBOutlet*/ NavitViewController *viewController;

@end


@implementation NavitAppDelegate

@synthesize window;
@synthesize viewController;


#if USE_UIKIT
- (BOOL)application:(UIApplication *)application
didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
#else
- (void)
applicationDidFinishLaunching:(NSNotification *)aNotification
#endif
{
	NSLog(@"DidFinishLaunching\n");
#if USE_UIKIT
	[[UIApplication sharedApplication] setStatusBarHidden:NO];
#endif

	NSRect appFrame = [UIScreen mainScreen].applicationFrame;

	self.viewController = [[[NavitViewController alloc] init_withFrame : appFrame] autorelease];

	CGRect windowRectcg = CGRectMake(0, 0, appFrame.size.width, appFrame.size.height);
	NSRect windowRect = *(NSRect *)&windowRectcg;

#if USE_UIKIT
	self.window = [[[UIWindow alloc] initWithFrame:windowRect] autorelease];
#else
	self.window = [[[UIWindow alloc] initWithContentRect:windowRect styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:NO] autorelease];
#endif

#if USE_UIKIT
	[window addSubview:viewController.view];
	[window makeKeyAndVisible];
#else
	NSWindowController * controller = [[NSWindowController alloc] initWithWindow : window];
	NSLog(@"Setting view\n");
	[viewController loadView];
	[window setContentView : viewController.view];
	[controller setWindow : window];
	[controller showWindow : nil];

#endif


	return YES;
}


- (void)dealloc {
	[viewController release];
	[window release];
	[super dealloc];
}


@end


struct graphics_priv {
	struct window win;
};

static void *
get_data(struct graphics_priv *this, const char *type)
{
	dbg(0,"enter\n");
	if (strcmp(type,"window"))
		return NULL;
	return &this->win;
}


static struct graphics_methods graphics_methods = {
	NULL, /* graphics_destroy, */
	NULL, /* draw_mode, */
	NULL, /* draw_lines, */
	NULL, /* draw_polygon, */
	NULL, /* draw_rectangle, */
	NULL, /* draw_circle, */
	NULL, /* draw_text, */
	NULL, /* draw_image, */
	NULL, /* draw_image_warp, */
	NULL, /* draw_restore, */
	NULL, /* draw_drag, */
	NULL, /* font_new, */
	NULL, /* gc_new, */
	NULL, /* background_gc, */
	NULL, /* overlay_new, */
	NULL, /* image_new, */
	get_data,
	NULL, /* image_free, */
	NULL, /* get_text_bbox, */
	NULL, /* overlay_disable, */
	NULL, /* overlay_resize, */
	NULL, /* set_attr, */
};



struct graphics_priv *
graphics_cocoa_new(struct navit *nav, struct graphics_methods *meth, struct attr **attrs, struct callback_list *cbl)
{
	*meth=graphics_methods;
	dbg(0,"enter\n");
	if(!event_request_system("cocoa","graphics_cocoa"))
		return NULL;
	return g_new0(struct graphics_priv, 1);
}

static void
event_cocoa_main_loop_run(void)
{

	dbg(0,"enter\n");
#if 0
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *documentsDirectory = [paths objectAtIndex:0];
	NSString *logPath = [documentsDirectory stringByAppendingPathComponent:@"console.log"];
	freopen("/tmp/log.txt","a+",stderr);
	NSLog(@"Test\n");
#endif
#if USE_UIKIT
	int argc=1;
	char *argv[]={"navit",NULL};
	int retVal = UIApplicationMain(argc, argv, nil, @"NavitAppDelegate");
#else
	NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
	NavitAppDelegate * delegate = [[NavitAppDelegate alloc] init];
	NSApplication * application = [NSApplication sharedApplication];
	[application setDelegate:delegate];
	NSLog(@"Test\n");

	[NSApp run];

	[delegate release];
#endif
	[pool release];
}

static void *
event_cocoa_add_timeout(void)
{
	dbg(0,"enter\n");
	return NULL;
}

static struct event_methods event_cocoa_methods = {
	event_cocoa_main_loop_run,
	NULL, /* event_cocoa_main_loop_quit, */
	NULL, /* event_cocoa_add_watch, */
	NULL, /* event_cocoa_remove_watch, */
	event_cocoa_add_timeout,
	NULL, /* event_cocoa_remove_timeout, */
	NULL, /* event_cocoa_add_idle, */
	NULL, /* event_cocoa_remove_idle, */
	NULL, /* event_cocoa_call_callback, */
};


static struct event_priv *
event_cocoa_new(struct event_methods *meth)
{
	dbg(0,"enter\n");
	*meth=event_cocoa_methods;
	return NULL;
}


void
plugin_init(void)
{
	dbg(0,"enter\n");
	plugin_register_graphics_type("cocoa", graphics_cocoa_new);
	plugin_register_event_type("cocoa", event_cocoa_new);
}
