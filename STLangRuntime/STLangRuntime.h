#pragma once

#ifdef STLANGRUNTIME_EXPORTS
#define STLANGRUNTIME_API __declspec(dllexport)
#else
#define STLANGRUNTIME_API __declspec(dllimport)
#endif

extern "C" STLANGRUNTIME_API int CreatePOUObject(char*);

extern "C" STLANGRUNTIME_API int GetInPutCount(int POUHandle);

extern "C" STLANGRUNTIME_API int GetOutPutCount(int POUHandle);

extern "C" STLANGRUNTIME_API int GetPOUType(int POUHandle);

extern "C" STLANGRUNTIME_API bool GetPOUName(char* Name, int POUHandle);

extern "C" STLANGRUNTIME_API int GetInputNames(char** Inputs, int POUHandle);

extern "C" STLANGRUNTIME_API int GetOutputNames(char** Outputs, int POUHandle);

extern "C" STLANGRUNTIME_API int GetRetainedNames(char** RetainedVars, int POUHandle);

extern "C" STLANGRUNTIME_API double GetOutputValue(char* Output, int POUHandle);

extern "C" STLANGRUNTIME_API double GetRetainedValue(char* RetainedVar, int POUHandle);

extern "C" STLANGRUNTIME_API void SetRetainedValue(char* RetainedVar, double Value, int POUHandle);

extern "C" STLANGRUNTIME_API double ExecutePOU(double ArgV[], int ArgC, int POUHandle);