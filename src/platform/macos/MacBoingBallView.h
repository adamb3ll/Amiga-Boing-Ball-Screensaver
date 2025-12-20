// MacBoingBallView.h — macOS ScreenSaver view implementation

#import <ScreenSaver/ScreenSaver.h>
#import <OpenGL/gl.h>
#import <OpenGL/glu.h>

class BoingPhysics;
class BoingRenderer;
class MacPlatform;
struct BoingConfig;
struct RenderConfig;

@interface MacBoingBallView : ScreenSaverView <NSWindowDelegate> {
@private
    BoingPhysics* _physics;
    BoingRenderer* _renderer;
    MacPlatform* _platform;
    BoingConfig* _config;
    RenderConfig* _renderConfig;
    
    NSOpenGLContext* _glContext;
    NSOpenGLPixelFormat* _glPixelFormat;
    
    double _prevTime;
    NSTimer* _fullscreenTimer;  // Timer for full-screen animation
    BOOL _isAnimating;  // Track if animation is active (prevents sounds after stop)
    
    // Cached values for rendering
    NSSize _cachedBounds;
    BOOL _cachedIsPreview;  // Cached isPreview state
    CGLContextObj _cachedCGLContext;  // Cached CGL context for cleanup
    
    // Configuration sheet
    IBOutlet NSWindow* _configSheet;
    IBOutlet NSButton* _floorShadowCheckbox;
    IBOutlet NSButton* _wallShadowCheckbox;
    IBOutlet NSButton* _gridCheckbox;
    IBOutlet NSButton* _soundCheckbox;
    IBOutlet NSButton* _geometryCheckbox;
    IBOutlet NSColorWell* _colorWell;
}

- (IBAction)closeConfigSheet:(id)sender;
- (IBAction)restoreDefaults:(id)sender;

@end
