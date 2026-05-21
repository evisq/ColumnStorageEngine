#include "batch.h"

#include "expr/expr.h"

void Batch::FilterMask(ExprPtr filter) {
    Column<ui8> res = *static_cast<Column<ui8>*>(filter->Eval(*this).get());
    for (ui64 i = 0; i < cnt_rows_; ++i) {
        mask_[i] = res.At(i);
    }
}

void Batch::FilterMaskConj(ExprPtr filter) {
    Column<ui8> res = *static_cast<Column<ui8>*>(filter->Eval(*this).get());
    for (ui64 i = 0; i < cnt_rows_; ++i) {
        mask_[i] &= res.At(i);
    }
}
