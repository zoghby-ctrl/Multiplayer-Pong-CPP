#include "KeyMapper.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __linux__
#include <X11/keysym.h>
#endif

Key mapKey(int platformKey) {

#ifdef _WIN32
    if (platformKey == VK_F3) return Key::F3;
    if (platformKey == VK_ESCAPE) return Key::ESC;
#endif

#ifdef __linux__
    if (platformKey == XK_F3) return Key::F3;
    if (platformKey == XK_Escape) return Key::ESC;
#endif

    return Key::UNKNOWN;
}