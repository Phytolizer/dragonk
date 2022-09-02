#include <stdio.h>

#include "dragon/core/buf.h"
#include "dragon/driver/run.h"

int main(int argc, char** argv)
{
	return run((CArgBuf)BUF_REF(argv, argc), stdout, stderr);
}
