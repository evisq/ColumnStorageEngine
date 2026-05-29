#pragma once

#include <fstream>
#include <string_view>
#include <vector>

#include "column/col_util.h"
#include "util/alias.h"

static constexpr ui64 kDefaultBatchSize = 65536;

ui64 BatchFlush(std::vector<VString> &col_bufs, std::vector<ColScheme> &scheme,
                std::vector<ui64> &col_starts, std::ofstream &out,
                ui64 cnt_cols);

void ConvertCsvToIsq(std::string_view data_path, std::string_view scheme_path,
                     std::string_view isq_path);
