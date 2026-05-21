#pragma once

#include <charconv>
#include <cstdio>
#include <fstream>
#include <string>
#include <string_view>

#include "column/batch.h"
#include "column/col_util.h"
#include "column/column.h"
#include "column/column_int_dt.h"
#include "column/column_string.h"
#include "util/alias.h"
#include "util/assert.h"

class Writer {
public:
    explicit Writer(std::string_view path) : file_(std::string(path)) {
        ASSERT_WITH_MESSAGE(file_.is_open(),
                            "Cannot open output file: " + std::string(path));
    }
    ~Writer() = default;
    Writer(const Writer&) = delete;
    Writer(Writer&&) = default;
    Writer& operator=(const Writer&) = delete;
    Writer& operator=(Writer&&) = default;

    void ExecBatchCSV(const Batch& batch) {
        if (batch.CntRows() == 0 || batch.NumCols() == 0) return;
        std::string buf;
        buf.reserve(batch.NumCols() * 8);
        for (ui64 row = 0; row < batch.CntRows(); ++row) {
            if (!batch.GetMask()[row]) continue;
            for (ui64 col = 0; col < batch.NumCols(); ++col) {
                if (col) buf += ',';
                AppendCell(buf, batch[col], row);
            }
            buf += '\n';
            file_.write(buf.data(), static_cast<std::streamsize>(buf.size()));
            buf.clear();
        }
    }

private:
    template <typename T>
    static void AppendInt(std::string& buf, T v) {
        char tmp[24];
        std::to_chars_result res = std::to_chars(tmp, tmp + sizeof(tmp), v);
        buf.append(tmp, res.ptr);
    }

    void AppendCell(std::string& buf, const ColPtr& col, ui64 row) {
        switch (col->GetType()) {
            case ColType::kInt8:
                AppendInt(buf,
                          static_cast<i16>(
                              static_cast<Column<i8>*>(col.get())->At(row)));
                break;
            case ColType::kInt16:
                AppendInt(buf, static_cast<Column<i16>*>(col.get())->At(row));
                break;
            case ColType::kInt32:
                AppendInt(buf, static_cast<Column<i32>*>(col.get())->At(row));
                break;
            case ColType::kInt64:
                AppendInt(buf, static_cast<Column<i64>*>(col.get())->At(row));
                break;
            case ColType::kDate: {
                ISQDate d = static_cast<Column<ISQDate>*>(col.get())->At(row);
                AppendInt(buf, d.year);
                buf += '-';
                AppendInt(buf, static_cast<i16>(d.month));
                buf += '-';
                AppendInt(buf, static_cast<i16>(d.day));
                break;
            }
            case ColType::kDatetime: {
                ISQDatetime d =
                    static_cast<Column<ISQDatetime>*>(col.get())->At(row);
                AppendInt(buf, d.year);
                buf += '-';
                AppendInt(buf, static_cast<i16>(d.month));
                buf += '-';
                AppendInt(buf, static_cast<i16>(d.day));
                buf += ' ';
                AppendInt(buf, static_cast<i16>(d.hour));
                buf += ':';
                AppendInt(buf, static_cast<i16>(d.minute));
                buf += ':';
                AppendInt(buf, static_cast<i16>(d.second));
                break;
            }
            case ColType::kString: {
                std::string_view sv =
                    static_cast<ColumnString*>(col.get())->At(row);
                buf.append(sv.data(), sv.size());
                break;
            }
        }
    }

    std::ofstream file_;
};
