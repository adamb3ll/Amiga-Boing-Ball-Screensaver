// MacBoingBallView.mm — macOS ScreenSaver view implementation

#import "MacBoingBallView.h"
#include "MacPlatform.h"
#include "../../core/BoingPhysics.h"
#include "../../core/BoingRenderer.h"
#include "../../core/BoingConfig.h"
#import <os/log.h>

static os_log_t getLog() {
    static os_log_t log = NULL;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        log = os_log_create("com.sinphaltimus.BoingBallSaver", "screensaver");
    });
    return log;
}

@implementation MacBoingBallView

+ (void)load {
    // Class loaded by runtime
}

+ (void)initialize {
    // Class initialized
}

+ (Class)class {
    return [super class];
}

- (instancetype)initWithFrame:(NSRect)frame isPreview:(BOOL)isPreview {
    self = [super initWithFrame:frame isPreview:isPreview];
    if (self) {
        _physics = nullptr;
        _renderer = nullptr;
        _platform = nullptr;
        _config = nullptr;
        _renderConfig = nullptr;
        _glContext = nil;
        _glPixelFormat = nil;
        _prevTime = 0.0;
        _fullscreenTimer = nil;
        _isAnimating = NO;  // Start with animation stopped
        _cachedIsPreview = isPreview;  // Cache isPreview
        
        [self setAnimationTimeInterval:1.0/60.0];  // 60 FPS
        
        // Initialize platform
        _platform = new MacPlatform();
        
        // Load configuration
        _config = new BoingConfig();
        *_config = _platform->LoadConfig();
        
        // Enable sounds immediately for fullscreen instances (not preview)
        // This ensures sounds are ready before startAnimation() is called
        // startAnimation() may be delayed, but sounds can play immediately
        if (!isPreview && _platform) {
            _platform->EnableSounds();
        }
        
        // Setup OpenGL
        [self setupOpenGL];
        
        // Initialize core modules
        [self initializeModules];
        
        // Initialize cached bounds for thread-safe access
        NSRect bounds = [self bounds];
        _cachedBounds = bounds.size;
        
        // Register for macOS screensaver stop notifications
        // Use DistributedNotificationCenter for cross-process notifications (full-screen mode)
        // But only register if NOT in preview mode to avoid issues with System Settings pane
        // See GPTInfo.md for details on macOS screensaver lifecycle quirks
        if (!isPreview) {
            [[NSDistributedNotificationCenter defaultCenter] addObserver:self
                                                                 selector:@selector(screensaverWillStop:)
                                                                     name:@"com.apple.screensaver.willstop"
                                                                   object:nil];
            [[NSDistributedNotificationCenter defaultCenter] addObserver:self
                                                                 selector:@selector(screensaverDidStop:)
                                                                     name:@"com.apple.screensaver.didstop"
                                                                   object:nil];
        }
    }
    return self;
}

- (void)dealloc {
    // Unregister from notifications (only if we registered, which is only in full-screen mode)
    if (![self isPreview]) {
        [[NSDistributedNotificationCenter defaultCenter] removeObserver:self];
    }
    
    // Stop animation and sounds immediately
    _isAnimating = NO;
    
    // Disable and stop all sounds before cleanup
    if (_platform) {
        _platform->DisableSounds();
    }
    
    // Invalidate timer
    if (_fullscreenTimer) {
        [_fullscreenTimer invalidate];
        _fullscreenTimer = nil;
    }
    
    [self cleanupOpenGL];
    
    if (_renderer) {
        delete _renderer;
        _renderer = nullptr;
    }
    if (_physics) {
        delete _physics;
        _physics = nullptr;
    }
    if (_renderConfig) {
        delete _renderConfig;
        _renderConfig = nullptr;
    }
    if (_config) {
        delete _config;
        _config = nullptr;
    }
    if (_platform) {
        delete _platform;
        _platform = nullptr;
    }
    
    [super dealloc];
}

- (void)setupOpenGL {
    // OpenGL pixel format attributes
    NSOpenGLPixelFormatAttribute attrs[] = {
        NSOpenGLPFAAccelerated,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFAMultisample,
        NSOpenGLPFASampleBuffers, 1,
        NSOpenGLPFASamples, 4,
        0
    };
    
    _glPixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    if (!_glPixelFormat) {
        // Fallback without multisampling
        NSOpenGLPixelFormatAttribute fallbackAttrs[] = {
            NSOpenGLPFAAccelerated,
            NSOpenGLPFADoubleBuffer,
            NSOpenGLPFAColorSize, 24,
            NSOpenGLPFADepthSize, 24,
            0
        };
        _glPixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:fallbackAttrs];
    }
    
    _glContext = [[NSOpenGLContext alloc] initWithFormat:_glPixelFormat shareContext:nil];
    [_glContext setView:self];
    [_glContext makeCurrentContext];
    
    // Cache the CGL context for thread-safe access from CVDisplayLink callback
    _cachedCGLContext = [_glContext CGLContextObj];
    
    // Enable vsync
    GLint swapInt = 1;
    [_glContext setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];
}

- (void)initializeModules {
    [_glContext makeCurrentContext];
    
    NSRect bounds = [self bounds];
    
    // Initialize renderer
    _renderer = new BoingRenderer();
    _renderer->Initialize((int)bounds.size.width, (int)bounds.size.height);
    
    // Get world bounds from renderer
    float wallX, wallZ, floorY;
    _renderer->SetViewport((int)bounds.size.width, (int)bounds.size.height, 
                          wallX, wallZ, floorY);
    
    // Initialize physics
    _physics = new BoingPhysics();
    _physics->Initialize(wallX, wallZ, floorY);
    _physics->SetTimeScale(0.5f);  // Half speed for classic look
    
    // Setup render config
    _renderConfig = new RenderConfig();
    _renderConfig->showFloorShadow = _config->enableFloorShadow;
    _renderConfig->showWallShadow = _config->enableWallShadow;
    _renderConfig->showGrid = _config->enableGrid;
    _renderConfig->smoothGeometry = _config->smoothGeometry;
    _config->GetBackgroundColorFloat(
        _renderConfig->backgroundColor[0],
        _renderConfig->backgroundColor[1],
        _renderConfig->backgroundColor[2]
    );
    
    _prevTime = _platform->GetHighResolutionTime();
}

- (void)cleanupOpenGL {
    if (_glContext) {
        [_glContext clearDrawable];
        [_glContext release];
        _glContext = nil;
    }
    if (_glPixelFormat) {
        [_glPixelFormat release];
        _glPixelFormat = nil;
    }
}

- (void)setFrameSize:(NSSize)newSize {
    [super setFrameSize:newSize];
    
    // Update cached bounds
    _cachedBounds = newSize;
    
    // Update viewport for both preview and full-screen modes
    if (_glContext && _renderer) {
        [_glContext makeCurrentContext];
        [_glContext update];
        
        float wallX, wallZ, floorY;
        _renderer->SetViewport((int)newSize.width, (int)newSize.height,
                              wallX, wallZ, floorY);
        
        if (_physics) {
            _physics->Initialize(wallX, wallZ, floorY);
        }
    }
}

- (BOOL)wantsLayer {
    return NO;  // Critical: Don't use layer-backing with OpenGL
}

- (BOOL)isOpaque {
    return YES;
}

- (void)viewWillMoveToWindow:(NSWindow *)newWindow {
    [super viewWillMoveToWindow:newWindow];
    // Cleanup handled by stopAnimation and dealloc
}

- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];
    
    // Set ourselves as window delegate to detect when window resigns key (backup cleanup)
    if (self.window && self.window.delegate != self) {
        self.window.delegate = self;
    }
    
    if (self.window && _glContext) {
        [_glContext makeCurrentContext];
        [_glContext update];  // Critical: update context when view moves to new window/screen
        
        // Update cached bounds
        NSRect bounds = [self bounds];
        _cachedBounds = bounds.size;
        
        // Update viewport and physics when view moves to ensure correct rendering on all screens
        if (_renderer && _physics) {
            float wallX, wallZ, floorY;
            _renderer->SetViewport((int)bounds.size.width, (int)bounds.size.height, wallX, wallZ, floorY);
            _physics->Initialize(wallX, wallZ, floorY);
        }
        
        // For full-screen mode, startAnimation is never called by the system
        // so we need to start our timer here
        if (![self isPreview] && !_fullscreenTimer && _physics && _platform) {
            _prevTime = _platform->GetHighResolutionTime();
            _isAnimating = YES;  // Mark animation as active
            
            _fullscreenTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                                               target:self
                                                             selector:@selector(timerFired:)
                                                             userInfo:nil
                                                              repeats:YES];
        }
    }
}

- (void)removeFromSuperview {
    // Cleanup handled by stopAnimation and dealloc
    [super removeFromSuperview];
}

- (void)startAnimation {
    [super startAnimation];
    
    _isAnimating = YES;  // Mark animation as active
    
    // Reload config when animation starts (respects config setting)
    if (_platform && _config) {
        *_config = _platform->LoadConfig();
    }
    
    // Enable sounds when animation starts (if not in preview)
    // This ensures sounds are enabled when the screensaver starts
    // IMPORTANT: Do this AFTER loading config so config is up-to-date
    // Note: Sounds are also enabled in init for fullscreen instances, but
    // calling EnableSounds() here is idempotent and ensures sounds stay enabled
    if (![self isPreview] && _platform) {
        _platform->EnableSounds();
    }
    
    if (_physics && _platform) {
        _prevTime = _platform->GetHighResolutionTime();
    }
    
    // Ensure context is properly set up
    if (_glContext) {
        [_glContext makeCurrentContext];
        [_glContext update];
    }
    
    // For full-screen mode, we need to explicitly create a timer
    // Preview mode uses ScreenSaverView's automatic animation
    if (![self isPreview] && !_fullscreenTimer) {
        _fullscreenTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                                           target:self
                                                         selector:@selector(timerFired:)
                                                         userInfo:nil
                                                          repeats:YES];
    }
}

- (void)stopAnimation {
    [super stopAnimation];
    
    // Mark animation as stopped to prevent any new sounds
    _isAnimating = NO;
    
    // Disable sounds at platform level
    if (_platform) {
        _platform->DisableSounds();
    }
    
    // Invalidate the timer
    if (_fullscreenTimer) {
        [_fullscreenTimer invalidate];
        _fullscreenTimer = nil;
    }
}

// Handle macOS screensaver stop notifications
// These are called by the system when the screensaver is about to stop
// More reliable than stopAnimation alone - works across processes
- (void)screensaverWillStop:(NSNotification*)notification {
    _isAnimating = NO;
    if (_platform) {
        _platform->DisableSounds();
    }
    if (_fullscreenTimer) {
        [_fullscreenTimer invalidate];
        _fullscreenTimer = nil;
    }
}

- (void)screensaverDidStop:(NSNotification*)notification {
    // Final cleanup - stopAnimation should have already been called
    _isAnimating = NO;
    if (_platform) {
        _platform->DisableSounds();
    }
}

#pragma mark - NSWindowDelegate

// Detect when window resigns key status (e.g., lock screen appears)
// screensaverWillStop should handle this, but this is a backup
- (void)windowDidResignKey:(NSNotification*)notification {
    _isAnimating = NO;
    if (_platform) {
        _platform->DisableSounds();
    }
    if (_fullscreenTimer) {
        [_fullscreenTimer invalidate];
        _fullscreenTimer = nil;
    }
}

- (void)drawRect:(NSRect)rect {
    // Render current frame for both preview and full-screen modes
    // animateOneFrame (called by the timer) handles physics updates
    [self renderFrame];
}

- (void)renderFrame {
    if (!_physics || !_renderer || !_platform || !_glContext) {
        return;
    }
    
    [_glContext makeCurrentContext];
    [_glContext update];  // Ensure context is updated - critical for multi-monitor setups
    
    // Get current bounds - always use bounds (view-relative) not frame (window-relative)
    NSRect bounds = [self bounds];
    
    // Ensure viewport matches current view size
    // For multi-monitor, bounds.origin should be (0,0) but size should match view
    // Update renderer's viewport to ensure projection matrix is correct
    float wallX, wallZ, floorY;
    _renderer->SetViewport((int)bounds.size.width, (int)bounds.size.height, wallX, wallZ, floorY);
    
    // Render
    _renderer->RenderFrame(*_physics, *_renderConfig);
    
    glFlush();  // Force OpenGL to execute commands
    [_glContext flushBuffer];
}

- (NSTimeInterval)animationTimeInterval {
    return 1.0/60.0;
}

- (BOOL)isAnimating {
    return YES;
}

// Timer callback for full-screen animation
- (void)timerFired:(NSTimer*)timer {
    [self animateOneFrame];
}

- (void)animateOneFrame {
    // Check if animation is still active - if not, don't do anything
    if (!_isAnimating) {
        return;
    }
    
    // Check if window still exists and is visible
    NSWindow* window = [self window];
    if (window == nil || ![window isVisible]) {
        // Window is gone or not visible - stop everything
        _isAnimating = NO;
        if (_platform) {
            _platform->DisableSounds();
        }
        if (_fullscreenTimer) {
            [_fullscreenTimer invalidate];
            _fullscreenTimer = nil;
        }
        return;
    }
    
    if (!_physics || !_renderer || !_platform || !_glContext) {
        return;
    }
    
    // Calculate delta time
    double currentTime = _platform->GetHighResolutionTime();
    float dt = (float)(currentTime - _prevTime);
    _prevTime = currentTime;
    
    // Cap delta time
    if (dt > 0.05f) dt = 0.05f;
    
    // Check for collisions before update (for sound)
    bool hadFloorCollision = _physics->DidFloorCollision();
    bool hadWallCollision = _physics->DidWallCollision();
    
    // Update physics only - rendering is handled by drawRect via setNeedsDisplay
    _physics->Update(dt);
    
    // Play sounds for new collisions ONLY if:
    // 1. Animation is still active
    // 2. Sound is enabled in config
    // 3. NOT in preview mode (preview shouldn't play sounds)
    // 4. Window is visible (for full-screen mode, we don't require key/main because
    //    screensaver windows might not always be key/main, especially on startup)
    // This defensive check ensures we don't play sounds if the window isn't visible
    BOOL shouldPlaySound = _isAnimating && _config->enableSound && ![self isPreview] && 
                           [window isVisible];
    
    if (shouldPlaySound) {
        if (_physics->DidFloorCollision() && !hadFloorCollision) {
            _platform->PlaySound(SoundType::FloorBounce);
        }
        if (_physics->DidWallCollision() && !hadWallCollision) {
            _platform->PlaySound(SoundType::WallHit);
        }
    }
    
    // Request redraw - this triggers drawRect on the main thread
    [self setNeedsDisplay:YES];
}

// Called by CVDisplayLink on a separate thread
- (BOOL)hasConfigureSheet {
    return YES;
}

- (NSWindow*)configureSheet {
    if (!_configSheet) {
        // Load the configuration sheet from XIB
        NSBundle* bundle = [NSBundle bundleForClass:[self class]];
        if (![bundle loadNibNamed:@"ConfigSheet" owner:self topLevelObjects:nil]) {
            // If XIB loading fails, create programmatically
            [self createConfigSheetProgrammatically];
        }
    }
    
    if (_configSheet) {
        // Update UI with current settings
        [_floorShadowCheckbox setState:_config->enableFloorShadow ? NSOnState : NSOffState];
        [_wallShadowCheckbox setState:_config->enableWallShadow ? NSOnState : NSOffState];
        [_gridCheckbox setState:_config->enableGrid ? NSOnState : NSOffState];
        [_soundCheckbox setState:_config->enableSound ? NSOnState : NSOffState];
        [_geometryCheckbox setState:_config->smoothGeometry ? NSOffState : NSOnState];
        
        NSColor* bgColor = [NSColor colorWithCalibratedRed:_config->bgColorR/255.0
                                                     green:_config->bgColorG/255.0
                                                      blue:_config->bgColorB/255.0
                                                     alpha:1.0];
        [_colorWell setColor:bgColor];
    }
    
    return _configSheet;
}

- (void)createConfigSheetProgrammatically {
    // Create window - wider to accommodate Cancel button
    _configSheet = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 420, 280)
                                               styleMask:NSWindowStyleMaskTitled
                                                 backing:NSBackingStoreBuffered
                                                   defer:NO];
    [_configSheet setTitle:@"Boing Ball Screensaver Settings"];
    
    NSView* contentView = [_configSheet contentView];
    CGFloat y = 240;
    
    // Floor shadow checkbox
    _floorShadowCheckbox = [[NSButton alloc] initWithFrame:NSMakeRect(20, y, 300, 20)];
    [_floorShadowCheckbox setButtonType:NSButtonTypeSwitch];
    [_floorShadowCheckbox setTitle:@"Enable floor shadow"];
    [contentView addSubview:_floorShadowCheckbox];
    y -= 30;
    
    // Wall shadow checkbox
    _wallShadowCheckbox = [[NSButton alloc] initWithFrame:NSMakeRect(20, y, 300, 20)];
    [_wallShadowCheckbox setButtonType:NSButtonTypeSwitch];
    [_wallShadowCheckbox setTitle:@"Enable wall shadow"];
    [contentView addSubview:_wallShadowCheckbox];
    y -= 30;
    
    // Grid checkbox
    _gridCheckbox = [[NSButton alloc] initWithFrame:NSMakeRect(20, y, 300, 20)];
    [_gridCheckbox setButtonType:NSButtonTypeSwitch];
    [_gridCheckbox setTitle:@"Enable grid"];
    [contentView addSubview:_gridCheckbox];
    y -= 30;
    
    // Sound checkbox
    _soundCheckbox = [[NSButton alloc] initWithFrame:NSMakeRect(20, y, 300, 20)];
    [_soundCheckbox setButtonType:NSButtonTypeSwitch];
    [_soundCheckbox setTitle:@"Enable sound"];
    [contentView addSubview:_soundCheckbox];
    y -= 30;
    
    // Geometry checkbox
    _geometryCheckbox = [[NSButton alloc] initWithFrame:NSMakeRect(20, y, 300, 20)];
    [_geometryCheckbox setButtonType:NSButtonTypeSwitch];
    [_geometryCheckbox setTitle:@"Classic ball geometry (16×8)"];
    [contentView addSubview:_geometryCheckbox];
    y -= 30;
    
    // Color well
    NSTextField* colorLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 150, 20)];
    [colorLabel setStringValue:@"Background color:"];
    [colorLabel setBezeled:NO];
    [colorLabel setDrawsBackground:NO];
    [colorLabel setEditable:NO];
    [colorLabel setSelectable:NO];
    [contentView addSubview:colorLabel];
    
    _colorWell = [[NSColorWell alloc] initWithFrame:NSMakeRect(180, y, 60, 24)];
    [contentView addSubview:_colorWell];
    y -= 40;
    
    // Restore defaults button
    NSButton* restoreButton = [[NSButton alloc] initWithFrame:NSMakeRect(20, 20, 120, 24)];
    [restoreButton setTitle:@"Restore Defaults"];
    [restoreButton setBezelStyle:NSBezelStyleRounded];
    [restoreButton setTarget:self];
    [restoreButton setAction:@selector(restoreDefaults:)];
    [contentView addSubview:restoreButton];
    
    // OK button
    NSButton* okButton = [[NSButton alloc] initWithFrame:NSMakeRect(250, 20, 70, 24)];
    [okButton setTitle:@"OK"];
    [okButton setBezelStyle:NSBezelStyleRounded];
    [okButton setTarget:self];
    [okButton setAction:@selector(closeConfigSheet:)];
    [okButton setKeyEquivalent:@"\r"];
    [contentView addSubview:okButton];
    
    // Cancel button - wider to fit "Cancel" text
    NSButton* cancelButton = [[NSButton alloc] initWithFrame:NSMakeRect(330, 20, 80, 24)];
    [cancelButton setTitle:@"Cancel"];
    [cancelButton setBezelStyle:NSBezelStyleRounded];
    [cancelButton setTarget:self];
    [cancelButton setAction:@selector(closeConfigSheet:)];
    [cancelButton setKeyEquivalent:@"\033"];
    [contentView addSubview:cancelButton];
}

- (IBAction)closeConfigSheet:(id)sender {
    // Safety checks - ensure all required objects exist
    if (!_configSheet || !_config || !_renderConfig || !_platform) {
        // If critical objects are missing, just close the sheet
        if (_configSheet) {
            [[NSApplication sharedApplication] endSheet:_configSheet];
        }
        return;
    }
    
    // Check if OK was clicked
    if ([sender isKindOfClass:[NSButton class]]) {
        NSButton* button = (NSButton*)sender;
        if ([[button title] isEqualToString:@"OK"]) {
            // Safety checks for UI elements
            if (!_floorShadowCheckbox || !_wallShadowCheckbox || !_gridCheckbox || 
                !_soundCheckbox || !_geometryCheckbox || !_colorWell) {
                // UI elements missing - close sheet without saving
                [[NSApplication sharedApplication] endSheet:_configSheet];
                return;
            }
            
            // Save settings
            _config->enableFloorShadow = ([_floorShadowCheckbox state] == NSOnState);
            _config->enableWallShadow = ([_wallShadowCheckbox state] == NSOnState);
            _config->enableGrid = ([_gridCheckbox state] == NSOnState);
            _config->enableSound = ([_soundCheckbox state] == NSOnState);
            _config->smoothGeometry = ([_geometryCheckbox state] == NSOffState);
            
            // Update sound state based on preference
            // If sound is disabled, disable sounds for this instance
            if (!_config->enableSound && _platform) {
                _platform->DisableSounds();
            } else if (_config->enableSound && _platform) {
                // Re-enable sounds if they were disabled and user wants them
                _platform->EnableSounds();
            }
            
            NSColor* color = [_colorWell color];
            if (color) {
                NSColor* rgbColor = [color colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
                if (rgbColor) {
                    _config->bgColorR = (unsigned char)([rgbColor redComponent] * 255.0);
                    _config->bgColorG = (unsigned char)([rgbColor greenComponent] * 255.0);
                    _config->bgColorB = (unsigned char)([rgbColor blueComponent] * 255.0);
                }
            }
            
            // Save to preferences
            _platform->SaveConfig(*_config);
            
            // Update render config
            _renderConfig->showFloorShadow = _config->enableFloorShadow;
            _renderConfig->showWallShadow = _config->enableWallShadow;
            _renderConfig->showGrid = _config->enableGrid;
            _renderConfig->smoothGeometry = _config->smoothGeometry;
            _config->GetBackgroundColorFloat(
                _renderConfig->backgroundColor[0],
                _renderConfig->backgroundColor[1],
                _renderConfig->backgroundColor[2]
            );
        }
    }
    
    [[NSApplication sharedApplication] endSheet:_configSheet];
}

- (IBAction)restoreDefaults:(id)sender {
    _config->RestoreDefaults();
    
    [_floorShadowCheckbox setState:NSOnState];
    [_wallShadowCheckbox setState:NSOnState];
    [_gridCheckbox setState:NSOnState];
    [_soundCheckbox setState:NSOnState];
    [_geometryCheckbox setState:NSOffState];
    
    NSColor* defaultColor = [NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0];
    [_colorWell setColor:defaultColor];
}

@end
