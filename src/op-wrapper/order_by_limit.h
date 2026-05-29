#pragma once

#include <algorithm>
#include <climits>
#include <queue>
#include <vector>

#include "column/col_ops.h"
#include "op-wrapper/op.h"

class OrderByLimitOp : public Op {
public:
    struct SortKey {
        ui64 col_idx;
        bool asc;
    };

    OrderByLimitOp(OpPtr op, std::vector<SortKey>&& keys, ui64 limit,
                   ui64 offset = 0)
        : op_(std::move(op)),
          keys_(std::move(keys)),
          limit_(limit),
          offset_(offset),
          cmp_{keys_},
          pq_(cmp_) {}

    bool HasNext() override { return !done_; }

    Batch Next() override {
        const ui64 cap = CalcCap();
        while (op_->HasNext()) {
            Batch b = op_->Next();
            if (col_protos_.empty()) {
                for (ui64 c = 0; c < b.NumCols(); ++c) {
                    col_protos_.push_back(b[c]);
                }
            }
            const std::vector<ui8>& mask = b.GetMask();
            for (ui64 row = 0; row < b.CntRows(); ++row) {
                if (!mask[row]) {
                    continue;
                }
                std::vector<ColPtr> r = ExtractRow(b, row);
                if (pq_.size() < cap) {
                    pq_.push(std::move(r));
                } else if (!pq_.empty() && cmp_(r, pq_.top())) {
                    pq_.pop();
                    pq_.push(std::move(r));
                }
            }
        }
        done_ = true;
        return BuildResult();
    }

private:
    ui64 CalcCap() const {
        if (offset_ > ULLONG_MAX - limit_) {
            return ULLONG_MAX;
        }
        return limit_ + offset_;
    }

    struct RowCmp {
        std::vector<SortKey> keys;
        bool operator()(const std::vector<ColPtr>& a,
                        const std::vector<ColPtr>& b) const {
            for (const SortKey& k : keys) {
                int cmp = ColOps::CompareCell(a[k.col_idx], 0, b[k.col_idx], 0);
                if (cmp != 0) {
                    return k.asc ? cmp < 0 : cmp > 0;
                }
            }
            return false;
        }
    };

    using PQ = std::priority_queue<std::vector<ColPtr>,
                                   std::vector<std::vector<ColPtr>>, RowCmp>;

    static std::vector<ColPtr> ExtractRow(const Batch& b, ui64 row) {
        std::vector<ColPtr> result;
        result.reserve(b.NumCols());
        for (ui64 c = 0; c < b.NumCols(); ++c) {
            result.push_back(ColOps::SliceRow(b[c], row));
        }
        return result;
    }

    Batch BuildResult() {
        const ui64 total = pq_.size();
        std::vector<std::vector<ColPtr>> rows;
        rows.reserve(total);
        while (!pq_.empty()) {
            rows.push_back(pq_.top());
            pq_.pop();
        }
        std::reverse(rows.begin(), rows.end());
        const ui64 start = std::min(offset_, total);
        const ui64 count = std::min(limit_, total - start);
        if (count == 0 || rows.empty()) {
            std::vector<ColPtr> result_cols;
            result_cols.reserve(col_protos_.size());
            for (const ColPtr& col : col_protos_) {
                result_cols.push_back(ColOps::EmptyLike(col));
            }
            return Batch(std::move(result_cols), 0);
        }
        const ui64 num_cols = rows[start].size();
        std::vector<ColPtr> result_cols;
        result_cols.reserve(num_cols);
        for (ui64 c = 0; c < num_cols; ++c) {
            result_cols.push_back(ColOps::ConcatCol(rows, c, start, count));
        }
        return Batch(std::move(result_cols), count);
    }

    OpPtr op_;
    std::vector<SortKey> keys_;
    ui64 limit_;
    ui64 offset_;
    RowCmp cmp_;
    PQ pq_;
    std::vector<ColPtr> col_protos_;
    bool done_ = false;
};
