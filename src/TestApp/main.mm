#import <Cocoa/Cocoa.h>
#import <ScreenSaver/ScreenSaver.h>
#import <IOKit/pwr_mgt/IOPMLib.h>
#import <IOKit/IOKitLib.h>
#import "../MacBoingBallView.h"

// Custom window class to handle key events
@interface TestWindow : NSWindow
@end

@implementation TestWindow

- (BOOL)canBecomeKeyWindow {
    return YES;
}

- (BOOL)canBecomeMainWindow {
    return YES;
}

- (void)keyDown:(NSEvent *)event {
    // ESC (keyCode 53) or Q (keyCode 12) to quit
    // Check if Cmd is pressed for Cmd+Q (handled by menu, but we can also handle it here)
    NSUInteger modifiers = [event modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;
    BOOL isCommandQ = ([event keyCode] == 12 && (modifiers & NSEventModifierFlagCommand));
    
    if ([event keyCode] == 53 || ([event keyCode] == 12 && !isCommandQ)) {
        // ESC or Q (without Cmd) - quit immediately
        [NSApp terminate:nil];
    } else {
        [super keyDown:event];
    }
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong, nonatomic) NSWindow *window;
@property (strong, nonatomic) MacBoingBallView *screensaverView;
@property (assign, nonatomic) IOPMAssertionID sleepAssertionID;
@property (strong, nonatomic) NSTimer *activityTimer;
@end

@implementation AppDelegate

- (instancetype)init {
    self = [super init];
    if (self) {
        _sleepAssertionID = 0;  // Invalid assertion ID
    }
    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Set up menu bar with Quit menu item (enables Cmd+Q)
    [self setupMenuBar];
    
    // Prevent system screensaver from activating while test app is running
    // This prevents interference with performance testing
    CFStringRef reasonForActivity = CFSTR("Boing Ball Performance Test - Preventing screensaver");
    IOReturn success = IOPMAssertionCreateWithName(kIOPMAssertionTypePreventUserIdleDisplaySleep,
                                                    255,  // kIOPMAssertionLevelOn
                                                    reasonForActivity,
                                                    &_sleepAssertionID);
    if (success == kIOReturnSuccess) {
        NSLog(@"System screensaver disabled for testing");
    } else {
        NSLog(@"Warning: Could not prevent system screensaver (error: %x)", success);
    }
    
    // Create fullscreen window for testing
    NSRect screenRect = [[NSScreen mainScreen] frame];
    self.window = [[TestWindow alloc] initWithContentRect:screenRect
                                              styleMask:NSWindowStyleMaskBorderless
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    [self.window setTitle:@"Boing Ball Performance Test"];
    [self.window setLevel:NSMainMenuWindowLevel + 1];
    [self.window setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces | NSWindowCollectionBehaviorFullScreenAuxiliary];
    [self.window setAcceptsMouseMovedEvents:YES];
    [self.window makeKeyAndOrderFront:nil];
    
    // Create screensaver view
    self.screensaverView = [[MacBoingBallView alloc] initWithFrame:[self.window.contentView bounds]
                                                         isPreview:NO];
    [self.screensaverView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [self.window.contentView addSubview:self.screensaverView];
    
    // Enable FPS counter for testing
    [self.screensaverView setShowFPS:YES];
    
    // Start animation
    [self.screensaverView startAnimation];
    
    NSLog(@"Test app started in fullscreen mode. Screensaver view: %@", self.screensaverView);
    NSLog(@"Press Cmd+Q, ESC, or Q to exit");
}

- (void)setupMenuBar {
    // Create main menu bar
    NSMenu *mainMenu = [[NSMenu alloc] init];
    [NSApp setMainMenu:mainMenu];
    
    // Create application menu
    NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
    [mainMenu addItem:appMenuItem];
    
    NSMenu *appMenu = [[NSMenu alloc] init];
    [appMenuItem setSubmenu:appMenu];
    
    // Add Quit menu item with Cmd+Q shortcut
    NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit Boing Ball Test"
                                                          action:@selector(terminate:)
                                                   keyEquivalent:@"q"];
    [quitMenuItem setKeyEquivalentModifierMask:NSEventModifierFlagCommand];
    [appMenu addItem:quitMenuItem];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    [self.screensaverView stopAnimation];
    
    // Stop the activity timer
    if (self.activityTimer) {
        [self.activityTimer invalidate];
        self.activityTimer = nil;
    }
    
    // Release the sleep assertion to allow screensaver again
    if (_sleepAssertionID != 0) {
        IOPMAssertionRelease(_sleepAssertionID);
        _sleepAssertionID = 0;
        NSLog(@"System screensaver re-enabled");
    }
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        AppDelegate *delegate = [[AppDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
}

