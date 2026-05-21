#pragma once

#include <algorithm>
#include <string_view>
#include <vector>

#include "column/column_int_dt.h"
#include "column/column_string.h"
#include "op.h"

class LimitOp : public Op {
public:
    LimitOp(OpPtr op, ui64 limit, ui64 offset = 0)
        : op_(std::move(op)), limit_(limit), offset_(offset) {}

    bool HasNext() override { return !done_; }

    Batch Next() override {
        Batch batch = op_->Next();
        done_ = true;
        const ui64 n = batch.CntRows();
        const ui64 start = std::min(offset_, n);
        const ui64 end = std::min(start + limit_, n);
        if (start == 0 && end == n) return batch;
        std::vector<ColPtr> result_cols;
        result_cols.reserve(batch.NumCols());
        for (ui64 c = 0; c < batch.NumCols(); ++c)
            result_cols.push_back(SliceCol(batch[c], start, end));
        return Batch(std::move(result_cols), end - start);
    }

private:
    static ColPtr SliceCol(const ColPtr& col, ui64 start, ui64 end) {
        const ui64 n = end - start;
        switch (col->GetType()) {
            case ColType::kInt8: {
                const Column<i8>* c = static_cast<const Column<i8>*>(col.get());
                std::vector<i8> vals; vals.reserve(n);
                for (ui64 i = start; i < end; ++i) vals.push_back(c->At(i));
                return std::make_shared<Column<i8>>(std::move(vals));
            }
            case ColType::kInt16: {
                const Column<i16>* c = static_cast<const Column<i16>*>(col.get());
                std::vector<i16> vals; vals.reserve(n);
                for (ui64 i = start; i < end; ++i) vals.push_back(c->At(i));
                return std::make_shared<Column<i16>>(std::move(vals));
            }
            case ColType::kInt32: {
                const Column<i32>* c = static_cast<const Column<i32>*>(col.get());
                std::vector<i32> vals; vals.reserve(n);
                for (ui64 i = start; i < end; ++i) vals.push_back(c->At(i));
                return std::make_shared<Column<i32>>(std::move(vals));
            }
            case ColType::kInt64: {
                const Column<i64>* c = static_cast<const Column<i64>*>(col.get());
                std::vector<i64> vals; vals.reserve(n);
                for (ui64 i = start; i < end; ++i) vals.push_back(c->At(i));
                return std::make_shared<Column<i64>>(std::move(vals));
            }
            case ColType::kDate: {
                const Column<ISQDate>* c = static_cast<const Column<ISQDate>*>(col.get());
                std::vector<ISQDate> vals; vals.reserve(n);
                for (ui64 i = start; i < end; ++i) vals.push_back(c->At(i));
                return std::make_shared<Column<ISQDate>>(std::move(vals));
            }
            case ColType::kDatetime: {
                const Column<ISQDatetime>* c = static_cast<const Column<ISQDatetime>*>(col.get());
                std::vector<ISQDatetime> vals; vals.reserve(n);
                for (ui64 i = start; i < end; ++i) vals.push_back(c->At(i));
                return std::make_shared<Column<ISQDatetime>>(std::move(vals));
            }
            case ColType::kString: {
                const ColumnString* c = static_cast<const ColumnString*>(col.get());
                std::vector<char> data; std::vector<ui64> offsets;
                offsets.reserve(n + 1); offsets.push_back(0);
                for (ui64 i = start; i < end; ++i) {
                    std::string_view sv = c->At(i);
                    data.insert(data.end(), sv.begin(), sv.end());
                    offsets.push_back(data.size());
                }
                return std::make_shared<ColumnString>(std::move(data), std::move(offsets));
            }
        }
        return nullptr;
    }

    OpPtr op_;
    ui64 limit_;
    ui64 offset_;
    bool done_ = false;
};
