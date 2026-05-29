#pragma once

#include <regex>
#include <string>
#include <string_view>
#include <vector>

#include "column/column_string.h"
#include "expr/expr.h"

class RegexpReplaceExpr : public Expr {
public:
    RegexpReplaceExpr(ExprPtr col_expr, std::string&& pattern,
                      std::string&& replacement)
        : col_expr_(std::move(col_expr)),
          regex_(std::move(pattern)),
          replacement_(ToStdRegexReplacement(replacement)) {}

    ColPtr Eval(const Batch& batch) const override {
        ColPtr col = col_expr_->Eval(batch);
        const ColumnString* col_str =
            static_cast<const ColumnString*>(col.get());

        std::vector<char> data;
        std::vector<ui64> offsets;
        offsets.reserve(batch.CntRows() + 1);
        offsets.push_back(0);

        for (ui64 row = 0; row < batch.CntRows(); ++row) {
            std::string_view src = col_str->At(row);
            std::string replaced =
                std::regex_replace(std::string(src), regex_, replacement_);
            data.insert(data.end(), replaced.begin(), replaced.end());
            offsets.push_back(data.size());
        }

        return std::make_shared<ColumnString>(std::move(data),
                                              std::move(offsets));
    }

private:
    static std::string ToStdRegexReplacement(std::string_view replacement) {
        std::string result;
        result.reserve(replacement.size());
        for (ui64 i = 0; i < replacement.size(); ++i) {
            if (replacement[i] == '\\' && i + 1 < replacement.size() &&
                replacement[i + 1] >= '0' && replacement[i + 1] <= '9') {
                result.push_back('$');
                result.push_back(replacement[++i]);
            } else {
                result.push_back(replacement[i]);
            }
        }
        return result;
    }

    ExprPtr col_expr_;
    std::regex regex_;
    std::string replacement_;
};
