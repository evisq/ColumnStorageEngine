#pragma once

#include <algorithm>
#include <string_view>
#include <vector>

#include "column/column_int_dt.h"
#include "column/column_string.h"
#include "op.h"

class OrderByOp : public Op {
public:
    struct SortKey {
        ui64 col_idx;
        bool asc;
    };

    OrderByOp(OpPtr op, std::vector<SortKey> keys)
        : op_(std::move(op)), keys_(std::move(keys)) {}

    bool HasNext() override { return !done_; }

    Batch Next() override {
        std::vector<Batch> batches;
        std::vector<RowRef> rows;

        while (op_->HasNext()) {
            Batch b = op_->Next();
            const std::vector<ui8>& mask = b.GetMask();
            ui64 bi = batches.size();
            for (ui64 row = 0; row < b.CntRows(); ++row) {
                if (mask[row]) rows.push_back({bi, row});
            }
            batches.push_back(std::move(b));
        }
        done_ = true;

        if (rows.empty()) return Batch({}, 0);

        std::sort(rows.begin(), rows.end(), [&](const RowRef& a, const RowRef& b) {
            for (const SortKey& k : keys_) {
                int cmp = CompareCell(
                    batches[a.batch_idx][k.col_idx], a.row_idx,
                    batches[b.batch_idx][k.col_idx], b.row_idx);
                if (cmp != 0) return k.asc ? cmp < 0 : cmp > 0;
            }
            return false;
        });

        const ui64 num_cols = batches[0].NumCols();
        const ui64 n = rows.size();
        std::vector<ColPtr> result_cols;
        result_cols.reserve(num_cols);
        for (ui64 c = 0; c < num_cols; ++c)
            result_cols.push_back(BuildCol(batches, rows, c));
        return Batch(std::move(result_cols), n);
    }

private:
    static int CompareCell(const ColPtr& ca, ui64 ra, const ColPtr& cb, ui64 rb) {
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
                ISQDate va = static_cast<const Column<ISQDate>*>(ca.get())->At(ra);
                ISQDate vb = static_cast<const Column<ISQDate>*>(cb.get())->At(rb);
                return (va > vb) - (va < vb);
            }
            case ColType::kDatetime: {
                ISQDatetime va = static_cast<const Column<ISQDatetime>*>(ca.get())->At(ra);
                ISQDatetime vb = static_cast<const Column<ISQDatetime>*>(cb.get())->At(rb);
                return (va > vb) - (va < vb);
            }
            case ColType::kString: {
                std::string_view va = static_cast<const ColumnString*>(ca.get())->At(ra);
                std::string_view vb = static_cast<const ColumnString*>(cb.get())->At(rb);
                if (va < vb) return -1;
                if (va > vb) return  1;
                return 0;
            }
        }
        return 0;
    }

    struct RowRef {
        ui64 batch_idx;
        ui64 row_idx;
    };

    static ColPtr BuildCol(const std::vector<Batch>& batches,
                           const std::vector<RowRef>& rows, ui64 c) {
        const ui64 n = rows.size();
        switch (batches[0][c]->GetType()) {
            case ColType::kInt8: {
                std::vector<i8> vals;
                vals.reserve(n);
                for (const RowRef& r : rows)
                    vals.push_back(static_cast<const Column<i8>*>(
                        batches[r.batch_idx][c].get())->At(r.row_idx));
                return std::make_shared<Column<i8>>(std::move(vals));
            }
            case ColType::kInt16: {
                std::vector<i16> vals;
                vals.reserve(n);
                for (const RowRef& r : rows)
                    vals.push_back(static_cast<const Column<i16>*>(
                        batches[r.batch_idx][c].get())->At(r.row_idx));
                return std::make_shared<Column<i16>>(std::move(vals));
            }
            case ColType::kInt32: {
                std::vector<i32> vals;
                vals.reserve(n);
                for (const RowRef& r : rows)
                    vals.push_back(static_cast<const Column<i32>*>(
                        batches[r.batch_idx][c].get())->At(r.row_idx));
                return std::make_shared<Column<i32>>(std::move(vals));
            }
            case ColType::kInt64: {
                std::vector<i64> vals;
                vals.reserve(n);
                for (const RowRef& r : rows)
                    vals.push_back(static_cast<const Column<i64>*>(
                        batches[r.batch_idx][c].get())->At(r.row_idx));
                return std::make_shared<Column<i64>>(std::move(vals));
            }
            case ColType::kDate: {
                std::vector<ISQDate> vals;
                vals.reserve(n);
                for (const RowRef& r : rows)
                    vals.push_back(static_cast<const Column<ISQDate>*>(
                        batches[r.batch_idx][c].get())->At(r.row_idx));
                return std::make_shared<Column<ISQDate>>(std::move(vals));
            }
            case ColType::kDatetime: {
                std::vector<ISQDatetime> vals;
                vals.reserve(n);
                for (const RowRef& r : rows)
                    vals.push_back(static_cast<const Column<ISQDatetime>*>(
                        batches[r.batch_idx][c].get())->At(r.row_idx));
                return std::make_shared<Column<ISQDatetime>>(std::move(vals));
            }
            case ColType::kString: {
                std::vector<char> data;
                std::vector<ui64> offsets;
                offsets.reserve(n + 1);
                offsets.push_back(0);
                for (const RowRef& r : rows) {
                    std::string_view sv = static_cast<const ColumnString*>(
                        batches[r.batch_idx][c].get())->At(r.row_idx);
                    data.insert(data.end(), sv.begin(), sv.end());
                    offsets.push_back(data.size());
                }
                return std::make_shared<ColumnString>(std::move(data), std::move(offsets));
            }
        }
        return nullptr;
    }

    OpPtr op_;
    std::vector<SortKey> keys_;
    bool done_ = false;
};
