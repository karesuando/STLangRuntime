#pragma once

#include <set>
#include <map>
#include <string>
#include "STLangRuntime.h"

#define STACK_SIZE	64

using namespace std;
class STLangPOUObject;
class STLangSubPOUObject;
typedef map<string, int> VariableIndexMap;
typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef unsigned short USHORT;
typedef __int64 int64;

// class STLangPOUObject
//
// Represents a function or function block (POU).
//
class STLangPOUObject
{
public:
	virtual ~STLangPOUObject();
	STLangPOUObject(const string&, int, BYTE[], BYTE[], int[], BYTE[], char*, short*, int*,
					float*, int64*, double*, char**, wchar_t**, const int*, const float*,
					const int64*, const double*, const char**, const wchar_t**, double, 
					double, double, double, int, int, const set<string>&, const set<string>&, 
					const set<string>&, const VariableIndexMap&, STLangSubPOUObject** = 0, int = 0);

	string Name() const;

	int InputCount() const;

	int OutputCount() const;

	int POUType() const;

	double Execute(const double[], int);

	const set<string>& RetainedNames() const;

	const set<string>& InputNames() const;

	const set<string>& OutputNames() const;

	double GetOutput(const string& Name) const;

	double GetRetainedValue(const string& Name) const;

	void SetRetainedValue(const string& Name, double Value);

	friend int CreatePOUObject(char* ExeFile);

protected:
	const string m_POUName;
	const int m_POUType;
	const BYTE* const m_OpCode;
	const BYTE* const m_RODataSegment;
	const int* const m_Operand;
	BYTE* const m_RWDataSegment;             // Pointer to beginning of RW data segement
	char* const m_ByteVar;                   // Frame pointer for 8-bit integer variables
	short* const m_ShortVar;                 // Frame pointer for 16-bit integer variables
	int* const m_IntVar;                     // Frame pointer for 32-bit integer variables
	float* const m_FloatVar;                 // Frame pointer for float variables
	double* const m_DoubleVar;               // Frame pointer for double variables            
	int64* const m_LongVar;                  // Frame pointer for 64-bit integer variables
	char** const m_StringVar;                // Frame pointer for ascii-string variables
	wchar_t** const m_WStringVar;            // Frame pointer for unicode-string variables
	const int64* const m_LongConst;
	const double* const m_DoubleConst;
	const float* const m_FloatConst;
	const int* const m_IntConst;
	const char** const m_StringConst;
	const wchar_t** const m_WStringConst;
	const double m_DoubleConst0;
	const double m_DoubleConst1;
	const double m_DoubleConst2;
	const double m_DoubleConst3;
	const int m_ExternCalls;
	const int m_InputCount;
	const int m_OutputCount;
	set<string> m_InputNames;
	set<string> m_OutputNames;
	set<string> m_RetainedNames;
	VariableIndexMap m_VariableIndexMap;
	STLangSubPOUObject** const m_ExternProgOrgUnit;
	bool m_Executed;

private:
	int* IntOperand;
	float* FloatOperand;
	double* DoubleOperand;
    int64* LongOperand;
	char** StringOperand;
	wchar_t** WStringOperand;
};

// class STLangSubPOUObject
//
// Represents a POU called from within another POU.
//
class STLangSubPOUObject : public STLangPOUObject
{
public:
	STLangSubPOUObject(const string&, int, BYTE[], BYTE[], int[], BYTE[], char*, short*, int*,
					   float*, int64*, double*, char**, wchar_t**, const int*, const float*,
					   const int64*, const double*, const char**, const wchar_t**, double, 
					   double, double, double, int, int, const set<string>&, const set<string>&, 
					   const set<string>&, const VariableIndexMap&, STLangSubPOUObject**, int, 
					   int*&, float*&, double*&, int64*&, char**&, wchar_t**&);
	int Execute();

private:
	int*& IntOperand;
	float*& FloatOperand;
	double*& DoubleOperand;
    int64*& LongOperand;
	char**& StringOperand;
	wchar_t**& WStringOperand;
};

inline string STLangPOUObject::Name() const
{
	return m_POUName;
}

inline int STLangPOUObject::POUType() const
{
	return m_POUType;
}

inline int STLangPOUObject::InputCount() const
{
	return m_InputCount;
}

inline int STLangPOUObject::OutputCount() const
{
	return m_OutputCount;
}

inline const set<string>& STLangPOUObject::InputNames() const
{
	return m_InputNames;
}

inline const set<string>& STLangPOUObject::OutputNames() const
{
	return m_OutputNames;
}

inline const set<string>& STLangPOUObject::RetainedNames() const
{
	return m_RetainedNames;
}

inline double STLangPOUObject::GetOutput(const string& Output) const
{
	VariableIndexMap::const_iterator It = m_VariableIndexMap.find(Output);

	if (It == m_VariableIndexMap.end())
		return -99.0;
	else
		return m_DoubleVar[It->second];
}

inline void STLangPOUObject::SetRetainedValue(const string& Name, double Value)
{
	VariableIndexMap::iterator It = m_VariableIndexMap.find(Name);
	if (It != m_VariableIndexMap.end())
		m_DoubleVar[It->second] = Value;
}

inline double STLangPOUObject::GetRetainedValue(const string& Name) const
{
	VariableIndexMap::const_iterator It = m_VariableIndexMap.find(Name);

	if (It == m_VariableIndexMap.end())
		return -99.0;
	else
		return m_DoubleVar[It->second];
}