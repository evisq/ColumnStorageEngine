#pragma once

#include <string_view>
#include <vector>

#include "column/column_int_dt.h"
#include "column/column_string.h"

struct ColOps {
    static ColPtr EmptyLike(const ColPtr& col) {
        switch (col->GetType()) {
            case ColType::kInt8:
                return std::make_shared<Column<i8>>(std::vector<i8>{});
            case ColType::kInt16:
                return std::make_shared<Column<i16>>(std::vector<i16>{});
            case ColType::kInt32:
                return std::make_shared<Column<i32>>(std::vector<i32>{});
            case ColType::kInt64:
                return std::make_shared<Column<i64>>(std::vector<i64>{});
            case ColType::kDate:
                return std::make_shared<Column<IsqDate>>(
                    std::vector<IsqDate>{});
            case ColType::kDatetime:
                return std::make_shared<Column<IsqDatetime>>(
                    std::vector<IsqDatetime>{});
            case ColType::kString:
                return std::make_shared<ColumnString>(std::vector<char>{},
                                                      std::vector<ui64>{0});
        }
        return nullptr;
    }

    static ColPtr SliceRow(const ColPtr& col, ui64 row) {
        switch (col->GetType()) {
            case ColType::kInt8:
                return std::make_shared<Column<i8>>(std::vector<i8>{
                    static_cast<const Column<i8>*>(col.get())->At(row)});
            case ColType::kInt16:
                return std::make_shared<Column<i16>>(std::vector<i16>{
                    static_cast<const Column<i16>*>(col.get())->At(row)});
            case ColType::kInt32:
                return std::make_shared<Column<i32>>(std::vector<i32>{
                    static_cast<const Column<i32>*>(col.get())->At(row)});
            case ColType::kInt64:
                return std::make_shared<Column<i64>>(std::vector<i64>{
                    static_cast<const Column<i64>*>(col.get())->At(row)});
            case ColType::kDate:
                return std::make_shared<Column<IsqDate>>(std::vector<IsqDate>{
                    static_cast<const Column<IsqDate>*>(col.get())->At(row)});
            case ColType::kDatetime:
                return std::make_shared<Column<IsqDatetime>>(
                    std::vector<IsqDatetime>{
                        static_cast<const Column<IsqDatetime>*>(col.get())->At(
                            row)});
            case ColType::kString: {
                std::string_view sv =
                    static_cast<const ColumnString*>(col.get())->At(row);
                std::vector<char> data(sv.begin(), sv.end());
                std::vector<ui64> offsets = {0, sv.size()};
                return std::make_shared<ColumnString>(std::move(data),
                                                      std::move(offsets));
            }
        }
        return nullptr;
    }

    static ColPtr ConcatCol(const std::vector<std::vector<ColPtr>>& rows,
                            ui64 c, ui64 start, ui64 count) {
        if (count == 0) {
            return nullptr;
        }
        switch (rows[start][c]->GetType()) {
            case ColType::kInt8: {
                std::vector<i8> vals;
                vals.reserve(count);
                for (ui64 i = start; i < start + count; ++i) {
                    vals.push_back(
                        static_cast<const Column<i8>*>(rows[i][c].get())
                            ->At(0));
                }
                return std::make_shared<Column<i8>>(std::move(vals));
            }
            case ColType::kInt16: {
                std::vector<i16> vals;
                vals.reserve(count);
                for (ui64 i = start; i < start + count; ++i) {
                    vals.push_back(
                        static_cast<const Column<i16>*>(rows[i][c].get())
                            ->At(0));
                }
                return std::make_shared<Column<i16>>(std::move(vals));
            }
            case ColType::kInt32: {
                std::vector<i32> vals;
                vals.reserve(count);
                for (ui64 i = start; i < start + count; ++i) {
                    vals.push_back(
                        static_cast<const Column<i32>*>(rows[i][c].get())
                            ->At(0));
                }
                return std::make_shared<Column<i32>>(std::move(vals));
            }
            case ColType::kInt64: {
                std::vector<i64> vals;
                vals.reserve(count);
                for (ui64 i = start; i < start + count; ++i) {
                    vals.push_back(
                        static_cast<const Column<i64>*>(rows[i][c].get())
                            ->At(0));
                }
                return std::make_shared<Column<i64>>(std::move(vals));
            }
            case ColType::kDate: {
                std::vector<IsqDate> vals;
                vals.reserve(count);
                for (ui64 i = start; i < start + count; ++i) {
                    vals.push_back(
                        static_cast<const Column<IsqDate>*>(rows[i][c].get())
                            ->At(0));
                }
                return std::make_shared<Column<IsqDate>>(std::move(vals));
            }
            case ColType::kDatetime: {
                std::vector<IsqDatetime> vals;
                vals.reserve(count);
                for (ui64 i = start; i < start + count; ++i) {
                    vals.push_back(static_cast<const Column<IsqDatetime>*>(
                                       rows[i][c].get())
                                       ->At(0));
                }
                return std::make_shared<Column<IsqDatetime>>(std::move(vals));
            }
            case ColType::kString: {
                std::vector<char> data;
                std::vector<ui64> offsets;
                offsets.reserve(count + 1);
                offsets.push_back(0);
                for (ui64 i = start; i < start + count; ++i) {
                    std::string_view sv =
                        static_cast<const ColumnString*>(rows[i][c].get())
                            ->At(0);
                    data.insert(data.end(), sv.begin(), sv.end());
                    offsets.push_back(data.size());
                }
                return std::make_shared<ColumnString>(std::move(data),
                                                      std::move(offsets));
            }
        }
        return nullptr;
    }

    static int CompareCell(const ColPtr& ca, ui64 ra, const ColPtr& cb,
                           ui64 rb) {
        switch (ca->GetType()) {
            case ColType::kInt8: {
                i8 va = static_cast<const Column<i8>*>(ca.get())->At(ra);
                i8 vb = static_cast<const Column<i8>*>(cb.get())->At(rb);
                return (va > vb) - (va < vb);
            }
            case ColType::kInt16: {
                i16 va = static_cast<const Column<i16>*>(ca.get())->At(ra);
                i16 vb = static_cast<const Column<i16>*>(cb.get())->At(rb);
                return (va > vb) - (va < vb);
            }
            case ColType::kInt32: {
                i32 va = static_cast<const Column<i32>*>(ca.get())->At(ra);
                i32 vb = static_cast<const Column<i32>*>(cb.get())->At(rb);
                return (va > vb) - (va < vb);
            }
            case ColType::kInt64: {
                i64 va = static_cast<const Column<i64>*>(ca.get())->At(ra);
                i64 vb = static_cast<const Column<i64>*>(cb.get())->At(rb);
                return (va > vb) - (va < vb);
            }
            case ColType::kDate: {
                IsqDate va =
                    static_cast<const Column<IsqDate>*>(ca.get())->At(ra);
                IsqDate vb =
                    static_cast<const Column<IsqDate>*>(cb.get())->At(rb);
                return (va > vb) - (va < vb);
            }
            case ColType::kDatetime: {
                IsqDatetime va =
                    static_cast<const Column<IsqDatetime>*>(ca.get())->At(ra);
                IsqDatetime vb =
                    static_cast<const Column<IsqDatetime>*>(cb.get())->At(rb);
                return (va > vb) - (va < vb);
            }
            case ColType::kString: {
                std::string_view va =
                    static_cast<const ColumnString*>(ca.get())->At(ra);
                std::string_view vb =
                    static_cast<const ColumnString*>(cb.get())->At(rb);
                if (va < vb) {
                    return -1;
                }
                if (va > vb) {
                    return 1;
                }
                return 0;
            }
        }
        return 0;
    }
};
