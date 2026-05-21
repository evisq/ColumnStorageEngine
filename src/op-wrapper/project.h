#pragma once

#include <vector>

#include "op.h"

class ProjectOp : public Op {
public:
    ProjectOp(OpPtr op, std::vector<ui64> cols)
        : op_(std::move(op)), cols_(std::move(cols)) {}

    bool HasNext() override { return op_->HasNext(); }

    Batch Next() override {
        Batch b = op_->Next();
        std::vector<ColPtr> result_cols;
        result_cols.reserve(cols_.size());
        for (ui64 idx : cols_) result_cols.push_back(b[idx]);
        return Batch(std::move(result_cols), b.CntRows());
    }

private:
    OpPtr op_;
    std::vector<ui64> cols_;
};
