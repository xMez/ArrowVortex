#import <AppKit/AppKit.h>
#include <unistd.h>

// Called from System.cpp to activate the application and prepare bundle
// resources when launched from Finder.
extern "C" void ActivateMacOSApp() {
    @autoreleasepool {
        NSBundle* bundle = [NSBundle mainBundle];
        NSString* bundlePath = [bundle bundlePath];
        if (bundlePath &&
            [[bundlePath pathExtension] caseInsensitiveCompare:@"app"] ==
                NSOrderedSame) {
            const char* resourcePath =
                [[bundle resourcePath] fileSystemRepresentation];
            if (resourcePath) chdir(resourcePath);
        }

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
