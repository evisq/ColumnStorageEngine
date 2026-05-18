#pragma once

#include "aggr.h"

template <typename T>
class AggrMin : public Aggr {
public:
    void Update(const ColPtr& col, const Column<ui8>& mask) override {
        const Column<T>* col_ref = static_cast<const Column<T>*>(col.get());
        for (ui64 i = 0; i < mask.Size(); ++i) {
            if (mask.At(i) && (first_ || col_ref->At(i) < min_)) {
                min_ = col_ref->At(i);
                first_ = false;
            }
        }
    }

    ColPtr Result() override {
        return std::make_shared<Column<T>>(std::vector<T>{min_});
    }
private:
    T min_{};
    bool first_ = true;
};

template <typename T>
class AggrMax : public Aggr {
public:
    void Update(const ColPtr& col, const Column<ui8>& mask) override {
        const Column<T>* col_ref = static_cast<const Column<T>*>(col.get());
        for (ui64 i = 0; i < mask.Size(); ++i) {
            if (mask.At(i) && (first_ || col_ref->At(i) > max_)) {
                max_ = col_ref->At(i);
                first_ = false;
            }
        }
    }

    ColPtr Result() override {
        return std::make_shared<Column<T>>(std::vector<T>{max_});
    }
private:
    T max_{};
    bool first_ = true;
};