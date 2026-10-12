#pragma once
#include <QString>

namespace SysEnvHelper {
    
    QString system_locale();
    
    int system_acp();
    
    bool is_china_locale();
}
