#pragma once

#include <memory>

#include "column/column.h"
#include "util/alias.h"

class Expr;
using ExprPtr = std::shared_ptr<Expr>;

class Batch {
public:
    Batch(std::vector<ColPtr> cols, ui64 cnt_rows)
        : cols_(std::move(cols)), mask_(cnt_rows, 1), cnt_rows_(cnt_rows) {}
    Batch(std::vector<ColPtr> cols, std::vector<ui8> mask, ui64 cnt_rows)
        : cols_(std::move(cols)), mask_(std::move(mask)), cnt_rows_(cnt_rows) {}
    ~Batch() = default;

    ui64 NumCols() const { return cols_.size(); }
    ui64 CntRows() const { return cnt_rows_; }
    const std::vector<ui8>& GetMask() const { return mask_; }

    void FilterMask(ExprPtr filter);
    void FilterMaskConj(ExprPtr filter);

    const ColPtr& operator[](ui64 j) const { return cols_[j]; }

private:
    std::vector<ColPtr> cols_;
    std::vector<ui8> mask_;
    ui64 cnt_rows_ = 0;
};