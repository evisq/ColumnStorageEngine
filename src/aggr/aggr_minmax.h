#pragma once

#include <string>
#include <string_view>

#include "aggr.h"
#include "column/column_string.h"

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

    void UpdateRow(ColPtr col, ui64 row) override {
        T v = static_cast<const Column<T>*>(col.get())->At(row);
        if (first_ || v < min_) {
            min_ = v;
            first_ = false;
        }
    }

    std::shared_ptr<Aggr> Clone() const override {
        return std::make_shared<AggrMin<T>>();
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

    void UpdateRow(ColPtr col, ui64 row) override {
        T v = static_cast<const Column<T>*>(col.get())->At(row);
        if (first_ || v > max_) {
            max_ = v;
            first_ = false;
        }
    }

    std::shared_ptr<Aggr> Clone() const override {
        return std::make_shared<AggrMax<T>>();
    }

    ColPtr Result() override {
        return std::make_shared<Column<T>>(std::vector<T>{max_});
    }

private:
    T max_{};
    bool first_ = true;
};

class AggrMinString : public Aggr {
public:
    void Update(const ColPtr& col, const Column<ui8>& mask) override {
        const ColumnString* c = static_cast<const ColumnString*>(col.get());
        for (ui64 i = 0; i < mask.Size(); ++i) {
            if (!mask.At(i)) continue;
            std::string_view sv = c->At(i);
            if (first_ || sv < min_) { min_ = sv; first_ = false; }
        }
    }

    void UpdateRow(ColPtr col, ui64 row) override {
        std::string_view sv = static_cast<const ColumnString*>(col.get())->At(row);
        if (first_ || sv < min_) { min_ = sv; first_ = false; }
    }

    std::shared_ptr<Aggr> Clone() const override {
        return std::make_shared<AggrMinString>();
    }

    ColPtr Result() override {
        std::vector<char> data(min_.begin(), min_.end());
        std::vector<ui64> offsets = {0, min_.size()};
        return std::make_shared<ColumnString>(std::move(data), std::move(offsets));
    }

private:
    std::string min_;
    bool first_ = true;
};

class AggrMaxString : public Aggr {
public:
    void Update(const ColPtr& col, const Column<ui8>& mask) override {
        const ColumnString* c = static_cast<const ColumnString*>(col.get());
        for (ui64 i = 0; i < mask.Size(); ++i) {
            if (!mask.At(i)) continue;
            std::string_view sv = c->At(i);
            if (first_ || sv > max_) { max_ = sv; first_ = false; }
        }
    }

    void UpdateRow(ColPtr col, ui64 row) override {
        std::string_view sv = static_cast<const ColumnString*>(col.get())->At(row);
        if (first_ || sv > max_) { max_ = sv; first_ = false; }
    }

    std::shared_ptr<Aggr> Clone() const override {
        return std::make_shared<AggrMaxString>();
    }

    ColPtr Result() override {
        std::vector<char> data(max_.begin(), max_.end());
        std::vector<ui64> offsets = {0, max_.size()};
        return std::make_shared<ColumnString>(std::move(data), std::move(offsets));
    }

private:
    std::string max_;
    bool first_ = true;
};