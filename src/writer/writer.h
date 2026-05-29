#pragma once

#include <charconv>
#include <fstream>
#include <string>
#include <string_view>

#include "column/batch.h"
#include "column/col_util.h"
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

    void ExecBatchCsv(const Batch& batch) {
        if (batch.CntRows() == 0 || batch.NumCols() == 0) {
            return;
        }
        std::string buf;
        buf.reserve(batch.NumCols() * 8);
        for (ui64 row = 0; row < batch.CntRows(); ++row) {
            if (!batch.GetMask()[row]) {
                continue;
            }
            for (ui64 col = 0; col < batch.NumCols(); ++col) {
                if (col) {
                    buf += ',';
                }
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

    template <typename T>
    static void AppendPaddedInt(std::string& buf, T v, ui64 width) {
        char tmp[24];
        std::to_chars_result res = std::to_chars(tmp, tmp + sizeof(tmp), v);
        ui64 len = static_cast<ui64>(res.ptr - tmp);
        while (len < width) {
            buf += '0';
            ++len;
        }
        buf.append(tmp, res.ptr);
    }

    static void AppendString(std::string& buf, std::string_view sv) {
        bool quote = false;
        for (char c : sv) {
            if (c == ',' || c == '"' || c == '\n' || c == '\r') {
                quote = true;
                break;
            }
        }
        if (!quote) {
            buf.append(sv.data(), sv.size());
            return;
        }
        buf += '"';
        for (char c : sv) {
            if (c == '"') {
                buf += '"';
            }
            buf += c;
        }
        buf += '"';
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
                IsqDate d = static_cast<Column<IsqDate>*>(col.get())->At(row);
                AppendPaddedInt(buf, d.year, 4);
                buf += '-';
                AppendPaddedInt(buf, static_cast<i16>(d.month), 2);
                buf += '-';
                AppendPaddedInt(buf, static_cast<i16>(d.day), 2);
                break;
            }
            case ColType::kDatetime: {
                IsqDatetime d =
                    static_cast<Column<IsqDatetime>*>(col.get())->At(row);
                AppendPaddedInt(buf, d.year, 4);
                buf += '-';
                AppendPaddedInt(buf, static_cast<i16>(d.month), 2);
                buf += '-';
                AppendPaddedInt(buf, static_cast<i16>(d.day), 2);
                buf += ' ';
                AppendPaddedInt(buf, static_cast<i16>(d.hour), 2);
                buf += ':';
                AppendPaddedInt(buf, static_cast<i16>(d.minute), 2);
                buf += ':';
                AppendPaddedInt(buf, static_cast<i16>(d.second), 2);
                break;
            }
            case ColType::kString: {
                std::string_view sv =
                    static_cast<ColumnString*>(col.get())->At(row);
                AppendString(buf, sv);
                break;
            }
        }
    }

    std::ofstream file_;
};
