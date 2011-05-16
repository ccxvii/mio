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

void sys_start_idle_loop(void)
{
	if (!timer) {
		timer = [NSTimer timerWithTimeInterval: 0.001
			target: view
			selector: @selector(timerFired:)
			userInfo: nil
			repeats: YES];
		[[NSRunLoop currentRunLoop] addTimer: timer forMode: NSDefaultRunLoopMode];
		[[NSRunLoop currentRunLoop] addTimer: timer forMode: NSEventTrackingRunLoopMode];
	}
}

void sys_stop_idle_loop(void)
{
	if (timer) {
		[timer invalidate];
		[timer release];
		timer = nil;
	}
}

- (void) drawRect: (NSRect)rect
{
	GLenum error;

	rect = [self bounds];

	sys_hook_draw(rect.size.width, rect.size.height);

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

int translate_modifiers(NSEvent *evt)
{
	int mod = 0, nsmod = [evt modifierFlags];
	if (nsmod & NSShiftKeyMask) mod |= SYS_MOD_SHIFT;
	if (nsmod & NSControlKeyMask) mod |= SYS_MOD_CTL;
	if (nsmod & NSAlternateKeyMask) mod |= SYS_MOD_ALT;
	return mod;
}

int translate_button(NSEvent *evt)
{
	int btn = [evt buttonNumber] + 1;
	if (btn == 2) return 3;
	if (btn == 3) return 2;
	return btn;
}

NSPoint translate_point(NSView *view, NSEvent *evt)
{
	NSPoint p = [evt locationInWindow];
	p = [view convertPoint: p fromView: nil];
	p.y = [view frame].size.height - p.y;
	return p;
}

- (void) mouseDown:(NSEvent *)evt
{
	NSPoint p = translate_point(self, evt);
	int mod = translate_modifiers(evt);
	int btn = translate_button(evt);
	sys_hook_mouse_down(p.x, p.y, btn, mod);
	[self setNeedsDisplay: YES];
}

- (void) mouseUp:(NSEvent *)evt
{
	NSPoint p = translate_point(self, evt);
	int mod = translate_modifiers(evt);
	int btn = translate_button(evt);
	sys_hook_mouse_up(p.x, p.y, btn, mod);
	[self setNeedsDisplay: YES];
}

- (void) mouseDragged:(NSEvent *)evt
{
	NSPoint p = translate_point(self, evt);
	int mod = translate_modifiers(evt);
	sys_hook_mouse_move(p.x, p.y, mod);
	[self setNeedsDisplay: YES];
}

- (void) rightMouseDown:(NSEvent *)evt { [self mouseDown:evt]; }
- (void) rightMouseUp:(NSEvent *)evt { [self mouseUp:evt]; }
- (void) rightMouseDragged:(NSEvent *)evt { [self mouseDragged:evt]; }
- (void) otherMouseDown:(NSEvent *)evt { [self mouseDown:evt]; }
- (void) otherMouseUp:(NSEvent *)evt { [self mouseUp:evt]; }
- (void) otherMouseDragged:(NSEvent *)evt { [self mouseDragged:evt]; }

- (void) keyDown:(NSEvent *)evt
{
	int mod = translate_modifiers(evt);
	NSString *str = [evt characters];
	int i, len = [str length];
	if (len && ![evt isARepeat])
		sys_hook_key_down([str characterAtIndex: 0], mod);
	for (i = 0; i < len; i++)
		sys_hook_key_char([str characterAtIndex: i], mod);
	[self setNeedsDisplay: YES];
}

- (void) keyUp:(NSEvent *)evt
{
	int mod = translate_modifiers(evt);
	NSString *str = [evt characters];
	int len = [str length];
	if (len)
		sys_hook_key_up([str characterAtIndex: 0], mod);
	[self setNeedsDisplay: YES];
}

- (void) flagsChanged:(NSEvent *)evt
{
	static int oldmod = 0;
	int newmod = translate_modifiers(evt);
	int change = oldmod ^ newmod;
	if (change & SYS_MOD_SHIFT) {
		if (newmod & SYS_MOD_SHIFT)
			sys_hook_key_down(SYS_KEY_SHIFT, newmod);
		else
			sys_hook_key_up(SYS_KEY_SHIFT, newmod);
	}
	if (change & SYS_MOD_CTL) {
		if (newmod & SYS_MOD_CTL)
			sys_hook_key_down(SYS_KEY_CTL, newmod);
		else
			sys_hook_key_up(SYS_KEY_CTL, newmod);
	}
	if (change & SYS_MOD_ALT) {
		if (newmod & SYS_MOD_ALT)
			sys_hook_key_down(SYS_KEY_ALT, newmod);
		else
			sys_hook_key_up(SYS_KEY_ALT, newmod);
	}
	oldmod = newmod;
	[self setNeedsDisplay: YES];
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

	[window makeKeyAndOrderFront: NULL];
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
