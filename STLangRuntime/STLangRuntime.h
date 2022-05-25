#pragma once

#ifdef STLANGRUNTIME_EXPORTS
#define STLANGRUNTIME_API __declspec(dllexport)
#else
#define STLANGRUNTIME_API __declspec(dllimport)
#endif

extern "C" STLANGRUNTIME_API long CreatePOUObject(char*);

extern "C" STLANGRUNTIME_API int GetInPutCount(long POUHandle);

extern "C" STLANGRUNTIME_API int GetOutPutCount(long POUHandle);

extern "C" STLANGRUNTIME_API int GetPOUType(long POUHandle);

extern "C" STLANGRUNTIME_API bool GetPOUName(char* Name, long POUHandle);

extern "C" STLANGRUNTIME_API int GetInputNames(char** Inputs, long POUHandle);

extern "C" STLANGRUNTIME_API int GetOutputNames(char** Outputs, long POUHandle);

extern "C" STLANGRUNTIME_API int GetRetainedNames(char** RetainedVars, long POUHandle);

extern "C" STLANGRUNTIME_API double GetOutputValue(char* Output, long POUHandle);

extern "C" STLANGRUNTIME_API double GetRetainedValue(char* RetainedVar, long POUHandle);

extern "C" STLANGRUNTIME_API void SetRetainedValue(char* RetainedVar, double Value, long POUHandle);

extern "C" STLANGRUNTIME_API double ExecutePOU(double ArgV[], int ArgC, long POUHandle);