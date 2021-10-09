#include <glib.h>
#include "config.h"
#include "config_.h"
#include "debug.h"
#include "plugin.h"
#include "point.h"
#include "window.h"
#include "graphics.h"
#include "navit/event.h"
#include "item.h"
#include "callback.h"
#include "color.h"
#include <iconv.h>
#include "navit.h"

#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED
#define USE_UIKIT 1
#else
#define USE_UIKIT 0
#endif

#include "graphics_cocoa.h"

#if USE_UIKIT
#import <Foundation/Foundation.h>
#import <CoreText/CoreText.h>
#define NSRect CGRect
#define NSMakeRect CGRectMake
#define REVERSE_Y 0

CGContextRef current_context(void) {
    return UIGraphicsGetCurrentContext();
}

#else
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
#define UIFont NSFont
#define UIColor NSColor

CGContextRef current_context(void) {
    return [[NSGraphicsContext currentContext] CGContext];
}

#endif


@interface NavitView : UIView {
@public
    struct graphics_priv *graphics;
}

@end

static struct graphics_priv {
    struct navit *navit;
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

int has_appeared = 0;
float startScale = 1;


@implementation NavitView

- (void)drawRect:(NSRect)rect {
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

#if (USE_UIKIT==0)
- (void)mouseDown:(UIEvent *)theEvent {
    struct point p;
    p.x=theEvent.locationInWindow.x;
    p.y=graphics->h-theEvent.locationInWindow.y;

    dbg(1,"Enter %d %d",p.x,p.y);
    callback_list_call_attr_3(graphics->cbl, attr_button, GINT_TO_POINTER(1), GINT_TO_POINTER(1), (void *)&p);
}

- (void)mouseUp:(UIEvent *)theEvent {
    struct point p;
    p.x=theEvent.locationInWindow.x;
    p.y=graphics->h-theEvent.locationInWindow.y;

    dbg(1,"Enter %d %d",p.x,p.y);
    callback_list_call_attr_3(graphics->cbl, attr_button, GINT_TO_POINTER(0), GINT_TO_POINTER(1), (void *)&p);
}

- (void)mouseDragged:(UIEvent *)theEvent {
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

@interface NavitViewController : UIViewController {
    NSRect frame;
    CGLayerRef layer;
}

@property (nonatomic) NSRect frame;

- (id) init_withFrame : (NSRect) _frame;

@end



@implementation NavitViewController

@synthesize frame;
#if USE_UIKIT
- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    return (UIInterfaceOrientationMaskAll);
}

- (IBAction)handlePinch:(UIPinchGestureRecognizer *)sender {
    if(sender.state == UIGestureRecognizerStateBegan) {
        startScale = sender.scale;
    } else if(sender.state == UIGestureRecognizerStateEnded || sender.state == UIGestureRecognizerStateChanged) {
        struct point p;
        p.x=[sender locationInView: self.view].x;
        p.y=[sender locationInView: self.view].y;

        if((startScale / sender.scale > 2) ||  (startScale / sender.scale < 0.5)) {
            if((sender.scale > 1)) {
                navit_zoom_in(global_graphics_cocoa->navit, 2, &p);
                startScale=sender.scale * 1.5;
            } else {
                navit_zoom_out(global_graphics_cocoa->navit, 2, &p);
                startScale=sender.scale * 0.75;
            }
        }
    }
}

- (IBAction)handlePan:(UIPanGestureRecognizer *)sender {

    if (sender.state == UIGestureRecognizerStateBegan) {
        struct point p;
        p.x=[sender locationInView: self.view].x;
        p.y=[sender locationInView: self.view].y;
        callback_list_call_attr_3(global_graphics_cocoa->cbl, attr_button, GINT_TO_POINTER(1), GINT_TO_POINTER(1), (void *)&p);

    }

    if (sender.state == UIGestureRecognizerStateChanged) {
        struct point p;
        p.x=[sender locationInView: self.view].x;
        p.y=[sender locationInView: self.view].y;
        callback_list_call_attr_1(global_graphics_cocoa->cbl, attr_motion, (void *)&p);
    }

    if(sender.state == UIGestureRecognizerStateEnded) {
        struct point p;
        p.x=[sender locationInView: self.view].x;
        p.y=[sender locationInView: self.view].y;
        callback_list_call_attr_3(global_graphics_cocoa->cbl, attr_button, GINT_TO_POINTER(0), GINT_TO_POINTER(1), (void *)&p);
    }
}

- (IBAction)handleTap:(UITapGestureRecognizer *)sender {
    if (sender.state == UIGestureRecognizerStateEnded) {
        struct CGPoint pc=[sender locationInView: self.view];
        struct point p;
        p.x=pc.x;
        p.y=pc.y;
        callback_list_call_attr_3(global_graphics_cocoa->cbl, attr_button, GINT_TO_POINTER(1), GINT_TO_POINTER(1), (void *)&p);
        callback_list_call_attr_3(global_graphics_cocoa->cbl, attr_button, GINT_TO_POINTER(0), GINT_TO_POINTER(1), (void *)&p);
    }
}

- (void)rotated:(NSNotification *)notification {

    NSLog(@"rotated enter");

    int width =(int)UIScreen.mainScreen.bounds.size.width;
    int height = (int)UIScreen.mainScreen.bounds.size.height;

    // Hack: issue on     iPad2: the width and height of the mainscreen bounds are not changing when orientation changes
    // Detect OS version and exchange width<->height when iOS < 10 detected and orientation is landscape
    UIDeviceOrientation orientation = [[UIDevice currentDevice] orientation];
    int lt_ten=1;

    NSLog(@"System Version: %f",[[[UIDevice currentDevice] systemVersion] floatValue]);

    if([[[UIDevice currentDevice] systemVersion] floatValue] >=10) {
        lt_ten=0;

    }

    if(lt_ten &&  (orientation==UIDeviceOrientationFaceDown
                   || orientation == UIDeviceOrientationFaceUp)) {
        return;
    }

    if (!UIDeviceOrientationIsValidInterfaceOrientation(orientation)) {
        return;
    }

    if (lt_ten && UIDeviceOrientationIsLandscape(orientation)) {
        global_graphics_cocoa->w=height;
        global_graphics_cocoa->h=width;
        callback_list_call_attr_2(global_graphics_cocoa->cbl, attr_resize, (int)height, (int)width);
        NSLog(@"Rotated 9 %i %i %i %ld", lt_ten, width, height, (long)orientation);
    } else {
        global_graphics_cocoa->w=width;
        global_graphics_cocoa->h=height;
        callback_list_call_attr_2(global_graphics_cocoa->cbl, attr_resize, (int)width, (int)height);
        NSLog(@"Rotated 10 %i %i %i %ld", lt_ten, width, height, (long)orientation);
    }

    dbg(1,"Rotated");
}

- (BOOL)prefersStatusBarHidden {
    return NO;
}

#endif

- (id) init_withFrame : (NSRect) _frame {
    NSLog(@"init with frame\n");
    frame = _frame;
    return [self init];
}

static
void free_graphics(struct graphics_priv *gr) {
    if (gr->layer) {
        CGLayerRelease(gr->layer);
        gr->layer=NULL;
    }
}

static void setup_graphics(struct graphics_priv *gr) {
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

- (void)loadView {
    NSLog(@"loadView");
    NavitView* myV = [NavitView alloc];

#if USE_UIKIT
    myV.tag = 100;
#endif
    
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

- (void)viewDidLoad {
    NSLog(@"View loaded!");
#if USE_UIKIT
    NSNotificationCenter *notficationcenter = NSNotificationCenter.defaultCenter;
    [notficationcenter addObserver:self selector:@selector(appMovedToBackground:) name:
                       UIApplicationWillResignActiveNotification object: nil];
    [notficationcenter addObserver:self selector:@selector(appMovedToForeground:) name:
                       UIApplicationDidBecomeActiveNotification object: nil];
#endif
}

- (void)viewDidAppear:(BOOL)animated {
    dbg(lvl_debug,"view appeared");
#if USE_UIKIT
    self.modalPresentationCapturesStatusBarAppearance = false;
#endif

    has_appeared = 1;
#if USE_UIKIT
    callback_list_call_attr_0(global_graphics_cocoa->cbl, attr_vehicle_request_location_authorization);

    UIPinchGestureRecognizer* pinch = [[UIPinchGestureRecognizer alloc]initWithTarget:self action:@selector(handlePinch:)];
    [self.view addGestureRecognizer:pinch];

    UITapGestureRecognizer* tap=[[UITapGestureRecognizer alloc]initWithTarget:self action:@selector(handleTap:)];
    [self.view addGestureRecognizer:tap];

    UIPanGestureRecognizer* pan=[[UIPanGestureRecognizer alloc]initWithTarget:self action:@selector(handlePan:)];
    [self.view addGestureRecognizer:pan];

    [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(rotated:) name:
                                          UIDeviceOrientationDidChangeNotification object:nil];

#endif
}

- (void) appMovedToBackground:(NSNotification*)note {
    NSLog(@"App moved to background!");
    //TODO: add a callback to deactivate speech instance. Otherwise an active announcement will keep the radio muted in HFP mode
}

#if USE_UIKIT
- (void) appMovedToForeground:(NSNotification*)note {
    NSLog(@"App moved to foreground!");
    [self rotated: (NULL)];
    //TODO: add a callback to deactivate speech instance. Otherwise an active announcement will keep the radio muted in HFP mode
}
#endif

- (void)didReceiveMemoryWarning {
    dbg(1,"enter");
}
#if USE_UIKIT
- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
    NSLog(@"didRotateFromInterfaceOrientation enter");
    [self rotated: (NULL)];
}
#endif

- (void)viewDidUnload {
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (void)dealloc {
    NSLog(@"Dealloc enter");
    [super dealloc];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

@end

@class NavitViewController;

@interface NavitAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    NavitViewController *viewController;
}

@property (nonatomic, retain) /*IBOutlet*/ UIWindow *window;
@property (nonatomic, retain) /*IBOutlet*/ NavitViewController *viewController;

void onUncaughtException(NSException* exception);

@end


@implementation NavitAppDelegate

@synthesize window;
@synthesize viewController;

void onUncaughtException(NSException* exception) {
    NSLog(@"onUncaughtException: %@ %@", exception.reason, exception.debugDescription);
#if (!USE_UIKIT)
    [NSApp performSelector:@selector(terminate:) withObject:nil afterDelay:0.0];
#endif
}

#if USE_UIKIT
- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
#else
- (void)
    applicationDidFinishLaunching:(NSNotification *)aNotification
#endif
{
    NSLog(@"DidFinishLaunching\n");
    //#if USE_UIKIT
    //    [[UIApplication sharedApplication] setStatusBarHidden:NO];
    //#endif
    
    NSSetUncaughtExceptionHandler(&onUncaughtException);
#if USE_UIKIT
    NSRect appFrame = [UIScreen mainScreen].bounds;
#else
    NSRect appFrame = [NSScreen mainScreen].frame;
#endif
    
    self.viewController = [[[NavitViewController alloc] init_withFrame : appFrame] autorelease];

#if USE_UIKIT
    self.viewController.modalPresentationStyle = UIModalPresentationFullScreen;
#endif
    NSRect windowRect = NSMakeRect(0, 0, appFrame.size.width, appFrame.size.height);

#if USE_UIKIT
    self.window = [[[UIWindow alloc] initWithFrame:windowRect] autorelease];
#else
    self.window = [[[UIWindow alloc] initWithContentRect:windowRect styleMask:NSWindowStyleMaskBorderless backing:
                                      NSBackingStoreBuffered defer:NO] autorelease];
#endif
    utf8_macosroman=iconv_open("MACROMAN","UTF-8");

#if USE_UIKIT
    [window setRootViewController:viewController];
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


static void draw_mode(struct graphics_priv *gr, enum draw_mode_num mode) {
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

static void draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count) {
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

static void draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count) {
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

static void draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h) {
    CGRect lr=CGRectMake(p->x, p->y, w, h);
    if (p->x <= 0 && p->y <= 0 && p->x+w+1 >= gr->w && p->y+h+1 >= gr->h) {
        dbg(1,"clear %p %dx%d",gr,w,h);
        free_graphics(gr);
        setup_graphics(gr);
    }
    CGContextSetFillColor(gr->layer_context, gc->rgba);
    CGContextFillRect(gr->layer_context, lr);
}

static void draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg,
                      struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy) {
#pragma unused (bg)
    size_t outlen=strlen(text)+1;
    char outb[outlen];
    char *inp=text;

    strcpy(outb, inp);

    CGContextRef context = gr->layer_context;
    CGContextSaveGState(context);

#if !USE_UIKIT
    NSGraphicsContext * oldctx = [NSGraphicsContext currentContext];
    NSGraphicsContext * nscg = [NSGraphicsContext graphicsContextWithCGContext:context flipped:NO];
    [NSGraphicsContext setCurrentContext:nscg];
#endif

    CGAffineTransform xform = CGAffineTransformMake(dx/65536.0, dy/65536.0, dy/65536.0, -dx/65536.0, p->x, p->y );
    CGContextTranslateCTM(context, 0, 0);
    CGContextConcatCTM(context, xform);

#if USE_UIKIT
    CGAffineTransform flipit = CGAffineTransformMakeScale(1, -1);
    CGContextConcatCTM(context, flipit);
#endif

    CGColorRef color = CGColorCreate(CGColorSpaceCreateDeviceRGB(), fg->rgba);
    NSDictionary *attrs = [NSDictionary dictionaryWithObjectsAndKeys:[UIFont systemFontOfSize:font->size/16.0],
                                        NSFontAttributeName, [UIColor colorWithCGColor:(CGColorRef) color], NSForegroundColorAttributeName, nil];

#if USE_UIKIT
    UIGraphicsPushContext(context);
#endif

    NSAttributedString *myText = [[NSAttributedString alloc] initWithString:[NSString stringWithUTF8String:outb] attributes
                                                             :attrs];

#if USE_UIKIT
    [myText drawAtPoint:CGPointMake(0, 0
                                    -font->size/16.0)];//[(id)mytext drawAtPoint:CGPointMake(p->x, p->y-font->size/16.0) withAttributes:attrs];
    UIGraphicsPopContext();
#else
    [myText drawAtPoint:CGPointMake(0 + font->size/16.0/4,
                                    0 - font->size/16.0/4)];//[(id)mytext drawAtPoint:CGPointMake(p->x, p->y-font->size/16.0) withAttributes:attrs];
    [NSGraphicsContext setCurrentContext:oldctx];
#endif

    CGContextRestoreGState(context);
}

static void draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p,
                       struct graphics_image_priv *img) {
#pragma unused (fg)
    CGImageRef imgc=(CGImageRef) img;
    int w=(int)CGImageGetWidth(imgc);
    int h=(int)CGImageGetHeight(imgc);
    CGRect rect=CGRectMake(0, 0, w, h);
    CGContextSaveGState(gr->layer_context);
    CGContextTranslateCTM(gr->layer_context, p->x, p->y+h);
    CGContextScaleCTM(gr->layer_context, 1.0, -1.0);
    CGContextDrawImage(gr->layer_context, rect, imgc);
    CGContextRestoreGState(gr->layer_context);
}

static void font_destroy(struct graphics_font_priv *font) {
    g_free(font);
}

static struct graphics_font_methods font_methods = {
    font_destroy
};

static void draw_drag(struct graphics_priv *gr, struct point *p) {
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

static struct graphics_font_priv *font_new(struct graphics_priv *gr, struct graphics_font_methods *meth, char *font,
        int size, int flags) {
#pragma unused (font, flags, gr)
    struct graphics_font_priv *ret=g_new0(struct graphics_font_priv, 1);
    *meth=font_methods;

    ret->size=size;
    ret->name="Helvetica";
    return ret;
}

static void gc_destroy(struct graphics_gc_priv *gc) {
    g_free(gc);
}

static void gc_set_linewidth(struct graphics_gc_priv *gc, int w) {
    gc->w=w;
}

static void gc_set_dashes(struct graphics_gc_priv *gc, int w, int offset, unsigned char *dash_list, int n) {
#pragma unused (gc, w, offset, dash_list, n)
}

static void gc_set_foreground(struct graphics_gc_priv *gc, struct color *c) {
    gc->rgba[0]=c->r/65535.0;
    gc->rgba[1]=c->g/65535.0;
    gc->rgba[2]=c->b/65535.0;
    gc->rgba[3]=c->a/65535.0;
}

static void gc_set_background(struct graphics_gc_priv *gc, struct color *c) {
#pragma unused (gc, c)
}

static void gc_set_texture(struct graphics_gc_priv *gc, struct graphics_image_priv *img) {
#pragma unused (gc, img)
}

static struct graphics_gc_methods gc_methods = {
    gc_destroy,
    gc_set_linewidth,
    gc_set_dashes,
    gc_set_foreground,
    gc_set_background,
    gc_set_texture,
};

static struct graphics_gc_priv *gc_new(struct graphics_priv *gr, struct graphics_gc_methods *meth) {
#pragma unused (gr, meth)
    struct graphics_gc_priv *gc=g_new(struct graphics_gc_priv, 1);
    gc->w=1;

    *meth=gc_methods;
    return gc;
}


static void background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc) {
#pragma unused (gr, gc)
}

static struct graphics_priv *overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p,
        int w, int h, int wraparound);


static struct graphics_image_priv *image_new(struct graphics_priv *gra, struct graphics_image_methods *meth, char *path,
        int *w, int *h, struct point *hot, int rotation) {
#pragma unused (gra, meth, rotation)
    NSString *s=[[NSString alloc]initWithCString:path encoding:NSMacOSRomanStringEncoding];
    CGDataProviderRef imgDataProvider = CGDataProviderCreateWithCFData((CFDataRef)[NSData dataWithContentsOfFile:s]);
    [s release];

    if (!imgDataProvider)
        return NULL;

    CGImageRef image = CGImageCreateWithPNGDataProvider(imgDataProvider, NULL, true, kCGRenderingIntentDefault);
    CGDataProviderRelease(imgDataProvider);
    dbg(1,"size %dx%d",(int)CGImageGetWidth(image),(int)CGImageGetHeight(image));
    if (w)
        *w=(int)CGImageGetWidth(image);
    if (h)
        *h=(int)CGImageGetHeight(image);
    if (hot) {
        hot->x=(int)CGImageGetWidth(image)/2;
        hot->y=(int)CGImageGetHeight(image)/2;
    }
    return (struct graphics_image_priv *)image;
}

static int cocoa_fullscreen(struct window *win, int on) {
	    return 0;
}

static void *get_data(struct graphics_priv *this, const char *type) {
    dbg(lvl_debug,"enter");
    if (strcmp(type,"window"))
        return NULL;
    this->win.fullscreen=cocoa_fullscreen;
    this->win.disable_suspend=0;
    return &this->win;
}

static void image_free(struct graphics_priv *gr, struct graphics_image_priv *priv) {
#pragma unused (gr)
    CGImageRelease((CGImageRef)priv);
}

static void get_text_bbox(struct graphics_priv *gr, struct graphics_font_priv *font, char *text, int dx, int dy,
                          struct point *ret, int estimate) {
#pragma unused (gr, dx, dy, estimate)
    int len = (int)g_utf8_strlen(text, -1);
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

static void overlay_disable(struct graphics_priv *gr, int disabled) {
    gr->overlay_disabled=disabled;
}

static int set_attr(struct graphics_priv *gr, struct attr * attr) {
    if(attr->type == attr_callback) {
        callback_list_add(gr->cbl, attr->u.callback);
    }
    return 1;
}

static void overlay_resize(struct graphics_priv *this, struct point *p, int w, int h, int wraparound) {
    //do not dereference parent for non overlay osds
    if(!this->parent) {
        return;
    }

    int changed = 0;
    int w2,h2;

    if (w == 0) {
        w2 = 1;
    } else {
        w2 = w;
    }

    if (h == 0) {
        h2 = 1;
    } else {
        h2 = h;
    }

    this->p = *p;
    if (this->w != w2) {
        this->w = w2;
        changed = 1;
    }

    if (this->h != h2) {
        this->h = h2;
        changed = 1;
    }

    this->wraparound = wraparound;

    if (changed) {
        callback_list_call_attr_2(this->cbl, attr_resize, GINT_TO_POINTER(w), GINT_TO_POINTER(h));
    }
}

static void graphics_destroy (struct graphics_priv *gr) {
#pragma unused (gr)
}

static struct graphics_methods graphics_methods = {
    graphics_destroy, /* graphics_destroy, */
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
    overlay_resize,
    set_attr,
    NULL, /* show_native_keyboard, */
    NULL, /* hide_native_keyboard, */
    NULL, /* navit_float, */
    NULL, /* draw_polygon_with_holes, */
};

static struct graphics_priv *overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p,
        int w, int h, int wraparound) {
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

static struct graphics_priv *graphics_cocoa_new(struct navit *nav, struct graphics_methods *meth, struct attr **attrs,
        struct callback_list *cbl) {
#pragma unused (attrs)
    struct graphics_priv *ret;
    *meth=graphics_methods;
    dbg(1,"enter");
    if(!event_request_system("cocoa","graphics_cocoa"))
        return NULL;
    ret=g_new0(struct graphics_priv, 1);
    ret->navit = nav;
    ret->cbl=cbl;
    global_graphics_cocoa=ret;
    return ret;
}



struct event_watch {
    GIOChannel *iochan;
    guint source;
};

static gboolean event_cocoa_call_watch(GIOChannel * iochan, GIOCondition condition, gpointer t) {
#pragma unused(iochan, condition)
    struct callback *cb=t;
    callback_call_0(cb);
    return TRUE;
}

static struct event_watch *event_cocoa_add_watch(int fd, enum event_watch_cond cond, struct callback *cb) {
    struct event_watch *ret=g_new0(struct event_watch, 1);
    int flags=0;
    ret->iochan = g_io_channel_unix_new(fd);
    switch (cond) {
    case event_watch_cond_read:
        flags=G_IO_IN;
        break;
    case event_watch_cond_write:
        flags=G_IO_OUT;
        break;
    case event_watch_cond_except:
        flags=G_IO_ERR|G_IO_HUP;
        break;
    }
    ret->source = g_io_add_watch(ret->iochan, flags, event_cocoa_call_watch, (gpointer)cb);
    return ret;
}

static void event_cocoa_remove_watch(struct event_watch *ev) {
    if (! ev)
        return;
    g_source_remove(ev->source);
    g_io_channel_unref(ev->iochan);
    g_free(ev);
}


static void event_cocoa_main_loop_run(void) {

    dbg(1,"enter");
#if 0
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    NSString *logPath = [documentsDirectory stringByAppendingPathComponent:@"console.log"];
    freopen("/tmp/log.txt","a+",stderr);
    NSLog(@"Test\n");
#endif
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
#if USE_UIKIT
    dbg(1,"calling main");
    int retval = UIApplicationMain(main_argc, (char * _Nullable * _Nonnull)main_argv, nil, @"NavitAppDelegate");
    dbg(1,"retval=%d",retval);
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

#if (!USE_UIKIT)
static void event_cocoa_main_loop_quit(void) {
    [NSApp performSelector:@selector(terminate:) withObject:nil afterDelay:0.0];
}
#endif

@interface NavitTimer : NSObject {
@public
    struct callback *cb;
    NSTimer *timer;
}
- (void)onTimer:(NSTimer*)theTimer;

@end

@implementation NavitTimer

- (void)onTimer:(NSTimer*)theTimer {
    callback_call_0(cb);
}


@end

struct event_idle {
    struct callback *cb;
    NSTimer *timer;
};

static struct event_timeout *event_cocoa_add_timeout(int timeout, int multi, struct callback *cb) {
    NavitTimer *ret=[[NavitTimer alloc]init];
    ret->cb=cb;
    ret->timer=[NSTimer scheduledTimerWithTimeInterval:(timeout/1000.0) target:ret selector:@selector(
                            onTimer:) userInfo:nil repeats:multi?YES:NO];
    dbg(1,"timer=%p",ret->timer);
    return (struct event_timeout *)ret;
}


static void event_cocoa_remove_timeout(struct event_timeout *ev) {
    NavitTimer *t=(NavitTimer *)ev;

    if(t) {
        [t->timer invalidate];
        [t release];
    }
}


static struct event_idle *event_cocoa_add_idle(int priority, struct callback *cb) {
#pragma unused (priority)
    NavitTimer *ret=[[NavitTimer alloc]init];
    ret->cb=cb;
    ret->timer=[NSTimer scheduledTimerWithTimeInterval:(0.0) target:ret selector:@selector(
                            onTimer:) userInfo:nil repeats:YES];

    dbg(1,"timer=%p",ret->timer);
    return (struct event_idle *)ret;
}

static void event_cocoa_remove_idle(struct event_idle *ev) {
    NavitTimer *t=(NavitTimer *)ev;

    [t->timer invalidate];
    [t release];
}

static struct event_methods event_cocoa_methods = {
    event_cocoa_main_loop_run,
#if (USE_UIKIT)
    NULL, /* event_cocoa_main_loop_quit */
    NULL, /* event_cocoa_add_watch */
    NULL, /* event_cocoa_remove_watch */
#else
    event_cocoa_main_loop_quit,
    event_cocoa_add_watch,
    event_cocoa_remove_watch,
#endif
    event_cocoa_add_timeout,
    event_cocoa_remove_timeout,
    event_cocoa_add_idle,
    event_cocoa_remove_idle,
    NULL, /* event_cocoa_call_callback, */
};


static struct event_priv *event_cocoa_new(struct event_methods *meth) {
    dbg(1,"enter");
    *meth=event_cocoa_methods;
    return NULL;
}


void plugin_init(void) {
    dbg(1,"enter");
    plugin_register_category_graphics("cocoa", graphics_cocoa_new);
    plugin_register_category_event("cocoa", event_cocoa_new);
}
