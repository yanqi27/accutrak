
#include <stdlib.h>

void FreeBlock(void* ipData)
{
	::free(ipData);
}
