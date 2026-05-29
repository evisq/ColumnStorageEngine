#pragma once

#include <memory>

#include "column/column.h"
#include "column/column_int_dt.h"
#include "column/deserialize.h"
#include "expr/arithm_op.h"
#include "expr/cmp_op.h"
#include "expr/expr.h"
#include "util/assert.h"

class BinExpr : public Expr {
public:
    BinExpr(ExprPtr left, ExprPtr right, kOp op)
        : left_(std::move(left)), right_(std::move(right)), op_(op) {}

    virtual ColPtr Eval(const Batch& batch) const override {
        ColPtr left_col = left_->Eval(batch);
        ColPtr right_col = right_->Eval(batch);
        ui8 num_op = static_cast<ui8>(op_);
        if (num_op < 6) {
            return Dispatch<CmpOp>(left_col, right_col, op_);
        }
        if (num_op < 8) {
            std::vector<ui8> lc8 =
                static_cast<Column<ui8>*>(left_col.get())->Data();
            std::vector<ui8> rc8 =
                static_cast<Column<ui8>*>(right_col.get())->Data();
            std::vector<ui8> res(lc8.size());
            for (ui64 i = 0; i < lc8.size(); ++i) {
                res[i] = (op_ == kOp::kAnd ? lc8[i] & rc8[i] : lc8[i] | rc8[i]);
            }
            return std::make_shared<Column<ui8>>(std::move(res));
        }
        if (num_op < 12) {
            return Dispatch<ArithmOp>(left_col, right_col, op_);
        }
        SEND_MESSAGE("Unknown binary operation type");
    }

private:
    ExprPtr left_;
    ExprPtr right_;
    kOp op_;
};
