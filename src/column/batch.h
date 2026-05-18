#pragma once

#include "column/column.h"

struct Batch {
    std::vector<ColPtr> cols;
    ui64 cnt_rows = 0;

    ui64 NumCols() const { return cols.size(); }

    const ColPtr &operator[](ui64 j) const { return cols[j]; }
};