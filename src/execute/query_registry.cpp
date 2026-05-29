#include "execute/query_registry.h"

#include <boost/unordered/unordered_flat_map.hpp>
#include <functional>
#include <utility>
#include <vector>

#include "execute/query_builder.h"

namespace {

using query::Add;
using query::AggrPtr;
using query::And;
using query::Avg;
using query::BuildPlan;
using query::CaseWhen;
using query::Col;
using query::Count;
using query::CountDistinct;
using query::CountDistinctString;
using query::DateForm;
using query::Eq;
using query::Extract;
using query::Ge;
using query::Gt;
using query::Le;
using query::Length;
using query::Like;
using query::Max;
using query::Min;
using query::MinString;
using query::Ne;
using query::NotLike;
using query::Or;
using query::QuerySpec;
using query::RegexpReplace;
using query::Str;
using query::Sub;
using query::Sum;
using query::Val;

static constexpr ui64 kWatchID = 0;
static constexpr ui64 kTitle = 2;
static constexpr ui64 kEventTime = 4;
static constexpr ui64 kEventDate = 5;
static constexpr ui64 kCounterID = 6;
static constexpr ui64 kClientIP = 7;
static constexpr ui64 kRegionID = 8;
static constexpr ui64 kUserID = 9;
static constexpr ui64 kURL = 13;
static constexpr ui64 kReferer = 14;
static constexpr ui64 kIsRefresh = 15;
static constexpr ui64 kResolutionWidth = 20;
static constexpr ui64 kMobilePhone = 33;
static constexpr ui64 kMobilePhoneModel = 34;
static constexpr ui64 kTraficSourceID = 37;
static constexpr ui64 kSearchEngineID = 38;
static constexpr ui64 kSearchPhrase = 39;
static constexpr ui64 kAdvEngineID = 40;
static constexpr ui64 kWindowClientWidth = 42;
static constexpr ui64 kWindowClientHeight = 43;
static constexpr ui64 kIsLink = 52;
static constexpr ui64 kIsDownload = 53;
static constexpr ui64 kDontCountHits = 61;
static constexpr ui64 kRefererHash = 102;
static constexpr ui64 kURLHash = 103;

using QueryFactory = std::function<QuerySpec()>;

QuerySpec Rows(std::vector<ui64> scan_cols, ExprPtr filter = nullptr) {
    QuerySpec spec;
    spec.scan_cols = std::move(scan_cols);
    spec.filter = std::move(filter);
    return spec;
}

QuerySpec AllRows(ExprPtr filter = nullptr) {
    QuerySpec spec;
    spec.scan_all = true;
    spec.filter = std::move(filter);
    return spec;
}

QuerySpec Global(std::vector<ui64> scan_cols, std::vector<AggrPtr> aggrs,
                 std::vector<ExprPtr> aggr_exprs, ExprPtr filter = nullptr) {
    QuerySpec spec;
    spec.scan_cols = std::move(scan_cols);
    spec.filter = std::move(filter);
    spec.result_kind = QuerySpec::ResultKind::kGlobalAggr;
    spec.aggrs = std::move(aggrs);
    spec.aggr_exprs = std::move(aggr_exprs);
    return spec;
}

QuerySpec Group(std::vector<ui64> scan_cols, std::vector<ExprPtr> keys,
                std::vector<AggrPtr> aggrs, std::vector<ExprPtr> aggr_exprs,
                ExprPtr filter = nullptr) {
    QuerySpec spec;
    spec.scan_cols = std::move(scan_cols);
    spec.filter = std::move(filter);
    spec.result_kind = QuerySpec::ResultKind::kGroupBy;
    spec.group_exprs = std::move(keys);
    spec.aggrs = std::move(aggrs);
    spec.aggr_exprs = std::move(aggr_exprs);
    return spec;
}

ExprPtr NonEmpty(ui64 col) { return Ne(Col(col), Str("")); }

ExprPtr Counter62InJuly(ui64 counter_col, ui64 date_col) {
    return And(And(Eq(Col(counter_col), Val<i32>(62)),
                   Ge(Col(date_col), Val<IsqDate>({2013, 7, 1}))),
               Le(Col(date_col), Val<IsqDate>({2013, 7, 31})));
}

void Order(QuerySpec& spec, ui64 col, bool asc) {
    spec.order_by = {{col, asc}};
}

void OrderLimit(QuerySpec& spec, ui64 col, bool asc, ui64 limit,
                ui64 offset = 0) {
    spec.order_by = {{col, asc}};
    spec.order_limit = limit;
    spec.order_offset = offset;
}

const boost::unordered_flat_map<int, QueryFactory>& Registry() {
    static const boost::unordered_flat_map<int, QueryFactory> registry = {
        {0, [] { return Global({0}, {Count()}, {Col(0)}); }},
        {1,
         [] {
             return Global({kAdvEngineID}, {Count()}, {Col(0)},
                           Ne(Col(0), Val<i16>(0)));
         }},
        {2,
         [] {
             return Global({kAdvEngineID, kResolutionWidth},
                           {Sum<i16>(), Count(), Avg<i16>()},
                           {Col(0), Col(0), Col(1)});
         }},
        {3, [] { return Global({kUserID}, {Avg<i64>()}, {Col(0)}); }},
        {4, [] { return Global({kUserID}, {CountDistinct<i64>()}, {Col(0)}); }},
        {5,
         [] {
             return Global({kSearchPhrase}, {CountDistinctString()}, {Col(0)});
         }},
        {6,
         [] {
             return Global({kEventDate}, {Min<IsqDate>(), Max<IsqDate>()},
                           {Col(0), Col(0)});
         }},
        {7,
         [] {
             QuerySpec spec = Group({kAdvEngineID}, {Col(0)}, {Count()},
                                    {Col(0)}, Ne(Col(0), Val<i16>(0)));
             Order(spec, 1, false);
             return spec;
         }},
        {8,
         [] {
             QuerySpec spec = Group({kRegionID, kUserID}, {Col(0)},
                                    {CountDistinct<i64>()}, {Col(1)});
             OrderLimit(spec, 1, false, 10);
             return spec;
         }},
        {9,
         [] {
             QuerySpec spec = Group(
                 {kRegionID, kAdvEngineID, kResolutionWidth, kUserID}, {Col(0)},
                 {Sum<i16>(), Count(), Avg<i16>(), CountDistinct<i64>()},
                 {Col(1), Col(0), Col(2), Col(3)});
             OrderLimit(spec, 2, false, 10);
             return spec;
         }},
        {10,
         [] {
             QuerySpec spec =
                 Group({kMobilePhoneModel, kUserID}, {Col(0)},
                       {CountDistinct<i64>()}, {Col(1)}, NonEmpty(0));
             OrderLimit(spec, 1, false, 10);
             return spec;
         }},
        {11,
         [] {
             QuerySpec spec = Group({kMobilePhone, kMobilePhoneModel, kUserID},
                                    {Col(0), Col(1)}, {CountDistinct<i64>()},
                                    {Col(2)}, NonEmpty(1));
             OrderLimit(spec, 2, false, 10);
             return spec;
         }},
        {12,
         [] {
             QuerySpec spec = Group({kSearchPhrase}, {Col(0)}, {Count()},
                                    {Col(0)}, NonEmpty(0));
             OrderLimit(spec, 1, false, 10);
             return spec;
         }},
        {13,
         [] {
             QuerySpec spec =
                 Group({kSearchPhrase, kUserID}, {Col(0)},
                       {CountDistinct<i64>()}, {Col(1)}, NonEmpty(0));
             OrderLimit(spec, 1, false, 10);
             return spec;
         }},
        {14,
         [] {
             QuerySpec spec =
                 Group({kSearchEngineID, kSearchPhrase}, {Col(0), Col(1)},
                       {Count()}, {Col(0)}, NonEmpty(1));
             OrderLimit(spec, 2, false, 10);
             return spec;
         }},
        {15,
         [] {
             QuerySpec spec = Group({kUserID}, {Col(0)}, {Count()}, {Col(0)});
             OrderLimit(spec, 1, false, 10);
             return spec;
         }},
        {16,
         [] {
             QuerySpec spec = Group({kUserID, kSearchPhrase}, {Col(0), Col(1)},
                                    {Count()}, {Col(0)});
             OrderLimit(spec, 2, false, 10);
             return spec;
         }},
        {17,
         [] {
             QuerySpec spec = Group({kUserID, kSearchPhrase}, {Col(0), Col(1)},
                                    {Count()}, {Col(0)});
             spec.group_limit = 10;
             return spec;
         }},
        {18,
         [] {
             QuerySpec spec = Group({kUserID, kEventTime, kSearchPhrase},
                                    {Col(0), Extract(Col(1), 4), Col(2)},
                                    {Count()}, {Col(0)});
             OrderLimit(spec, 3, false, 10);
             return spec;
         }},
        {19,
         [] {
             return Rows({kUserID}, Eq(Col(0), Val<i64>(435090932899640449LL)));
         }},
        {20,
         [] {
             return Global({kURL}, {Count()}, {Col(0)},
                           Like(Col(0), "%google%"));
         }},
        {21,
         [] {
             QuerySpec spec = Group({kURL, kSearchPhrase}, {Col(1)},
                                    {MinString(), Count()}, {Col(0), Col(1)},
                                    And(Like(Col(0), "%google%"), NonEmpty(1)));
             OrderLimit(spec, 2, false, 10);
             return spec;
         }},
        {22,
         [] {
             QuerySpec spec = Group(
                 {kTitle, kURL, kSearchPhrase, kUserID}, {Col(2)},
                 {MinString(), MinString(), Count(), CountDistinct<i64>()},
                 {Col(1), Col(0), Col(2), Col(3)},
                 And(And(Like(Col(0), "%Google%"),
                         NotLike(Col(1), "%.google.%")),
                     NonEmpty(2)));
             OrderLimit(spec, 3, false, 10);
             return spec;
         }},
        {23,
         [] {
             QuerySpec spec = AllRows(Like(Col(kURL), "%google%"));
             OrderLimit(spec, kEventTime, true, 10);
             return spec;
         }},
        {24,
         [] {
             QuerySpec spec = Rows({kSearchPhrase, kEventTime}, NonEmpty(0));
             OrderLimit(spec, 1, true, 10);
             spec.projection = {0};
             return spec;
         }},
        {25,
         [] {
             QuerySpec spec = Rows({kSearchPhrase}, NonEmpty(0));
             OrderLimit(spec, 0, true, 10);
             return spec;
         }},
        {26,
         [] {
             QuerySpec spec = Rows({kSearchPhrase, kEventTime}, NonEmpty(0));
             spec.order_by = {{1, true}, {0, true}};
             spec.order_limit = 10;
             spec.projection = {0};
             return spec;
         }},
        {27,
         [] {
             QuerySpec spec =
                 Group({kCounterID, kURL}, {Col(0)}, {Avg<i64>(), Count()},
                       {Length(Col(1)), Col(0)}, NonEmpty(1));
             spec.post_filter = Gt(Col(2), Val<i64>(100000LL));
             OrderLimit(spec, 1, false, 25);
             return spec;
         }},
        {28,
         [] {
             ExprPtr host = RegexpReplace(
                 Col(0), "^https?://(?:www\\.)?([^/]+)/.*$", "\\1");
             QuerySpec spec =
                 Group({kReferer}, {host}, {Avg<i64>(), Count(), MinString()},
                       {Length(Col(0)), Col(0), Col(0)}, NonEmpty(0));
             spec.post_filter = Gt(Col(2), Val<i64>(100000LL));
             OrderLimit(spec, 1, false, 25);
             return spec;
         }},
        {29,
         [] {
             std::vector<AggrPtr> aggrs;
             std::vector<ExprPtr> exprs;
             aggrs.reserve(90);
             exprs.reserve(90);
             for (i16 k = 0; k < 90; ++k) {
                 aggrs.push_back(Sum<i64>());
                 exprs.push_back(Add(Col(0), Val<i16>(k)));
             }
             return Global({kResolutionWidth}, std::move(aggrs),
                           std::move(exprs));
         }},
        {30,
         [] {
             QuerySpec spec =
                 Group({kSearchEngineID, kClientIP, kIsRefresh,
                        kResolutionWidth, kSearchPhrase},
                       {Col(0), Col(1)}, {Count(), Sum<i16>(), Avg<i16>()},
                       {Col(0), Col(2), Col(3)}, NonEmpty(4));
             OrderLimit(spec, 2, false, 10);
             return spec;
         }},
        {31,
         [] {
             QuerySpec spec =
                 Group({kWatchID, kClientIP, kIsRefresh, kResolutionWidth,
                        kSearchPhrase},
                       {Col(0), Col(1)}, {Count(), Sum<i16>(), Avg<i16>()},
                       {Col(0), Col(2), Col(3)}, NonEmpty(4));
             OrderLimit(spec, 2, false, 10);
             return spec;
         }},
        {32,
         [] {
             QuerySpec spec =
                 Group({kWatchID, kClientIP, kIsRefresh, kResolutionWidth},
                       {Col(0), Col(1)}, {Count(), Sum<i16>(), Avg<i16>()},
                       {Col(0), Col(2), Col(3)});
             OrderLimit(spec, 2, false, 10);
             return spec;
         }},
        {33,
         [] {
             QuerySpec spec = Group({kURL}, {Col(0)}, {Count()}, {Col(0)});
             OrderLimit(spec, 1, false, 10);
             return spec;
         }},
        {34,
         [] {
             QuerySpec spec =
                 Group({kURL}, {Val<i32>(1), Col(0)}, {Count()}, {Col(0)});
             OrderLimit(spec, 2, false, 10);
             return spec;
         }},
        {35,
         [] {
             QuerySpec spec =
                 Group({kClientIP},
                       {Col(0), Sub(Col(0), Val<i32>(1)),
                        Sub(Col(0), Val<i32>(2)), Sub(Col(0), Val<i32>(3))},
                       {Count()}, {Col(0)});
             OrderLimit(spec, 4, false, 10);
             return spec;
         }},
        {36,
         [] {
             QuerySpec spec = Group(
                 {kCounterID, kEventDate, kDontCountHits, kIsRefresh, kURL},
                 {Col(4)}, {Count()}, {Col(0)},
                 And(And(Counter62InJuly(0, 1),
                         And(Eq(Col(2), Val<i16>(0)), Eq(Col(3), Val<i16>(0)))),
                     NonEmpty(4)));
             OrderLimit(spec, 1, false, 10);
             return spec;
         }},
        {37,
         [] {
             QuerySpec spec = Group(
                 {kCounterID, kEventDate, kDontCountHits, kIsRefresh, kTitle},
                 {Col(4)}, {Count()}, {Col(0)},
                 And(And(Counter62InJuly(0, 1),
                         And(Eq(Col(2), Val<i16>(0)), Eq(Col(3), Val<i16>(0)))),
                     NonEmpty(4)));
             OrderLimit(spec, 1, false, 10);
             return spec;
         }},
        {38,
         [] {
             QuerySpec spec = Group(
                 {kCounterID, kEventDate, kIsRefresh, kIsLink, kIsDownload,
                  kURL},
                 {Col(5)}, {Count()}, {Col(0)},
                 And(Counter62InJuly(0, 1), And(Eq(Col(2), Val<i16>(0)),
                                                And(Ne(Col(3), Val<i16>(0)),
                                                    Eq(Col(4), Val<i16>(0))))));
             OrderLimit(spec, 1, false, 10, 1000);
             return spec;
         }},
        {39,
         [] {
             ExprPtr src = CaseWhen(
                 {{And(Eq(Col(4), Val<i16>(0)), Eq(Col(5), Val<i16>(0))),
                   Col(6)}});
             QuerySpec spec = Group(
                 {kCounterID, kEventDate, kIsRefresh, kTraficSourceID,
                  kSearchEngineID, kAdvEngineID, kReferer, kURL},
                 {Col(3), Col(4), Col(5), src, Col(7)}, {Count()}, {Col(0)},
                 And(Counter62InJuly(0, 1), Eq(Col(2), Val<i16>(0))));
             OrderLimit(spec, 5, false, 10, 1000);
             return spec;
         }},
        {40,
         [] {
             ExprPtr src_in =
                 Or(Eq(Col(3), Val<i16>(-1)), Eq(Col(3), Val<i16>(6)));
             QuerySpec spec =
                 Group({kCounterID, kEventDate, kIsRefresh, kTraficSourceID,
                        kRefererHash, kURLHash},
                       {Col(5), Col(1)}, {Count()}, {Col(0)},
                       And(And(Counter62InJuly(0, 1),
                               And(Eq(Col(2), Val<i16>(0)), src_in)),
                           Eq(Col(4), Val<i64>(3594120000172545465LL))));
             OrderLimit(spec, 2, false, 10, 100);
             return spec;
         }},
        {41,
         [] {
             QuerySpec spec = Group(
                 {kCounterID, kEventDate, kIsRefresh, kDontCountHits, kURLHash,
                  kWindowClientWidth, kWindowClientHeight},
                 {Col(5), Col(6)}, {Count()}, {Col(0)},
                 And(And(Counter62InJuly(0, 1),
                         And(Eq(Col(2), Val<i16>(0)), Eq(Col(3), Val<i16>(0)))),
                     Eq(Col(4), Val<i64>(2868770270353813622LL))));
             OrderLimit(spec, 2, false, 10, 10000);
             return spec;
         }},
        {42,
         [] {
             ExprPtr hour = DateForm(Col(4), "%Y-%m-%d %H:00:00");
             QuerySpec spec = Group(
                 {kCounterID, kEventDate, kIsRefresh, kDontCountHits,
                  kEventTime},
                 {hour}, {Count()}, {Col(0)},
                 And(And(And(Eq(Col(0), Val<i32>(62)),
                             Ge(Col(1), Val<IsqDate>({2013, 7, 14}))),
                         Le(Col(1), Val<IsqDate>({2013, 7, 15}))),
                     And(Eq(Col(2), Val<i16>(0)), Eq(Col(3), Val<i16>(0)))));
             OrderLimit(spec, 0, true, 10, 1000);
             return spec;
         }},
    };
    return registry;
}

}

OpPtr BuildQueryPlan(int query_num, const std::string& isq_path) {
    auto it = Registry().find(query_num);
    if (it == Registry().end()) {
        return nullptr;
    }
    return BuildPlan(isq_path, it->second());
}
