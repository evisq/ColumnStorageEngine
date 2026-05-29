#pragma once

#include "column/column_int_dt.h"
#include "column/column_string.h"

inline ColPtr DeserializeColumn(const char *src, ui64 byte_size, ColType type,
                                ui64 cnt_rows) {
    switch (type) {
        case ColType::kInt8:
            return Column<i8>::FromBytes(src, cnt_rows);
        case ColType::kInt16:
            return Column<i16>::FromBytes(src, cnt_rows);
        case ColType::kInt32:
            return Column<i32>::FromBytes(src, cnt_rows);
        case ColType::kInt64:
            return Column<i64>::FromBytes(src, cnt_rows);
        case ColType::kDate:
            return Column<IsqDate>::FromBytes(src, cnt_rows);
        case ColType::kDatetime:
            return Column<IsqDatetime>::FromBytes(src, cnt_rows);
        case ColType::kString:
            return ColumnString::FromBytes(src, byte_size, cnt_rows);
    }
    SEND_MESSAGE("Unknown ColType");
}

template <typename Op, typename... Args>
auto Dispatch(const ColPtr &col, Args &&...args) {
    switch (col->GetType()) {
        case ColType::kInt8:
            return Op::Exec(*static_cast<Column<i8> *>(col.get()),
                            std::forward<Args>(args)...);
        case ColType::kInt16:
            return Op::Exec(*static_cast<Column<i16> *>(col.get()),
                            std::forward<Args>(args)...);
        case ColType::kInt32:
            return Op::Exec(*static_cast<Column<i32> *>(col.get()),
                            std::forward<Args>(args)...);
        case ColType::kInt64:
            return Op::Exec(*static_cast<Column<i64> *>(col.get()),
                            std::forward<Args>(args)...);
        case ColType::kDate:
            return Op::Exec(*static_cast<Column<IsqDate> *>(col.get()),
                            std::forward<Args>(args)...);
        case ColType::kDatetime:
            return Op::Exec(*static_cast<Column<IsqDatetime> *>(col.get()),
                            std::forward<Args>(args)...);
        case ColType::kString:
            return Op::Exec(*static_cast<ColumnString *>(col.get()),
                            std::forward<Args>(args)...);
    }
    SEND_MESSAGE("Unknown ColType");
}
