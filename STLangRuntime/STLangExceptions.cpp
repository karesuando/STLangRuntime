#include "pch.h"
#include "STLangExceptions.h"

const char* STLoadException::ErrorMsg[] =
{
/* 0 */	    "%s: Illegal type of file.",
/* 1 */		"Can't open file %s.",
/* 2 */		"Failed to read file header.",
/* 3 */		"Magic number mismatch.",
/* 4 */		"Failed to read names of externals.",
/* 5 */		"_aligned_malloc() failed: Could not allocate memory for RW datasegment.",
/* 6 */		"Failed to read code segment.",
/* 7 */		"Couldn't allocate memory for code segment.",
/* 8 */		"Couldn't allocate memory for operands.",
/* 9 */		"Couldn't allocate memory for RO data segment.",
/* 10 */	"Failed to read RO data segment.",
/* 11 */	"_aligned_malloc() failed: Could not allocate memory for RW data segment.",
/* 12 */	"Failed to read RW data segment initialization data.",
/* 13 */	"memcpy(): Source offset out of range.",
/* 14 */	"memcpy(): Destination offset out of range.",
/* 15 */    "memcpy(): Source data block overlaps RO data segment.",
/* 16 */    "memcpy(): Destination data block overlaps RW data segment.",
/* 17 */    "File %s does not exist.",
/* 18 */    "Maximum code size exceeded.",
/* 19 */    "Failed to read input names.",
/* 20 */    "Failed to read output names.",
/* 21 */    "Failed to read retained variable names.",
/* 22 */    "Failed to read initializer data.",
/* 23 */    "Failed to read external POU-name.",
/* 24 */    "Failed to read string variable data.",
/* 25 */    "Failed to read double constants."
};

const char* STRuntimeException::ErrorMsg[] =
{
/* 0 */	 ">> Runtime error: Division by 0 at PC = %d and instruction %s.",
/* 1 */	 ">> Runtime error: Unknown instruction with operation code %d at PC = %d.",
/* 2 */	 ">> Runtime error: Invalid value of PC = %d.",
/* 3 */	 ">> Runtime error: Array index out of bounds.",
/* 4 */	 ">> Runtime error: Stackoverflow.",
/* 5 */	 ">> Runtime error: Stackunderflow.",
/* 6 */  ">> Runtime error: TimeSpan addition overflow at PC = %d and instruction %s.",
/* 7 */  ">> Runtime error: TimeSpan subtraction overflow at PC = %d and instruction %s.",
/* 8 */  ">> Runtime error: DateTime + TimeSpan overflow at PC = %d and instruction %s.",
/* 9 */  ">> Runtime error: DateTime - TimeSpan overflow at PC = %d and instruction %s.",
/* 10 */ ">> Runtime error: DateTime subtraction overflow at PC = %d and instruction %s.",
/* 11 */ ">> Runtime error: Bit shift function %s at PC = %d has negative argument.",
/* 12 */ ">> Runtime error: MUX selector K = %d out of range at PC = %d."
};