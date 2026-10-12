#include "sys_env_helper.h"
#include <windows.h>
#include <QString>

namespace SysEnvHelper {

QString system_locale() {
    WCHAR buf[LOCALE_NAME_MAX_LENGTH] = {0};
    if (GetUserDefaultLocaleName(buf, LOCALE_NAME_MAX_LENGTH) > 0) {
        return QString::fromWCharArray(buf);
    }
    return QStringLiteral("en-US");
}

int system_acp() {
    return GetACP();
}

bool is_china_locale() {
    QString locale = system_locale();
    int acp = system_acp();
    return (locale.compare("zh-CN", Qt::CaseInsensitive) == 0 ||
            locale.startsWith("zh", Qt::CaseInsensitive)) && acp == 936;
}

} // namespace SysEnvHelper
