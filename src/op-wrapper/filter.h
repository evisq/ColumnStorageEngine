#pragma once

#include "column/batch.h"
#include "expr/expr.h"
#include "op.h"

class FilterOp : public Op {
public:
    FilterOp(OpPtr op, ExprPtr filter) : op_(op), filter_(filter) {}

    bool HasNext() override { return op_->HasNext(); }
    Batch Next() override {
        Batch batch = op_->Next();
        batch.FilterMask(filter_);
        return batch;
    }

private:
    OpPtr op_;
    ExprPtr filter_;
};