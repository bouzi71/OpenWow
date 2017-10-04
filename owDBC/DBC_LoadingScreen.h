#pragma once

#include "DBCFile.h"

DBC_DEF_BEGIN(DBC_LoadingScreen)

__DBC_STRING(Name, 2);
__DBC_STRING(FileName, 3);
__DBC_TVALUE(uint32_t, HasWideScreen, 4);

DBC_DEF_END
