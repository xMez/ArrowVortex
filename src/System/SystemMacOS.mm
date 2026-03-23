#import <AppKit/AppKit.h>

// Called from System.cpp to ensure the app is recognized as a foreground
// GUI application. Required for unbundled executables on macOS (no .app bundle
// / no Info.plist).
extern "C" void ActivateMacOSApp() {
    @autoreleasepool {
        if (!NSApp) {
            [NSApplication sharedApplication];
        }
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        if (@available(macOS 14.0, *)) {
            [NSApp activate];
        } else {
            [NSApp activateIgnoringOtherApps:YES];
        }
    }
}
