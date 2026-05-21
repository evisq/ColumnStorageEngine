#pragma once

#include "aggr.h"

class AggrCount : public Aggr {
public:
    void Update(const ColPtr&, const Column<ui8>& mask) override {
        for (ui64 i = 0; i < mask.Size(); ++i) {
            count_ += mask.At(i);
        }
    }

    void UpdateRow(ColPtr, ui64) override { ++count_; }

    std::shared_ptr<Aggr> Clone() const override {
        return std::make_shared<AggrCount>();
    }

    ColPtr Result() override {
        return std::make_shared<Column<i64>>(
            std::vector<i64>{static_cast<i64>(count_)});
    }

private:
    ui64 count_ = 0;
};