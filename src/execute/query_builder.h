#pragma once

#include <climits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "aggr/aggr.h"
#include "aggr/aggr_avg.h"
#include "aggr/aggr_count_distinct.h"
#include "aggr/aggr_minmax.h"
#include "aggr/aggr_sum.h"
#include "expr/expr.h"
#include "op-wrapper/op.h"

namespace query {

using AggrPtr = std::shared_ptr<Aggr>;

struct SortKey {
    ui64 col_idx;
    bool asc;
};

struct QuerySpec {
    enum class ResultKind {
        kRows,
        kGlobalAggr,
        kGroupBy,
    };

    std::vector<ui64> scan_cols;
    bool scan_all = false;
    ExprPtr filter;

    ResultKind result_kind = ResultKind::kRows;
    std::vector<ExprPtr> group_exprs;
    std::vector<AggrPtr> aggrs;
    std::vector<ExprPtr> aggr_exprs;
    ui64 group_limit = ULLONG_MAX;
    ui64 group_offset = 0;

    ExprPtr post_filter;
    std::vector<SortKey> order_by;
    std::optional<ui64> order_limit;
    ui64 order_offset = 0;

    std::vector<ui64> projection;
};

ExprPtr Col(ui64 ind);

template <typename T>
ExprPtr Val(T val) {
    return std::make_shared<ConstVal<T>>(val);
}

ExprPtr Str(std::string val);
ExprPtr Bin(ExprPtr left, ExprPtr right, kOp op);
ExprPtr Eq(ExprPtr left, ExprPtr right);
ExprPtr Ne(ExprPtr left, ExprPtr right);
ExprPtr Gt(ExprPtr left, ExprPtr right);
ExprPtr Ge(ExprPtr left, ExprPtr right);
ExprPtr Le(ExprPtr left, ExprPtr right);
ExprPtr Add(ExprPtr left, ExprPtr right);
ExprPtr Sub(ExprPtr left, ExprPtr right);
ExprPtr And(ExprPtr left, ExprPtr right);
ExprPtr Or(ExprPtr left, ExprPtr right);
ExprPtr Like(ExprPtr value, std::string pattern);
ExprPtr NotLike(ExprPtr value, std::string pattern);
ExprPtr Extract(ExprPtr value, ui8 ind);
ExprPtr Length(ExprPtr value);
ExprPtr DateForm(ExprPtr value, std::string pattern);
ExprPtr CaseWhen(std::vector<std::pair<ExprPtr, ExprPtr>> cases);
ExprPtr RegexpReplace(ExprPtr value, std::string pattern,
                      std::string replacement);

AggrPtr Count();
AggrPtr CountDistinctString();
AggrPtr MinString();

template <typename T>
AggrPtr Sum() {
    return std::make_shared<AggrSum<T>>();
}

template <typename T>
AggrPtr Avg() {
    return std::make_shared<AggrAvg<T>>();
}

template <typename T>
AggrPtr CountDistinct() {
    return std::make_shared<AggrCountDistinct<T>>();
}

template <typename T>
AggrPtr Min() {
    return std::make_shared<AggrMin<T>>();
}

template <typename T>
AggrPtr Max() {
    return std::make_shared<AggrMax<T>>();
}

OpPtr BuildPlan(const std::string& isq_path, QuerySpec spec);

}
