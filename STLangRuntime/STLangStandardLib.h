#pragma once

namespace StandardLibrary
{
	typedef enum 
	{
		//
		// Numerical functions
		//
		NONE,
		DSIN,
		DCOS,
		DTAN,
		DLOGN,
		DLOG10,
		DEXP,
		DEXPT,
		DASIN,
		DACOS,
		DATAN,
		IABS,
		LABS,
		FABS,
		DABS,
		DSQRT,
		FTRUNC,
		DTRUNC,
		FSIN,
		FCOS,
		FTAN,
		FLOGN,
		FLOG10,
		FEXP,
		FEXPT,
		FASIN,
		FACOS,
		FATAN,
		FSQRT,
		IEXP,
		IEXPT,
		LEXPT,
		IMAX,
		LMAX,
		DMAX,
		FMAX,
		SMAX,
		WSMAX,
		TMAX,
		DTMAX,
		DTTMAX,
		TODMAX,
		IMIN,
		LMIN,
		DMIN,
		FMIN,
		SMIN,
		WSMIN,
		TMIN,
		DTMIN,
		DTTMIN,
		TODMIN,

		//
		// Selection and limit functions
		//
		ILIMIT,   // Integer LIMIT
		LLIMIT,   // Long LIMIT
		DLIMIT,   // Double LIMIT
		FLIMIT,   // Float LIMIT
		TLIMIT,   // Time LIMIT
		DTLIMIT,  // Date LIMIT
		DTTLIMIT, // DateTime LIMIT
		TODLIMIT, // Time Of Day LIMIT
		SLIMIT,   // String LIMIT
		WSLIMIT,  // WString LIMIT
		SELECT,  
		MUX,

		//
		// DateTime arithmetic functions
		//
		TADD,         // Time Time ADD
		TODTADD,      // TimeOfDay Time ADD
		TODTSUB,      // TimeOfDay Time SUBtract
		DTTADD,       // DateTime Time ADD
		TSUB,         // Time Time SUBtraction
		TODSUB,       // TimeOfDay TimeOfDay SUBtraction
		DTTSUB,       // DateTime Time SUBtraction
		DTSUB,        // Date Date SUB , DateTime DateTime SUB
		TIMUL,        // Time Int MULtiply
		TFMUL,        // Time Float MULtiply
		TDMUL,        // Time Double MULtiply
		TIDIV,        // Time Int DIVide
		TFDIV,        // Time Float DIVide
		TDDIV,        // Time Double DIVide

		//
		// Character string functions
		//
		SLEN,
		SLEFT,
		SRIGHT,
		SMID,
		SCONCAT,
		SCONCAT2,
		SINSERT,
		SDELETE,
		SREPLACE,
		SFIND,
		WSLEN,
		WSLEFT,
		WSRIGHT,
		WSMID,
		WSCONCAT,
		WSCONCAT2,
		WSINSERT,
		WSDELETE,
		WSREPLACE,
		WSFIND,
		STRCMP,       // STRing CoMPare
		WSTRCMP,      // WSTRing CoMPare

		//
		// Conversion functions
		//
		BCDTO,          // Binary Coded Decimal TO integer
		TOBCD,          // Integer to Binary Coded Decimal
		CONV,           // Convert from type A to type B
		TO_SSHORT,      // Convert Int32 to signed short
        TO_SBYTE,       // Convert Int32 to signed Byte
        TO_USHORT,      // Convert Int32 to unsigned short
        TO_UBYTE,       // Convert Int32 to unsigned Byte

		//
		// Stack operations
		//
		COPY_DSTACK,    // COPY Double STACK to memory
		COPY_FSTACK,    // COPY Float STACK to memory
		COPY_LSTACK,    // COPY Long STACK to memory
		COPY_WSTACK,    // COPY DInt STACK to memory
		COPY_ISTACK,    // COPY Int STACK to memory
		COPY_BSTACK,    // COPY Byte STACK to memory
		COPY_SSTACK,    // COPY String STACK to memory
		COPY_WSSTACK    // COPY WString STACK to memory
	} StdLibraryFunction;
}