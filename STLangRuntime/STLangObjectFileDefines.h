#pragma once

#define MINOR_VERSION            1
#define MAJOR_VERSION            2
#define MAGIC_NUMBER             0xABADFACE
#define MAX_ID_LENGTH            63 


enum POUType
{
    FUNCTION = 0xadeafdad,
    FUNCTION_BLOCK = 0xabaddeed,
    PROGRAM = 0xadeaddad
};

struct ExternalSymbol
{
	char Name[MAX_ID_LENGTH + 1]; 
	POUType Type;       
	int RealOffset;    
	int LRealOffset;     
	int DIntOffset;
	int LIntOffset;
	int ByteOffset;
	int IntOffset;
	int StringOffset;
	int WStringOffset;
};

struct IOParameter
{
	char Name[MAX_ID_LENGTH + 1];
	int Index;
};

struct InitializerData
{
	int RODataSegmentOffset;
	
	int RWDataSegmentOffset;

	int ByteCount;
};

struct StringVariableData
{    
    int BufferOffset;

	int StringIndex;

    int BufferSize;
           
    int ElementCount;
          
    int StringType;
};

struct ExecutableFileHeader
{
	unsigned int MagicNumber;

	unsigned int MinorVersion;

	unsigned int MajorVersion;

	int ExecutableType;

	char POUName[MAX_ID_LENGTH + 1];

	int RODataSegmentSize;

	int CodeSegmentSize;

	int ConstantDataOffset;

	int RWDataSegmentSize;

	int ExternalCount;

	int InputCount;

	int OutputCount;

	int RetainedCount;

	int InitializerCount;

	int StringDataCount;

	int LRealVarCount;

	int RealVarCount;

	int LIntVarCount;

	int DIntVarCount;

	int IntVarCount;

	int StringVarCount;

	int WStringVarCount;

	int LRealConstCount;

	int RealConstCount;

	int LIntConstCount;

	int DIntConstCount;

	int StringCount;

	int WStringCount;

	int RegisterCount;
};


template <class ElemType>
struct TableEntry
{
	ElemType Value;
	unsigned short Offset;
};


struct SwitchInfo
{
	int64 MinValue;
	int64 MaxValue;
	int IsInt64;
	int DefaultOffset;
	int ElemSize;
    int TableSize;
};