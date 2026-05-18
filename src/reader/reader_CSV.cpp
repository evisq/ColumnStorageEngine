#include "reader_CSV.h"

#include <cstring>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

#include "parser.h"
#include "serialize.h"
#include "util/assert.h"
#include "util/stream.h"

ui64 BatchFlush(std::vector<VString> &col_bufs, std::vector<ColScheme> &scheme,
                std::vector<ui64> &col_starts, std::ofstream &out,
                ui64 cnt_cols) {
    for (ui64 i = 0; i < cnt_cols; i++) {
        col_starts[i] = static_cast<ui64>(out.tellp());
        std::vector<char> bytes = SerializeColumn(col_bufs[i], scheme[i].type);
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        col_bufs[i].clear();
    }
    ui64 result = static_cast<ui64>(out.tellp());
    for (ui64 i = 0; i < cnt_cols; i++) {
        Put(out, col_starts[i]);
    }
    return result;
}

void ReaderCSV(std::string_view data_path, std::string_view scheme_path,
               std::string_view isq_path) {
    std::vector<ColScheme> scheme = ParseScheme(scheme_path);
    const ui64 cnt_cols = static_cast<ui64>(scheme.size());
    std::ifstream data((std::string(data_path)));
    ASSERT_WITH_MESSAGE(data.is_open(), "CSV Data file is not opening: " +
                                            std::string(data_path));
    std::ofstream out(std::string(isq_path),
                      std::ios::binary | std::ios::trunc);
    ASSERT_WITH_MESSAGE(out.is_open(),
                        "ISQ file is not opening: " + std::string(isq_path));

    std::vector<VString> col_bufs(cnt_cols);
    std::vector<ui64> batch_idx_offsets;
    std::vector<ui64> col_starts(cnt_cols);

    VString row;
    ui64 rows_in_batch = 0;
    while (data.good()) {
        GetLineVString(row, data);
        if (row.empty()) continue;
        if (row.size() != cnt_cols) continue;
        for (ui64 j = 0; j < cnt_cols; j++) col_bufs[j].push_back(row[j]);
        if (++rows_in_batch == kDefaultBatchSize) {
            batch_idx_offsets.push_back(
                BatchFlush(col_bufs, scheme, col_starts, out, cnt_cols));
            Put(out, rows_in_batch);
            rows_in_batch = 0;
        }
    }
    if (rows_in_batch) {
        batch_idx_offsets.push_back(
            BatchFlush(col_bufs, scheme, col_starts, out, cnt_cols));
        Put(out, rows_in_batch);
    }

    ui64 footer_off = static_cast<ui64>(out.tellp());
    Put(out, static_cast<ui64>(batch_idx_offsets.size()));
    Put(out, cnt_cols);
    Put(out, kDefaultBatchSize);
    for (ui64 pos : batch_idx_offsets) Put(out, pos);

    for (const auto &col_elem : scheme) {
        Put(out, static_cast<ui8>(col_elem.type));
    }
    std::vector<char> col_names;
    for (ui64 i = 0; i < cnt_cols; ++i) {
        Put(out, (static_cast<ui64>(col_names.size())));
        col_names.insert(col_names.end(), scheme[i].name.begin(),
                         scheme[i].name.end());
    }
    Put(out, (static_cast<ui64>(col_names.size())));
    out.write(col_names.data(), static_cast<std::streamsize>(col_names.size()));
    Put(out, footer_off);
}
