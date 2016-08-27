#pragma once

#if defined(EVENTUAL_HEADER_ONLY)
#define EVENTUAL_API inline
#elif defined(EVENTUAL_EXPORT)
#define EVENTUAL_API __declspec(dllexport)
#elif defined(EVENTUAL_IMPORT)
#define EVENTUAL_API __declspec(dllimport)
#else
#define EVENTUAL_API
#endif
