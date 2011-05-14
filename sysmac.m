#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>
#import <OpenGL/glu.h>

#include "syshook.h"

@interface SysView : NSOpenGLView
{
}
+ (NSOpenGLPixelFormat*) defaultPixelFormat;
- (void) timerFired: (NSTimer*)timer;
@end

@interface SysDelegate : NSObject <NSApplicationDelegate>
{
}
+(void)populateMainMenu;
+(void)populateApplicationMenu:(NSMenu *)aMenu;
+(void)populateWindowMenu:(NSMenu *)aMenu;
@end

static int g_argc;
static char **g_argv;

static int isfullscreen = 0;
static NSWindow *window;
static NSTimer *timer;
static SysDelegate *delegate;
static SysView *view;

@implementation SysView

+ (NSOpenGLPixelFormat*) defaultPixelFormat
{
	NSOpenGLPixelFormatAttribute atts[] = {
		NSOpenGLPFAWindow,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFANoRecovery,
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAStencilSize, 8,
		NSOpenGLPFAAccumSize, 0,
		0
	};
	return [[[NSOpenGLPixelFormat alloc] initWithAttributes:atts] autorelease];
}

- (void) prepareOpenGL
{
	GLint swap = 1;
	[[self openGLContext] setValues: &swap
		forParameter: NSOpenGLCPSwapInterval];

	sys_hook_init(g_argc, g_argv);
}

- (void) timerFired: (NSTimer*)timer
{
	[self setNeedsDisplay: YES];
	/* this may not be the right thing to do, but it works */
	if ([self inLiveResize])
		[self drawRect: [self bounds]];
}

- (void) drawRect: (NSRect)rect
{
	GLenum error;

	rect = [self bounds];

	sys_hook_loop(rect.size.width, rect.size.height);

	[[self openGLContext] flushBuffer];

#ifndef NDEBUG
	error = glGetError();
	if (error != GL_NO_ERROR)
		fprintf(stderr, "error: opengl (%s)\n", gluErrorString(error));
#endif
}

- (BOOL) acceptsFirstResponder
{
	return YES;
}

- (void) mouseDown:(NSEvent *)anEvent
{
	puts("mousedown!");
}

- (void) mouseUp:(NSEvent *)anEvent
{
	puts("mouseup!");
}

- (void) mouseDragged:(NSEvent *)anEvent
{
	puts("mousemoved!");
}

- (void) keyDown:(NSEvent *)anEvent
{
	puts("keydown!");
}

- (void) keyUp:(NSEvent *)anEvent
{
	puts("keyup!");
}

@end

@implementation SysDelegate

+(void) populateMainMenu
{
	NSMenu *mainMenu = [[NSMenu alloc] initWithTitle:@"MainMenu"];
	NSMenuItem *menuItem;
	NSMenu *submenu;

	menuItem = [mainMenu addItemWithTitle:@"Apple" action:NULL keyEquivalent:@""];
	submenu = [[NSMenu alloc] initWithTitle:@"Apple"];
	[NSApp performSelector:@selector(setAppleMenu:) withObject:submenu];
	[self populateApplicationMenu:submenu];
	[mainMenu setSubmenu:submenu forItem:menuItem];

	menuItem = [mainMenu addItemWithTitle:@"Window" action:NULL keyEquivalent:@""];
	submenu = [[NSMenu alloc] initWithTitle:NSLocalizedString(@"Window", @"The Window menu")];
	[self populateWindowMenu:submenu];
	[mainMenu setSubmenu:submenu forItem:menuItem];
	[NSApp setWindowsMenu:submenu];

	[NSApp setMainMenu:mainMenu];
}

+(void) populateApplicationMenu:(NSMenu *)aMenu
{
	NSString *applicationName = [[NSProcessInfo processInfo] processName];
	NSMenuItem *menuItem;

	menuItem = [aMenu addItemWithTitle:[NSString stringWithFormat:@"%@ %@", NSLocalizedString(@"About", nil), applicationName]
		action:@selector(orderFrontStandardAboutPanel:)
		keyEquivalent:@""];
	[menuItem setTarget:NSApp];

	[aMenu addItem:[NSMenuItem separatorItem]];

	menuItem = [aMenu addItemWithTitle:NSLocalizedString(@"Preferences...", nil)
		action:NULL
		keyEquivalent:@","];

	[aMenu addItem:[NSMenuItem separatorItem]];

	menuItem = [aMenu addItemWithTitle:NSLocalizedString(@"Services", nil)
		action:NULL
		keyEquivalent:@""];
	NSMenu * servicesMenu = [[NSMenu alloc] initWithTitle:@"Services"];
	[aMenu setSubmenu:servicesMenu forItem:menuItem];
	[NSApp setServicesMenu:servicesMenu];

	[aMenu addItem:[NSMenuItem separatorItem]];

	menuItem = [aMenu addItemWithTitle:[NSString stringWithFormat:@"%@ %@", NSLocalizedString(@"Hide", nil), applicationName]
		action:@selector(hide:)
		keyEquivalent:@"h"];
	[menuItem setTarget:NSApp];

	menuItem = [aMenu addItemWithTitle:NSLocalizedString(@"Hide Others", nil)
		action:@selector(hideOtherApplications:)
		keyEquivalent:@"h"];
	[menuItem setKeyEquivalentModifierMask:NSCommandKeyMask | NSAlternateKeyMask];
	[menuItem setTarget:NSApp];

	menuItem = [aMenu addItemWithTitle:NSLocalizedString(@"Hide Others", nil)
		action:@selector(hideOtherApplications:)
		keyEquivalent:@"h"];
	[menuItem setKeyEquivalentModifierMask:NSCommandKeyMask | NSAlternateKeyMask];
	[menuItem setTarget:NSApp];

	menuItem = [aMenu addItemWithTitle:NSLocalizedString(@"Show All", nil)
		action:@selector(unhideAllApplications:)
		keyEquivalent:@""];
	[menuItem setTarget:NSApp];

	[aMenu addItem:[NSMenuItem separatorItem]];

	menuItem = [aMenu addItemWithTitle:[NSString stringWithFormat:@"%@ %@", NSLocalizedString(@"Quit", nil), applicationName]
		action:@selector(terminate:)
		keyEquivalent:@"q"];
	[menuItem setTarget:NSApp];
}

+(void) populateWindowMenu:(NSMenu *)aMenu
{
	NSMenuItem *menuItem;

	menuItem = [aMenu addItemWithTitle:NSLocalizedString(@"Minimize", nil)
		action:@selector(miniaturize:)
		keyEquivalent:@"m"];
	[menuItem setKeyEquivalentModifierMask:NSCommandKeyMask];

	menuItem = [aMenu addItemWithTitle:NSLocalizedString(@"Zoom", nil)
		action:@selector(zoom:)
		keyEquivalent:@""];

	menuItem = [aMenu addItemWithTitle:NSLocalizedString(@"Fullscreen", nil)
		action:@selector(fullscreen:)
		keyEquivalent:@"F"];
	[menuItem setKeyEquivalentModifierMask:NSCommandKeyMask];
	[menuItem setTarget:[NSApp delegate]];

	[aMenu addItem:[NSMenuItem separatorItem]];
}

- (void) applicationWillFinishLaunching: (NSNotification *)notification
{
}

- (void) applicationDidFinishLaunching:(NSNotification *)notification
{
	CGRect cgr;
	NSRect rect;

	[SysDelegate populateMainMenu];

	cgr = CGDisplayBounds(CGMainDisplayID());
	rect = NSMakeRect(50, 100, 640, 480);

	window = [[NSWindow alloc] initWithContentRect: rect
		styleMask: NSClosableWindowMask | NSTitledWindowMask |
			NSMiniaturizableWindowMask | NSResizableWindowMask
		backing: NSBackingStoreBuffered
		defer: YES
		screen: [NSScreen mainScreen]];
	[window setTitle: [[NSProcessInfo processInfo] processName]];
	[window setAcceptsMouseMovedEvents: YES];
	[window setDelegate: [NSApp delegate]];

	view = [[SysView alloc] initWithFrame: rect
		pixelFormat: [SysView defaultPixelFormat]];
	[window setContentView: view];
	[view release];

	// TODO: seticon

	// [[NSApp dockTile] setShowsApplicationBadge: YES];
	// [[NSApp dockTile] display];

	[window makeKeyAndOrderFront: NULL];

	timer = [NSTimer timerWithTimeInterval: 0.001
		target: view
		selector: @selector(timerFired:)
		userInfo: nil
		repeats: YES];
	[[NSRunLoop currentRunLoop] addTimer: timer forMode: NSDefaultRunLoopMode];
	[[NSRunLoop currentRunLoop] addTimer: timer forMode: NSEventTrackingRunLoopMode];
}

- (void) fullscreen:(NSNotification *)notification
{
	if (isfullscreen) {
		isfullscreen = 0;
		[view exitFullScreenModeWithOptions:nil];
	} else {
		isfullscreen = 1;
		[view enterFullScreenMode:[window screen] withOptions:nil];
	}
}

- (void) windowWillClose:(NSNotification *)notification
{
	[NSApp stop:self];
}

@end

int sys_is_fullscreen(void)
{
	return isfullscreen;
}

void sys_enter_fullscreen(void)
{
	if (!isfullscreen)
		[delegate fullscreen:NULL];
}

void sys_leave_fullscreen(void)
{
	if (isfullscreen)
		[delegate fullscreen:NULL];
}

void sys_start_idle_loop(void)
{
}

void sys_stop_idle_loop(void)
{
}

int main(int argc, char **argv)
{
	NSAutoreleasePool *pool;
	NSApplication *app;

	g_argc = argc;
	g_argv = argv;

	pool = [[NSAutoreleasePool alloc] init];
	app = [NSApplication sharedApplication];
	delegate = [[SysDelegate alloc] init];

	/* carbon voodoo to get icon and menu without bundle */
	ProcessSerialNumber psn = { 0, kCurrentProcess };
	TransformProcessType(&psn, kProcessTransformToForegroundApplication);
	SetFrontProcess(&psn);

	[app setDelegate: delegate];
	[app run];
	[app setDelegate: NULL];
	[delegate release];
	[pool release];
	return 0;
}
