#include "pch.h"
#include "STLangPOUObject.h"
#include <math.h>
#include <ctime>
#include <string.h>
#include <memory.h>
#include "STLangExceptions.h"
#include "STLangStandardLib.h"
#include "STLangVMInstructionSet.h"
#include "STLangObjectFileDefines.h"


static int64 Int64Pow (
	int64 a, 
	int64 e)
{
	if (e < 0)
		return 0;
	else {
		int64 Base,Res;

		Res  = 1;
		Base = a;
		while (e > 0)
		{
			if (e & 1)
				Res *= Base;
			Base *= Base;
			e   >>= 1;
		}
		return Res;
	}
}

static int Int32Pow (
	int a, 
	int e)
{
	if (e < 0)
		return 0;
	else {
		int Base,Res;

		Res  = 1;
		Base = a;
		while (e > 0)
		{
			if (e & 1)
				Res *= Base;
			Base *= Base;
			e   >>= 1;
		}
		return Res;
	}
}

template <class ElemType>
static int GetJumpTableOffset(const SwitchInfo* Switch, int64 IntTemp)
{
	int64 Diff;
	int Low,Mid,High;
	
	const TableEntry<ElemType>* const SearchTable = (TableEntry<ElemType> *)((BYTE *)Switch + sizeof(SwitchInfo));
	Low  = 0;
	Diff = 0;
	Mid = 0;
	High = Switch->TableSize;

	// Sök efter värdet 'CaseVal' i tabellen med binärsökning.
	while (Low <= High)
	{
		Mid  = (Low + High) >> 1;
		Diff = SearchTable[Mid].Value - IntTemp;
		if (Diff > 0)
			High = Mid - 1;
		else if (Diff < 0)
			Low = Mid + 1;
		else
			break;
	}
	if (Diff == 0)
		return SearchTable[Mid].Offset;
	else
		return Switch->DefaultOffset;
}

STLangSubPOUObject::STLangSubPOUObject (
	const string& POUName,
	int POUType,
	BYTE OpCode[],
	BYTE ROData[],
	int Operands[],
	BYTE RWDataSegment[],
	char* ByteVar,
	short* ShortVar,
	int* IntVar,
	float* FloatVar,
	int64* LongVar,
	double* DoubleVar,
	char** StringVar,
	wchar_t** WStringVar,
	const int* IntConst,
	const float* FloatConst,
	const int64* LongConst,
	const double* DoubleConst,
	const char** StringConst,
	const wchar_t** WStringConst,
	double D0,
	double D1,
	double D2,
	double D3,
	int InputCount,
	int OutputCount,
	const set<string>& Inputs,
	const set<string>& Outputs,
	const set<string>& Retained,
	const VariableIndexMap& IndexMap,
	STLangSubPOUObject** Externals,
	int ExternCount,
	int*& IntStack, 
	float*& FloatStack, 
	double*& DoubleStack, 
	int64*& LongStack, 
	char**& StringStack, 
	wchar_t**& WStringStack) : STLangPOUObject(POUName, POUType, OpCode, ROData, Operands, RWDataSegment, 
							                   ByteVar, ShortVar, IntVar, FloatVar, LongVar, DoubleVar,
							                   StringVar, WStringVar, IntConst, FloatConst, LongConst, 
							                   DoubleConst, StringConst, WStringConst, D0, D1, D2, D3,
							                   InputCount, OutputCount, Inputs, Outputs, Retained,
							                   IndexMap, Externals, ExternCount), IntOperand(IntStack),
											   FloatOperand(FloatStack), DoubleOperand(DoubleStack),
											   LongOperand(LongStack), StringOperand(StringStack), WStringOperand(WStringStack)
{
}

STLangPOUObject::STLangPOUObject (
	const string& POUName,
	int POUType,
	BYTE OpCode[],
	BYTE Data[],
	int Operand[],
	BYTE Memory[],
	char* ByteVar,
	short* ShortVar,
	int* IntVar,
	float* FloatVar,
	int64* LongVar,
	double* DoubleVar,
	char** StringVar,
	wchar_t** WStringVar,
	const int* IntConst,
	const float* FloatConst,
	const int64* LongConst,
	const double* DoubleConst,
	const char** StrConst,
	const wchar_t** WStrConst,
	double D0,
	double D1,
	double D2,
	double D3,
	int InputCount,
	int OutputCount,
	const set<string>& Inputs,
	const set<string>& Outputs,
	const set<string>& RetainedVars,
	const VariableIndexMap& IndexMap,
	STLangSubPOUObject** Externals,
	int ExternCount) : m_POUName(POUName), m_POUType(POUType), m_OpCode(OpCode), m_RODataSegment(Data), 
	                   m_Operand(Operand), m_RWDataSegment(Memory), m_ByteVar(ByteVar), m_ShortVar(ShortVar), 
					   m_IntVar(IntVar), m_FloatVar(FloatVar), m_DoubleVar(DoubleVar),
					   m_LongVar(LongVar), m_StringVar(StringVar), m_WStringVar(WStringVar),
					   m_LongConst(LongConst), m_DoubleConst(DoubleConst), m_FloatConst(FloatConst), 
					   m_IntConst(IntConst), m_StringConst(StrConst), m_WStringConst(WStrConst),
					   m_DoubleConst0(D0), m_DoubleConst1(D1), m_DoubleConst2(D2), m_DoubleConst3(D3),
					   m_ExternCalls(ExternCount), m_InputCount(InputCount), m_OutputCount(OutputCount), 
					   m_InputNames(Inputs), m_OutputNames(Outputs), m_RetainedNames(RetainedVars), 
					   m_VariableIndexMap(IndexMap), m_ExternProgOrgUnit(Externals), m_Executed(false)
{
}


STLangPOUObject::~STLangPOUObject(void)
{
	m_InputNames.clear();
	m_OutputNames.clear();
	_aligned_free(m_RWDataSegment);
	for (int i = 0; i < m_ExternCalls; i++)
		delete m_ExternProgOrgUnit[i];
	delete[] m_ExternProgOrgUnit;
}

double STLangPOUObject::Execute (
	const double ArgV[],
	int Length)
{
	// Temporary values
	int IntTemp;
	float FloatTemp;
	double DoubleTemp;
	int64 LongTemp;

	// Register variables;
	char b0,b1,b2,b3;
	short s0,s1,s2,s3;
	int i0,i1,i2,i3;
	float f0,f1,f2,f3;
	double d0,d1,d2,d3;
	int64 li0,li1,li2,li3;

	// Operand stacks
	int IntStack[STACK_SIZE] = { 0 };
	float FloatStack[STACK_SIZE] = { 0 };
	int64 LongStack[STACK_SIZE] = { 0 };
	double DoubleStack[STACK_SIZE] = { 0 };
	char* StringStack[STACK_SIZE] = { 0 };
	wchar_t* WStringStack[STACK_SIZE] = { 0 };

	// Initialize stack pointers
	IntOperand = IntStack - 1;
	FloatOperand = FloatStack - 1;
	DoubleOperand = DoubleStack - 1;
    LongOperand = LongStack - 1;
	StringOperand = StringStack - 1;
	WStringOperand = WStringStack - 1;
	b0 = b1 = b2 = b3 = 0;
	s0 = s1 = s2 = s3 = 0;
	i0 = i1 = i2 = i3 = 0;
	f0 = f1 = f2 = f3 = 0;
	d0 = d1 = d2 = d3 = 0;
	li0 = li1 = li2 = li3 = 0;
	// Program Counter (Instruction Pointer)
	register int pc = -1;

	// Push (double) arguments on stack.
	for (int i = Length; i > 0; i--)
		*++DoubleOperand = *ArgV++;
	while (true)
	{
		pc++;
decode:	switch (m_OpCode[pc])
		{
        case VirtualMachineInstr::IADD:
			IntTemp = *IntOperand--;
            *IntOperand += IntTemp;
            continue;

        case VirtualMachineInstr::LADD:
			LongTemp = *LongOperand--;
            *LongOperand += LongTemp;
            continue;

        case VirtualMachineInstr::FADD:
			FloatTemp = *FloatOperand--;
            *FloatOperand += FloatTemp;
            continue;

        case VirtualMachineInstr::DADD:
			DoubleTemp = *DoubleOperand--;
            *DoubleOperand += DoubleTemp;
            continue;

        case VirtualMachineInstr::ISUB:
			IntTemp = *IntOperand--;
            *IntOperand -= IntTemp;
            continue;

        case VirtualMachineInstr::LSUB:
			LongTemp = *LongOperand--;
            *LongOperand -= LongTemp;
            continue;

        case VirtualMachineInstr::FSUB:
			FloatTemp = *FloatOperand--;
            *FloatOperand -= FloatTemp;
            continue;

        case VirtualMachineInstr::DSUB:
			DoubleTemp = *DoubleOperand--;
            *DoubleOperand -= DoubleTemp;
            continue;

        case VirtualMachineInstr::IMUL:
			IntTemp = *IntOperand--;
            *IntOperand *= IntTemp;
            continue;

        case VirtualMachineInstr::LMUL:
			LongTemp = *LongOperand--;
            *LongOperand *= LongTemp;
            continue;

        case VirtualMachineInstr::FMUL:
			FloatTemp = *FloatOperand--;
            *FloatOperand *= FloatTemp;
            continue;

        case VirtualMachineInstr::DMUL:
			DoubleTemp = *DoubleOperand--;
            *DoubleOperand *= DoubleTemp;
            continue;

	    case VirtualMachineInstr::IDIV:
            IntTemp = *IntOperand--;
            if (IntTemp != 0)
                *IntOperand /= IntTemp;
		    else 
#if THROW_RUNTIME_EXCEPTION
				throw new STRuntimeException(0, "IDIV", pc);
#else 
                *IntOperand = 0;
#endif
		    continue;

		case VirtualMachineInstr::LDIV:
            LongTemp = *LongOperand--;
            if (LongTemp != 0)
                *LongOperand /= LongTemp;
			else  
#if THROW_RUNTIME_EXCEPTION
				throw new STRuntimeException(0, "LDIV", pc);
#else 
                *LongOperand = 0;
#endif
			continue;

	    case VirtualMachineInstr::FDIV:
            FloatTemp = *FloatOperand--;
            if (FloatTemp != 0.0)
                *FloatOperand /= FloatTemp;
		    else
#if THROW_RUNTIME_EXCEPTION
				throw new STRuntimeException(0, "FDIV", pc);
#else 
                *FloatOperand = 0.0;
#endif
		    continue;

        case VirtualMachineInstr::DDIV:
            DoubleTemp = *DoubleOperand--;
            if (DoubleTemp != 0.0)
                *DoubleOperand /= DoubleTemp;
	        else 
#if THROW_RUNTIME_EXCEPTION
			    throw new STRuntimeException(0, "DDIV", pc);
#else 
                *DoubleOperand = 0.0;
#endif
	        continue;

		case VirtualMachineInstr::IMOD:  // Integer modulus
            IntTemp = *IntOperand--;
            if (IntTemp != 0)
                *IntOperand %= IntTemp;
			else 
#if THROW_RUNTIME_EXCEPTION
				throw new STRuntimeException(0, "IMOD", pc);
#else 
                *IntOperand = 0;
#endif
			continue;

		case VirtualMachineInstr::LMOD:  // Integer modulus
            LongTemp = *LongOperand--;
            if (LongTemp != 0)
                *LongOperand %= LongTemp;
			else 
#if THROW_RUNTIME_EXCEPTION
				throw new STRuntimeException(0, "LMOD", pc);
#else 
                *LongOperand = 0;
#endif
			continue;

		case VirtualMachineInstr::CALL:
			{
				//
				// Standard library function call
				//
				BYTE StdLibFunction = (BYTE)m_Operand[pc];

				switch (StdLibFunction)
				{
				case StandardLibrary::IMIN:
				{
					int Count    = *IntOperand--; // Antal parametrar
					int MinInt32 = *IntOperand--;
					while (--Count > 0)
					{
						IntTemp = *IntOperand--;
						if (IntTemp < MinInt32)
							MinInt32 = IntTemp;
					}
					*++IntOperand = MinInt32;
				}
				continue;

				case StandardLibrary::LMIN:
				{
					int Count      = *IntOperand--; // Antal parametrar
					int64 MinInt64 = *LongOperand--;
					while (--Count > 0)
					{
						LongTemp = *LongOperand--;
						if (LongTemp < MinInt64)
							MinInt64 = LongTemp;
					}
					*++LongOperand = MinInt64;
				}
				continue;

				case StandardLibrary::FMIN: 
				{
					IntTemp    = *IntOperand--;   // Antal parametrar
					float FMin = *FloatOperand--;
					while (--IntTemp > 0)
					{
						FloatTemp = *FloatOperand--;
						if (FloatTemp < FMin)
							FMin = FloatTemp;
					}
					*++FloatOperand = FMin;
				}
				continue;

				case StandardLibrary::DMIN:
				{
					IntTemp     = *IntOperand--;   // Antal parametrar
					double DMin = *DoubleOperand--;
					while (--IntTemp > 0)
					{
						DoubleTemp = *DoubleOperand--;
						if (DoubleTemp < DMin)
							DMin = DoubleTemp;
					}
					*++DoubleOperand = DMin;
				}
				continue;

				case StandardLibrary::IMAX:
				{
					int Count    = *IntOperand--; // Antal parametrar
					int MaxInt32 = *IntOperand--;
					while (--Count > 0)
					{
						IntTemp = *IntOperand--;
						if (IntTemp > MaxInt32)
							MaxInt32 = IntTemp;
					}
					*++IntOperand = MaxInt32;
				}
				continue;

				case StandardLibrary::LMAX:
				{
					int Count      = *IntOperand--; // Antal parametrar
					int64 MaxInt64 = *LongOperand--;
					while (--Count > 0)
					{
						LongTemp = *LongOperand--;
						if (LongTemp > MaxInt64)
							MaxInt64 = LongTemp;
					}
					*++LongOperand = MaxInt64;
				}
				continue;

				case StandardLibrary::FMAX: 
				{
					int Count  = *IntOperand--;   // Antal parametrar
					float FMax = *FloatOperand--;
					while (--Count > 0)
					{
						FloatTemp = *FloatOperand--;
						if (FloatTemp > FMax)
							FMax = FloatTemp;
					}
					*++FloatOperand = FMax;
				}
				continue;

				case StandardLibrary::DMAX: 
				{
					int Count   = *IntOperand--;   // Antal parametrar
					double DMax = *DoubleOperand--;
					while (--Count > 0)
					{
						DoubleTemp = *DoubleOperand--;
						if (DoubleTemp > DMax)
							DMax = DoubleTemp;
					}
					*++DoubleOperand = DMax;
				}
				continue;

				case StandardLibrary::IABS: 
					if (*IntOperand < 0)
						*IntOperand = -*IntOperand;
					continue;

				case StandardLibrary::LABS:
					if (*LongOperand < 0)
						*LongOperand = -*LongOperand;
					continue;

				case StandardLibrary::FABS: 
					if (*FloatOperand < 0.0f)
						*FloatOperand = -*FloatOperand;
					continue;

				case StandardLibrary::DABS:
					if (*DoubleOperand < 0.0)
						*DoubleOperand = -*DoubleOperand;
					continue;

				case StandardLibrary::FSIN: 
					*FloatOperand = (float)sin(*FloatOperand);
					continue; 

				case StandardLibrary::FCOS: 
					*FloatOperand = (float)cos(*FloatOperand);
					continue; 

				case StandardLibrary::FTAN: 
					*FloatOperand = (float)tan(*FloatOperand);
					continue; 

				case StandardLibrary::FLOGN: 
					*FloatOperand = (float)log(*FloatOperand);
					continue;

				case StandardLibrary::FLOG10: 
					*FloatOperand = (float)log10(*FloatOperand);
					continue;

				case StandardLibrary::FEXP: 
					*FloatOperand = (float)exp(*FloatOperand);
					continue;
			
				case StandardLibrary::FASIN: 
					*FloatOperand = (float)asin(*FloatOperand);
					continue;

				case StandardLibrary::FACOS: 
					*FloatOperand = (float)acos(*FloatOperand);
					continue;

				case StandardLibrary::FATAN: 
					*FloatOperand = (float)atan(*FloatOperand);
					continue; 

				case StandardLibrary::FSQRT:
					*FloatOperand = (float)sqrt(*FloatOperand);
					continue; 

				case StandardLibrary::DSIN: 
					*DoubleOperand = (float)sin(*DoubleOperand);
					continue; 

				case StandardLibrary::DCOS: 
					*DoubleOperand = cos(*DoubleOperand);
					continue; 

				case StandardLibrary::DTAN: 
					*DoubleOperand = tan(*DoubleOperand);
					continue; 

				case StandardLibrary::DLOGN: 
					*DoubleOperand = log(*DoubleOperand);
					continue;

				case StandardLibrary::DLOG10: 
					*DoubleOperand = log10(*DoubleOperand);
					continue;

				case StandardLibrary::DEXP: 
					*DoubleOperand = exp(*DoubleOperand);
					continue;
			
				case StandardLibrary::DASIN: 
					*DoubleOperand = asin(*DoubleOperand);
					continue;

				case StandardLibrary::DACOS: 
					*DoubleOperand = acos(*DoubleOperand);
					continue;

				case StandardLibrary::DATAN: 
					*DoubleOperand = atan(*DoubleOperand);
					continue; 

				case StandardLibrary::DSQRT:
					*DoubleOperand = sqrt(*DoubleOperand);
					continue; 

				case StandardLibrary::IEXPT:
					IntTemp = *IntOperand--;
					*IntOperand = Int32Pow(*IntOperand, IntTemp);
					continue;

				case StandardLibrary::LEXPT:
					LongTemp = *LongOperand--;
					*LongOperand = Int64Pow(*LongOperand, LongTemp);
					continue;

				case StandardLibrary::FEXPT: 
					FloatTemp = *FloatOperand--;
					*FloatOperand = (float)pow(*FloatOperand, FloatTemp);
					continue;

				case StandardLibrary::DEXPT: 
					DoubleTemp = *DoubleOperand--;
					*DoubleOperand = pow(*DoubleOperand, DoubleTemp);
					continue;

				case StandardLibrary::ILIMIT:
					{
						int MaxInt = *IntOperand--;
						IntTemp = *IntOperand--;
						int MinInt = *IntOperand;
						if (IntTemp < MinInt)
							*IntOperand = MinInt;
						else if (IntTemp > MaxInt)
							*IntOperand = MaxInt;
						else
							*IntOperand = IntTemp;
					}
					continue;

				case StandardLibrary::LLIMIT:
					{
						int64 MaxInt = *LongOperand--;
						LongTemp = *LongOperand--;
						int64 MinInt = *LongOperand;
						if (IntTemp < MinInt)
							*LongOperand = MinInt;
						else if (IntTemp > MaxInt)
							*LongOperand = MaxInt;
						else
							*LongOperand = LongTemp;
					}
					continue;

				case StandardLibrary::DLIMIT:
					{
						double MaxDouble = *DoubleOperand--;
						DoubleTemp = *DoubleOperand--;
						double MinDouble = *DoubleOperand;
						if (DoubleTemp < MinDouble)
							*DoubleOperand = MinDouble;
						else if (DoubleTemp > MaxDouble)
							*DoubleOperand = MaxDouble;
						else
							*DoubleOperand = DoubleTemp;
					}
					continue;

				case StandardLibrary::FLIMIT:
					{
						float MaxFloat = *FloatOperand--;
						FloatTemp = *FloatOperand--;
						float MinFloat = *FloatOperand;
						if (FloatTemp < MinFloat)
							*FloatOperand = MinFloat;
						else if (FloatTemp > MaxFloat)
							*FloatOperand = MaxFloat;
						else
							*FloatOperand = FloatTemp;
					}
					continue;

				case StandardLibrary::TLIMIT:
				case StandardLibrary::TODLIMIT:
					{
						time_t MaxTime = *LongOperand--;
						time_t TimeSp = *LongOperand--;
						time_t MinTime = *LongOperand;
						if (difftime(TimeSp, MinTime) < 0)
							*LongOperand = MinTime;
						else if (difftime(TimeSp, MaxTime) > 0)
							*LongOperand = MaxTime;
						else
							*LongOperand = TimeSp;
					}
					continue;
		
				case StandardLibrary::DTLIMIT:
				case StandardLibrary::DTTLIMIT:
					{
						time_t MaxDate = *LongOperand--;
						time_t Date = *LongOperand--;
						time_t MinDate = *LongOperand;
						if (difftime(Date,MinDate) < 0)
							*LongOperand = MinDate;
						else if (difftime(Date, MaxDate) > 0)
							*LongOperand = MaxDate;
						else
							*LongOperand = Date;
					}
					continue;

				case StandardLibrary::SLEN:
					*++IntOperand = (int)strlen(*StringOperand--);
					continue;

				case StandardLibrary::WSLEN:
					*++IntOperand = (int)wcslen(*WStringOperand--);
					continue;

				case StandardLibrary::STRCMP:
					{
						const char* TmpString = *StringOperand--;
						*++IntOperand = strcmp(*StringOperand--, TmpString);
					}
					continue;

				case StandardLibrary::WSTRCMP:
					{
						const wchar_t* TmpWString = *WStringOperand--;
						*++IntOperand = wcscmp(*WStringOperand--, TmpWString);
					}
					continue;

				case StandardLibrary::TADD: 
					continue; 

				case StandardLibrary::TSUB: 
					continue;

				case StandardLibrary::DTTADD: 
					continue;

				case StandardLibrary::DTTSUB:
					continue;

				case StandardLibrary::DTSUB:
					continue;

				case StandardLibrary::TODTADD:
					continue;

				case StandardLibrary::TODTSUB:
					continue;

				case StandardLibrary::TODSUB:
					continue;

				case StandardLibrary::TIMUL:
					LongTemp = *LongOperand--;
					*LongOperand *= LongTemp;
					continue;

				case StandardLibrary::TFMUL: 
					FloatTemp = *FloatOperand--;
					*LongOperand = (time_t)(FloatTemp * *LongOperand);
					continue;

				case StandardLibrary::TDMUL: 
					DoubleTemp = *DoubleOperand--;
					*LongOperand = (time_t)(DoubleTemp * *LongOperand);
					continue;

				case StandardLibrary::TIDIV:
					IntTemp = *IntOperand--;
					if (IntTemp != 0)
						*LongOperand = *LongOperand/IntTemp;
					else
						#ifdef THROW_EXCEPTION
							throw new STRuntimeException(0, "TIDIV", pc);
						#else 
							*LongOperand = 0;
						#endif
					continue;

				case StandardLibrary::TFDIV: 
					FloatTemp = *FloatOperand--;
					if (fabs(FloatTemp) > 1.0e-6)
						*LongOperand = (time_t)(*LongOperand/FloatTemp);
					else
						#ifdef THROW_EXCEPTION
							throw new STRuntimeException(0, "TFDIV", pc);
						#else 
							*LongOperand = 0;
						#endif
					continue;

				case StandardLibrary::TDDIV: 
					DoubleTemp = *DoubleOperand--;
					if (fabs(DoubleTemp) > 1.0e-6)
						*LongOperand = (time_t)(*LongOperand/DoubleTemp);
					else
						#ifdef THROW_EXCEPTION
							throw new STRuntimeException(0, "TDDIV", pc);
						#else 
							*LongOperand = 0;
						#endif
					continue;

				default:
					continue;
			}
		}
	    continue;

		case VirtualMachineInstr::INEG:
			*IntOperand = -*IntOperand;
			continue;

		case VirtualMachineInstr::LNEG:
			*LongOperand = -*LongOperand;
			continue;

		case VirtualMachineInstr::FNEG: 
			*FloatOperand = -*FloatOperand;
			continue;

		case VirtualMachineInstr::DNEG: 
			*DoubleOperand = -*DoubleOperand;
			continue;

		case VirtualMachineInstr::IJGT:
			IntTemp = *IntOperand--;
			if (*IntOperand-- <= IntTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::LJGT:
			LongTemp = *LongOperand--;
			if (*LongOperand-- <= LongTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJGT:
			FloatTemp = *FloatOperand--;
			if (*FloatOperand-- <= FloatTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJGT:
			DoubleTemp = *DoubleOperand--;
			if (*DoubleOperand-- <= DoubleTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::IJLT:
			IntTemp = *IntOperand--;
			if (*IntOperand-- >= IntTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::LJLT:
			LongTemp = *LongOperand--;
			if (*LongOperand-- >= LongTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJLT:
			FloatTemp = *FloatOperand--;
			if (*FloatOperand-- >= FloatTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJLT:
			DoubleTemp = *DoubleOperand--;
			if (*DoubleOperand-- >= DoubleTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::IJLE:
			IntTemp = *IntOperand--;
			if (*IntOperand-- > IntTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::LJLE:
			LongTemp = *LongOperand--;
			if (*LongOperand-- > LongTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;
			
		case VirtualMachineInstr::FJLE:
			FloatTemp = *FloatOperand--;
			if (*FloatOperand-- > FloatTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJLE:
			DoubleTemp = *DoubleOperand--;
			if (*DoubleOperand-- > DoubleTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::IJGE:
			IntTemp = *IntOperand--;
			if (*IntOperand-- < IntTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::LJGE:
			LongTemp = *LongOperand--;
			if (*LongOperand-- < LongTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJGE: 
			FloatTemp = *FloatOperand--;
			if (*FloatOperand-- < FloatTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJGE:
			DoubleTemp = *DoubleOperand--;
			if (*DoubleOperand-- < DoubleTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::IJNE: 
			IntTemp = *IntOperand--;
			if (*IntOperand-- == IntTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::LJNE:
			LongTemp = *LongOperand--;
			if (*LongOperand-- == LongTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJNE:
			FloatTemp = *FloatOperand--;
			if (*FloatOperand-- == FloatTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJNE:
			DoubleTemp = *DoubleOperand--;
			if (*DoubleOperand-- == DoubleTemp)
			{
				pc = m_Operand[pc]; 
				goto decode;
			}
			continue;

		case VirtualMachineInstr::IJEQ:
			IntTemp = *IntOperand--;
			if (*IntOperand-- != IntTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::LJEQ:
			LongTemp = *LongOperand--;
			if (*LongOperand-- != LongTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJEQ:
			FloatTemp = *FloatOperand--;
			if (*FloatOperand-- != FloatTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJEQ:
			DoubleTemp = *DoubleOperand--;
			if (*DoubleOperand-- != DoubleTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::JUMP:
			pc = m_Operand[pc];
			goto decode;

		case VirtualMachineInstr::L_SWITCH:
			{
				// Case-sats implementerad som hopptabell.
				const SwitchInfo *Switch;

				Switch = (SwitchInfo *)(m_RODataSegment + m_Operand[pc]);
				const int64 MinValue = Switch->MinValue;
				if (Switch->IsInt64)
					LongTemp = *LongOperand--;
				else
					LongTemp = *IntOperand--;
				if (LongTemp < MinValue || LongTemp > Switch->MaxValue)
					pc += Switch->DefaultOffset;
				else {
					const unsigned short *JumpTable;

					JumpTable = (unsigned short *)((BYTE *)Switch + sizeof(SwitchInfo));
					pc       += JumpTable[LongTemp - MinValue]; 
				}
			}
			goto decode; 

		case VirtualMachineInstr::T_SWITCH:
			{
				// Case-sats implementerad som en tabell f�r bin�rs�kning.
				const SwitchInfo *Switch;

				Switch = (SwitchInfo *)(m_RODataSegment + m_Operand[pc]);
				if (Switch->IsInt64)
					LongTemp = *LongOperand--;
				else
					LongTemp = *IntOperand--;
				if (LongTemp < Switch->MinValue || LongTemp > Switch->MaxValue)
					pc += Switch->DefaultOffset;
				else {
					switch (Switch->ElemSize)
					{
					case 1:
						pc += GetJumpTableOffset<char>(Switch, LongTemp);
						break;
					case 2:
						pc += GetJumpTableOffset<short>(Switch, LongTemp);
						break;
					case 4:
						pc += GetJumpTableOffset<int>(Switch, LongTemp);
						break;
					case 8:
						pc += GetJumpTableOffset<int64>(Switch, LongTemp);
						break;
					default:
						pc += Switch->DefaultOffset;
					}
				}
			}
			goto decode;

		case VirtualMachineInstr::M_SWITCH:
			{
				const unsigned short* JumpTable;
				int InputCount = *IntOperand--;
				int Selector = *IntOperand--;
				if (Selector < 0 || Selector > InputCount - 1)
				{
				#ifdef THROW_EXCEPTION
					throw new STRuntimeException(12, Selector, pc);
				#else 
					Selector = 0;
				#endif
				}
				JumpTable = (const unsigned short*)(m_RODataSegment + m_Operand[pc]);
				pc += JumpTable[Selector];
			}
			goto decode;

		case VirtualMachineInstr::IJEQZ:
			if (*IntOperand--)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJEQZ:
			if (*FloatOperand-- != 0.0f)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJEQZ:
			if (*DoubleOperand-- != 0.0)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::IJNEZ:
			if (! *IntOperand--)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJNEZ:
			if (*FloatOperand-- == 0.0f)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJNEZ:
			if (*DoubleOperand-- == 0.0)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::IJGTZ:
			if (*IntOperand-- <= 0)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJGTZ:
			if (*FloatOperand-- <= 0.0f)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJGTZ:
			if (*DoubleOperand-- <= 0.0)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::IJLTZ:
			if (*IntOperand-- >= 0)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJLTZ:
			if (*FloatOperand-- >= 0.0f)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJLTZ:
			if (*DoubleOperand-- >= 0.0)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::IJGEZ:
			if (*IntOperand-- < 0)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJGEZ:
			if (*FloatOperand-- < 0.0f)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJGEZ:
			if (*DoubleOperand-- < 0.0)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::IJLEZ:
			if (*IntOperand-- > 0)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJLEZ:
			if (*FloatOperand-- > 0.0f)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJLEZ:
			if (*DoubleOperand-- > 0.0)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;
		
		case VirtualMachineInstr::EXE_ONCE:
			if (m_Executed)
			{
				// Skip block of code
				pc = m_Operand[pc];
				goto decode;
			}
			else {
				// Continue with next instruction
				m_Executed = true;
			}
			continue;

		case VirtualMachineInstr::IBAND: 
			IntTemp      = *IntOperand--;
			*IntOperand &= IntTemp;
			continue;

		case VirtualMachineInstr::LBAND: 
			LongTemp      = *LongOperand--;
			*LongOperand &= LongTemp;
			continue;
			
		case VirtualMachineInstr::IBIOR:
			IntTemp      = *IntOperand--;
			*IntOperand |= IntTemp;
			continue;

		case VirtualMachineInstr::LBIOR:
			LongTemp      = *LongOperand--;
			*LongOperand |= LongTemp;
			continue;
			
		case VirtualMachineInstr::IBXOR:
			IntTemp      = *IntOperand--;
			*IntOperand ^= IntTemp;
			continue;

		case VirtualMachineInstr::LBXOR:
			LongTemp      = *LongOperand--;
			*LongOperand ^= LongTemp;
			continue;
			
		case VirtualMachineInstr::IBNOT: 
			*IntOperand = ~*IntOperand;
			continue; 

		case VirtualMachineInstr::LBNOT: 
			*LongOperand = ~*LongOperand;
			continue; 
			
		case VirtualMachineInstr::IBSHL:
			IntTemp     = *IntOperand--;
			*IntOperand <<= IntTemp;
			continue; 
			
		case VirtualMachineInstr::IBSHR: 
			IntTemp       = *IntOperand--;
			*IntOperand >>= IntTemp;
			continue;

		case VirtualMachineInstr::LBSHL:
			IntTemp     = *IntOperand--;
			*LongOperand <<= IntTemp;
			continue; 
			
		case VirtualMachineInstr::LBSHR: 
			IntTemp     = *IntOperand--;
			*LongOperand >>= IntTemp;
			continue;
			
		case VirtualMachineInstr::IBSHL_1:
			*IntOperand <<= 1;
			continue; 
			
		case VirtualMachineInstr::IBSHR_1: 
			*IntOperand >>= 1;
			continue;
			
		case VirtualMachineInstr::BROR:
			{
				int BitCount = *IntOperand--;

				if (BitCount < 0)
#ifdef THROW_EXCEPTION
					throw new STRuntimeException(11, "BROR", pc);
#else
					BitCount = 0;
#endif
				BitCount     &= 0x7;
				BYTE ByteTemp = (BYTE)*IntOperand;
				*IntOperand   = (ByteTemp << (8 - BitCount)) | (ByteTemp >> BitCount);
			}
			continue;

		case VirtualMachineInstr::WROR:
			{
				int BitCount = *IntOperand--;

				if (BitCount < 0)
#ifdef THROW_EXCEPTION
					throw new STRuntimeException(11, "WROR", pc);
#else
					BitCount = 0;
#endif
				BitCount        &= 0xf;
				USHORT ShortTemp = (USHORT)*IntOperand;
				*IntOperand      = (ShortTemp << (16 - BitCount)) | (ShortTemp >> BitCount);
			}
			continue;

		case VirtualMachineInstr::DROR:
			{
				int BitCount = *IntOperand--;

				if (BitCount < 0)
#ifdef THROW_EXCEPTION
					throw new STRuntimeException(11, "DROR", pc);
#else
					BitCount = 0;
#endif
				BitCount   &= 0x1f;
				IntTemp     = *IntOperand;
				*IntOperand = (IntTemp << (32 - BitCount)) | (IntTemp >> BitCount);
			}
			continue;

		case VirtualMachineInstr::LROR:
			{
				int BitCount = *IntOperand--;

				if (BitCount < 0)
#ifdef THROW_EXCEPTION
					throw new STRuntimeException(11, "LROR", pc);
#else
					BitCount = 0;
#endif
				BitCount    &= 0x3f;
				LongTemp     = *LongOperand;
				*LongOperand = (LongTemp << (64 - BitCount)) | (LongTemp >> BitCount);
			}
			continue;

		case VirtualMachineInstr::BROL:
			{
				int BitCount = *IntOperand--;

				if (BitCount < 0)
#ifdef THROW_EXCEPTION
					throw new STRuntimeException(11, "BROL", pc);
#else
					BitCount = 0;
#endif
				BitCount     &= 0x7;
				BYTE ByteTemp = (BYTE)*IntOperand;
				*IntOperand   = (ByteTemp << BitCount) | (ByteTemp >> (8 - BitCount));
			}
			continue;

		case VirtualMachineInstr::WROL:
			{
				int BitCount = *IntOperand--;

				if (BitCount < 0)
#ifdef THROW_EXCEPTION
					throw new STRuntimeException(11, "WROL", pc);
#else
					BitCount = 0;
#endif
				BitCount        &= 0xf;
				USHORT ShortTemp = (USHORT)*IntOperand;
				*IntOperand      = (ShortTemp << BitCount) | (ShortTemp >> (16 - BitCount));
			}
			continue;

		case VirtualMachineInstr::DROL:
			{
				int BitCount = *IntOperand--;

				if (BitCount < 0)
#ifdef THROW_EXCEPTION
					throw new STRuntimeException(11, "DROL", pc);
#else
					BitCount = 0;
#endif
				BitCount   &= 0x1f;
				IntTemp     = *IntOperand;
				*IntOperand = (IntTemp << BitCount) | (IntTemp >> (32 - BitCount));
			}
			continue;

		case VirtualMachineInstr::LROL:
			{
				int BitCount = *IntOperand--;

				if (BitCount < 0)
#ifdef THROW_EXCEPTION
					throw new STRuntimeException(11, "LROL", pc);
#else
					BitCount = 0;
#endif
				BitCount    &= 0x3f;
				LongTemp     = *LongOperand;
				*LongOperand = (LongTemp << BitCount) | (LongTemp >> (64 - BitCount));
			}
			continue;

		case VirtualMachineInstr::IINCR:
			m_IntVar[m_Operand[pc]]++;
			continue;

        case VirtualMachineInstr::IINCR0:
			s0++;
			continue;

        case VirtualMachineInstr::IINCR1:
			s1++;
			continue;

        case VirtualMachineInstr::IINCR2:
			s2++;
			continue;
 
        case VirtualMachineInstr::WINCR0:
			i0++;
			continue;
  
        case VirtualMachineInstr::WINCR1:
			i1++;
			continue;
   
        case VirtualMachineInstr::WINCR2:
			i2++;
			continue;
  
        case VirtualMachineInstr::IDECR:
			m_IntVar[m_Operand[pc]]--;
			continue;
    
        case VirtualMachineInstr::IDECR0:
			s0--;
			continue;
  
        case VirtualMachineInstr::IDECR1:
			s1--;
			continue;
   
        case VirtualMachineInstr::IDECR2:
			s2--;
			continue;
   
        case VirtualMachineInstr::WDECR0:
			i0--;
			continue;
   
        case VirtualMachineInstr::WDECR1:
			i1--;
			continue;
   
        case VirtualMachineInstr::WDECR2:
			i2--;
			continue;
   
		case VirtualMachineInstr::BLOD0:
			*++IntOperand = b0;
			continue;

		case VirtualMachineInstr::ILOD0:
			*++IntOperand = s0;
			continue;

		case VirtualMachineInstr::WLOD0:
			*++IntOperand = i0;
			continue;

		case VirtualMachineInstr::LLOD0:
			*++LongOperand = li0;
			continue;

		case VirtualMachineInstr::FLOD0:
			*++FloatOperand = f0;
			continue;

		case VirtualMachineInstr::DLOD0:
			*++DoubleOperand = d0;
			continue;

		case VirtualMachineInstr::BLOD1:
			*++IntOperand = b1;
			continue;

		case VirtualMachineInstr::ILOD1:
			*++IntOperand = s1;
			continue;

		case VirtualMachineInstr::WLOD1:
			*++IntOperand = i1;
			continue;

		case VirtualMachineInstr::LLOD1:
			*++LongOperand = li1;
			continue;

		case VirtualMachineInstr::FLOD1:
			*++FloatOperand = f1;
			continue;

		case VirtualMachineInstr::DLOD1:
			*++DoubleOperand = d1;
			continue;

		case VirtualMachineInstr::BLOD2:
			*++IntOperand = b2;
			continue;

		case VirtualMachineInstr::ILOD2:
			*++IntOperand = s2;
			continue;

		case VirtualMachineInstr::WLOD2:
			*++IntOperand = i2;
			continue;

		case VirtualMachineInstr::LLOD2:
			*++LongOperand = li2;
			continue;

		case VirtualMachineInstr::FLOD2:
			*++FloatOperand = f2;
			continue;

		case VirtualMachineInstr::DLOD2:
			*++DoubleOperand = d2;
			continue;

		case VirtualMachineInstr::BLOD3:
			*++IntOperand = b3;
			continue;

		case VirtualMachineInstr::ILOD3:
			*++IntOperand = s3;
			continue;

		case VirtualMachineInstr::WLOD3:
			*++IntOperand = i3;
			continue;

		case VirtualMachineInstr::LLOD3:
			*++LongOperand = li3;
			continue;

		case VirtualMachineInstr::FLOD3:
			*++FloatOperand = f3;
			continue;

		case VirtualMachineInstr::DLOD3:
			*++DoubleOperand = d3;
			continue;

		case VirtualMachineInstr::BSTO0:
			b0 = (BYTE)*IntOperand--;
			continue;

		case VirtualMachineInstr::ISTO0:
			s0 = (short)*IntOperand--;
			continue;

		case VirtualMachineInstr::WSTO0:
			i0 = *IntOperand--;
			continue;

		case VirtualMachineInstr::LSTO0:
			li0 = *LongOperand--;
			continue;

		case VirtualMachineInstr::FSTO0:
			f0 = *FloatOperand--;
			continue;

		case VirtualMachineInstr::DSTO0:
			d0 = *DoubleOperand--;
			continue;

		case VirtualMachineInstr::BSTO1:
			b1 = (BYTE)*IntOperand--;
			continue;

		case VirtualMachineInstr::ISTO1:
			s1 = (short)*IntOperand--;
			continue;

		case VirtualMachineInstr::WSTO1:
			i1 = *IntOperand--;
			continue;

		case VirtualMachineInstr::LSTO1:
			li1 = *LongOperand--;
			continue;

		case VirtualMachineInstr::FSTO1:
			f1 = *FloatOperand--;
			continue;

		case VirtualMachineInstr::DSTO1:
			d1 = *DoubleOperand--;
			continue;

		case VirtualMachineInstr::BSTO2:
			b2 = (BYTE)*IntOperand--;
			continue;

		case VirtualMachineInstr::ISTO2:
			s2 = (short)*IntOperand--;
			continue;

		case VirtualMachineInstr::WSTO2:
			i2 = *IntOperand--;
			continue;

		case VirtualMachineInstr::LSTO2:
			li2 = *LongOperand--;
			continue;

		case VirtualMachineInstr::FSTO2:
			f2 = *FloatOperand--;
			continue;

		case VirtualMachineInstr::DSTO2:
			d2 = *DoubleOperand--;
			continue;

		case VirtualMachineInstr::BSTO3:
			b3 = (BYTE)*IntOperand--;
			continue;

		case VirtualMachineInstr::ISTO3:
			s3 = (short)*IntOperand--;
			continue;

		case VirtualMachineInstr::WSTO3:
			i3 = *IntOperand--;
			continue;

		case VirtualMachineInstr::LSTO3:
			li3 = *LongOperand--;
			continue;

		case VirtualMachineInstr::FSTO3:
			f3 = *FloatOperand--;
			continue;

		case VirtualMachineInstr::DSTO3:
			d3 = *DoubleOperand--;
			continue;

		case VirtualMachineInstr::BLOD:
			*++IntOperand = m_ByteVar[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::ILOD: 
			*++IntOperand = m_ShortVar[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::WLOD: 
			*++IntOperand = m_IntVar[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::LLOD:
			*++LongOperand = m_LongVar[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::DLOD:
			*++DoubleOperand = m_DoubleVar[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::BSTO: 
			m_ByteVar[m_Operand[pc]] = (BYTE)*IntOperand--;
			continue;

		case VirtualMachineInstr::ISTO: 
			m_ShortVar[m_Operand[pc]] = (short)*IntOperand--;
			continue;

		case VirtualMachineInstr::WSTO: 
			m_IntVar[m_Operand[pc]] = *IntOperand--;
			continue;

		case VirtualMachineInstr::LSTO:
			m_LongVar[m_Operand[pc]] = *LongOperand--;
			continue;

		case VirtualMachineInstr::FSTO:
			m_FloatVar[m_Operand[pc]] = *FloatOperand--;
			continue;

		case VirtualMachineInstr::DSTO:
			m_DoubleVar[m_Operand[pc]] = *DoubleOperand--;
			continue;

		case VirtualMachineInstr::SLOD:
			*++StringOperand = m_StringVar[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::WSLOD:
			*++WStringOperand = m_WStringVar[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::BLODX:
			IntTemp = *IntOperand--; 
			*++IntOperand = m_ByteVar[IntTemp];
			continue;

		case VirtualMachineInstr::ILODX:
			IntTemp = *IntOperand--; 
			*++IntOperand = m_ShortVar[IntTemp];
			continue;

		case VirtualMachineInstr::WLODX:
			IntTemp = *IntOperand--; 
			*++IntOperand = m_IntVar[IntTemp];
			continue;

		case VirtualMachineInstr::LLODX:
			*++LongOperand = m_LongVar[*IntOperand--];
			continue;

		case VirtualMachineInstr::FLODX: 
			*++FloatOperand = m_FloatVar[*IntOperand--]; 
			continue;

		case VirtualMachineInstr::DLODX:
			*++DoubleOperand = m_DoubleVar[*IntOperand--];
			continue;

		case VirtualMachineInstr::SLODX:
			*++StringOperand = m_StringVar[*IntOperand--];
			continue;

		case VirtualMachineInstr::WSLODX:
			*++WStringOperand = m_WStringVar[*IntOperand--];
			continue;

		case VirtualMachineInstr::BSTOX: 
			IntTemp = *IntOperand--;
			m_ByteVar[IntTemp] = (BYTE)*IntOperand--;
			continue;

		case VirtualMachineInstr::ISTOX: 
			IntTemp = *IntOperand--;
			m_ShortVar[IntTemp] = (short)*IntOperand--;
			continue;

		case VirtualMachineInstr::WSTOX: 
			IntTemp = *IntOperand--;
			m_IntVar[IntTemp] = *IntOperand--;
			continue;

		case VirtualMachineInstr::LSTOX:
			IntTemp  = *IntOperand--;
			m_LongVar[IntTemp] = *LongOperand--;
			continue;

		case VirtualMachineInstr::FSTOX:
			IntTemp = *IntOperand--;
			m_FloatVar[IntTemp] = *FloatOperand--;
			continue;

		case VirtualMachineInstr::DSTOX:
			IntTemp = *IntOperand--;
			m_DoubleVar[IntTemp] = *DoubleOperand--;
			continue;

		case VirtualMachineInstr::SSTO:
			IntTemp = *IntOperand--; // Size
			strcpy_s(m_StringVar[m_Operand[pc]], IntTemp, *StringOperand--);
			continue;

		case VirtualMachineInstr::WSSTO:
			IntTemp = *IntOperand--; // Size
			wcscpy_s(m_WStringVar[m_Operand[pc]], IntTemp, *WStringOperand--);
			continue;

		case VirtualMachineInstr::SSTOX:
			{
				size_t Size = *IntOperand--;
				IntTemp = *IntOperand--;
				strcpy_s(m_StringVar[IntTemp], Size, *StringOperand--);
			}
			continue;

		case VirtualMachineInstr::WSSTOX:
			{
				size_t Size = *IntOperand--;
				IntTemp = *IntOperand--;
				wcscpy_s(m_WStringVar[IntTemp], Size, *WStringOperand--);
			}
			continue;
			
		case VirtualMachineInstr::RETN: 
			return *DoubleOperand;
			
	    case VirtualMachineInstr::IDUPL:
			IntTemp = *IntOperand++;
			*IntOperand = IntTemp;
			continue;

		case VirtualMachineInstr::LDUPL:
			LongTemp = *LongOperand++;
			*LongOperand = LongTemp;
			continue;

		case VirtualMachineInstr::FDUPL:
			FloatTemp = *FloatOperand++;
			*FloatOperand = FloatTemp;
			continue;

		case VirtualMachineInstr::DDUPL: 
			DoubleTemp = *DoubleOperand++;
			*DoubleOperand = DoubleTemp;
			continue;
				
		case VirtualMachineInstr::I2C:
			*IntOperand = (char)*IntOperand;
			continue;

		case VirtualMachineInstr::I2B:
			*IntOperand = (unsigned char)*IntOperand;
			continue;

		case VirtualMachineInstr::I2S:
			*IntOperand = (short)*IntOperand;
			continue;

		case VirtualMachineInstr::I2US:
			*IntOperand = (unsigned short)*IntOperand;
			continue;

		case VirtualMachineInstr::F2I: 
			*++IntOperand = int(*FloatOperand--);
			continue;
			
		case VirtualMachineInstr::I2F:
			*++FloatOperand = float(*IntOperand--);
			continue;

		case VirtualMachineInstr::D2I:
			*++IntOperand = int(*DoubleOperand--);
			continue;

		case VirtualMachineInstr::D2F: 
			*++FloatOperand = float(*DoubleOperand--);
			continue;

		case VirtualMachineInstr::I2D: 
			*++DoubleOperand = *IntOperand--;
			continue;

		case VirtualMachineInstr::I2L:
			*++LongOperand = *IntOperand--;
			continue;

		case VirtualMachineInstr::L2D:
			*++DoubleOperand = double(*LongOperand--);
			continue;

		case VirtualMachineInstr::F2D:
			*++DoubleOperand = *FloatOperand--;
			continue;

		case VirtualMachineInstr::D2L:
			*++LongOperand = __int64(*DoubleOperand--);
			continue;

		case VirtualMachineInstr::L2I:
			*++IntOperand = int(*LongOperand--);
			continue;

		case VirtualMachineInstr::L2F:
			*++FloatOperand = float(*LongOperand--);
			continue;

		case VirtualMachineInstr::FLOD:
			*++FloatOperand = m_FloatVar[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::FCONST:
			*++FloatOperand = m_FloatConst[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::FCONST_0:
			*++FloatOperand = 0.0f;
			continue;

		case VirtualMachineInstr::FCONST_1:
			*++FloatOperand = 1.0f;
			continue;

		case VirtualMachineInstr::FCONST_N1:
			*++FloatOperand = -1.0f;
			continue;

		case VirtualMachineInstr::DCONST:
			*++DoubleOperand = m_DoubleConst[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::DCONST0:
			*++DoubleOperand = m_DoubleConst0;
			continue;

		case VirtualMachineInstr::DCONST1:
			*++DoubleOperand = m_DoubleConst1;
			continue;

		case VirtualMachineInstr::DCONST2:
			*++DoubleOperand = m_DoubleConst2;
			continue;

		case VirtualMachineInstr::DCONST3:
			*++DoubleOperand = m_DoubleConst3;
			continue;

		case VirtualMachineInstr::DCONST_0:
			*++DoubleOperand = 0.0;
			continue;

		case VirtualMachineInstr::DCONST_1:
			*++DoubleOperand = 1.0;
			continue;

		case VirtualMachineInstr::DCONST_N1:
			*++DoubleOperand = -1.0;
			continue;

		case VirtualMachineInstr::ICONST:
			*++IntOperand = (short)m_Operand[pc];
			continue;

		case VirtualMachineInstr::WCONST:
			*++IntOperand = m_IntConst[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::LCONST:
			*++LongOperand = m_LongConst[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::SCONST:
			*++StringOperand = (char *)m_StringConst[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::WSCONST:
			*++WStringOperand = (wchar_t*)m_WStringConst[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::ICONST_0:
			*++IntOperand = 0;
			continue;

		case VirtualMachineInstr::ICONST_1:
			*++IntOperand = 1;
			continue;

		case VirtualMachineInstr::ICONST_N1:
			*++IntOperand = -1;
			continue;

		case VirtualMachineInstr::LCONST_0:
			*++LongOperand = 0;
			continue;

		case VirtualMachineInstr::LCONST_1:
			*++LongOperand = 1;
			continue;

		case VirtualMachineInstr::LCONST_N1:
			*++LongOperand = -1;
			continue;

		case VirtualMachineInstr::FINV:
			*FloatOperand = 1.0f/ *FloatOperand;
			continue;

		case VirtualMachineInstr::DINV:
			*DoubleOperand = 1.0/ *DoubleOperand;
			continue;

		case VirtualMachineInstr::RANGE_CHECK:
			{
				// Check that array index is not out of bounds.
				int LowerBound = *IntOperand--;
				int UpperBound = *IntOperand--;
				IntTemp        = *IntOperand;
				if (IntTemp < LowerBound || IntTemp > UpperBound)
				{
				#ifdef THROW_EXCEPTION
					throw new STRuntimeException(3);
                #else
					 *IntOperand = LowerBound;
				#endif
				}
			}
			continue;
			
		case VirtualMachineInstr::JSBR:  // Call user defined function/functionblock
			m_ExternProgOrgUnit[m_Operand[pc]]->Execute();
			continue;

		case VirtualMachineInstr::ROM_COPY: // Read only data segment copy.
			{
				int Count           = *IntOperand--;
				int SrcOffset       = *IntOperand--;
				int DstOffset       = *IntOperand--;
				BYTE* DstAddr       = m_RWDataSegment + DstOffset;
				const BYTE* SrcAddr = m_RODataSegment + SrcOffset;
				while (Count-- > 0)
					*DstAddr++ = *SrcAddr++;
			}
			continue;

		case VirtualMachineInstr::MEM_COPY: // Read write data segment copy.
			{
				int Count           = *IntOperand--;
				int DstOffset       = *IntOperand--;
				int SrcOffset       = *IntOperand--;
				BYTE* DstAddr       = m_RWDataSegment + DstOffset;
				const BYTE* SrcAddr = m_RWDataSegment + SrcOffset;
				while (Count-- > 0)
					*DstAddr++ = *SrcAddr++;
			}
			continue;

		case VirtualMachineInstr::MEM_SETZ: // MEMory SET to Zero.
			{
				int Offset    = *IntOperand--;
				int Count     = *IntOperand--;
				BYTE* DstAddr = m_RWDataSegment + Offset;
				while (Count-- > 0)
					*DstAddr++ = 0;
			}
			continue;

		case VirtualMachineInstr::MEM_SETB: // MEMory SET to Bytes = 1, 2, 4 or 8 bytes.
			{
				char ElemSize = (char)m_Operand[pc];
				int ElemCount = *IntOperand--;
				int Offset    = *IntOperand--;
				
				switch (ElemSize)
				{
				case 1:
					{
						const BYTE Byte = (BYTE)*IntOperand--;
						BYTE* DstAddr   = m_RWDataSegment + Offset;
						while (ElemCount-- > 0)
							*DstAddr++ = Byte;
					}
					break;

				case 2:
					{
						const short Short = (short)*IntOperand--;
						short* DstAddr    = (short *)(m_RWDataSegment + Offset);
						while (ElemCount-- > 0)
							*DstAddr++ = Short;
					}
					break;

				case 4:
					{
						const int Int = *IntOperand--;
						int* DstAddr  = (int *)(m_RWDataSegment + Offset);
						while (ElemCount-- > 0)
							*DstAddr++ = Int;
					}
					break;

				case 8:
					{
						const int64 Long = *LongOperand--;
						int64* DstAddr   = (int64 *)(m_RWDataSegment + Offset);
						while (ElemCount-- > 0)
							*DstAddr++ = Long;
					}
					break;

				default:
					break;
				}
				
			}
			continue;

		case VirtualMachineInstr::NOOP:
			continue;

		default:
			#ifdef THROW_EXCEPTION
				throw new STRuntimeException(1, pc, m_OpCode[pc]);
			#endif
				;
		}
	}
	return 0;
}

int STLangSubPOUObject::Execute()
{
	int IntTemp;
	float FloatTemp;
	double DoubleTemp;
	int64 LongTemp;
	register int pc;
	char b0,b1,b2,b3;
	short s0,s1,s2,s3;
	int i0,i1,i2,i3;
	float f0,f1,f2,f3;
	double d0,d1,d2,d3;
	int64 li0,li1,li2,li3;

	pc = -1;
	b0 = b1 = b2 = b3 = 0;
	s0 = s1 = s2 = s3 = 0;
	i0 = i1 = i2 = i3 = 0;
	f0 = f1 = f2 = f3 = 0;
	d0 = d1 = d2 = d3 = 0;
	li0 = li1 = li2 = li3 = 0;
	while (true)
	{
		pc++;
decode:	switch (m_OpCode[pc])
		{
        case VirtualMachineInstr::IADD:
			IntTemp = *IntOperand--;
            *IntOperand += IntTemp;
            continue;

        case VirtualMachineInstr::LADD:
			LongTemp = *LongOperand--;
            *LongOperand += LongTemp;
            continue;

        case VirtualMachineInstr::FADD:
			FloatTemp = *FloatOperand--;
            *FloatOperand += FloatTemp;
            continue;

        case VirtualMachineInstr::DADD:
			DoubleTemp = *DoubleOperand--;
            *DoubleOperand += DoubleTemp;
            continue;

        case VirtualMachineInstr::ISUB:
			IntTemp = *IntOperand--;
            *IntOperand -= IntTemp;
            continue;

        case VirtualMachineInstr::LSUB:
			LongTemp = *LongOperand--;
            *LongOperand -= LongTemp;
            continue;

        case VirtualMachineInstr::FSUB:
			FloatTemp = *FloatOperand--;
            *FloatOperand -= FloatTemp;
            continue;

        case VirtualMachineInstr::DSUB:
			DoubleTemp = *DoubleOperand--;
            *DoubleOperand -= DoubleTemp;
            continue;

        case VirtualMachineInstr::IMUL:
			IntTemp = *IntOperand--;
            *IntOperand *= IntTemp;
            continue;

        case VirtualMachineInstr::LMUL:
			LongTemp = *LongOperand--;
            *LongOperand *= LongTemp;
            continue;

        case VirtualMachineInstr::FMUL:
			FloatTemp = *FloatOperand--;
            *FloatOperand *= FloatTemp;
            continue;

        case VirtualMachineInstr::DMUL:
			DoubleTemp = *DoubleOperand--;
            *DoubleOperand *= DoubleTemp;
            continue;

	    case VirtualMachineInstr::IDIV:
            IntTemp = *IntOperand--;
            if (IntTemp != 0)
                *IntOperand /= IntTemp;
		    else 
#if THROW_RUNTIME_EXCEPTION
				throw new STRuntimeException(0, "IDIV", pc);
#else 
                *IntOperand = 0;
#endif
		    continue;

		case VirtualMachineInstr::LDIV:
            LongTemp = *LongOperand--;
            if (LongTemp != 0)
                *LongOperand /= LongTemp;
			else  
#if THROW_RUNTIME_EXCEPTION
				throw new STRuntimeException(0, "LDIV", pc);
#else 
                *LongOperand = 0;
#endif
			continue;

	    case VirtualMachineInstr::FDIV:
            FloatTemp = *FloatOperand--;
            if (FloatTemp != 0.0)
                *FloatOperand /= FloatTemp;
		    else
#if THROW_RUNTIME_EXCEPTION
				throw new STRuntimeException(0, "FDIV", pc);
#else 
                *FloatOperand = 0.0;
#endif
		    continue;

        case VirtualMachineInstr::DDIV:
            DoubleTemp = *DoubleOperand--;
            if (DoubleTemp != 0.0)
                *DoubleOperand /= DoubleTemp;
	        else 
#if THROW_RUNTIME_EXCEPTION
			    throw new STRuntimeException(0, "DDIV", pc);
#else 
                *DoubleOperand = 0.0;
#endif
	        continue;

		case VirtualMachineInstr::IMOD:  // Integer modulus
            IntTemp = *IntOperand--;
            if (IntTemp != 0)
                *IntOperand %= IntTemp;
			else 
#if THROW_RUNTIME_EXCEPTION
				throw new STRuntimeException(0, "IMOD", pc);
#else 
                *IntOperand = 0;
#endif
			continue;

		case VirtualMachineInstr::LMOD:  // Integer modulus
            LongTemp = *LongOperand--;
            if (LongTemp != 0)
                *LongOperand %= LongTemp;
			else 
#if THROW_RUNTIME_EXCEPTION
				throw new STRuntimeException(0, "LMOD", pc);
#else 
                *LongOperand = 0;
#endif
			continue;

		case VirtualMachineInstr::CALL:
			{
				//
				// Standard library function call
				//
				BYTE StdLibFunction = (BYTE)m_Operand[pc];

				switch (StdLibFunction)
				{
				case StandardLibrary::IMIN:
				{
					int Count    = *IntOperand--; // Antal parametrar
					int MinInt32 = *IntOperand--;
					while (--Count > 0)
					{
						IntTemp = *IntOperand--;
						if (IntTemp < MinInt32)
							MinInt32 = IntTemp;
					}
					*++IntOperand = MinInt32;
				}
				continue;

				case StandardLibrary::LMIN:
				{
					int Count        = *IntOperand--; // Antal parametrar
					int64 MinInt64 = *LongOperand--;
					while (--Count > 0)
					{
						LongTemp = *LongOperand--;
						if (LongTemp < MinInt64)
							MinInt64 = LongTemp;
					}
					*++LongOperand = MinInt64;
				}
				continue;

				case StandardLibrary::FMIN: 
				{
					IntTemp    = *IntOperand--;   // Antal parametrar
					float FMin = *FloatOperand--;
					while (--IntTemp > 0)
					{
						FloatTemp = *FloatOperand--;
						if (FloatTemp < FMin)
							FMin = FloatTemp;
					}
					*++FloatOperand = FMin;
				}
				continue;

				case StandardLibrary::DMIN:
				{
					IntTemp     = *IntOperand--;   // Antal parametrar
					double DMin = *DoubleOperand--;
					while (--IntTemp > 0)
					{
						DoubleTemp = *DoubleOperand--;
						if (DoubleTemp < DMin)
							DMin = DoubleTemp;
					}
					*++DoubleOperand = DMin;
				}
				continue;

				case StandardLibrary::IMAX:
				{
					int Count    = *IntOperand--; // Antal parametrar
					int MaxInt32 = *IntOperand--;
					while (--Count > 0)
					{
						IntTemp = *IntOperand--;
						if (IntTemp > MaxInt32)
							MaxInt32 = IntTemp;
					}
					*++IntOperand = MaxInt32;
				}
				continue;

				case StandardLibrary::LMAX:
				{
					int Count        = *IntOperand--; // Antal parametrar
					int64 MaxInt64 = *LongOperand--;
					while (--Count > 0)
					{
						LongTemp = *LongOperand--;
						if (LongTemp > MaxInt64)
							MaxInt64 = LongTemp;
					}
					*++LongOperand = MaxInt64;
				}
				continue;

				case StandardLibrary::FMAX: 
				{
					int Count = *IntOperand--;   // Antal parametrar
					float FMax = *FloatOperand--;
					while (--Count > 0)
					{
						FloatTemp = *FloatOperand--;
						if (FloatTemp > FMax)
							FMax = FloatTemp;
					}
					*++FloatOperand = FMax;
				}
				continue;

				case StandardLibrary::DMAX: 
				{
					int Count = *IntOperand--;   // Antal parametrar
					double DMax = *DoubleOperand--;
					while (--Count > 0)
					{
						DoubleTemp = *DoubleOperand--;
						if (DoubleTemp > DMax)
							DMax = DoubleTemp;
					}
					*++DoubleOperand = DMax;
				}
				continue;

				case StandardLibrary::IABS: 
					if (*IntOperand < 0)
						*IntOperand = -*IntOperand;
					continue;

				case StandardLibrary::LABS:
					if (*LongOperand < 0)
						*LongOperand = -*LongOperand;
					continue;

				case StandardLibrary::FABS: 
					if (*FloatOperand < 0.0f)
						*FloatOperand = -*FloatOperand;
					continue;

				case StandardLibrary::DABS:
					if (*DoubleOperand < 0.0)
						*DoubleOperand = -*DoubleOperand;
					continue;

				case StandardLibrary::FSIN: 
					*FloatOperand = (float)sin(*FloatOperand);
					continue; 

				case StandardLibrary::FCOS: 
					*FloatOperand = (float)cos(*FloatOperand);
					continue; 

				case StandardLibrary::FTAN: 
					*FloatOperand = (float)tan(*FloatOperand);
					continue; 

				case StandardLibrary::FLOGN: 
					*FloatOperand = (float)log(*FloatOperand);
					continue;

				case StandardLibrary::FLOG10: 
					*FloatOperand = (float)log10(*FloatOperand);
					continue;

				case StandardLibrary::FEXP: 
					*FloatOperand = (float)exp(*FloatOperand);
					continue;
			
				case StandardLibrary::FASIN: 
					*FloatOperand = (float)asin(*FloatOperand);
					continue;

				case StandardLibrary::FACOS: 
					*FloatOperand = (float)acos(*FloatOperand);
					continue;

				case StandardLibrary::FATAN: 
					*FloatOperand = (float)atan(*FloatOperand);
					continue; 

				case StandardLibrary::FSQRT:
					*FloatOperand = (float)sqrt(*FloatOperand);
					continue; 

				case StandardLibrary::DSIN: 
					*DoubleOperand = sin(*DoubleOperand);
					continue; 

				case StandardLibrary::DCOS: 
					*DoubleOperand = cos(*DoubleOperand);
					continue; 

				case StandardLibrary::DTAN: 
					*DoubleOperand = tan(*DoubleOperand);
					continue; 

				case StandardLibrary::DLOGN: 
					*DoubleOperand = log(*DoubleOperand);
					continue;

				case StandardLibrary::DLOG10: 
					*DoubleOperand = log10(*DoubleOperand);
					continue;

				case StandardLibrary::DEXP: 
					*DoubleOperand = exp(*DoubleOperand);
					continue;
			
				case StandardLibrary::DASIN: 
					*DoubleOperand = asin(*DoubleOperand);
					continue;

				case StandardLibrary::DACOS: 
					*DoubleOperand = acos(*DoubleOperand);
					continue;

				case StandardLibrary::DATAN: 
					*DoubleOperand = atan(*DoubleOperand);
					continue; 

				case StandardLibrary::DSQRT:
					*DoubleOperand = sqrt(*DoubleOperand);
					continue; 

				case StandardLibrary::IEXPT:
					IntTemp = *IntOperand--;
					*IntOperand = Int32Pow(*IntOperand, IntTemp);
					continue;

				case StandardLibrary::LEXPT:
					LongTemp = *LongOperand--;
					*LongOperand = Int64Pow(*LongOperand, LongTemp);
					continue;

				case StandardLibrary::FEXPT: 
					FloatTemp = *FloatOperand--;
					*FloatOperand = (float)pow(*FloatOperand, FloatTemp);
					continue;

				case StandardLibrary::DEXPT: 
					DoubleTemp = *DoubleOperand--;
					*DoubleOperand = pow(*DoubleOperand, DoubleTemp);
					continue;

				case StandardLibrary::ILIMIT:
					{
						int MaxInt = *IntOperand--;
						IntTemp = *IntOperand--;
						int MinInt = *IntOperand;
						if (IntTemp < MinInt)
							*IntOperand = MinInt;
						else if (IntTemp > MaxInt)
							*IntOperand = MaxInt;
						else
							*IntOperand = IntTemp;
					}
					continue;

				case StandardLibrary::LLIMIT:
					{
						int64 MaxInt = *LongOperand--;
						LongTemp = *LongOperand--;
						int64 MinInt = *LongOperand;
						if (IntTemp < MinInt)
							*LongOperand = MinInt;
						else if (IntTemp > MaxInt)
							*LongOperand = MaxInt;
						else
							*LongOperand = LongTemp;
					}
					continue;

				case StandardLibrary::DLIMIT:
					{
						double MaxDouble = *DoubleOperand--;
						DoubleTemp = *DoubleOperand--;
						double MinDouble = *DoubleOperand;
						if (DoubleTemp < MinDouble)
							*DoubleOperand = MinDouble;
						else if (DoubleTemp > MaxDouble)
							*DoubleOperand = MaxDouble;
						else
							*DoubleOperand = DoubleTemp;
					}
					continue;

				case StandardLibrary::FLIMIT:
					{
						float MaxFloat = *FloatOperand--;
						FloatTemp = *FloatOperand--;
						float MinFloat = *FloatOperand;
						if (FloatTemp < MinFloat)
							*FloatOperand = MinFloat;
						else if (FloatTemp > MaxFloat)
							*FloatOperand = MaxFloat;
						else
							*FloatOperand = FloatTemp;
					}
					continue;

				case StandardLibrary::TLIMIT:
				case StandardLibrary::TODLIMIT:
					{
						time_t MaxTime = *LongOperand--;
						time_t TimeSp = *LongOperand--;
						time_t MinTime = *LongOperand;
						if (difftime(TimeSp, MinTime) < 0)
							*LongOperand = MinTime;
						else if (difftime(TimeSp, MaxTime) > 0)
							*LongOperand = MaxTime;
						else
							*LongOperand = TimeSp;
					}
					continue;
		
				case StandardLibrary::DTLIMIT:
				case StandardLibrary::DTTLIMIT:
					{
						time_t MaxDate = *LongOperand--;
						time_t Date = *LongOperand--;
						time_t MinDate = *LongOperand;
						if (difftime(Date,MinDate) < 0)
							*LongOperand = MinDate;
						else if (difftime(Date, MaxDate) > 0)
							*LongOperand = MaxDate;
						else
							*LongOperand = Date;
					}
					continue;

				case StandardLibrary::SLEN:
					*++IntOperand = (int)strlen(*StringOperand--);
					continue;

				case StandardLibrary::WSLEN:
					*++IntOperand = (int)wcslen(*WStringOperand--);
					continue;

				case StandardLibrary::STRCMP:
					{
						const char* TmpString = *StringOperand--;
						*++IntOperand = strcmp(*StringOperand--, TmpString);
					}
					continue;

				case StandardLibrary::WSTRCMP:
					{
						const wchar_t* TmpWString = *WStringOperand--;
						*++IntOperand = wcscmp(*WStringOperand, TmpWString);
					}
					continue;

				case StandardLibrary::TADD: 
					continue; 

				case StandardLibrary::TSUB: 
					continue;

				case StandardLibrary::DTTADD: 
					continue;

				case StandardLibrary::DTTSUB:
					continue;

				case StandardLibrary::DTSUB:
					continue;

				case StandardLibrary::TODTADD:
					continue;

				case StandardLibrary::TODTSUB:
					continue;

				case StandardLibrary::TODSUB:
					continue;

				case StandardLibrary::TIMUL:
					LongTemp = *LongOperand--;
					*LongOperand *= LongTemp;
					continue;

				case StandardLibrary::TFMUL: 
					FloatTemp = *FloatOperand--;
					*LongOperand = (time_t)(FloatTemp * *LongOperand);
					continue;

				case StandardLibrary::TDMUL: 
					DoubleTemp = *DoubleOperand--;
					*LongOperand = (time_t)(DoubleTemp * *LongOperand);
					continue;

				case StandardLibrary::TIDIV:
					IntTemp = *IntOperand--;
					if (IntTemp != 0)
						*LongOperand = *LongOperand/IntTemp;
					else
						#ifdef THROW_EXCEPTION
							throw new STRuntimeException(0, "TIDIV", pc);
						#else 
							*LongOperand = 0;
						#endif
					continue;

				case StandardLibrary::TFDIV: 
					FloatTemp = *FloatOperand--;
					if (fabs(FloatTemp) > 1.0e-6)
						*LongOperand = (time_t)(*LongOperand/FloatTemp);
					else
						#ifdef THROW_EXCEPTION
							throw new STRuntimeException(0, "TFDIV", pc);
						#else 
							*LongOperand = 0;
						#endif
					continue;

				case StandardLibrary::TDDIV: 
					DoubleTemp = *DoubleOperand--;
					if (fabs(DoubleTemp) > 1.0e-6)
						*LongOperand = (time_t)(*LongOperand/DoubleTemp);
					else
						#ifdef THROW_EXCEPTION
							throw new STRuntimeException(0, "TDDIV", pc);
						#else 
							*LongOperand = 0;
						#endif
					continue;

				default:
					continue;
			}
		}
	    continue;

		case VirtualMachineInstr::INEG:
			*IntOperand = -*IntOperand;
			continue;

		case VirtualMachineInstr::LNEG:
			*LongOperand = -*LongOperand;
			continue;

		case VirtualMachineInstr::FNEG: 
			*FloatOperand = -*FloatOperand;
			continue;

		case VirtualMachineInstr::DNEG: 
			*DoubleOperand = -*DoubleOperand;
			continue;

		case VirtualMachineInstr::IJGT:
			IntTemp = *IntOperand--;
			if (*IntOperand-- <= IntTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::LJGT:
			LongTemp = *LongOperand--;
			if (*LongOperand-- <= LongTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJGT:
			FloatTemp = *FloatOperand--;
			if (*FloatOperand-- <= FloatTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJGT:
			DoubleTemp = *DoubleOperand--;
			if (*DoubleOperand-- <= DoubleTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::IJLT:
			IntTemp = *IntOperand--;
			if (*IntOperand-- >= IntTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::LJLT:
			LongTemp = *LongOperand--;
			if (*LongOperand-- >= LongTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJLT:
			FloatTemp = *FloatOperand--;
			if (*FloatOperand-- >= FloatTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJLT:
			DoubleTemp = *DoubleOperand--;
			if (*DoubleOperand-- >= DoubleTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::IJLE:
			IntTemp = *IntOperand--;
			if (*IntOperand-- > IntTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::LJLE:
			LongTemp = *LongOperand--;
			if (*LongOperand-- > LongTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;
			
		case VirtualMachineInstr::FJLE:
			FloatTemp = *FloatOperand--;
			if (*FloatOperand-- > FloatTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJLE:
			DoubleTemp = *DoubleOperand--;
			if (*DoubleOperand-- > DoubleTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::IJGE:
			IntTemp = *IntOperand--;
			if (*IntOperand-- < IntTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::LJGE:
			LongTemp = *LongOperand--;
			if (*LongOperand-- < LongTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJGE: 
			FloatTemp = *FloatOperand--;
			if (*FloatOperand-- < FloatTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJGE:
			DoubleTemp = *DoubleOperand--;
			if (*DoubleOperand-- < DoubleTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::IJNE: 
			IntTemp = *IntOperand--;
			if (*IntOperand-- == IntTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::LJNE:
			LongTemp = *LongOperand--;
			if (*LongOperand-- == LongTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJNE:
			FloatTemp = *FloatOperand--;
			if (*FloatOperand-- == FloatTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJNE:
			DoubleTemp = *DoubleOperand--;
			if (*DoubleOperand-- == DoubleTemp)
			{
				pc = m_Operand[pc]; 
				goto decode;
			}
			continue;

		case VirtualMachineInstr::IJEQ:
			IntTemp = *IntOperand--;
			if (*IntOperand-- != IntTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::LJEQ:
			LongTemp = *LongOperand--;
			if (*LongOperand-- != LongTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJEQ:
			FloatTemp = *FloatOperand--;
			if (*FloatOperand-- != FloatTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJEQ:
			DoubleTemp = *DoubleOperand--;
			if (*DoubleOperand-- != DoubleTemp)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::JUMP:
			pc = m_Operand[pc];
			goto decode;

		case VirtualMachineInstr::L_SWITCH:
			{
				// Case-statement implemented as a jump table.
				const SwitchInfo *Switch;

				Switch = (SwitchInfo *)(m_RODataSegment + m_Operand[pc]);
				const int64 MinValue = Switch->MinValue;
				if (Switch->IsInt64)
					LongTemp = *LongOperand--;
				else
					LongTemp = *IntOperand--;
				if (LongTemp < MinValue || LongTemp > Switch->MaxValue)
					pc += Switch->DefaultOffset;
				else {
					const unsigned short *JumpTable;

					JumpTable = (unsigned short *)((BYTE *)Switch + sizeof(SwitchInfo));
					pc       += JumpTable[LongTemp - MinValue]; 
				}
			}
			goto decode; 

		case VirtualMachineInstr::T_SWITCH:
			{
				// Case-statement implemented as a binary search table.
				const SwitchInfo *Switch;

				Switch = (SwitchInfo *)(m_RODataSegment + m_Operand[pc]);
				if (Switch->IsInt64)
					LongTemp = *LongOperand--;
				else
					LongTemp = *IntOperand--;
				if (LongTemp < Switch->MinValue || LongTemp > Switch->MaxValue)
					pc += Switch->DefaultOffset;
				else {
					switch (Switch->ElemSize)
					{
					case 1:
						pc += GetJumpTableOffset<char>(Switch, LongTemp);
						break;
					case 2:
						pc += GetJumpTableOffset<short>(Switch, LongTemp);
						break;
					case 4:
						pc += GetJumpTableOffset<int>(Switch, LongTemp);
						break;
					case 8:
						pc += GetJumpTableOffset<int64>(Switch, LongTemp);
						break;
					default:
						pc += Switch->DefaultOffset;
					}
				}
			}
			goto decode;

		case VirtualMachineInstr::M_SWITCH:
			{
				const unsigned short* JumpTable;
				int InputCount = *IntOperand--;
				int Selector = *IntOperand--;
				if (Selector < 0 || Selector > InputCount - 1)
				{
				#ifdef THROW_EXCEPTION
					throw new STRuntimeException(12, Selector, pc);
				#else 
					Selector = 0;
				#endif
				}
				JumpTable = (const unsigned short*)(m_RODataSegment + m_Operand[pc]);
				pc += JumpTable[Selector];
			}
			goto decode;

		case VirtualMachineInstr::IJEQZ:
			if (*IntOperand--)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJEQZ:
			if (*FloatOperand-- != 0.0f)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJEQZ:
			if (*DoubleOperand-- != 0.0)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::IJNEZ:
			if (! *IntOperand--)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJNEZ:
			if (*FloatOperand-- == 0.0f)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJNEZ:
			if (*DoubleOperand-- == 0.0)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::IJGTZ:
			if (*IntOperand-- <= 0)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJGTZ:
			if (*FloatOperand-- <= 0.0f)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJGTZ:
			if (*DoubleOperand-- <= 0.0)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::IJLTZ:
			if (*IntOperand-- >= 0)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJLTZ:
			if (*FloatOperand-- >= 0.0f)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJLTZ:
			if (*DoubleOperand-- >= 0.0)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::IJGEZ:
			if (*IntOperand-- < 0)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJGEZ:
			if (*FloatOperand-- < 0.0f)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJGEZ:
			if (*DoubleOperand-- < 0.0)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::IJLEZ:
			if (*IntOperand-- > 0)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::FJLEZ:
			if (*FloatOperand-- > 0.0f)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::DJLEZ:
			if (*DoubleOperand-- > 0.0)
			{
				pc = m_Operand[pc];
				goto decode;
			}
			continue;

		case VirtualMachineInstr::EXE_ONCE:
			if (m_Executed)
			{
				// Skip block of code
				pc = m_Operand[pc];
				goto decode;
			}
			else {
				// Continue with next instruction
				m_Executed = true;
			}
			continue;

		case VirtualMachineInstr::IBAND: 
			IntTemp      = *IntOperand--;
			*IntOperand &= IntTemp;
			continue;

		case VirtualMachineInstr::LBAND: 
			LongTemp      = *LongOperand--;
			*LongOperand &= LongTemp;
			continue;
			
		case VirtualMachineInstr::IBIOR:
			IntTemp      = *IntOperand--;
			*IntOperand |= IntTemp;
			continue;

		case VirtualMachineInstr::LBIOR:
			LongTemp      = *LongOperand--;
			*LongOperand |= LongTemp;
			continue;
			
		case VirtualMachineInstr::IBXOR:
			IntTemp      = *IntOperand--;
			*IntOperand ^= IntTemp;
			continue;

		case VirtualMachineInstr::LBXOR:
			LongTemp      = *LongOperand--;
			*LongOperand ^= LongTemp;
			continue;
			
		case VirtualMachineInstr::IBNOT: 
			*IntOperand = ~*IntOperand;
			continue; 

		case VirtualMachineInstr::LBNOT: 
			*LongOperand = ~*LongOperand;
			continue; 
			
		case VirtualMachineInstr::IBSHL:
			IntTemp     = *IntOperand--;
			*IntOperand <<= IntTemp;
			continue; 
			
		case VirtualMachineInstr::IBSHR: 
			IntTemp       = *IntOperand--;
			*IntOperand >>= IntTemp;
			continue;

		case VirtualMachineInstr::LBSHL:
			IntTemp     = *IntOperand--;
			*LongOperand <<= IntTemp;
			continue; 
			
		case VirtualMachineInstr::LBSHR: 
			IntTemp     = *IntOperand--;
			*LongOperand >>= IntTemp;
			continue;
			
		case VirtualMachineInstr::IBSHL_1:
			*IntOperand <<= 1;
			continue; 
			
		case VirtualMachineInstr::IBSHR_1: 
			*IntOperand >>= 1;
			continue;

	    case VirtualMachineInstr::LBSHL_1:
			*LongOperand <<= 1;
			continue; 
			
		case VirtualMachineInstr::LBSHR_1: 
			*LongOperand >>= 1;
			continue;
			
		case VirtualMachineInstr::BROR:
			{
				int BitCount = *IntOperand--;

				if (BitCount < 0)
#ifdef THROW_EXCEPTION
					throw new STRuntimeException(11, "BROR", pc);
#else
					BitCount = 0;
#endif
				BitCount     &= 0x7;
				BYTE ByteTemp = (BYTE)*IntOperand;
				*IntOperand   = (ByteTemp << (8 - BitCount)) | (ByteTemp >> BitCount);
			}
			continue;

		case VirtualMachineInstr::WROR:
			{
				int BitCount = *IntOperand--;

				if (BitCount < 0)
#ifdef THROW_EXCEPTION
					throw new STRuntimeException(11, "WROR", pc);
#else
					BitCount = 0;
#endif
				BitCount        &= 0xf;
				USHORT ShortTemp = (USHORT)*IntOperand;
				*IntOperand      = (ShortTemp << (16 - BitCount)) | (ShortTemp >> BitCount);
			}
			continue;

		case VirtualMachineInstr::DROR:
			{
				int BitCount = *IntOperand--;

				if (BitCount < 0)
#ifdef THROW_EXCEPTION
					throw new STRuntimeException(11, "DROR", pc);
#else
					BitCount = 0;
#endif
				BitCount   &= 0x1f;
				IntTemp     = *IntOperand;
				*IntOperand = (IntTemp << (32 - BitCount)) | (IntTemp >> BitCount);
			}
			continue;

		case VirtualMachineInstr::LROR:
			{
				int BitCount = *IntOperand--;

				if (BitCount < 0)
#ifdef THROW_EXCEPTION
					throw new STRuntimeException(11, "LROR", pc);
#else
					BitCount = 0;
#endif
				BitCount    &= 0x3f;
				LongTemp     = *LongOperand;
				*LongOperand = (LongTemp << (64 - BitCount)) | (LongTemp >> BitCount);
			}
			continue;

		case VirtualMachineInstr::BROL:
			{
				int BitCount = *IntOperand--;

				if (BitCount < 0)
#ifdef THROW_EXCEPTION
					throw new STRuntimeException(11, "BROL", pc);
#else
					BitCount = 0;
#endif
				BitCount     &= 0x7;
				BYTE ByteTemp = (BYTE)*IntOperand;
				*IntOperand   = (ByteTemp << BitCount) | (ByteTemp >> (8 - BitCount));
			}
			continue;

		case VirtualMachineInstr::WROL:
			{
				int BitCount = *IntOperand--;

				if (BitCount < 0)
#ifdef THROW_EXCEPTION
					throw new STRuntimeException(11, "WROL", pc);
#else
					BitCount = 0;
#endif
				BitCount        &= 0xf;
				USHORT ShortTemp = (USHORT)*IntOperand;
				*IntOperand      = (ShortTemp << BitCount) | (ShortTemp >> (16 - BitCount));
			}
			continue;

		case VirtualMachineInstr::DROL:
			{
				int BitCount = *IntOperand--;

				if (BitCount < 0)
#ifdef THROW_EXCEPTION
					throw new STRuntimeException(11, "DROL", pc);
#else
					BitCount = 0;
#endif
				BitCount   &= 0x1f;
				IntTemp     = *IntOperand;
				*IntOperand = (IntTemp << BitCount) | (IntTemp >> (32 - BitCount));
			}
			continue;

		case VirtualMachineInstr::LROL:
			{
				int BitCount = *IntOperand--;

				if (BitCount < 0)
#ifdef THROW_EXCEPTION
					throw new STRuntimeException(11, "LROL", pc);
#else
					BitCount = 0;
#endif
				BitCount    &= 0x3f;
				LongTemp     = *LongOperand;
				*LongOperand = (LongTemp << BitCount) | (LongTemp >> (64 - BitCount));
			}
			continue;

		case VirtualMachineInstr::IINCR:
			m_IntVar[m_Operand[pc]]++;
			continue;

        case VirtualMachineInstr::IINCR0:
			s0++;
			continue;

        case VirtualMachineInstr::IINCR1:
			s1++;
			continue;

        case VirtualMachineInstr::IINCR2:
			s2++;
			continue;
 
        case VirtualMachineInstr::WINCR0:
			i0++;
			continue;
  
        case VirtualMachineInstr::WINCR1:
			i1++;
			continue;
   
        case VirtualMachineInstr::WINCR2:
			i2++;
			continue;
  
        case VirtualMachineInstr::IDECR:
			m_IntVar[m_Operand[pc]]--;
			continue;
    
        case VirtualMachineInstr::IDECR0:
			s0--;
			continue;
  
        case VirtualMachineInstr::IDECR1:
			s1--;
			continue;
   
        case VirtualMachineInstr::IDECR2:
			s2--;
			continue;
   
        case VirtualMachineInstr::WDECR0:
			i0--;
			continue;
   
        case VirtualMachineInstr::WDECR1:
			i1--;
			continue;
   
        case VirtualMachineInstr::WDECR2:
			i2--;
			continue;
   
		case VirtualMachineInstr::BLOD0:
			*++IntOperand = b0;
			continue;

		case VirtualMachineInstr::ILOD0:
			*++IntOperand = s0;
			continue;

		case VirtualMachineInstr::WLOD0:
			*++IntOperand = i0;
			continue;

		case VirtualMachineInstr::LLOD0:
			*++LongOperand = li0;
			continue;

		case VirtualMachineInstr::FLOD0:
			*++FloatOperand = f0;
			continue;

		case VirtualMachineInstr::DLOD0:
			*++DoubleOperand = d0;
			continue;

		case VirtualMachineInstr::BLOD1:
			*++IntOperand = b1;
			continue;

		case VirtualMachineInstr::ILOD1:
			*++IntOperand = s1;
			continue;

		case VirtualMachineInstr::WLOD1:
			*++IntOperand = i1;
			continue;

		case VirtualMachineInstr::LLOD1:
			*++LongOperand = li1;
			continue;

		case VirtualMachineInstr::FLOD1:
			*++FloatOperand = f1;
			continue;

		case VirtualMachineInstr::DLOD1:
			*++DoubleOperand = d1;
			continue;

		case VirtualMachineInstr::BLOD2:
			*++IntOperand = b2;
			continue;

		case VirtualMachineInstr::ILOD2:
			*++IntOperand = s2;
			continue;

		case VirtualMachineInstr::WLOD2:
			*++IntOperand = i2;
			continue;

		case VirtualMachineInstr::LLOD2:
			*++LongOperand = li2;
			continue;

		case VirtualMachineInstr::FLOD2:
			*++FloatOperand = f2;
			continue;

		case VirtualMachineInstr::DLOD2:
			*++DoubleOperand = d2;
			continue;

		case VirtualMachineInstr::BLOD3:
			*++IntOperand = b3;
			continue;

		case VirtualMachineInstr::ILOD3:
			*++IntOperand = s3;
			continue;

		case VirtualMachineInstr::WLOD3:
			*++IntOperand = i3;
			continue;

		case VirtualMachineInstr::LLOD3:
			*++LongOperand = li3;
			continue;

		case VirtualMachineInstr::FLOD3:
			*++FloatOperand = f3;
			continue;

		case VirtualMachineInstr::DLOD3:
			*++DoubleOperand = d3;
			continue;

		case VirtualMachineInstr::BSTO0:
			b0 = (BYTE)*IntOperand--;
			continue;

		case VirtualMachineInstr::ISTO0:
			s0 = (short)*IntOperand--;
			continue;

		case VirtualMachineInstr::WSTO0:
			i0 = *IntOperand--;
			continue;

		case VirtualMachineInstr::LSTO0:
			li0 = *LongOperand--;
			continue;

		case VirtualMachineInstr::FSTO0:
			f0 = *FloatOperand--;
			continue;

		case VirtualMachineInstr::DSTO0:
			d0 = *DoubleOperand--;
			continue;

		case VirtualMachineInstr::BSTO1:
			b1 = (BYTE)*IntOperand--;
			continue;

		case VirtualMachineInstr::ISTO1:
			s1 = (short)*IntOperand--;
			continue;

		case VirtualMachineInstr::WSTO1:
			i1 = *IntOperand--;
			continue;

		case VirtualMachineInstr::LSTO1:
			li1 = *LongOperand--;
			continue;

		case VirtualMachineInstr::FSTO1:
			f1 = *FloatOperand--;
			continue;

		case VirtualMachineInstr::DSTO1:
			d1 = *DoubleOperand--;
			continue;

		case VirtualMachineInstr::BSTO2:
			b2 = (BYTE)*IntOperand--;
			continue;

		case VirtualMachineInstr::ISTO2:
			s2 = (short)*IntOperand--;
			continue;

		case VirtualMachineInstr::WSTO2:
			i2 = *IntOperand--;
			continue;

		case VirtualMachineInstr::LSTO2:
			li2 = *LongOperand--;
			continue;

		case VirtualMachineInstr::FSTO2:
			f2 = *FloatOperand--;
			continue;

		case VirtualMachineInstr::DSTO2:
			d2 = *DoubleOperand--;
			continue;

		case VirtualMachineInstr::BSTO3:
			b3 = (BYTE)*IntOperand--;
			continue;

		case VirtualMachineInstr::ISTO3:
			s3 = (short)*IntOperand--;
			continue;

		case VirtualMachineInstr::WSTO3:
			i3 = *IntOperand--;
			continue;

		case VirtualMachineInstr::LSTO3:
			li3 = *LongOperand--;
			continue;

		case VirtualMachineInstr::FSTO3:
			f3 = *FloatOperand--;
			continue;

		case VirtualMachineInstr::DSTO3:
			d3 = *DoubleOperand--;
			continue;

		case VirtualMachineInstr::BLOD:
			*++IntOperand = m_ByteVar[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::ILOD: 
			*++IntOperand = m_ShortVar[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::WLOD: 
			*++IntOperand = m_IntVar[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::LLOD:
			*++LongOperand = m_LongVar[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::DLOD:
			*++DoubleOperand = m_DoubleVar[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::BSTO: 
			m_ByteVar[m_Operand[pc]] = (BYTE)*IntOperand--;
			continue;

		case VirtualMachineInstr::ISTO: 
			m_ShortVar[m_Operand[pc]] = (short)*IntOperand--;
			continue;

		case VirtualMachineInstr::WSTO: 
			m_IntVar[m_Operand[pc]] = *IntOperand--;
			continue;

		case VirtualMachineInstr::LSTO:
			m_LongVar[m_Operand[pc]] = *LongOperand--;
			continue;

		case VirtualMachineInstr::FSTO:
			m_FloatVar[m_Operand[pc]] = *FloatOperand--;
			continue;

		case VirtualMachineInstr::DSTO:
			m_DoubleVar[m_Operand[pc]] = *DoubleOperand--;
			continue;

		case VirtualMachineInstr::BLODX:
			IntTemp = *IntOperand--;
			*++IntOperand = m_ByteVar[m_Operand[pc] + IntTemp];
			continue;

		case VirtualMachineInstr::ILODX:
			IntTemp = *IntOperand--;
			*++IntOperand = m_ShortVar[m_Operand[pc] + IntTemp];
			continue;

		case VirtualMachineInstr::WLODX:
			IntTemp = *IntOperand--;
			*++IntOperand = m_IntVar[m_Operand[pc] + IntTemp];
			continue;

		case VirtualMachineInstr::LLODX:
			*++LongOperand = m_LongVar[m_Operand[pc] + *IntOperand--];
			continue;

		case VirtualMachineInstr::FLODX: 
			*++FloatOperand = m_FloatVar[m_Operand[pc] + *IntOperand--];
			continue;

		case VirtualMachineInstr::DLODX:
			*++DoubleOperand = m_DoubleVar[m_Operand[pc] + *IntOperand--];
			continue;

		case VirtualMachineInstr::SLODX:
			*++StringOperand = m_StringVar[m_Operand[pc] + *IntOperand--];
			continue;

		case VirtualMachineInstr::WSLODX:
			*++WStringOperand = m_WStringVar[m_Operand[pc] + *IntOperand--];
			continue;

		case VirtualMachineInstr::BSTOX: 
			IntTemp = *IntOperand--;
			m_ByteVar[m_Operand[pc] + IntTemp] = (BYTE)*IntOperand--;
			continue;

		case VirtualMachineInstr::ISTOX: 
			IntTemp = *IntOperand--;
			m_ShortVar[m_Operand[pc] + IntTemp] = (short)*IntOperand--;
			continue;

		case VirtualMachineInstr::WSTOX: 
			IntTemp = *IntOperand--;
			m_IntVar[m_Operand[pc] + IntTemp] = *IntOperand--;
			continue;

		case VirtualMachineInstr::LSTOX:
			m_LongVar[m_Operand[pc] + *IntOperand--] = *LongOperand--;
			continue;

		case VirtualMachineInstr::FSTOX:
			m_FloatVar[m_Operand[pc] + *IntOperand--] = *FloatOperand--;
			continue;

		case VirtualMachineInstr::DSTOX:
			m_DoubleVar[m_Operand[pc] + *IntOperand--] = *DoubleOperand--;
			continue;

		case VirtualMachineInstr::SSTO:
			IntTemp = *IntOperand--; // Size
			strcpy_s(m_StringVar[m_Operand[pc]], IntTemp, *StringOperand--);
			continue;

		case VirtualMachineInstr::WSSTO:
			IntTemp = *IntOperand--; // Size
			wcscpy_s(m_WStringVar[m_Operand[pc]], IntTemp, *WStringOperand--);
			continue;

		case VirtualMachineInstr::SSTOX:
			{
				IntTemp = *IntOperand--;
				int Size = *IntOperand--;
				strcpy_s(m_StringVar[m_Operand[pc] + IntTemp], Size, *StringOperand--);
			}
			continue;

		case VirtualMachineInstr::WSSTOX:
			{
				IntTemp = *IntOperand--;
				int Size = *IntOperand--;
				wcscpy_s(m_WStringVar[m_Operand[pc] + IntTemp], Size, *WStringOperand--);
			}
			continue;
			
		case VirtualMachineInstr::RETN: 
			return 0;
			
	    case VirtualMachineInstr::IDUPL:
			IntTemp = *IntOperand++;
			*IntOperand = IntTemp;
			continue;

		case VirtualMachineInstr::LDUPL:
			LongTemp = *LongOperand++;
			*LongOperand = LongTemp;
			continue;

		case VirtualMachineInstr::FDUPL:
			FloatTemp = *FloatOperand++;
			*FloatOperand = FloatTemp;
			continue;

		case VirtualMachineInstr::DDUPL: 
			DoubleTemp = *DoubleOperand++;
			*DoubleOperand = DoubleTemp;
			continue;
			
		case VirtualMachineInstr::F2I: 
			*++IntOperand = int(*FloatOperand--);
			continue;
			
		case VirtualMachineInstr::I2F:
			*++FloatOperand = float(*IntOperand--);
			continue;

		case VirtualMachineInstr::D2F: 
			*++FloatOperand = float(*DoubleOperand--);
			continue;

		case VirtualMachineInstr::I2D: 
			*++DoubleOperand = *IntOperand--;
			continue;

		case VirtualMachineInstr::I2L:
			*++LongOperand = *IntOperand--;
			continue;

		case VirtualMachineInstr::L2D:
			*++DoubleOperand = double(*LongOperand--);
			continue;

		case VirtualMachineInstr::F2D:
			*++DoubleOperand = *FloatOperand--;
			continue;

		case VirtualMachineInstr::FLOD:
			*++FloatOperand = m_FloatVar[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::FCONST:
			*++FloatOperand = m_FloatConst[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::FCONST_0:
			*++FloatOperand = 0.0f;
			continue;

		case VirtualMachineInstr::FCONST_1:
			*++FloatOperand = 1.0f;
			continue;

		case VirtualMachineInstr::FCONST_N1:
			*++FloatOperand = -1.0f;
			continue;

		case VirtualMachineInstr::DCONST:
			*++DoubleOperand = m_DoubleConst[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::DCONST0:
			*++DoubleOperand = m_DoubleConst0;
			continue;

		case VirtualMachineInstr::DCONST1:
			*++DoubleOperand = m_DoubleConst1;
			continue;

		case VirtualMachineInstr::DCONST2:
			*++DoubleOperand = m_DoubleConst2;
			continue;

		case VirtualMachineInstr::DCONST3:
			*++DoubleOperand = m_DoubleConst3;
			continue;

		case VirtualMachineInstr::DCONST_0:
			*++DoubleOperand = 0.0;
			continue;

		case VirtualMachineInstr::DCONST_1:
			*++DoubleOperand = 1.0;
			continue;

		case VirtualMachineInstr::DCONST_N1:
			*++DoubleOperand = -1.0;
			continue;

		case VirtualMachineInstr::ICONST:
			*++IntOperand = (short)m_Operand[pc];
			continue;

		case VirtualMachineInstr::WCONST:
			*++IntOperand = m_IntConst[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::LCONST:
			*++LongOperand = m_LongConst[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::SCONST:
			*++StringOperand = (char *)m_StringConst[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::WSCONST:
			*++WStringOperand = (wchar_t*)m_WStringConst[m_Operand[pc]];
			continue;

		case VirtualMachineInstr::ICONST_0:
			*++IntOperand = 0;
			continue;

		case VirtualMachineInstr::ICONST_1:
			*++IntOperand = 1;
			continue;

		case VirtualMachineInstr::ICONST_N1:
			*++IntOperand = -1;
			continue;

		case VirtualMachineInstr::LCONST_0:
			*++LongOperand = 0;
			continue;

		case VirtualMachineInstr::LCONST_1:
			*++LongOperand = 1;
			continue;

		case VirtualMachineInstr::LCONST_N1:
			*++LongOperand = -1;
			continue;

		case VirtualMachineInstr::RANGE_CHECK:
			{
				// Check that the index are within bounds.
				int LowerBound = *IntOperand--;
				int UpperBound = *IntOperand--;
				IntTemp        = *IntOperand;
				if (IntTemp < LowerBound || IntTemp > UpperBound)
				{
				#ifdef THROW_EXCEPTION
					throw new STRuntimeException(3);
                #else
					 *IntOperand = LowerBound;
				#endif
				}
			}
			continue;
			
		case VirtualMachineInstr::JSBR:  // Call user defined function/functionblock
			m_ExternProgOrgUnit[m_Operand[pc]]->Execute();
			continue;

		case VirtualMachineInstr::ROM_COPY: // RO data segment to RW data segment copy.
			{
				int Count           = *IntOperand--;
				int SrcOffset       = *IntOperand--;
				int DstOffset       = *IntOperand--;
				BYTE* DstAddr       = m_RWDataSegment + DstOffset;
				const BYTE* SrcAddr = m_RODataSegment + SrcOffset;
				while (Count-- > 0)
					*DstAddr++ = *SrcAddr++;
			}
			continue;

		case VirtualMachineInstr::MEM_COPY: // RW data segment to RW data segment copy.
			{
				int Count           = *IntOperand--;
				int DstOffset       = *IntOperand--;
				int SrcOffset       = *IntOperand--;
				BYTE* DstAddr       = m_RWDataSegment + DstOffset;
				const BYTE* SrcAddr = m_RWDataSegment + SrcOffset;
				while (Count-- > 0)
					*DstAddr++ = *SrcAddr++;
			}
			continue;

		case VirtualMachineInstr::MEM_SETZ: // MEMory SET to Zero.
			{
				int Offset    = *IntOperand--;
				int Count     = *IntOperand--;
				BYTE* DstAddr = m_RWDataSegment + Offset;
				while (Count-- > 0)
					*DstAddr++ = 0;
			}
			continue;

		case VirtualMachineInstr::MEM_SETB: // MEMory SET to Bytes = 1, 2, 4 or 8 bytes.
			{
				char ElemSize = (char)m_Operand[pc];
				int ElemCount = *IntOperand--;
				int Offset    = *IntOperand--;
				
				switch (ElemSize)
				{
				case 1:
					{
						const BYTE Byte = (BYTE)*IntOperand--;
						BYTE* DstAddr   = m_RWDataSegment + Offset;
						while (ElemCount-- > 0)
							*DstAddr++ = Byte;
					}
					break;

				case 2:
					{
						const short Short = (short)*IntOperand--;
						short* DstAddr    = (short *)(m_RWDataSegment + Offset);
						while (ElemCount-- > 0)
							*DstAddr++ = Short;
					}
					break;

				case 4:
					{
						const int Int = *IntOperand--;
						int* DstAddr  = (int *)(m_RWDataSegment + Offset);
						while (ElemCount-- > 0)
							*DstAddr++ = Int;
					}
					break;

				case 8:
					{
						const int64 Long = *LongOperand--;
						int64* DstAddr   = (int64 *)(m_RWDataSegment + Offset);
						while (ElemCount-- > 0)
							*DstAddr++ = Long;
					}
					break;

				default:
					break;
				}
				
			}
			continue;

		case VirtualMachineInstr::NOOP:
			continue;

		default:
			#ifdef THROW_EXCEPTION
				throw new STRuntimeException(1, pc, m_OpCode[pc]);
			#endif
				;
		}
	}
	return 0;
}
