#include "I18n.h"
std::mutex gettext_mutex;
bool gettext_initialized = false;

void ensure_gettext_initialized() {
    std::lock_guard<std::mutex> lock(gettext_mutex);
    if (!gettext_initialized) {
        setlocale(LC_ALL, "");
        bindtextdomain("gmsh", "./locale");
        textdomain("gmsh");
        gettext_initialized = true;
    }
}

const char* _translate(const char* msg) {
    ensure_gettext_initialized();
    return gettext(msg);
}