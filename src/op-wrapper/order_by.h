#pragma once

#include <algorithm>
#include <string_view>
#include <vector>

#include "column/col_ops.h"
#include "column/column_int_dt.h"
#include "column/column_string.h"
#include "op-wrapper/op.h"

class OrderByOp : public Op {
public:
    struct SortKey {
        ui64 col_idx;
        bool asc;
    };

    OrderByOp(OpPtr op, std::vector<SortKey>&& keys)
        : op_(std::move(op)), keys_(std::move(keys)) {}

    bool HasNext() override { return !done_; }

    Batch Next() override {
        std::vector<Batch> batches;
        std::vector<RowRef> rows;

        while (op_->HasNext()) {
            Batch b = op_->Next();
            if (col_protos_.empty()) {
                for (ui64 c = 0; c < b.NumCols(); ++c) {
                    col_protos_.push_back(b[c]);
                }
            }
            const std::vector<ui8>& mask = b.GetMask();
            ui64 bi = batches.size();
            for (ui64 row = 0; row < b.CntRows(); ++row) {
                if (mask[row]) {
                    rows.push_back({bi, row});
                }
            }
            batches.push_back(std::move(b));
        }
        done_ = true;

        if (rows.empty()) {
            std::vector<ColPtr> result_cols;
            result_cols.reserve(col_protos_.size());
            for (const ColPtr& col : col_protos_) {
                result_cols.push_back(ColOps::EmptyLike(col));
            }
            return Batch(std::move(result_cols), 0);
        }

        std::sort(rows.begin(), rows.end(),
                  [&](const RowRef& a, const RowRef& b) {
                      for (const SortKey& k : keys_) {
                          int cmp = ColOps::CompareCell(
                              batches[a.batch_idx][k.col_idx], a.row_idx,
                              batches[b.batch_idx][k.col_idx], b.row_idx);
                          if (cmp != 0) {
                              return k.asc ? cmp < 0 : cmp > 0;
                          }
                      }
                      return false;
                  });

        const ui64 num_cols = batches[0].NumCols();
        const ui64 n = rows.size();
        std::vector<ColPtr> result_cols(num_cols);
        for (ui64 i = 0; i < num_cols; ++i) {
            result_cols[i] = BuildCol(batches, rows, i);
        }
        return Batch(std::move(result_cols), n);
    }

private:
    struct RowRef {
        ui64 batch_idx;
        ui64 row_idx;
    };

    static ColPtr BuildCol(const std::vector<Batch>& batches,
                           const std::vector<RowRef>& rows, ui64 ind) {
        const ui64 n = rows.size();
        switch (batches[0][ind]->GetType()) {
            case ColType::kInt8: {
                std::vector<i8> vals;
                vals.reserve(n);
                for (const RowRef& r : rows) {
                    vals.push_back(static_cast<const Column<i8>*>(
                                       batches[r.batch_idx][ind].get())
                                       ->At(r.row_idx));
                }
                return std::make_shared<Column<i8>>(std::move(vals));
            }
            case ColType::kInt16: {
                std::vector<i16> vals;
                vals.reserve(n);
                for (const RowRef& r : rows) {
                    vals.push_back(static_cast<const Column<i16>*>(
                                       batches[r.batch_idx][ind].get())
                                       ->At(r.row_idx));
                }
                return std::make_shared<Column<i16>>(std::move(vals));
            }
            case ColType::kInt32: {
                std::vector<i32> vals;
                vals.reserve(n);
                for (const RowRef& r : rows) {
                    vals.push_back(static_cast<const Column<i32>*>(
                                       batches[r.batch_idx][ind].get())
                                       ->At(r.row_idx));
                }
                return std::make_shared<Column<i32>>(std::move(vals));
            }
            case ColType::kInt64: {
                std::vector<i64> vals;
                vals.reserve(n);
                for (const RowRef& r : rows) {
                    vals.push_back(static_cast<const Column<i64>*>(
                                       batches[r.batch_idx][ind].get())
                                       ->At(r.row_idx));
                }
                return std::make_shared<Column<i64>>(std::move(vals));
            }
            case ColType::kDate: {
                std::vector<IsqDate> vals;
                vals.reserve(n);
                for (const RowRef& r : rows) {
                    vals.push_back(static_cast<const Column<IsqDate>*>(
                                       batches[r.batch_idx][ind].get())
                                       ->At(r.row_idx));
                }
                return std::make_shared<Column<IsqDate>>(std::move(vals));
            }
            case ColType::kDatetime: {
                std::vector<IsqDatetime> vals;
                vals.reserve(n);
                for (const RowRef& r : rows) {
                    vals.push_back(static_cast<const Column<IsqDatetime>*>(
                                       batches[r.batch_idx][ind].get())
                                       ->At(r.row_idx));
                }
                return std::make_shared<Column<IsqDatetime>>(std::move(vals));
            }
            case ColType::kString: {
                std::vector<char> data;
                std::vector<ui64> offsets;
                offsets.reserve(n + 1);
                offsets.push_back(0);
                for (const RowRef& r : rows) {
                    std::string_view sv = static_cast<const ColumnString*>(
                                              batches[r.batch_idx][ind].get())
                                              ->At(r.row_idx);
                    data.insert(data.end(), sv.begin(), sv.end());
                    offsets.push_back(data.size());
                }
                return std::make_shared<ColumnString>(std::move(data),
                                                      std::move(offsets));
            }
        }
        return nullptr;
    }

    OpPtr op_;
    std::vector<SortKey> keys_;
    std::vector<ColPtr> col_protos_;
    bool done_ = false;
};
