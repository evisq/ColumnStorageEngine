#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "column/column_int_dt.h"
#include "column/column_string.h"
#include "expr.h"

class LikeExpr : public Expr {
public:
    LikeExpr(ExprPtr col_expr, std::string pattern, bool negated = false)
        : col_expr_(std::move(col_expr)), pattern_(std::move(pattern)),
          negated_(negated) {}

    ColPtr Eval(const Batch& batch) const override {
        ColPtr col = col_expr_->Eval(batch);
        const ColumnString* c = static_cast<const ColumnString*>(col.get());
        std::vector<ui8> result;
        result.reserve(batch.CntRows());
        for (ui64 i = 0; i < batch.CntRows(); ++i)
            result.push_back(static_cast<ui8>(Match(c->At(i), pattern_) ^ negated_));
        return std::make_shared<Column<ui8>>(std::move(result));
    }

private:
    static bool Match(std::string_view s, std::string_view p) {
        if (p.empty()) return s.empty();
        if (p[0] == '%') {
            for (ui64 i = 0; i <= s.size(); ++i)
                if (Match(s.substr(i), p.substr(1))) return true;
            return false;
        }
        if (s.empty()) return false;
        if (p[0] == '_' || p[0] == s[0])
            return Match(s.substr(1), p.substr(1));
        return false;
    }

    ExprPtr col_expr_;
    std::string pattern_;
    bool negated_;
};
