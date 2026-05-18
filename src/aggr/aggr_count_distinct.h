#pragma once

#include <string>
#include <unordered_set>

#include "aggr.h"
#include "column/column_string.h"

template <typename T>
class AggrCountDistinct : public Aggr {
public:
    void Update(const ColPtr& col, const Column<ui8>& mask) override {
        const Column<T>* c = static_cast<const Column<T>*>(col.get());
        for (ui64 i = 0; i < mask.Size(); ++i)
            if (mask.At(i)) seen_.insert(c->At(i));
    }

    ColPtr Result() override {
        return std::make_shared<Column<i64>>(std::vector<i64>{static_cast<i64>(seen_.size())});
    }
private:
    std::unordered_set<T> seen_;
};

class AggrCountDistinctString : public Aggr {
public:
    void Update(const ColPtr& col, const Column<ui8>& mask) override {
        const ColumnString* c = static_cast<const ColumnString*>(col.get());
        for (ui64 i = 0; i < mask.Size(); ++i)
            if (mask.At(i)) seen_.emplace(c->At(i));
    }

    ColPtr Result() override {
        return std::make_shared<Column<i64>>(std::vector<i64>{static_cast<i64>(seen_.size())});
    }
private:
    std::unordered_set<std::string> seen_;
};
