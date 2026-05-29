#pragma once

#include <string>

#include "op-wrapper/op.h"

OpPtr BuildQueryPlan(int query_num, const std::string& isq_path);
