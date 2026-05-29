#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "column/batch.h"
#include "column/col_util.h"
#include "column/column_int_dt.h"
#include "column/column_string.h"
#include "util/alias.h"
#include "util/assert.h"

enum class kOp : ui8 {
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
class ConstVal : public Expr {
public:
    explicit ConstVal(T val) : val_(val) {}

    ColPtr Eval(const Batch& batch) const override {
        return std::make_shared<Column<T>>(
            std::vector<T>(batch.CntRows(), val_));
    }

private:
    T val_;
};

class ConstValString : public Expr {
public:
    explicit ConstValString(std::string&& val) : val_(std::move(val)) {}

    ColPtr Eval(const Batch& batch) const override {
        const ui64 n = batch.CntRows();
        std::vector<char> data;
        data.reserve(n * val_.size());
        std::vector<ui64> offsets(n + 1);
        for (ui64 i = 0; i < n; ++i) {
            data.insert(data.end(), val_.begin(), val_.end());
            offsets[i + 1] = data.size();
        }
        return std::make_shared<ColumnString>(std::move(data),
                                              std::move(offsets));
    }

private:
    std::string val_;
};

class ExtractExpr : public Expr {
public:
    explicit ExtractExpr(ExprPtr col_expr, ui8 ind)
        : col_expr_(std::move(col_expr)), ind_(ind) {}

    ColPtr Eval(const Batch& batch) const override {
        ColPtr col = col_expr_->Eval(batch);
        if (col->GetType() == ColType::kDate) {
            const Column<IsqDate>* col_date =
                static_cast<const Column<IsqDate>*>(col.get());
            std::vector<i16> result(batch.CntRows());
            for (ui64 i = 0; i < batch.CntRows(); ++i) {
                result[i] = col_date->At(i)[ind_];
            }
            return std::make_shared<Column<i16>>(std::move(result));
        } else if (col->GetType() == ColType::kDatetime) {
            const Column<IsqDatetime>* col_date =
                static_cast<const Column<IsqDatetime>*>(col.get());
            std::vector<i16> result(batch.CntRows());
            for (ui64 i = 0; i < batch.CntRows(); ++i) {
                result[i] = col_date->At(i)[ind_];
            }
            return std::make_shared<Column<i16>>(std::move(result));
        }
        SEND_MESSAGE(
            "This expression is not supported with this type of column");
    }

private:
    ExprPtr col_expr_;
    ui8 ind_;
};

class LengthExpr : public Expr {
public:
    explicit LengthExpr(ExprPtr col_expr) : col_expr_(std::move(col_expr)) {}

    ColPtr Eval(const Batch& batch) const override {
        ColPtr col = col_expr_->Eval(batch);
        const ColumnString* col_str =
            static_cast<const ColumnString*>(col.get());
        std::vector<i64> result(batch.CntRows());
        for (ui64 i = 0; i < batch.CntRows(); ++i) {
            result[i] = static_cast<i64>(col_str->At(i).size());
        }
        return std::make_shared<Column<i64>>(std::move(result));
    }

private:
    ExprPtr col_expr_;
};
