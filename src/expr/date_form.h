#pragma once

#include <string>
#include <vector>

#include "column/column_int_dt.h"
#include "column/column_string.h"
#include "expr/expr.h"

class DateFormExpr : public Expr {
public:
    DateFormExpr(ExprPtr col_expr, std::string&& pattern)
        : col_expr_(std::move(col_expr)), pattern_(std::move(pattern)) {
        for (size_t ind = 0; ind + 1 < pattern_.size(); ++ind) {
            if (pattern_[ind] == '%') {
                switch (pattern_[ind + 1]) {
                    case 'Y': {
                        replacements_.push_back({0, static_cast<i32>(ind), 4});
                        pattern_.insert(ind + 2, 2, 'X');
                        break;
                    }
                    case 'm': {
                        replacements_.push_back({1, static_cast<i32>(ind), 2});
                        break;
                    }
                    case 'd': {
                        replacements_.push_back({2, static_cast<i32>(ind), 2});
                        break;
                    }
                    case 'H': {
                        replacements_.push_back({3, static_cast<i32>(ind), 2});
                        break;
                    }
                    case 'M': {
                        replacements_.push_back({4, static_cast<i32>(ind), 2});
                        break;
                    }
                    case 'S': {
                        replacements_.push_back({5, static_cast<i32>(ind), 2});
                        break;
                    }
                }
            }
        }
    }

    ColPtr Eval(const Batch& batch) const override {
        ColPtr col = col_expr_->Eval(batch);
        const Column<IsqDatetime>* col_date =
            static_cast<const Column<IsqDatetime>*>(col.get());

        const ui64 cnt_rows = batch.CntRows();
        const ui64 row_size = pattern_.size();
        std::vector<char> data;
        data.reserve(cnt_rows * row_size);
        std::vector<ui64> offsets(cnt_rows + 1);

        for (ui64 row_ind = 0; row_ind < cnt_rows; ++row_ind) {
            data.insert(data.end(), pattern_.begin(), pattern_.end());
            offsets[row_ind + 1] = data.size();

            const IsqDatetime& date = col_date->At(row_ind);
            char* row = row_size ? data.data() + row_ind * row_size : nullptr;
            for (const Replacement& replacement : replacements_) {
                StrReplace(row, date[replacement.date_ind], replacement.ind,
                           replacement.cnt);
            }
        }
        return std::make_shared<ColumnString>(std::move(data),
                                              std::move(offsets));
    }

private:
    static void StrReplace(char* str, i16 num, i32 ind, i8 cnt) {
        for (int i = ind + cnt - 1; i >= ind; --i) {
            str[i] = '0' + (num % 10);
            num /= 10;
        }
    }

    struct Replacement {
        ui8 date_ind;
        i32 ind;
        i8 cnt;
    };

    std::vector<Replacement> replacements_;
    ExprPtr col_expr_;
    std::string pattern_;
};
