#include "pch.h"
#include <list>
#include <fstream>
#include "TypeDefs.h"
#include "STLangRuntime.h"
#include "STLangPOUObject.h"
#include "STLangExceptions.h"
#include "STLangObjectFileDefines.h"

struct ExecutableData
{
	ExecutableData()
	{
		Operands = 0;
		CodeSegment = 0;
		RODataSegment = 0;
		Inputs = 0;
		Outputs = 0;
		Retained = 0;
		IndexMap = 0;
		Initializers = 0;
		StringVar = 0;
		ExternalPOUs = 0;
		LongConstants = 0;
		DoubleConstants = 0;
		FloatConstants = 0;
		IntConstants = 0;
		StringConstants = 0;
		WStringConstants = 0;
	}

	int *Operands;
	string POUName;
	BYTE *CodeSegment;
	BYTE *RODataSegment;
	set<string> *Inputs;
	set<string> *Outputs;
	set<string> *Retained;
	double D0, D1, D2, D3;
	const double* DoubleConstants;
	const float* FloatConstants;
	const int* IntConstants;
	const int64* LongConstants;
	const char** StringConstants;
	const wchar_t** WStringConstants;
	VariableIndexMap *IndexMap;
	ExecutableFileHeader FileHeader;
	list<ExternalSymbol> *ExternalPOUs;
	list<InitializerData> *Initializers;
	list<StringVariableData> *StringVar;
};

typedef map<string, ExecutableData> ExecutableDictionary;

typedef ExecutableDictionary::iterator DictionaryIterator;

typedef list<StringVariableData>::iterator StringDataIterator;

typedef list<InitializerData>::iterator InitializerDataIterator;

static ExecutableDictionary ExecutableTable;

template <typename StructType>
void CopyToStruct (
	StructType& Struct, 
	const BYTE Bytes[])
{
	int ByteCount = sizeof(StructType);
	BYTE* StructPtr = (BYTE *)&Struct;

	while (ByteCount-- > 0)
		*StructPtr++ = *Bytes++;
}

string ToUpper(const char* String)
{
	string TempString = "";

	for (char ch = *String++; ch != '\0'; ch = *String++)
	{
		if (ch >= 'a' && ch <= 'z')
			ch = ch - 32;
		else if (ch == 'å')
			ch = (char)'Å';
		else if (ch == 'ä')
			ch = (char)'Ä';
		else if (ch == 'ö')
			ch = (char)'Ö';
		TempString += ch;
	}
	return TempString;
}


// STLangSubPOUObject* CreateSubPOUObject(const char* ExeFile, ...)
//
// Create a POU-object for POUs called from within another POU. Instances of 
// functionblocks declared inside other functionblocks share the same
// RW data segment and operand stack pointers as the caller.

STLangSubPOUObject* CreateSubPOUObject (
	const char* ExeFile,
	BYTE* RWDataSegment,
	double* const LRealVar, 
	__int64* const LIntVar,
	float* const RealVar,
	int* const DIntVar,
	wchar_t** const WStringVar,
	char** const StringVar,
	short* const IntVar,
	char* const ByteVar,
	int*& IntOperand, // int stack pointer
	float*& FloatOperand, // float stack pointer
	double*& DoubleOperand,
	int64*& LongOperand,
	char**& StringOperand,
	wchar_t**& WStringOperand)
{
	int POUType;
	int* Operands;
	string POUName;
	int ExternalCount;
	int CodeSegmentSize;
	int RODataSegmentSize;
	int RWDataSegmentSize;
	double D0, D1, D2, D3;
	VariableIndexMap *IndexMap;
	const int* DIntConst;
	const float* RealConst;
	const int64* LIntConst;
	const double* DoubleConst;
	const char** StringConst = NULL;
	const wchar_t** WStringConst = NULL;
	ExecutableFileHeader FileHeader;
	BYTE *CodeSegment, *RODataSegment;
	list<ExternalSymbol> *ExternalPOUs;
	list<InitializerData> *Initializers;
	list<StringVariableData> *StringVarData;
	set<string> *InputSet, *OutputSet, *RetainedSet;
	DictionaryIterator Iter = ExecutableTable.find(ExeFile);

	if (Iter != ExecutableTable.end())
	{
		const ExecutableData &Executable = Iter->second;
		POUName           = Executable.POUName;
		CodeSegment       = Executable.CodeSegment;
		RODataSegment     = Executable.RODataSegment;
		Operands          = Executable.Operands;
		InputSet          = Executable.Inputs;
		OutputSet         = Executable.Outputs;
		RetainedSet       = Executable.Retained;
		IndexMap          = Executable.IndexMap;
		D0                = Executable.D0;
		D1                = Executable.D1;
		D2                = Executable.D2;
		D3                = Executable.D3;
		DIntConst         = Executable.IntConstants;
		RealConst         = Executable.FloatConstants;
		LIntConst         = Executable.LongConstants;
		DoubleConst       = Executable.DoubleConstants;
		StringConst       = Executable.StringConstants;
		WStringConst      = Executable.WStringConstants;
		ExternalPOUs      = Executable.ExternalPOUs;
		Initializers      = Executable.Initializers;
		StringVarData     = Executable.StringVar;
		FileHeader        = Executable.FileHeader;
		POUType           = FileHeader.ExecutableType;
		RODataSegmentSize = FileHeader.RODataSegmentSize;
		RWDataSegmentSize = FileHeader.RWDataSegmentSize;
		CodeSegmentSize   = FileHeader.CodeSegmentSize;
		ExternalCount     = FileHeader.ExternalCount;
	}
	else {
		const string ExecutableFile = ExeFile;
		ifstream InStream(ExecutableFile, ifstream::binary);

		if (! InStream)
			throw new STLoadException(7, ExecutableFile); // Can't open file
		else {
			InStream.read((char *)&FileHeader, sizeof(FileHeader));
			if (! InStream)
				throw new STLoadException(2); //  Failed to read file header
			else {
				POUName           = FileHeader.POUName;
				POUType           = FileHeader.ExecutableType;
				RODataSegmentSize = FileHeader.RODataSegmentSize;
				RWDataSegmentSize = FileHeader.RWDataSegmentSize;
				CodeSegmentSize   = FileHeader.CodeSegmentSize;
				ExternalCount     = FileHeader.ExternalCount;
				Operands          = new int[CodeSegmentSize];
				CodeSegment       = new BYTE[CodeSegmentSize];
				RODataSegment     = (BYTE *)_aligned_malloc(RODataSegmentSize, __alignof(double));
				if (CodeSegment == 0)
					throw new STLoadException(7);
				else if (Operands == 0)
					throw new STLoadException(8);
				else if (RODataSegment == 0)
					throw new STLoadException(9);
				else {
					UINT Instructions[20000];
					const int CodeSize = sizeof(UINT)*CodeSegmentSize;

					InStream.read((char *)Instructions, CodeSize);
					if (! InStream)
						throw new STLoadException(6); // Failed to read code segment
					else {
						double RegisterDoubleConst[4];

						// Extract instruction + operand
						//
						for (int i = 0; i < CodeSegmentSize; i++)
						{
							UINT Instruction = Instructions[i];
							CodeSegment[i] = BYTE(Instruction >> 24);
							Operands[i] = int(Instruction & 0x00ffffff);
						}
						InStream.read((char *)RODataSegment, RODataSegmentSize);
						if (! InStream)
							throw new STLoadException(10);
						const int WStringCount = FileHeader.WStringCount;
						const int StringCount  = FileHeader.StringCount;

						// Initialize arrays of constants (doubles, floats, ints and strings)
						//
						DoubleConst            = (double *)(RODataSegment + FileHeader.ConstantDataOffset);
						LIntConst              = (__int64 *)(DoubleConst + FileHeader.LRealConstCount);
						RealConst              = (float *)(LIntConst + FileHeader.LIntConstCount);
						DIntConst              = (int *)(RealConst + FileHeader.RealConstCount);
						if (WStringCount > 0)
						{
							wchar_t* WStrPointer;
							WStringConst = (const wchar_t**)(DIntConst + FileHeader.DIntConstCount);
                          
							if (StringCount == 0)
							{
								WStrPointer = (wchar_t *)(WStringConst + WStringCount);
								for (int i = 0; i < WStringCount; i++)
								{
									int Length = (int)WStringConst[i];
									WStringConst[i] = WStrPointer;
									WStrPointer += Length;
								}
							}
							else {
								StringConst = (const char**)(WStringConst + WStringCount);
								WStrPointer = (wchar_t *)(StringConst + StringCount);
								for (int i = 0; i < WStringCount; i++)
								{
									int Length = (int)WStringConst[i];
									WStringConst[i] = WStrPointer;
									WStrPointer += Length;
								}
								const char *StrPointer = (const char*)WStrPointer; 
								for (int i = 0; i < StringCount; i++)
								{
									int Length = (int)StringConst[i];
									StringConst[i] = StrPointer;
									StrPointer += Length;
								}
							}
						}
						else if (StringCount > 0)
						{
							StringConst = (const char**)(DIntConst + FileHeader.DIntConstCount);
							const char *StrPointer = (const char*)(StringConst + StringCount);
							for (int i = 0; i < StringCount; i++)
							{
								int Length = (int)StringConst[i];
								StringConst[i] = StrPointer;
								StrPointer += Length;
							}
						}
						InStream.read((char *)RegisterDoubleConst, sizeof(RegisterDoubleConst));
						if (! InStream)
							throw new STLoadException(25);
						D0            = RegisterDoubleConst[0];
						D1            = RegisterDoubleConst[1];
						D2            = RegisterDoubleConst[2];
						D3            = RegisterDoubleConst[3];
						InputSet      = new set<string>();
						OutputSet     = new set<string>();
						RetainedSet   = new set<string>();
						IndexMap      = new VariableIndexMap();
						ExternalPOUs  = new list<ExternalSymbol>();
						Initializers  = new list<InitializerData>();
						StringVarData = new list<StringVariableData>();
						if (FileHeader.InitializerCount > 0)
						{
							InitializerData InitData;
							int InitializerCount = FileHeader.InitializerCount;

							while (InitializerCount-- > 0)
							{
								InStream.read((char *)&InitData, sizeof(InitData));
								if (! InStream)
									throw new STLoadException(22);
								else 
									Initializers->push_back(InitData);
							}
						}
						if (FileHeader.InputCount > 0)
						{
							IOParameter Input;
							int InputCount = FileHeader.InputCount;
                 
							while (InputCount-- > 0)
							{
								InStream.read((char *)&Input, sizeof(Input));
								if (! InStream)
									throw new STLoadException(19);
								else 
									InputSet->insert(Input.Name);
							}
						}
						if (FileHeader.OutputCount > 0)
						{
							IOParameter Output;
							int OutputCount = FileHeader.OutputCount;

							while (OutputCount-- > 0)
							{
								InStream.read((char *)&Output, sizeof(Output));
								if (! InStream)
									throw new STLoadException(20);
								else {
									string Name = ToUpper(Output.Name);
									pair<string, int> Pair(Name, Output.Index);
									IndexMap->insert(Pair);
									OutputSet->insert(Output.Name);
								}
							}
						}
						if (FileHeader.RetainedCount > 0)
						{
							IOParameter RetainedVar;
							int RetainedCount = FileHeader.RetainedCount;

							while (RetainedCount-- > 0)
							{
								InStream.read((char *)&RetainedVar, sizeof(RetainedVar));
								if (! InStream)
									throw new STLoadException(21);
								else {
									string Name = ToUpper(RetainedVar.Name);
									pair<string, int> Pair(Name, RetainedVar.Index);
									IndexMap->insert(Pair);
									RetainedSet->insert(RetainedVar.Name);
								}
							}
						}
						if (ExternalCount > 0)
						{
							ExternalSymbol POU;
							int Count = ExternalCount;

							while (Count-- > 0)
							{        
								InStream.read((char *)&POU, sizeof(POU));
								if (! InStream)
									throw new STLoadException(23);
								else 
									ExternalPOUs->push_back(POU);
							}
						}
						if (FileHeader.StringDataCount > 0)
						{
							int StringDataCount;
							StringVariableData StringData;

							StringDataCount = FileHeader.StringDataCount;
							while (StringDataCount-- > 0)
							{
								InStream.read((char *)&StringData, sizeof(StringData));
								if (! InStream)
									throw new STLoadException(24);
								else
									StringVarData->push_back(StringData);
							}
						}
						//
						// Save executable data in table.
						//
						ExecutableData Executable;
						Executable.POUName          = POUName;
						Executable.CodeSegment      = CodeSegment;
						Executable.RODataSegment    = RODataSegment;
						Executable.Operands         = Operands;
						Executable.Inputs           = InputSet;
						Executable.Outputs          = OutputSet;
						Executable.Retained         = RetainedSet;
						Executable.IndexMap         = IndexMap;
						Executable.Initializers     = Initializers;
						Executable.StringVar        = StringVarData;
						Executable.ExternalPOUs     = ExternalPOUs;
						Executable.D0               = D0;
						Executable.D1               = D1;
						Executable.D2               = D2;
						Executable.D3               = D3;
		                Executable.IntConstants     = DIntConst;
						Executable.LongConstants    = LIntConst;
		                Executable.FloatConstants   = RealConst;
		                Executable.DoubleConstants  = DoubleConst;
		                Executable.StringConstants  = StringConst;
		                Executable.WStringConstants = WStringConst;
						Executable.FileHeader       = FileHeader;
						pair<string, ExecutableData> Pair(ExecutableFile, Executable);
						ExecutableTable.insert(Pair);
					}
				}
			}
		}
		InStream.close();
	}
	// Initialize RW segment with constant data
	if (FileHeader.InitializerCount > 0)
	{
		InitializerDataIterator Initializer;

		Initializer = Initializers->begin();
		while (Initializer != Initializers->end())
		{
			int ByteCount = Initializer->ByteCount;
			int SrcOffset = Initializer->RODataSegmentOffset;
			int DstOffset = Initializer->RWDataSegmentOffset;
			if (SrcOffset > RODataSegmentSize)
				throw new STLoadException(13);
			else if (DstOffset > RWDataSegmentSize)
				throw new STLoadException(14);
			else if (SrcOffset + ByteCount > RODataSegmentSize)
				throw new STLoadException(15);
			else if (DstOffset + ByteCount > RWDataSegmentSize)
				throw new STLoadException(16);
			else {
				void* SrcAddress = RODataSegment + SrcOffset;
				void* DstAddress = RWDataSegment + DstOffset;
				memcpy(DstAddress, SrcAddress, ByteCount);
			}
			Initializer++;
		}
	}
	STLangSubPOUObject** Externals = 0;
	const int InputCount           = FileHeader.InputCount;
	const int OutputCount          = FileHeader.OutputCount;

	if (FileHeader.StringDataCount > 0)
	{
		// Initialize string variables StringVar[i] and WStringVar[i] i = 0,1,2,...
		// so that they point at resp. string buffer where the string is stored.

		StringDataIterator String;
	
		String = StringVarData->begin();
		while (String != StringVarData->end())
		{
			int i = String->StringIndex;
			int Count = String->ElementCount;
			if (String->StringType == 0)
			{
				char* StringBuffer;
				StringBuffer = ByteVar + String->BufferOffset;
				StringVar[i++] = StringBuffer;
				if (Count > 1)
				{
					Count--;
					const int BufferSize = String->BufferSize;
					while (Count > 0)
					{
						StringBuffer += BufferSize;
						StringVar[i++] = StringBuffer;
						Count--;
					}
				}
			}
			else {
				wchar_t* WStringBuffer;
				WStringBuffer = (wchar_t*)(IntVar + String->BufferOffset);
				WStringVar[i++] = WStringBuffer;
				if (Count > 1)
				{
					Count--;
					const int BufferSize = String->BufferSize;
					while (Count > 0)
					{
						WStringBuffer += BufferSize;
						WStringVar[i++] = WStringBuffer;
						Count--;
					}
				}
			}
			String++;
		}
	}
	if (ExternalCount > 0)
	{
		// Create POU-objects for all external functions/function blocks called
		// from within this POU.
		int i;
		STLangSubPOUObject *POUObject;
		list<ExternalSymbol>::iterator POU;

		i = 0;
		POU = ExternalPOUs->begin();
		Externals = new STLangSubPOUObject*[ExternalCount];
		while (POU != ExternalPOUs->end())
		{   
			POUObject = CreateSubPOUObject(POU->Name, RWDataSegment, LRealVar + POU->LRealOffset, 
					                       LIntVar + POU->LIntOffset, RealVar + POU->RealOffset, 
										   DIntVar + POU->DIntOffset, WStringVar + POU->WStringOffset, 
										   StringVar + POU->StringOffset, IntVar + POU->IntOffset, ByteVar + POU->ByteOffset,
										   IntOperand, FloatOperand, DoubleOperand, LongOperand, StringOperand, WStringOperand);
			Externals[i++] = POUObject;
			POU++;
		}
	}
	return new STLangSubPOUObject 
				(
					POUName, POUType, CodeSegment, RODataSegment, Operands, RWDataSegment, 
					ByteVar, IntVar, DIntVar, RealVar, LIntVar, LRealVar,
					StringVar, WStringVar, DIntConst, RealConst, LIntConst, 
					DoubleConst, StringConst, WStringConst, D0, D1, D2, D3,
					InputCount, OutputCount, *InputSet, *OutputSet, *RetainedSet,
					*IndexMap, Externals, ExternalCount, IntOperand, FloatOperand, 
					DoubleOperand, LongOperand, StringOperand, WStringOperand
				);
}

int CreatePOUObject(char* ExeFile)
{
	int POUType;
	int* Operands;
	string POUName;
	int ExternalCount;
	int CodeSegmentSize;
	int RODataSegmentSize;
	int RWDataSegmentSize;
	double D0, D1, D2, D3;
	VariableIndexMap *IndexMap;
	const int* DIntConst;
	const float* RealConst;
	const __int64* LIntConst;
	const double* DoubleConst;
	const char** StringConst = NULL;
	const wchar_t** WStringConst = NULL;
	ExecutableFileHeader FileHeader;
	list<ExternalSymbol> *ExternalPOUs;
	list<InitializerData> *Initializers;
	list<StringVariableData> *StringVarData;
	set<string> *InputSet, *OutputSet, *RetainedSet;
	BYTE *CodeSegment, *RODataSegment, *RWDataSegment;
	DictionaryIterator Iter = ExecutableTable.find(ExeFile);

	if (Iter != ExecutableTable.end())
	{
		const ExecutableData &Executable = Iter->second;
		POUName           = Executable.POUName;
		CodeSegment       = Executable.CodeSegment;
		RODataSegment     = Executable.RODataSegment;
		Operands          = Executable.Operands;
		InputSet          = Executable.Inputs;
		OutputSet         = Executable.Outputs;
		RetainedSet       = Executable.Retained;
		IndexMap          = Executable.IndexMap;
		D0                = Executable.D0;
		D1                = Executable.D1;
		D2                = Executable.D2;
		D3                = Executable.D3;
		DIntConst         = Executable.IntConstants;
		LIntConst         = Executable.LongConstants;
		RealConst         = Executable.FloatConstants;
		DoubleConst       = Executable.DoubleConstants;
		StringConst       = Executable.StringConstants;
		WStringConst      = Executable.WStringConstants;
		ExternalPOUs      = Executable.ExternalPOUs;
		Initializers      = Executable.Initializers;
		StringVarData     = Executable.StringVar;
		FileHeader        = Executable.FileHeader;
		POUType           = FileHeader.ExecutableType;
		RODataSegmentSize = FileHeader.RODataSegmentSize;
		RWDataSegmentSize = FileHeader.RWDataSegmentSize;
		CodeSegmentSize   = FileHeader.CodeSegmentSize;
		ExternalCount     = FileHeader.ExternalCount;
	}
	else {
		const string ExecutableFile = ExeFile;
		ifstream InStream(ExecutableFile, ifstream::binary);

		if (! InStream)
			throw new STLoadException(7, ExecutableFile); // Can't open file
		else {
			InStream.read((char *)&FileHeader, sizeof(FileHeader));
			if (! InStream)
				throw new STLoadException(2); //  Failed to read file header
			else {
				POUName           = FileHeader.POUName;
				RODataSegmentSize = FileHeader.RODataSegmentSize;
				RWDataSegmentSize = FileHeader.RWDataSegmentSize;
				CodeSegmentSize   = FileHeader.CodeSegmentSize;
				ExternalCount     = FileHeader.ExternalCount;
				Operands          = new int[CodeSegmentSize];
				CodeSegment       = new BYTE[CodeSegmentSize];
				RODataSegment     = (BYTE *)_aligned_malloc(RODataSegmentSize, __alignof(double));
				if (CodeSegment == 0)
					throw new STLoadException(7);
				else if (Operands == 0)
					throw new STLoadException(8);
				else if (RODataSegment == 0)
					throw new STLoadException(9);
				else {
					const int CodeSize = sizeof(UINT)*CodeSegmentSize;
					const UINT* Instructions = new UINT[CodeSegmentSize];

					InStream.read((char *)Instructions, CodeSize);
					if (! InStream)
						throw new STLoadException(6); // Failed to read code segment
					else {
						double RegisterDoubleConst[4];
						// Extract instruction + operand
						for (int i = 0; i < CodeSegmentSize; i++)
						{
							UINT Instruction = Instructions[i];
							CodeSegment[i] = BYTE(Instruction >> 24);
							Operands[i] = int(Instruction & 0x00ffffff);
						}
						InStream.read((char *)RODataSegment, RODataSegmentSize);
						if (! InStream)
							throw new STLoadException(10);
						const int WStringCount = FileHeader.WStringCount;
						const int StringCount  = FileHeader.StringCount;

						// Initialize arrays of constants (doubles, floats, ints and strings)
						//
						DoubleConst            = (double *)(RODataSegment + FileHeader.ConstantDataOffset);
						LIntConst              = (__int64 *)(DoubleConst + FileHeader.LRealConstCount);
						RealConst              = (float *)(LIntConst + FileHeader.LIntConstCount);
						DIntConst              = (int *)(RealConst + FileHeader.RealConstCount);
						if (WStringCount > 0)
						{
							wchar_t* WStrPointer;
							WStringConst = (const wchar_t**)(DIntConst + FileHeader.DIntConstCount);
                          
							if (StringCount == 0)
							{
								WStrPointer = (wchar_t *)(WStringConst + WStringCount);
								for (int i = 0; i < WStringCount; i++)
								{
									int Length = (int)WStringConst[i];
									WStringConst[i] = WStrPointer;
									WStrPointer += Length;
								}
							}
							else {
								StringConst = (const char**)(WStringConst + WStringCount);
								WStrPointer = (wchar_t *)(StringConst + StringCount);
								for (int i = 0; i < WStringCount; i++)
								{
									int Length = (int)WStringConst[i];
									WStringConst[i] = WStrPointer;
									WStrPointer += Length;
								}
								const char *StrPointer = (const char*)WStrPointer; 
								for (int i = 0; i < StringCount; i++)
								{
									int Length = (int)StringConst[i];
									StringConst[i] = StrPointer;
									StrPointer += Length;
								}
							}
						}
						else if (StringCount > 0)
						{
							StringConst = (const char**)(DIntConst + FileHeader.DIntConstCount);
							const char *StrPointer = (const char*)(StringConst + StringCount);
							for (int i = 0; i < StringCount; i++)
							{
								int Length = (int)StringConst[i];
								StringConst[i] = StrPointer;
								StrPointer += Length;
							}
						}
						InStream.read((char *)RegisterDoubleConst, sizeof(RegisterDoubleConst));
						if (! InStream)
							throw new STLoadException(25);

						// Double register constants
						//
						D0            = RegisterDoubleConst[0]; 
						D1            = RegisterDoubleConst[1];
						D2            = RegisterDoubleConst[2];
						D3            = RegisterDoubleConst[3];
						InputSet      = new set<string>();
						OutputSet     = new set<string>();
						RetainedSet   = new set<string>();
						IndexMap      = new VariableIndexMap();
						ExternalPOUs  = new list<ExternalSymbol>();
						Initializers  = new list<InitializerData>();
						StringVarData = new list<StringVariableData>();
						if (FileHeader.InitializerCount > 0)
						{
							InitializerData InitData;
							int InitializerCount = FileHeader.InitializerCount;

							while (InitializerCount-- > 0)
							{
								InStream.read((char *)&InitData, sizeof(InitData));
								if (! InStream)
									throw new STLoadException(22);
								else 
									Initializers->push_back(InitData);
							}
						}
						if (FileHeader.InputCount > 0)
						{
							IOParameter Input;
							int InputCount = FileHeader.InputCount;
                 
							while (InputCount-- > 0)
							{
								InStream.read((char *)&Input, sizeof(Input));
								if (! InStream)
									throw new STLoadException(19);
								else 
									InputSet->insert(Input.Name);
							}
						}
						if (FileHeader.OutputCount > 0)
						{
							IOParameter Output;
							int OutputCount = FileHeader.OutputCount;

							while (OutputCount-- > 0)
							{
								InStream.read((char *)&Output, sizeof(Output));
								if (! InStream)
									throw new STLoadException(20);
								else {
									string Name = ToUpper(Output.Name);
									pair<string, int> Pair(Name, Output.Index);
									IndexMap->insert(Pair);
									OutputSet->insert(Output.Name);
								}
							}
						}
						if (FileHeader.RetainedCount > 0)
						{
							IOParameter RetainedVar;
							int RetainedCount = FileHeader.RetainedCount;

							while (RetainedCount-- > 0)
							{
								InStream.read((char *)&RetainedVar, sizeof(RetainedVar));
								if (! InStream)
									throw new STLoadException(21);
								else {
									string Name = ToUpper(RetainedVar.Name);
									pair<string, int> Pair(Name, RetainedVar.Index);
									IndexMap->insert(Pair);
									RetainedSet->insert(RetainedVar.Name);
								}
							}
						}
						if (ExternalCount > 0)
						{
							int Count = ExternalCount;
							ExternalSymbol POU;

							while (Count-- > 0)
							{        
								InStream.read((char *)&POU, sizeof(POU));
								if (! InStream)
									throw new STLoadException(23);
								else 
									ExternalPOUs->push_back(POU);
							}
						}
						if (FileHeader.StringDataCount > 0)
						{
							int StringDataCount;
							StringVariableData StringData;

							StringDataCount = FileHeader.StringDataCount;
							while (StringDataCount-- > 0)
							{
								InStream.read((char *)&StringData, sizeof(StringData));
								if (!InStream)
									//throw new STLoadException(24);
									return StringDataCount;
								else
									StringVarData->push_back(StringData);
							}
						}
						// Save executable data in table.
						//
						ExecutableData Executable;
						Executable.POUName          = POUName;
						Executable.CodeSegment      = CodeSegment;
						Executable.RODataSegment    = RODataSegment;
						Executable.Operands         = Operands;
						Executable.Inputs           = InputSet;
						Executable.Outputs          = OutputSet;
						Executable.Retained         = RetainedSet;
						Executable.IndexMap         = IndexMap;
						Executable.Initializers     = Initializers;
						Executable.StringVar        = StringVarData;
						Executable.ExternalPOUs     = ExternalPOUs;
						Executable.D0               = D0;
						Executable.D1               = D1;
						Executable.D2               = D2;
						Executable.D3               = D3;
		                Executable.IntConstants     = DIntConst;
						Executable.LongConstants    = LIntConst;
		                Executable.FloatConstants   = RealConst;
		                Executable.DoubleConstants  = DoubleConst;
		                Executable.StringConstants  = StringConst;
		                Executable.WStringConstants = WStringConst;
						Executable.FileHeader       = FileHeader;
						pair<string, ExecutableData> Pair(ExecutableFile, Executable);
						ExecutableTable.insert(Pair);
					}
				}
			}
		}
	}
	// Allocate memory for local variables
	RWDataSegment = (BYTE *)_aligned_malloc(RWDataSegmentSize, __alignof(double));
	if (RWDataSegment == 0)
		throw new STLoadException(11);
	// Initialize RW segment with constant data
	if (FileHeader.InitializerCount > 0)
	{
		InitializerDataIterator Initializer;

		Initializer = Initializers->begin();
		while (Initializer != Initializers->end())
		{
			int ByteCount = Initializer->ByteCount;
			int SrcOffset = Initializer->RODataSegmentOffset;
			int DstOffset = Initializer->RWDataSegmentOffset;
			if (SrcOffset > RODataSegmentSize)
				throw new STLoadException(13);
			else if (DstOffset > RWDataSegmentSize)
				throw new STLoadException(14);
			else if (SrcOffset + ByteCount > RODataSegmentSize)
				throw new STLoadException(15);
			else if (DstOffset + ByteCount > RWDataSegmentSize)
				throw new STLoadException(16);
			else {
				void* SrcAddress = RODataSegment + SrcOffset;
				void* DstAddress = RWDataSegment + DstOffset;
				memcpy(DstAddress, SrcAddress, ByteCount);
			}
			Initializer++;
		}
	}
	STLangSubPOUObject** Externals = 0;
	const int InputCount       = FileHeader.InputCount;
	const int OutputCount      = FileHeader.OutputCount;
	double* const LRealVar     = (double *)RWDataSegment;
	__int64* const LIntVar     = (__int64 *)(LRealVar + FileHeader.LRealVarCount);
	float* const RealVar       = (float *)(LIntVar + FileHeader.LIntVarCount);
	int* const DIntVar         = (int *)(RealVar + FileHeader.RealVarCount);
	wchar_t** const WStringVar = (wchar_t **)(DIntVar + FileHeader.DIntVarCount);
	char** const StringVar     = (char**)(WStringVar + FileHeader.WStringVarCount);
	short* const IntVar        = (short *)(StringVar + FileHeader.StringVarCount);
	char* const ByteVar        = (char *)(IntVar + FileHeader.IntVarCount);

	if (FileHeader.StringDataCount > 0)
	{
		// Initialize string variables StringVar[i] and WStringVar[i] i = 0,1,2,...
		// so that they point at resp. string buffer where the string is stored.

		StringDataIterator String;
	
		String = StringVarData->begin();
		while (String != StringVarData->end())
		{
			int i = String->StringIndex;
			int Count = String->ElementCount;
			if (String->StringType == 0)
			{
				char* StringBuffer;
				StringBuffer = ByteVar + String->BufferOffset;
				StringVar[i++] = StringBuffer;
				if (Count > 1)
				{
					Count--;
					const int BufferSize = String->BufferSize;
					while (Count > 0)
					{
						StringBuffer += BufferSize;
						StringVar[i++] = StringBuffer;
						Count--;
					}
				}
			}
			else {
				wchar_t* WStringBuffer;
				WStringBuffer = (wchar_t*)(IntVar + String->BufferOffset);
				WStringVar[i++] = WStringBuffer;
				if (Count > 1)
				{
					Count--;
					const int BufferSize = String->BufferSize;
					while (Count > 0)
					{
						WStringBuffer += BufferSize;
						WStringVar[i++] = WStringBuffer;
						Count--;
					}
				}
			}
			String++;
		}
	}
	STLangPOUObject* TopLevelPOUObject;
	if (ExternalCount == 0)
	{
		TopLevelPOUObject = new STLangPOUObject 
							(
								POUName, POUType, CodeSegment, RODataSegment, Operands, RWDataSegment, 
								ByteVar, IntVar, DIntVar, RealVar, LIntVar, LRealVar,
								StringVar, WStringVar, DIntConst, RealConst, LIntConst, 
								DoubleConst, StringConst, WStringConst, D0, D1, D2, D3,
								InputCount, OutputCount, *InputSet, *OutputSet, *RetainedSet, *IndexMap
							);
	}
	else {
		// Create (sub)POU-objects for all external, user defined functions/function blocks called
		// from within this POU. 

		int i;
		STLangSubPOUObject *SubPOUObject;
		list<ExternalSymbol>::iterator POU;

		i = 0;
		POU = ExternalPOUs->begin();
		Externals = new STLangSubPOUObject*[ExternalCount];
		TopLevelPOUObject = new STLangPOUObject 
							(
								POUName, POUType, CodeSegment, RODataSegment, Operands, RWDataSegment, 
								ByteVar, IntVar, DIntVar, RealVar, LIntVar, LRealVar,
								StringVar, WStringVar, DIntConst, RealConst, LIntConst, 
								DoubleConst, StringConst, WStringConst, D0, D1, D2, D3,
								InputCount, OutputCount, *InputSet, *OutputSet, *RetainedSet,
								*IndexMap, Externals, ExternalCount
							);
		while (POU != ExternalPOUs->end())
		{   
			SubPOUObject = CreateSubPOUObject (
				              POU->Name, RWDataSegment, LRealVar + POU->LRealOffset, 
					          LIntVar + POU->LIntOffset, RealVar + POU->RealOffset,			
			                  DIntVar + POU->DIntOffset, WStringVar + POU->WStringOffset, 
							  StringVar + POU->StringOffset, IntVar + POU->IntOffset, ByteVar + POU->ByteOffset,
							  TopLevelPOUObject->IntOperand, TopLevelPOUObject->FloatOperand, // Pass pointers to this POU's operand stacks to sub-POUs
							  TopLevelPOUObject->DoubleOperand, TopLevelPOUObject->LongOperand, 
							  TopLevelPOUObject->StringOperand, TopLevelPOUObject->WStringOperand);
			Externals[i++] = SubPOUObject;
			POU++;
		}
	}
	return (int)TopLevelPOUObject;
}

int GetPOUType(int POUHandle)
{
	if (POUHandle == 0)
		return -1;
	else {
		const STLangPOUObject* POUObject = (STLangPOUObject *)POUHandle;

		return POUObject->POUType();
	}
}

int GetInPutCount(int POUHandle)
{
	if (POUHandle == 0)
		return -1;
	else {
		const STLangPOUObject* POUObject = (STLangPOUObject *)POUHandle;

		return POUObject->InputCount();
	}
}

int GetOutPutCount(int POUHandle)
{
	if (POUHandle == 0)
		return -1;
	else {
		const STLangPOUObject* POUObject = (STLangPOUObject *)POUHandle;

		return POUObject->OutputCount();
	}
}

bool GetPOUName(char* POUName, int POUHandle)
{
	if (POUHandle == 0)
		return false;
	else {
		const STLangPOUObject* POUObject = (STLangPOUObject *)POUHandle;
		const string Name = POUObject->Name();

		strcpy_s(POUName, MAX_ID_LENGTH, Name.c_str());
		return true;
	}
}

int GetInputNames (
	char** Inputs, 
	int POUHandle)
{
	if (POUHandle == 0)
		return -1;
	else {
		const STLangPOUObject* POUObject = (STLangPOUObject *)POUHandle;
		const set<string>& InputNames = POUObject->InputNames();
		set<string>::const_iterator Input = InputNames.begin();
		int i = 0;

		while (Input != InputNames.end())
		{
			strcpy_s(Inputs[i++], MAX_ID_LENGTH, Input->c_str());
			Input++;
		};
		return i;
	}
}

int GetOutputNames (
	char** Outputs, 
	int POUHandle)
{
	if (POUHandle == 0)
		return -1;
	else {
		const STLangPOUObject* POUObject = (STLangPOUObject *)POUHandle;
		const set<string>& OutputNames = POUObject->OutputNames();
		set<string>::const_iterator Output = OutputNames.begin();
		int i = 0;

		while (Output != OutputNames.end())
		{
			strcpy_s(Outputs[i++], MAX_ID_LENGTH, Output->c_str());
			Output++;
		};
		return i;
	}
}

int GetRetainedNames (
	char** RetainedNames, 
	int POUHandle)
{
	if (POUHandle == 0)
		return -1;
	else {
		const STLangPOUObject* POUObject = (STLangPOUObject *)POUHandle;
		const set<string>& RetainedVars = POUObject->RetainedNames();
		set<string>::const_iterator RetainedVar = RetainedVars.begin();
		int i = 0;

		while (RetainedVar != RetainedVars.end())
		{
			strcpy_s(RetainedNames[i++], MAX_ID_LENGTH, RetainedVar->c_str());
			RetainedVar++;
		};
		return i;
	}
}

double GetOutputValue (
	char* Output, 
	int POUHandle)
{
	if (POUHandle == 0)
		return -99.0;
	else {
		STLangPOUObject* POUObject = (STLangPOUObject *)POUHandle;

		return POUObject->GetOutput(Output);
	}
}

double GetRetainedValue (
	char* RetainedVar, 
	int POUHandle)
{
	if (POUHandle == 0)
		return -99.0;
	else {
		STLangPOUObject* POUObject = (STLangPOUObject *)POUHandle;

		return POUObject->GetRetainedValue(RetainedVar);
	}
}

void SetRetainedValue (
	char* Name,
	double Value,
	int POUHandle)
{
	if (POUHandle != 0)
	{
		STLangPOUObject* POUObject = (STLangPOUObject *)POUHandle;

		POUObject->SetRetainedValue(Name, Value);
	}
}

double ExecutePOU (
	double ArgV[], 
	int ArgC,
	int POUHandle)
{
	if (POUHandle == 0)
		return -99.0;
	else {
		STLangPOUObject* POUObject = (STLangPOUObject *)POUHandle;

		return POUObject->Execute(ArgV, ArgC);
	}
}