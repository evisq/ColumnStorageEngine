#pragma once

#include "aggr/aggr.h"

template <typename T>
class AggrAvg : public Aggr {
public:
    void Update(const ColPtr& col, const Column<ui8>& mask) override {
        const Column<T>* col_ref = static_cast<const Column<T>*>(col.get());
        for (ui64 i = 0; i < mask.Size(); ++i) {
            if (mask.At(i)) {
                sum_ += col_ref->At(i);
                ++count_;
            }
        }
    }

    void UpdateRow(ColPtr col, ui64 row) override {
        sum_ += static_cast<const Column<T>*>(col.get())->At(row);
        ++count_;
    }

    std::shared_ptr<Aggr> Clone() const override {
        return std::make_shared<AggrAvg<T>>();
    }

    ColPtr Result() override {
        if (!count_) {
            return std::make_shared<Column<i64>>(std::vector<i64>{0});
        }
        return std::make_shared<Column<i64>>(
            std::vector<i64>{static_cast<i64>(sum_ / count_)});
    }

private:
    __int128_t sum_ = 0;
    __int128_t count_ = 0;
};
