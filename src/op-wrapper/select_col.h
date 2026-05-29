#pragma once

#include <vector>

#include "op-wrapper/op.h"

class SelectColOp : public Op {
public:
    SelectColOp(OpPtr op, std::vector<ui64>&& cols)
        : op_(std::move(op)), cols_(std::move(cols)) {}

    bool HasNext() override { return op_->HasNext(); }

    Batch Next() override {
        Batch b = op_->Next();
        if (b.NumCols() == 0) {
            return Batch({}, b.GetMask(), b.CntRows());
        }
        std::vector<ColPtr> result_cols(cols_.size());
        for (ui64 i = 0; i < cols_.size(); ++i) {
            result_cols[i] = b[cols_[i]];
        }
        return Batch(std::move(result_cols), b.GetMask(), b.CntRows());
    }

private:
    OpPtr op_;
    std::vector<ui64> cols_;
};
