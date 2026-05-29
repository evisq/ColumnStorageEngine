#pragma once

#include "column/col_util.h"
#include "util/alias.h"

void GetLineVString(VString &vec, std::ifstream &file);

std::vector<ColScheme> ParseScheme(std::string_view path);
