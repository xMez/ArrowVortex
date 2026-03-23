#import <AppKit/AppKit.h>

#include <System/Debug.h>
#include <System/NativeMenuBarMacOS.h>
#include <string>

// Forward declaration — defined in Editor module, accessed via gEditor global.
namespace Vortex {
struct Editor;
extern Editor* gEditor;
}  // namespace Vortex

// We need onMenuAction, declared in Editor.h. Include it indirectly via the
// extern global and a forward-declared method call.
extern "C++" {
namespace Vortex {
void NativeMenuActionDispatch(int actionId);
}
}

// ================================================================================================
// AVMenuTarget — Objective-C class that receives NSMenuItem actions.
// Each NSMenuItem's tag stores the Action::Type int.

@interface AVMenuTarget : NSObject
+ (instancetype)shared;
- (void)menuItemClicked:(id)sender;
@end

@implementation AVMenuTarget

+ (instancetype)shared {
    static AVMenuTarget* instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[AVMenuTarget alloc] init];
    });
    return instance;
}

- (void)menuItemClicked:(id)sender {
    NSMenuItem* item = (NSMenuItem*)sender;
    int actionId = (int)[item tag];
    Vortex::Debug::menuLog("NSMenuItem clicked: tag=%d title='%s'\n",
                           actionId, [[item title] UTF8String]);
    Vortex::NativeMenuActionDispatch(actionId);
}

@end

// ================================================================================================
// C++ functions called from SystemSDL3.cpp via the NativeMenuBarMacOS.h interface.

void* CreateNativeMenu() {
    @autoreleasepool {
        NSMenu* menu = [[NSMenu alloc] initWithTitle:@""];
        [menu setAutoenablesItems:NO];
        Vortex::Debug::menuLog("CreateNativeMenu() -> %p\n", (void*)menu);
        return (__bridge_retained void*)menu;
    }
}

void NativeMenuAddItem(void* menu, int actionId, const char* text) {
    @autoreleasepool {
        NSMenu* nsMenu = (__bridge NSMenu*)menu;
        std::string fullText(text);
        Vortex::Debug::menuLog("  AddItem: menu=%p actionId=%d text='%s'\n",
                               menu, actionId, text);

        // Split on \t to separate label from shortcut display text
        std::string label = fullText;
        std::string shortcut;
        size_t tabPos = fullText.find('\t');
        if (tabPos != std::string::npos) {
            label = fullText.substr(0, tabPos);
            shortcut = fullText.substr(tabPos + 1);
        }

        NSString* nsLabel = [NSString stringWithUTF8String:label.c_str()];
        if (!nsLabel) {
            // Fallback: use lossy Latin1 conversion if UTF-8 fails
            Vortex::Debug::menuLog("  WARNING: UTF-8 conversion failed for label, using fallback\n");
            NSData* data = [NSData dataWithBytes:label.c_str() length:label.size()];
            nsLabel = [[NSString alloc] initWithData:data encoding:NSISOLatin1StringEncoding];
            if (!nsLabel) nsLabel = @"(invalid)";
        }

        NSMenuItem* item = [[NSMenuItem alloc]
            initWithTitle:nsLabel
                   action:@selector(menuItemClicked:)
            keyEquivalent:@""];
        [item setTarget:[AVMenuTarget shared]];
        [item setTag:actionId];

        // If there's shortcut text, show it via attributed string or just in title
        // NSMenu doesn't natively show shortcut text without keyEquivalent,
        // so we embed it in the title for display purposes.
        if (!shortcut.empty()) {
            NSString* nsShortcut = [NSString stringWithUTF8String:shortcut.c_str()];
            if (!nsShortcut) nsShortcut = @"";
            NSString* fullTitle = [NSString stringWithFormat:@"%@\t%@",
                nsLabel, nsShortcut];
            [item setTitle:fullTitle];
        }

        [nsMenu addItem:item];
    }
}

void NativeMenuAddSeparator(void* menu) {
    @autoreleasepool {
        NSMenu* nsMenu = (__bridge NSMenu*)menu;
        Vortex::Debug::menuLog("  AddSeparator: menu=%p\n", menu);
        [nsMenu addItem:[NSMenuItem separatorItem]];
    }
}

void NativeMenuAddSubmenu(void* menu, void* submenu, const char* text, bool grayed) {
    @autoreleasepool {
        NSMenu* nsMenu = (__bridge NSMenu*)menu;
        NSMenu* nsSubmenu = (__bridge NSMenu*)submenu;
        NSString* nsText = [NSString stringWithUTF8String:text];
        if (!nsText) nsText = @"(invalid)";
        Vortex::Debug::menuLog("  AddSubmenu: menu=%p submenu=%p text='%s' grayed=%d\n",
                               menu, submenu, text, grayed);

        [nsSubmenu setTitle:nsText];
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:nsText
                                                      action:nil
                                               keyEquivalent:@""];
        [item setSubmenu:nsSubmenu];
        [item setEnabled:!grayed];
        [nsMenu addItem:item];
    }
}

void NativeMenuReplaceSubmenu(void* menu, int pos, void* submenu, const char* text, bool grayed) {
    @autoreleasepool {
        NSMenu* nsMenu = (__bridge NSMenu*)menu;
        NSMenu* nsSubmenu = (__bridge NSMenu*)submenu;
        NSString* nsText = [NSString stringWithUTF8String:text];
        if (!nsText) nsText = @"(invalid)";
        Vortex::Debug::menuLog("  ReplaceSubmenu: menu=%p pos=%d submenu=%p text='%s' grayed=%d\n",
                               menu, pos, submenu, text, grayed);

        // Remove existing item at position if valid
        if (pos >= 0 && pos < (int)[nsMenu numberOfItems]) {
            [nsMenu removeItemAtIndex:pos];
        }

        [nsSubmenu setTitle:nsText];
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:nsText
                                                      action:nil
                                               keyEquivalent:@""];
        [item setSubmenu:nsSubmenu];
        [item setEnabled:!grayed];

        if (pos >= 0 && pos <= (int)[nsMenu numberOfItems]) {
            [nsMenu insertItem:item atIndex:pos];
        } else {
            [nsMenu addItem:item];
        }
    }
}

// Recursive helper to find and update menu item state by tag (actionId).
static void FindAndSetMenuItem(NSMenu* menu, int actionId, NSNumber* checkedVal, NSNumber* enabledVal) {
    for (NSMenuItem* item in [menu itemArray]) {
        if ([item isSeparatorItem]) continue;
        if ([item hasSubmenu]) {
            FindAndSetMenuItem([item submenu], actionId, checkedVal, enabledVal);
        } else if ([item tag] == actionId) {
            if (checkedVal) {
                [item setState:[checkedVal boolValue] ? NSControlStateValueOn : NSControlStateValueOff];
            }
            if (enabledVal) {
                [item setEnabled:[enabledVal boolValue]];
            }
            return;
        }
    }
}

void NativeMenuSetChecked(void* menu, int actionId, bool checked) {
    @autoreleasepool {
        NSMenu* nsMenu = (__bridge NSMenu*)menu;
        FindAndSetMenuItem(nsMenu, actionId, @(checked), nil);
    }
}

void NativeMenuSetEnabled(void* menu, int actionId, bool enabled) {
    @autoreleasepool {
        NSMenu* nsMenu = (__bridge NSMenu*)menu;
        FindAndSetMenuItem(nsMenu, actionId, nil, @(enabled));
    }
}

void InstallNativeMenuBar(void* rootMenu) {
    @autoreleasepool {
        NSMenu* nsRootMenu = (__bridge NSMenu*)rootMenu;
        Vortex::Debug::menuLog("InstallNativeMenuBar: rootMenu=%p with %ld items\n",
                               rootMenu, (long)[nsRootMenu numberOfItems]);

        // Log what SDL3 set as the current main menu
        NSMenu* existingMenu = [NSApp mainMenu];
        Vortex::Debug::menuLog("  Existing mainMenu=%p with %ld items\n",
                               (void*)existingMenu,
                               existingMenu ? (long)[existingMenu numberOfItems] : 0);

        // Create the proper root menu structure.
        // On macOS, the main menu bar is an NSMenu whose items are submenus.
        // The first item is typically the "application" menu.
        NSMenu* mainMenu = [[NSMenu alloc] initWithTitle:@"MainMenu"];
        [mainMenu setAutoenablesItems:NO];

        // Add standard macOS "application" menu (ArrowVortex)
        Vortex::Debug::menuLog("  Creating application menu\n");
        NSMenu* appMenu = [[NSMenu alloc] initWithTitle:@"ArrowVortex"];
        [appMenu setAutoenablesItems:NO];

        NSMenuItem* aboutItem = [[NSMenuItem alloc]
            initWithTitle:@"About ArrowVortex"
                   action:@selector(menuItemClicked:)
            keyEquivalent:@""];
        [aboutItem setTarget:[AVMenuTarget shared]];
        // Use the ABOUT action tag — Action::ABOUT. We look it up by scanning
        // the root menu's Help submenu for an "About" item tag.
        // For now, hardcode a search through the provided root menu entries.
        int aboutTag = 0;
        for (NSMenuItem* topItem in [nsRootMenu itemArray]) {
            if ([topItem hasSubmenu]) {
                for (NSMenuItem* sub in [[topItem submenu] itemArray]) {
                    if ([[sub title] containsString:@"About"]) {
                        aboutTag = (int)[sub tag];
                        break;
                    }
                }
            }
            if (aboutTag) break;
        }
        [aboutItem setTag:aboutTag];
        [appMenu addItem:aboutItem];
        [appMenu addItem:[NSMenuItem separatorItem]];

        // Quit item
        NSMenuItem* quitItem = [[NSMenuItem alloc]
            initWithTitle:@"Quit ArrowVortex"
                   action:@selector(menuItemClicked:)
            keyEquivalent:@"q"];
        [quitItem setTarget:[AVMenuTarget shared]];
        // Find EXIT_PROGRAM tag from File menu
        int exitTag = 0;
        for (NSMenuItem* topItem in [nsRootMenu itemArray]) {
            if ([topItem hasSubmenu]) {
                for (NSMenuItem* sub in [[topItem submenu] itemArray]) {
                    NSString* title = [sub title];
                    if ([title containsString:@"Exit"] || [title containsString:@"Quit"]) {
                        exitTag = (int)[sub tag];
                        break;
                    }
                }
            }
            if (exitTag) break;
        }
        [quitItem setTag:exitTag];
        [quitItem setKeyEquivalentModifierMask:NSEventModifierFlagCommand];
        [appMenu addItem:quitItem];

        NSMenuItem* appMenuItem = [[NSMenuItem alloc] initWithTitle:@"ArrowVortex"
                                                             action:nil
                                                      keyEquivalent:@""];
        [appMenuItem setSubmenu:appMenu];
        [mainMenu addItem:appMenuItem];
        Vortex::Debug::menuLog("  Application menu added (aboutTag=%d, exitTag=%d)\n",
                               aboutTag, exitTag);

        // Add all the menus from the root menu as top-level items
        // We need to move items from nsRootMenu to mainMenu since NSMenuItem
        // can only belong to one menu at a time.
        while ([nsRootMenu numberOfItems] > 0) {
            NSMenuItem* item = [[nsRootMenu itemAtIndex:0] retain];
            [nsRootMenu removeItemAtIndex:0];
            [mainMenu addItem:item];
            [item release];
            Vortex::Debug::menuLog("  Moved top-level menu: '%s'\n",
                                   [[item title] UTF8String]);
        }

        Vortex::Debug::menuLog("  Setting mainMenu with %ld items\n",
                               (long)[mainMenu numberOfItems]);
        [NSApp setMainMenu:mainMenu];
        Vortex::Debug::menuLog("InstallNativeMenuBar: done\n");
    }
}
