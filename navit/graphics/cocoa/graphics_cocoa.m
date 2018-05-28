#include <glib.h>
#include "config.h"
#include "config_.h"
#include "debug.h"
#include "plugin.h"
#include "point.h"
#include "window.h"
#include "graphics.h"
#include "event.h"
#include "item.h"
#include "callback.h"
#include "color.h"
#include <iconv.h>

#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED
#define USE_UIKIT 1
#else
#define USE_UIKIT 0
#endif

#if USE_UIKIT
#import <UIKit/UIKit.h>
#define NSRect CGRect
#define NSMakeRect CGRectMake
#define REVERSE_Y 0

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
#define UIEvent NSEvent
#define applicationFrame frame
#define REVERSE_Y 1

CGContextRef
current_context(void)
{
	return [[NSGraphicsContext currentContext] graphicsPort];
}

#endif


@interface NavitView : UIView {
@public
	struct graphics_priv *graphics;
}

@end

static struct graphics_priv {
	struct window win;
	NavitView *view;
	CGLayerRef layer;
	CGContextRef layer_context;
	struct callback_list *cbl;
	struct point p, pclean;
	int w, h, wraparound, overlay_disabled, cleanup;
	struct graphics_priv *parent, *next, *overlays;
} *global_graphics_cocoa;

iconv_t utf8_macosroman;

struct graphics_gc_priv {
	CGFloat rgba[4];
	int w;
};

struct graphics_font_priv {
	int size;
	char *name;
};



@implementation NavitView

- (void)drawRect:(NSRect)rect
{
	struct graphics_priv *gr=NULL;
#if 0
	NSLog(@"NavitView:drawRect...");
#endif

	CGContextRef X = current_context();

	CGContextDrawLayerAtPoint(X, CGPointZero, graphics->layer);
	if (!graphics->overlay_disabled)
		gr=graphics->overlays;
	while (gr) {
		if (!gr->overlay_disabled) {
			struct CGPoint pc;
			pc.x=gr->p.x;
			pc.y=gr->p.y;
			if (gr->wraparound) {
				if (pc.x < 0)
					pc.x+=graphics->w;
				if (pc.y < 0)
					pc.y+=graphics->h;
			}
	#if REVERSE_Y
			pc.y=graphics->h-pc.y-gr->h;
	#endif
			dbg(1,"draw %dx%d at %f,%f",gr->w,gr->h,pc.x,pc.y);
			CGContextDrawLayerAtPoint(X, pc, gr->layer);
		}
		gr=gr->next;
	}
}

#if USE_UIKIT
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	NSArray *arr=touches.allObjects;
	UITouch *touch=[arr objectAtIndex: 0];
	struct CGPoint pc=[touch locationInView: self];
	struct point p;
	p.x=pc.x;
	p.y=pc.y;
	dbg(1,"Enter count=%d %d %d",touches.count,p.x,p.y);
        callback_list_call_attr_3(graphics->cbl, attr_button, GINT_TO_POINTER(1), GINT_TO_POINTER(1), (void *)&p);
}


- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	NSArray *arr=touches.allObjects;
	UITouch *touch=[arr objectAtIndex: 0];
	struct CGPoint pc=[touch locationInView: self];
	struct point p;
	p.x=pc.x;
	p.y=pc.y;
	dbg(1,"Enter count=%d %d %d",touches.count,p.x,p.y);
        callback_list_call_attr_3(graphics->cbl, attr_button, GINT_TO_POINTER(0), GINT_TO_POINTER(1), (void *)&p);
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	NSArray *arr=touches.allObjects;
	UITouch *touch=[arr objectAtIndex: 0];
	struct CGPoint pc=[touch locationInView: self];
	struct point p;
	p.x=pc.x;
	p.y=pc.y;
	dbg(1,"Enter count=%d %d %d",touches.count,p.x,p.y);
        callback_list_call_attr_3(graphics->cbl, attr_button, GINT_TO_POINTER(0), GINT_TO_POINTER(1), (void *)&p);
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	NSArray *arr=touches.allObjects;
	UITouch *touch=[arr objectAtIndex: 0];
	struct CGPoint pc=[touch locationInView: self];
	struct point p;
	p.x=pc.x;
	p.y=pc.y;
	dbg(1,"Enter count=%d %d %d",touches.count,p.x,p.y);
	callback_list_call_attr_1(graphics->cbl, attr_motion, (void *)&p);
}

#else
- (void)mouseDown:(UIEvent *)theEvent
{
	struct point p;
	p.x=theEvent.locationInWindow.x;
	p.y=graphics->h-theEvent.locationInWindow.y;

	dbg(1,"Enter %d %d",p.x,p.y);
        callback_list_call_attr_3(graphics->cbl, attr_button, GINT_TO_POINTER(1), GINT_TO_POINTER(1), (void *)&p);
}

- (void)mouseUp:(UIEvent *)theEvent
{
	struct point p;
	p.x=theEvent.locationInWindow.x;
	p.y=graphics->h-theEvent.locationInWindow.y;

	dbg(1,"Enter %d %d",p.x,p.y);
        callback_list_call_attr_3(graphics->cbl, attr_button, GINT_TO_POINTER(0), GINT_TO_POINTER(1), (void *)&p);
}

- (void)mouseDragged:(UIEvent *)theEvent
{
	struct point p;
	p.x=theEvent.locationInWindow.x;
	p.y=graphics->h-theEvent.locationInWindow.y;

	dbg(1,"Enter %d %d",p.x,p.y);
	callback_list_call_attr_1(graphics->cbl, attr_motion, (void *)&p);
}
#endif

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

static
void free_graphics(struct graphics_priv *gr)
{
	if (gr->layer) {
		CGLayerRelease(gr->layer);
		gr->layer=NULL;
	}
}

static void
setup_graphics(struct graphics_priv *gr)
{
	CGRect lr=CGRectMake(0, 0, gr->w, gr->h);
	gr->layer=CGLayerCreateWithContext(current_context(), lr.size, NULL);
	gr->layer_context=CGLayerGetContext(gr->layer);
#if REVERSE_Y
	CGContextScaleCTM(gr->layer_context, 1, -1);
	CGContextTranslateCTM(gr->layer_context, 0, -gr->h);
#endif
	CGContextSetRGBFillColor(gr->layer_context, 0, 0, 0, 0);
  	CGContextSetRGBStrokeColor(gr->layer_context, 0, 0, 0, 0);
	CGContextClearRect(gr->layer_context, lr);
}



- (void)loadView
{
	NSLog(@"loadView");
	NavitView* myV = [NavitView alloc];

	if (global_graphics_cocoa) {
		global_graphics_cocoa->view=myV;
		myV->graphics=global_graphics_cocoa;
		global_graphics_cocoa->w=frame.size.width;
		global_graphics_cocoa->h=frame.size.height;
		setup_graphics(global_graphics_cocoa);
	}

	[myV initWithFrame: frame];

	[self setView: myV];

	[myV release];
}

- (void)didReceiveMemoryWarning {
	dbg(0,"enter");
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

	NSRect windowRect = NSMakeRect(0, 0, appFrame.size.width, appFrame.size.height);

#if USE_UIKIT
	self.window = [[[UIWindow alloc] initWithFrame:windowRect] autorelease];
#else
	self.window = [[[UIWindow alloc] initWithContentRect:windowRect styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:NO] autorelease];
#endif
	utf8_macosroman=iconv_open("MACROMAN","UTF-8");

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

	if (global_graphics_cocoa) {
		callback_list_call_attr_2(global_graphics_cocoa->cbl, attr_resize, (int)appFrame.size.width, (int)appFrame.size.height);

	}

#if USE_UIKIT
	return YES;
#endif
}


- (void)dealloc {
	[viewController release];
	[window release];
	[super dealloc];
}


@end


static void
draw_mode(struct graphics_priv *gr, enum draw_mode_num mode)
{
	if (mode == draw_mode_end) {
		dbg(1,"end %p",gr);
		if (!gr->parent) {
#if USE_UIKIT
			[gr->view setNeedsDisplay];
#else
			[gr->view display];
#endif
		}
	}
}

static void
draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	CGPoint points[count];
	int i;
	for (i = 0 ; i < count ; i++) {
		points[i].x=p[i].x;
		points[i].y=p[i].y;
	}
	CGContextSetStrokeColor(gr->layer_context, gc->rgba);
	CGContextSetLineWidth(gr->layer_context, gc->w);
	CGContextSetLineCap(gr->layer_context, kCGLineCapRound);
    	CGContextBeginPath(gr->layer_context);
	CGContextAddLines(gr->layer_context, points, count);
	CGContextStrokePath(gr->layer_context);

}

static void
draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	CGPoint points[count];
	int i;
	for (i = 0 ; i < count ; i++) {
		points[i].x=p[i].x;
		points[i].y=p[i].y;
	}
	CGContextSetFillColor(gr->layer_context, gc->rgba);
    	CGContextBeginPath(gr->layer_context);
	CGContextAddLines(gr->layer_context, points, count);
	CGContextFillPath(gr->layer_context);
}

static void
draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h)
{
	CGRect lr=CGRectMake(p->x, p->y, w, h);
	if (p->x <= 0 && p->y <= 0 && p->x+w+1 >= gr->w && p->y+h+1 >= gr->h) {
		dbg(1,"clear %p %dx%d",gr,w,h);
		free_graphics(gr);
		setup_graphics(gr);
	}
	CGContextSetFillColor(gr->layer_context, gc->rgba);
	CGContextFillRect(gr->layer_context, lr);
}

static void
draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg, struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy)
{
	size_t len,inlen=strlen(text)+1,outlen=strlen(text)+1;
	char outb[outlen];
	char *inp=text,*outp=outb;

	len=iconv (utf8_macosroman, &inp, &inlen, &outp, &outlen);

	CGContextSetFillColor(gr->layer_context, fg->rgba);

	CGContextSelectFont(gr->layer_context, font->name, font->size/16.0, kCGEncodingMacRoman);
	CGContextSetTextDrawingMode(gr->layer_context, kCGTextFill);
	CGAffineTransform xform = CGAffineTransformMake(dx/65536.0, dy/65536.0, dy/65536.0, -dx/65536.0, 0.0f, 0.0f);
	CGContextSetTextMatrix(gr->layer_context, xform);
	CGContextShowTextAtPoint(gr->layer_context, p->x, p->y, outb, strlen(outb));
}

static void
draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, struct graphics_image_priv *img)
{
	CGImageRef imgc=(CGImageRef) img;
	int w=CGImageGetWidth(imgc);
	int h=CGImageGetHeight(imgc);
	CGRect rect=CGRectMake(0, 0, w, h);
	CGContextSaveGState(gr->layer_context);
	CGContextTranslateCTM(gr->layer_context, p->x, p->y+h);
	CGContextScaleCTM(gr->layer_context, 1.0, -1.0);
	CGContextDrawImage(gr->layer_context, rect, imgc);
	CGContextRestoreGState(gr->layer_context);
}

static void font_destroy(struct graphics_font_priv *font)
{
	g_free(font);
}

static struct graphics_font_methods font_methods = {
	font_destroy
};

static void
draw_drag(struct graphics_priv *gr, struct point *p)
{
	if (!gr->cleanup) {
		gr->pclean=gr->p;
		gr->cleanup=1;
	}
	if (p)
		gr->p=*p;
	else {
		gr->p.x=0;
		gr->p.y=0;
	}
}

static struct graphics_font_priv *font_new(struct graphics_priv *gr, struct graphics_font_methods *meth, char *font,  int size, int flags)
{
	struct graphics_font_priv *ret=g_new0(struct graphics_font_priv, 1);
	*meth=font_methods;

	ret->size=size;
	ret->name="Helvetica";
	return ret;
}

static void
gc_destroy(struct graphics_gc_priv *gc)
{
	g_free(gc);
}

static void
gc_set_linewidth(struct graphics_gc_priv *gc, int w)
{
	gc->w=w;
}

static void
gc_set_dashes(struct graphics_gc_priv *gc, int w, int offset, unsigned char *dash_list, int n)
{
}

static void
gc_set_foreground(struct graphics_gc_priv *gc, struct color *c)
{
	gc->rgba[0]=c->r/65535.0;
	gc->rgba[1]=c->g/65535.0;
	gc->rgba[2]=c->b/65535.0;
	gc->rgba[3]=c->a/65535.0;
}

static void
gc_set_background(struct graphics_gc_priv *gc, struct color *c)
{
}

static struct graphics_gc_methods gc_methods = {
	gc_destroy,
	gc_set_linewidth,
	gc_set_dashes,
	gc_set_foreground,
	gc_set_background,
};

static struct graphics_gc_priv *gc_new(struct graphics_priv *gr, struct graphics_gc_methods *meth)
{
        struct graphics_gc_priv *gc=g_new(struct graphics_gc_priv, 1);
	gc->w=1;

        *meth=gc_methods;
        return gc;
}


static void
background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc)
{
}

static struct graphics_priv *
overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h, int wraparound);


static struct graphics_image_priv *
image_new(struct graphics_priv *gra, struct graphics_image_methods *meth, char *path, int *w, int *h, struct point *hot, int rotation)
{
	NSString *s=[[NSString alloc]initWithCString:path encoding:NSMacOSRomanStringEncoding];
	CGDataProviderRef imgDataProvider = CGDataProviderCreateWithCFData((CFDataRef)[NSData dataWithContentsOfFile:s]);
	[s release];

	if (!imgDataProvider)
		return NULL;

	CGImageRef image = CGImageCreateWithPNGDataProvider(imgDataProvider, NULL, true, kCGRenderingIntentDefault);
	CGDataProviderRelease(imgDataProvider);
	dbg(1,"size %dx%d",CGImageGetWidth(image),CGImageGetHeight(image));
	if (w)
		*w=CGImageGetWidth(image);
	if (h)
		*h=CGImageGetHeight(image);
	if (hot) {
		hot->x=CGImageGetWidth(image)/2;
		hot->y=CGImageGetHeight(image)/2;
	}
	return (struct graphics_image_priv *)image;
}

static void *
get_data(struct graphics_priv *this, const char *type)
{
	dbg(0,"enter");
	if (strcmp(type,"window"))
		return NULL;
	return &this->win;
}

static void
image_free(struct graphics_priv *gr, struct graphics_image_priv *priv)
{
	CGImageRelease((CGImageRef)priv);
}

static void
get_text_bbox(struct graphics_priv *gr, struct graphics_font_priv *font, char *text, int dx, int dy, struct point *ret, int estimate)
{
	int len = g_utf8_strlen(text, -1);
	int xMin = 0;
	int yMin = 0;
	int yMax = 13*font->size/256;
	int xMax = 9*font->size*len/256;

	ret[0].x = xMin;
	ret[0].y = -yMin;
	ret[1].x = xMin;
	ret[1].y = -yMax;
	ret[2].x = xMax;
	ret[2].y = -yMax;
	ret[3].x = xMax;
	ret[3].y = -yMin;
}

static void
overlay_disable(struct graphics_priv *gr, int disabled)
{
	gr->overlay_disabled=disabled;
}

static struct graphics_methods graphics_methods = {
	NULL, /* graphics_destroy, */
	draw_mode,
	draw_lines,
	draw_polygon,
	draw_rectangle,
	NULL, /* draw_circle, */
	draw_text,
	draw_image,
	NULL, /* draw_image_warp, */
	draw_drag,
	font_new,
	gc_new,
	background_gc,
	overlay_new,
	image_new,
	get_data,
	image_free,
	get_text_bbox,
	overlay_disable,
	NULL, /* overlay_resize, */
	NULL, /* set_attr, */
};


static struct graphics_priv *
overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h, int wraparound)
{
	struct graphics_priv *ret=g_new0(struct graphics_priv, 1);
	*meth=graphics_methods;
	ret->p=*p;
	ret->w=w;
	ret->h=h;
	ret->parent=gr;
	ret->next=gr->overlays;
	ret->wraparound=wraparound;
	gr->overlays=ret;
	setup_graphics(gr);
	return ret;
}


struct graphics_priv *
graphics_cocoa_new(struct navit *nav, struct graphics_methods *meth, struct attr **attrs, struct callback_list *cbl)
{
	struct graphics_priv *ret;
	*meth=graphics_methods;
	dbg(0,"enter");
	if(!event_request_system("cocoa","graphics_cocoa"))
		return NULL;
	ret=g_new0(struct graphics_priv, 1);
	ret->cbl=cbl;
	global_graphics_cocoa=ret;
	return ret;
}

static void
event_cocoa_main_loop_run(void)
{

	dbg(0,"enter");
#if 0
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *documentsDirectory = [paths objectAtIndex:0];
	NSString *logPath = [documentsDirectory stringByAppendingPathComponent:@"console.log"];
	freopen("/tmp/log.txt","a+",stderr);
	NSLog(@"Test\n");
#endif
	NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
#if USE_UIKIT
	dbg(0,"calling main");
	int retval = UIApplicationMain(main_argc, main_argv, nil, @"NavitAppDelegate");
	dbg(0,"retval=%d",retval);
#else
	NavitAppDelegate * delegate = [[NavitAppDelegate alloc] init];
	NSApplication * application = [NSApplication sharedApplication];
	[application setDelegate:delegate];
	NSLog(@"Test\n");

	[NSApp run];

	[delegate release];
#endif
	[pool release];
}

@interface NavitTimer : NSObject{
@public
	struct callback *cb;
	NSTimer *timer;
}
- (void)onTimer:(NSTimer*)theTimer;

@end

@implementation NavitTimer

- (void)onTimer:(NSTimer*)theTimer
{
	callback_call_0(cb);
}


@end

struct event_idle {
	struct callback *cb;
	NSTimer *timer;
};

static struct event_timeout *
event_cocoa_add_timeout(int timeout, int multi, struct callback *cb)
{
	NavitTimer *ret=[[NavitTimer alloc]init];
	ret->cb=cb;
	ret->timer=[NSTimer scheduledTimerWithTimeInterval:(timeout/1000.0) target:ret selector:@selector(onTimer:) userInfo:nil repeats:multi?YES:NO];
	dbg(1,"timer=%p",ret->timer);
	return (struct event_timeout *)ret;
}


static void
event_cocoa_remove_timeout(struct event_timeout *ev)
{
	NavitTimer *t=(NavitTimer *)ev;

	[t->timer invalidate];
	[t release];
}


static struct event_idle *
event_cocoa_add_idle(int priority, struct callback *cb)
{
	NavitTimer *ret=[[NavitTimer alloc]init];
	ret->cb=cb;
	ret->timer=[NSTimer scheduledTimerWithTimeInterval:(0.0) target:ret selector:@selector(onTimer:) userInfo:nil repeats:YES];

	dbg(1,"timer=%p",ret->timer);
	return (struct event_idle *)ret;
}

static void
event_cocoa_remove_idle(struct event_idle *ev)
{
	NavitTimer *t=(NavitTimer *)ev;

	[t->timer invalidate];
	[t release];
}

static struct event_methods event_cocoa_methods = {
	event_cocoa_main_loop_run,
	NULL, /* event_cocoa_main_loop_quit, */
	NULL, /* event_cocoa_add_watch, */
	NULL, /* event_cocoa_remove_watch, */
	event_cocoa_add_timeout,
	event_cocoa_remove_timeout,
	event_cocoa_add_idle,
	event_cocoa_remove_idle,
	NULL, /* event_cocoa_call_callback, */
};


static struct event_priv *
event_cocoa_new(struct event_methods *meth)
{
	dbg(0,"enter");
	*meth=event_cocoa_methods;
	return NULL;
}


void
plugin_init(void)
{
	dbg(0,"enter");
	plugin_register_category_graphics("cocoa", graphics_cocoa_new);
	plugin_register_category_event("cocoa", event_cocoa_new);
}
