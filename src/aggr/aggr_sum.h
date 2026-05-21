#pragma once

#include "aggr.h"

template <typename T>
class AggrSum : public Aggr {
public:
    void Update(const ColPtr& col, const Column<ui8>& mask) override {
        const Column<T>* col_ref = static_cast<const Column<T>*>(col.get());
        for (ui64 i = 0; i < mask.Size(); ++i) {
            sum_ += mask.At(i) * col_ref->At(i);
        }
    }

    void UpdateRow(ColPtr col, ui64 row) override {
        sum_ += static_cast<const Column<T>*>(col.get())->At(row);
    }

    std::shared_ptr<Aggr> Clone() const override {
        return std::make_shared<AggrSum<T>>();
    }

    ColPtr Result() override {
        return std::make_shared<Column<i64>>(
            std::vector<i64>{static_cast<i64>(sum_)});
    }

private:
    i64 sum_ = 0;
};