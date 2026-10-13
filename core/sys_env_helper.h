/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#pragma once
#include <QString>

namespace SysEnvHelper {
    
    QString system_locale();
    
    int system_acp();
    
    bool is_china_locale();
}
