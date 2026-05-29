#include "execute/query_builder.h"

#include <utility>

#include "aggr/aggr_count.h"
#include "expr/bin_expr.h"
#include "expr/case_when.h"
#include "expr/date_form.h"
#include "expr/like_expr.h"
#include "expr/regexp_replace.h"
#include "op-wrapper/filter.h"
#include "op-wrapper/global_aggr.h"
#include "op-wrapper/group_by.h"
#include "op-wrapper/order_by.h"
#include "op-wrapper/order_by_limit.h"
#include "op-wrapper/scan.h"
#include "op-wrapper/select_col.h"

namespace query {

ExprPtr Col(ui64 ind) { return std::make_shared<GetCol>(ind); }

ExprPtr Str(std::string val) {
    return std::make_shared<ConstValString>(std::move(val));
}

ExprPtr Bin(ExprPtr left, ExprPtr right, kOp op) {
    return std::make_shared<BinExpr>(std::move(left), std::move(right), op);
}

ExprPtr Eq(ExprPtr left, ExprPtr right) {
    return Bin(std::move(left), std::move(right), kOp::kEq);
}

ExprPtr Ne(ExprPtr left, ExprPtr right) {
    return Bin(std::move(left), std::move(right), kOp::kNe);
}

ExprPtr Gt(ExprPtr left, ExprPtr right) {
    return Bin(std::move(left), std::move(right), kOp::kGs);
}

ExprPtr Ge(ExprPtr left, ExprPtr right) {
    return Bin(std::move(left), std::move(right), kOp::kGe);
}

ExprPtr Le(ExprPtr left, ExprPtr right) {
    return Bin(std::move(left), std::move(right), kOp::kLe);
}

ExprPtr Add(ExprPtr left, ExprPtr right) {
    return Bin(std::move(left), std::move(right), kOp::kAdd);
}

ExprPtr Sub(ExprPtr left, ExprPtr right) {
    return Bin(std::move(left), std::move(right), kOp::kSub);
}

ExprPtr And(ExprPtr left, ExprPtr right) {
    return Bin(std::move(left), std::move(right), kOp::kAnd);
}

ExprPtr Or(ExprPtr left, ExprPtr right) {
    return Bin(std::move(left), std::move(right), kOp::kOr);
}

ExprPtr Like(ExprPtr value, std::string pattern) {
    return std::make_shared<LikeExpr>(std::move(value), std::move(pattern));
}

ExprPtr NotLike(ExprPtr value, std::string pattern) {
    return std::make_shared<LikeExpr>(std::move(value), std::move(pattern),
                                      true);
}

ExprPtr Extract(ExprPtr value, ui8 ind) {
    return std::make_shared<ExtractExpr>(std::move(value), ind);
}

ExprPtr Length(ExprPtr value) {
    return std::make_shared<LengthExpr>(std::move(value));
}

ExprPtr DateForm(ExprPtr value, std::string pattern) {
    return std::make_shared<DateFormExpr>(std::move(value), std::move(pattern));
}

ExprPtr CaseWhen(std::vector<std::pair<ExprPtr, ExprPtr>> cases) {
    return std::make_shared<CaseWhenExpr>(std::move(cases));
}

ExprPtr RegexpReplace(ExprPtr value, std::string pattern,
                      std::string replacement) {
    return std::make_shared<RegexpReplaceExpr>(
        std::move(value), std::move(pattern), std::move(replacement));
}

AggrPtr Count() { return std::make_shared<AggrCount>(); }

AggrPtr CountDistinctString() {
    return std::make_shared<AggrCountDistinctString>();
}

AggrPtr MinString() { return std::make_shared<AggrMinString>(); }

static std::vector<OrderByLimitOp::SortKey> ToLimitKeys(
    std::vector<SortKey> keys) {
    std::vector<OrderByLimitOp::SortKey> result;
    result.reserve(keys.size());
    for (const SortKey& key : keys) {
        result.push_back({key.col_idx, key.asc});
    }
    return result;
}

static std::vector<OrderByOp::SortKey> ToOrderKeys(std::vector<SortKey> keys) {
    std::vector<OrderByOp::SortKey> result;
    result.reserve(keys.size());
    for (const SortKey& key : keys) {
        result.push_back({key.col_idx, key.asc});
    }
    return result;
}

OpPtr BuildPlan(const std::string& isq_path, QuerySpec spec) {
    OpPtr op = spec.scan_all ? std::make_shared<ScanOp>(isq_path)
                             : std::make_shared<ScanOp>(
                                   isq_path, std::move(spec.scan_cols));

    if (spec.filter) {
        op = std::make_shared<FilterOp>(std::move(op), std::move(spec.filter));
    }

    switch (spec.result_kind) {
        case QuerySpec::ResultKind::kRows:
            break;
        case QuerySpec::ResultKind::kGlobalAggr:
            op = std::make_shared<GlobalAggrOp>(std::move(op),
                                                std::move(spec.aggrs),
                                                std::move(spec.aggr_exprs));
            break;
        case QuerySpec::ResultKind::kGroupBy:
            op = std::make_shared<GroupByOp>(
                std::move(op), std::move(spec.group_exprs),
                std::move(spec.aggrs), std::move(spec.aggr_exprs),
                spec.group_limit, spec.group_offset);
            break;
    }

    if (spec.post_filter) {
        op = std::make_shared<FilterOp>(std::move(op),
                                        std::move(spec.post_filter));
    }

    if (!spec.order_by.empty()) {
        if (spec.order_limit) {
            op = std::make_shared<OrderByLimitOp>(
                std::move(op), ToLimitKeys(std::move(spec.order_by)),
                *spec.order_limit, spec.order_offset);
        } else {
            op = std::make_shared<OrderByOp>(
                std::move(op), ToOrderKeys(std::move(spec.order_by)));
        }
    }

    if (!spec.projection.empty()) {
        op = std::make_shared<SelectColOp>(std::move(op),
                                           std::move(spec.projection));
    }

    return op;
}

}
