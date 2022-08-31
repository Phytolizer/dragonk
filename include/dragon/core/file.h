#pragma once

#include "dragon/core/str.h"
#include "dragon/core/sum.h"

typedef RESULT(str, str) SlurpFileResult;

SlurpFileResult slurp_file(str filename);
