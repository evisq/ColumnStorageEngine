#pragma once

#include "column/column.h"
#include "column/column_int_dt.h"

class Aggr {
public:
    virtual void UpdateRow(ColPtr col, ui64 row) = 0;
    virtual std::shared_ptr<Aggr> Clone() const = 0;
    virtual ~Aggr() = default;
    virtual void Update(const ColPtr& col, const Column<ui8>& mask) = 0;
    virtual ColPtr Result() = 0;
};