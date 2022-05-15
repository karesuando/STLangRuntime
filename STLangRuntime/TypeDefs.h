#pragma once

typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef unsigned short USHORT;
typedef __int64 int64;
#ifdef _WIN32
   #define sprintfs sprintf_s
#elif __linux__
   #define sprintfs snprintf
#endif