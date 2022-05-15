#pragma once

#include <string>

using namespace std;

class STLoadException
{
public:
	STLoadException(int ErrorCode) : m_ErrorMessage(ErrorMsg[ErrorCode])
	{
	}

	STLoadException(int ErrorCode, string Arg) : m_ErrorMessage(MakeErrorMsg(ErrorCode, Arg.c_str()))
	{
	}

private:
	static string MakeErrorMsg(int ErrorCode, const char Arg[])
	{
		char Buffer[256];

		sprintf_s(Buffer, sizeof(Buffer), ErrorMsg[ErrorCode], Arg);
		return Buffer;
	}

	static const char* ErrorMsg[];

	const string m_ErrorMessage;
};

class STRuntimeException
{
public:
	STRuntimeException(int ErrorCode) : m_ErrorMessage(ErrorMsg[ErrorCode])
	{
	}

	STRuntimeException(int ErrorCode, int Arg1, int Arg2) 
		: m_ErrorMessage(MakeErrorMsg(ErrorCode, Arg1, Arg2))
	{
	}

	STRuntimeException(int ErrorCode, const char Arg1[], int Arg2) 
		: m_ErrorMessage(MakeErrorMsg(ErrorCode, Arg1, Arg2))
	{
	}

private:
	static string MakeErrorMsg(int ErrorCode, const char Arg1[])
	{
		char Buffer[256];

		sprintf_s(Buffer, sizeof(Buffer), ErrorMsg[ErrorCode], Arg1);
		return Buffer;
	}

	static string MakeErrorMsg(int ErrorCode, const char Arg1[], int Arg2)
	{
		char Buffer[256];

		sprintf_s(Buffer, sizeof(Buffer), ErrorMsg[ErrorCode], Arg1, Arg2);
		return Buffer;
	}

	static string MakeErrorMsg(int ErrorCode, int Arg1, int Arg2)
	{
		char Buffer[256];

		sprintf_s(Buffer, sizeof(Buffer), ErrorMsg[ErrorCode], Arg1, Arg2);
		return Buffer;
	}

	static const char* ErrorMsg[];

	const string m_ErrorMessage;
};