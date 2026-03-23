#pragma once

// C++ declarations for the native macOS menu bar backend.
// Implementation in NativeMenuBarMacOS.mm (Objective-C++).

/// Creates a new native NSMenu, returns opaque pointer.
void* CreateNativeMenu();

/// Adds a menu item with the given action ID and text.
/// Text may contain '\t' to separate label from shortcut hint.
void NativeMenuAddItem(void* menu, int actionId, const char* text);

/// Adds a separator line to the menu.
void NativeMenuAddSeparator(void* menu);

/// Adds a submenu with the given label.
void NativeMenuAddSubmenu(void* menu, void* submenu, const char* text,
                          bool grayed);

/// Replaces the submenu at the given position.
void NativeMenuReplaceSubmenu(void* menu, int pos, void* submenu,
                              const char* text, bool grayed);

/// Recursively finds the item with the given action ID and sets its checked
/// state.
void NativeMenuSetChecked(void* menu, int actionId, bool checked);

/// Recursively finds the item with the given action ID and sets its enabled
/// state.
void NativeMenuSetEnabled(void* menu, int actionId, bool enabled);

/// Installs the root menu as the application's main menu bar.
void InstallNativeMenuBar(void* rootMenu);
