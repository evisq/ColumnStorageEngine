#pragma once

#include <memory>

#include "column/batch.h"
#include "column/column_int_dt.h"
#include "util/alias.h"

enum class Op : ui8 {
    kLs = 0,
    kLe = 1,
    kGs = 2,
    kGe = 3,
    kEq = 4,
    kNe = 5,
    kOr = 6,
    kAnd = 7,
    kAdd = 8,
    kSub = 9,
    kMul = 10,
    kDiv = 11,
};

class Expr {
public:
    virtual ~Expr() = default;
    virtual ColPtr Eval(const Batch& batch) const = 0;
};

using ExprPtr = std::shared_ptr<Expr>;

class GetCol : public Expr {
public:
    explicit GetCol(ui64 idx) : idx_(idx) {}

    ColPtr Eval(const Batch& batch) const override { return batch[idx_]; }

private:
    ui64 idx_;
};

template <typename T>
class СonstVal : public Expr {
public:
    explicit СonstVal(T val) : val_(val) {}

    ColPtr Eval(const Batch& batch) const override {
        return std::make_shared<Column<T>>(
            std::vector<T>(batch.cnt_rows, val_));
    }

private:
    T val_;
};
