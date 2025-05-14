// i18n.h
//

#ifndef I18N_H
#define I18N_H

#include <iostream>
#include <locale.h>
#include <libintl.h>
#include <mutex>

void ensure_gettext_initialized();
const char* _translate(const char* msg);

#ifndef _
#define _(STRING) _translate(STRING)
#endif
#endif