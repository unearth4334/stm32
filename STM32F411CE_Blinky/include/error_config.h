#ifndef ERROR_CONFIG_H
#define ERROR_CONFIG_H

#include "project_config.h"

#if PROJECT_USE_ASSERT
void Error_Handler(void);
#define PROJECT_ASSERT(cond)                 do { if (!(cond)) { Error_Handler(); } } while (0)
#else
#define PROJECT_ASSERT(cond)                 do { (void)(cond); } while (0)
#endif

#endif
