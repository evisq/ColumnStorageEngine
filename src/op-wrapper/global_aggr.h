#pragma once

#include <vector>

#include "aggr/aggr.h"
#include "column/batch.h"
#include "column/column_int_dt.h"
#include "expr/expr.h"
#include "op-wrapper/op.h"

class GlobalAggrOp : public Op {
public:
    GlobalAggrOp(OpPtr op, std::vector<std::shared_ptr<Aggr>>&& aggrs,
                 std::vector<ExprPtr>&& agg_exprs)
        : op_(std::move(op)),
          aggrs_(std::move(aggrs)),
          agg_exprs_(std::move(agg_exprs)) {}

    bool HasNext() override { return !done_; }

    Batch Next() override {
        while (op_->HasNext()) {
            Batch batch = op_->Next();
            Column<ui8> mask(batch.GetMask());
            for (ui64 i = 0; i < aggrs_.size(); ++i) {
                aggrs_[i]->Update(agg_exprs_[i]->Eval(batch), mask);
            }
        }
        done_ = true;

        std::vector<ColPtr> cols;
        cols.reserve(aggrs_.size());
        for (std::shared_ptr<Aggr>& aggr : aggrs_) {
            cols.push_back(aggr->Result());
        }
        return Batch(std::move(cols), 1);
    }

private:
    OpPtr op_;
    std::vector<std::shared_ptr<Aggr>> aggrs_;
    std::vector<ExprPtr> agg_exprs_;
    bool done_ = false;
};
