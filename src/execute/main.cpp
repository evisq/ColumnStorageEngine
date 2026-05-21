#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "aggr/aggr_avg.h"
#include "aggr/aggr_count.h"
#include "aggr/aggr_count_distinct.h"
#include "aggr/aggr_minmax.h"
#include "aggr/aggr_sum.h"
#include "column/col_util.h"
#include "execute/queries.h"
#include "expr/bin_expr.h"
#include "expr/expr.h"
#include "expr/like_expr.h"
#include "op-wrapper/filter.h"
#include "op-wrapper/global_aggr.h"
#include "op-wrapper/groub_by.h"
#include "op-wrapper/limit.h"
#include "op-wrapper/order_by.h"
#include "op-wrapper/project.h"
#include "op-wrapper/scan.h"
#include "reader/reader_CSV.h"
#include "writer/writer.h"

static constexpr ui64 kWatchID           = 0;
static constexpr ui64 kTitle             = 2;
static constexpr ui64 kEventTime         = 4;
static constexpr ui64 kEventDate         = 5;
static constexpr ui64 kCounterID         = 6;
static constexpr ui64 kClientIP          = 7;
static constexpr ui64 kRegionID          = 8;
static constexpr ui64 kUserID            = 9;
static constexpr ui64 kURL               = 13;
static constexpr ui64 kIsRefresh         = 15;
static constexpr ui64 kResolutionWidth   = 20;
static constexpr ui64 kMobilePhone       = 33;
static constexpr ui64 kMobilePhoneModel  = 34;
static constexpr ui64 kTraficSourceID    = 37;
static constexpr ui64 kSearchEngineID    = 38;
static constexpr ui64 kSearchPhrase      = 39;
static constexpr ui64 kAdvEngineID       = 40;
static constexpr ui64 kWindowClientWidth = 42;
static constexpr ui64 kWindowClientHeight= 43;
static constexpr ui64 kIsLink            = 52;
static constexpr ui64 kIsDownload        = 53;
static constexpr ui64 kDontCountHits     = 61;
static constexpr ui64 kRefererHash       = 102;
static constexpr ui64 kURLHash           = 103;

static void RunQuery(int query_num, const std::string& isq_path,
                     Writer& writer) {
    switch (query_num) {
        case 0: {
            std::shared_ptr<ScanOp> scan =
                std::make_shared<ScanOp>(isq_path, std::vector<ui64>{0});
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            GlobalAggrOp aggr(scan, {cnt}, {std::make_shared<GetCol>(0)});
            writer.ExecBatchCSV(aggr.Next());
            break;
        }
        case 1: {
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path, std::vector<ui64>{kAdvEngineID});
            std::shared_ptr<BinExpr> filter = std::make_shared<BinExpr>(
                std::make_shared<GetCol>(0), std::make_shared<СonstVal<i16>>(0),
                kOp::kNe);
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            GlobalAggrOp aggr(std::make_shared<FilterOp>(scan, filter), {cnt},
                              {std::make_shared<GetCol>(0)});
            writer.ExecBatchCSV(aggr.Next());
            break;
        }
        case 2: {
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path, std::vector<ui64>{kAdvEngineID, kResolutionWidth});
            std::shared_ptr<AggrSum<i16>> sum_adv =
                std::make_shared<AggrSum<i16>>();
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<AggrAvg<i16>> avg_res =
                std::make_shared<AggrAvg<i16>>();
            GlobalAggrOp aggr(
                scan, {sum_adv, cnt, avg_res},
                {std::make_shared<GetCol>(0), std::make_shared<GetCol>(0),
                 std::make_shared<GetCol>(1)});
            writer.ExecBatchCSV(aggr.Next());
            break;
        }
        case 3: {
            std::shared_ptr<ScanOp> scan =
                std::make_shared<ScanOp>(isq_path, std::vector<ui64>{kUserID});
            std::shared_ptr<AggrAvg<i64>> avg =
                std::make_shared<AggrAvg<i64>>();
            GlobalAggrOp aggr(scan, {avg}, {std::make_shared<GetCol>(0)});
            writer.ExecBatchCSV(aggr.Next());
            break;
        }
        case 4: {
            std::shared_ptr<ScanOp> scan =
                std::make_shared<ScanOp>(isq_path, std::vector<ui64>{kUserID});
            std::shared_ptr<AggrCountDistinct<i64>> cnt_dist =
                std::make_shared<AggrCountDistinct<i64>>();
            GlobalAggrOp aggr(scan, {cnt_dist}, {std::make_shared<GetCol>(0)});
            writer.ExecBatchCSV(aggr.Next());
            break;
        }
        case 5: {
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path, std::vector<ui64>{kSearchPhrase});
            std::shared_ptr<AggrCountDistinctString> cnt_dist =
                std::make_shared<AggrCountDistinctString>();
            GlobalAggrOp aggr(scan, {cnt_dist}, {std::make_shared<GetCol>(0)});
            writer.ExecBatchCSV(aggr.Next());
            break;
        }
        case 6: {
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path, std::vector<ui64>{kEventDate});
            std::shared_ptr<AggrMin<ISQDate>> mn =
                std::make_shared<AggrMin<ISQDate>>();
            std::shared_ptr<AggrMax<ISQDate>> mx =
                std::make_shared<AggrMax<ISQDate>>();
            GlobalAggrOp aggr(
                scan, {mn, mx},
                {std::make_shared<GetCol>(0), std::make_shared<GetCol>(0)});
            writer.ExecBatchCSV(aggr.Next());
            break;
        }
        case 7: {
            // SELECT AdvEngineID, COUNT(*) FROM hits
            // WHERE AdvEngineID <> 0 GROUP BY AdvEngineID ORDER BY COUNT(*) DESC
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path, std::vector<ui64>{kAdvEngineID});
            std::shared_ptr<BinExpr> filter = std::make_shared<BinExpr>(
                std::make_shared<GetCol>(0),
                std::make_shared<СonstVal<i16>>(0), kOp::kNe);
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                std::make_shared<FilterOp>(scan, filter),
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)},
                std::vector<std::shared_ptr<Aggr>>{cnt},
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)});
            OrderByOp ord(grp, {{1, false}});
            writer.ExecBatchCSV(ord.Next());
            break;
        }
        case 8: {
            // SELECT RegionID, COUNT(DISTINCT UserID) AS u FROM hits
            // GROUP BY RegionID ORDER BY u DESC LIMIT 10
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path, std::vector<ui64>{kRegionID, kUserID});
            std::shared_ptr<AggrCountDistinct<i64>> cnt =
                std::make_shared<AggrCountDistinct<i64>>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                scan,
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)},
                std::vector<std::shared_ptr<Aggr>>{cnt},
                std::vector<ExprPtr>{std::make_shared<GetCol>(1)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{1, false}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 9: {
            // SELECT RegionID, SUM(AdvEngineID), COUNT(*) AS c,
            //        AVG(ResolutionWidth), COUNT(DISTINCT UserID) FROM hits
            // GROUP BY RegionID ORDER BY c DESC LIMIT 10
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path,
                std::vector<ui64>{kRegionID, kAdvEngineID, kResolutionWidth, kUserID});
            std::shared_ptr<AggrSum<i16>> sum_adv = std::make_shared<AggrSum<i16>>();
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<AggrAvg<i16>> avg_res = std::make_shared<AggrAvg<i16>>();
            std::shared_ptr<AggrCountDistinct<i64>> cnt_dist =
                std::make_shared<AggrCountDistinct<i64>>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                scan,
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)},
                std::vector<std::shared_ptr<Aggr>>{sum_adv, cnt, avg_res, cnt_dist},
                std::vector<ExprPtr>{
                    std::make_shared<GetCol>(1), std::make_shared<GetCol>(0),
                    std::make_shared<GetCol>(2), std::make_shared<GetCol>(3)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{2, false}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 10: {
            // SELECT MobilePhoneModel, COUNT(DISTINCT UserID) AS u FROM hits
            // WHERE MobilePhoneModel <> ''
            // GROUP BY MobilePhoneModel ORDER BY u DESC LIMIT 10
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path, std::vector<ui64>{kMobilePhoneModel, kUserID});
            std::shared_ptr<BinExpr> filter = std::make_shared<BinExpr>(
                std::make_shared<GetCol>(0),
                std::make_shared<ConstValString>(""), kOp::kNe);
            std::shared_ptr<AggrCountDistinct<i64>> cnt =
                std::make_shared<AggrCountDistinct<i64>>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                std::make_shared<FilterOp>(scan, filter),
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)},
                std::vector<std::shared_ptr<Aggr>>{cnt},
                std::vector<ExprPtr>{std::make_shared<GetCol>(1)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{1, false}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 11: {
            // SELECT MobilePhone, MobilePhoneModel, COUNT(DISTINCT UserID) AS u
            // FROM hits WHERE MobilePhoneModel <> ''
            // GROUP BY MobilePhone, MobilePhoneModel ORDER BY u DESC LIMIT 10
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path,
                std::vector<ui64>{kMobilePhone, kMobilePhoneModel, kUserID});
            std::shared_ptr<BinExpr> filter = std::make_shared<BinExpr>(
                std::make_shared<GetCol>(1),
                std::make_shared<ConstValString>(""), kOp::kNe);
            std::shared_ptr<AggrCountDistinct<i64>> cnt =
                std::make_shared<AggrCountDistinct<i64>>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                std::make_shared<FilterOp>(scan, filter),
                std::vector<ExprPtr>{
                    std::make_shared<GetCol>(0), std::make_shared<GetCol>(1)},
                std::vector<std::shared_ptr<Aggr>>{cnt},
                std::vector<ExprPtr>{std::make_shared<GetCol>(2)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{2, false}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 12: {
            // SELECT SearchPhrase, COUNT(*) AS c FROM hits
            // WHERE SearchPhrase <> ''
            // GROUP BY SearchPhrase ORDER BY c DESC LIMIT 10
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path, std::vector<ui64>{kSearchPhrase});
            std::shared_ptr<BinExpr> filter = std::make_shared<BinExpr>(
                std::make_shared<GetCol>(0),
                std::make_shared<ConstValString>(""), kOp::kNe);
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                std::make_shared<FilterOp>(scan, filter),
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)},
                std::vector<std::shared_ptr<Aggr>>{cnt},
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{1, false}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 13: {
            // SELECT SearchPhrase, COUNT(DISTINCT UserID) AS u FROM hits
            // WHERE SearchPhrase <> ''
            // GROUP BY SearchPhrase ORDER BY u DESC LIMIT 10
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path, std::vector<ui64>{kSearchPhrase, kUserID});
            std::shared_ptr<BinExpr> filter = std::make_shared<BinExpr>(
                std::make_shared<GetCol>(0),
                std::make_shared<ConstValString>(""), kOp::kNe);
            std::shared_ptr<AggrCountDistinct<i64>> cnt =
                std::make_shared<AggrCountDistinct<i64>>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                std::make_shared<FilterOp>(scan, filter),
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)},
                std::vector<std::shared_ptr<Aggr>>{cnt},
                std::vector<ExprPtr>{std::make_shared<GetCol>(1)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{1, false}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 14: {
            // SELECT SearchEngineID, SearchPhrase, COUNT(*) AS c FROM hits
            // WHERE SearchPhrase <> ''
            // GROUP BY SearchEngineID, SearchPhrase ORDER BY c DESC LIMIT 10
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path, std::vector<ui64>{kSearchEngineID, kSearchPhrase});
            std::shared_ptr<BinExpr> filter = std::make_shared<BinExpr>(
                std::make_shared<GetCol>(1),
                std::make_shared<ConstValString>(""), kOp::kNe);
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                std::make_shared<FilterOp>(scan, filter),
                std::vector<ExprPtr>{
                    std::make_shared<GetCol>(0), std::make_shared<GetCol>(1)},
                std::vector<std::shared_ptr<Aggr>>{cnt},
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{2, false}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 15: {
            // SELECT UserID, COUNT(*) FROM hits
            // GROUP BY UserID ORDER BY COUNT(*) DESC LIMIT 10
            std::shared_ptr<ScanOp> scan =
                std::make_shared<ScanOp>(isq_path, std::vector<ui64>{kUserID});
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                scan,
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)},
                std::vector<std::shared_ptr<Aggr>>{cnt},
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{1, false}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 16: {
            // SELECT UserID, SearchPhrase, COUNT(*) FROM hits
            // GROUP BY UserID, SearchPhrase ORDER BY COUNT(*) DESC LIMIT 10
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path, std::vector<ui64>{kUserID, kSearchPhrase});
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                scan,
                std::vector<ExprPtr>{
                    std::make_shared<GetCol>(0), std::make_shared<GetCol>(1)},
                std::vector<std::shared_ptr<Aggr>>{cnt},
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{2, false}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 17: {
            // SELECT UserID, SearchPhrase, COUNT(*) FROM hits
            // GROUP BY UserID, SearchPhrase LIMIT 10
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path, std::vector<ui64>{kUserID, kSearchPhrase});
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                scan,
                std::vector<ExprPtr>{
                    std::make_shared<GetCol>(0), std::make_shared<GetCol>(1)},
                std::vector<std::shared_ptr<Aggr>>{cnt},
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)});
            LimitOp lim(grp, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 18: {
            // SELECT UserID, extract(minute FROM EventTime) AS m,
            //        SearchPhrase, COUNT(*) FROM hits
            // GROUP BY UserID, m, SearchPhrase ORDER BY COUNT(*) DESC LIMIT 10
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path,
                std::vector<ui64>{kUserID, kEventTime, kSearchPhrase});
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                scan,
                std::vector<ExprPtr>{
                    std::make_shared<GetCol>(0),
                    std::make_shared<ExtractMinuteExpr>(std::make_shared<GetCol>(1)),
                    std::make_shared<GetCol>(2)},
                std::vector<std::shared_ptr<Aggr>>{cnt},
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{3, false}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 19: {
            // SELECT UserID FROM hits WHERE UserID = 435090932899640449
            std::shared_ptr<ScanOp> scan =
                std::make_shared<ScanOp>(isq_path, std::vector<ui64>{kUserID});
            std::shared_ptr<BinExpr> filter = std::make_shared<BinExpr>(
                std::make_shared<GetCol>(0),
                std::make_shared<СonstVal<i64>>(435090932899640449LL), kOp::kEq);
            FilterOp filtered(scan, filter);
            while (filtered.HasNext()) writer.ExecBatchCSV(filtered.Next());
            break;
        }
        case 20: {
            // SELECT COUNT(*) FROM hits WHERE URL LIKE '%google%'
            std::shared_ptr<ScanOp> scan =
                std::make_shared<ScanOp>(isq_path, std::vector<ui64>{kURL});
            std::shared_ptr<LikeExpr> like =
                std::make_shared<LikeExpr>(std::make_shared<GetCol>(0), "%google%");
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            GlobalAggrOp aggr(std::make_shared<FilterOp>(scan, like), {cnt},
                              {std::make_shared<GetCol>(0)});
            writer.ExecBatchCSV(aggr.Next());
            break;
        }
        case 21: {
            // SELECT SearchPhrase, MIN(URL), COUNT(*) AS c FROM hits
            // WHERE URL LIKE '%google%' AND SearchPhrase <> ''
            // GROUP BY SearchPhrase ORDER BY c DESC LIMIT 10
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path, std::vector<ui64>{kURL, kSearchPhrase});
            std::shared_ptr<BinExpr> filter = std::make_shared<BinExpr>(
                std::make_shared<LikeExpr>(std::make_shared<GetCol>(0), "%google%"),
                std::make_shared<BinExpr>(
                    std::make_shared<GetCol>(1),
                    std::make_shared<ConstValString>(""), kOp::kNe),
                kOp::kAnd);
            std::shared_ptr<AggrMinString> min_url = std::make_shared<AggrMinString>();
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                std::make_shared<FilterOp>(scan, filter),
                std::vector<ExprPtr>{std::make_shared<GetCol>(1)},
                std::vector<std::shared_ptr<Aggr>>{min_url, cnt},
                std::vector<ExprPtr>{
                    std::make_shared<GetCol>(0), std::make_shared<GetCol>(1)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{2, false}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 22: {
            // SELECT SearchPhrase, MIN(URL), MIN(Title), COUNT(*) AS c,
            //        COUNT(DISTINCT UserID) FROM hits
            // WHERE Title LIKE '%Google%' AND URL NOT LIKE '%.google.%'
            //   AND SearchPhrase <> ''
            // GROUP BY SearchPhrase ORDER BY c DESC LIMIT 10
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path,
                std::vector<ui64>{kTitle, kURL, kSearchPhrase, kUserID});
            std::shared_ptr<BinExpr> filter = std::make_shared<BinExpr>(
                std::make_shared<BinExpr>(
                    std::make_shared<LikeExpr>(std::make_shared<GetCol>(0), "%Google%"),
                    std::make_shared<LikeExpr>(
                        std::make_shared<GetCol>(1), "%.google.%", true),
                    kOp::kAnd),
                std::make_shared<BinExpr>(
                    std::make_shared<GetCol>(2),
                    std::make_shared<ConstValString>(""), kOp::kNe),
                kOp::kAnd);
            std::shared_ptr<AggrMinString> min_url = std::make_shared<AggrMinString>();
            std::shared_ptr<AggrMinString> min_title = std::make_shared<AggrMinString>();
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<AggrCountDistinct<i64>> cnt_dist =
                std::make_shared<AggrCountDistinct<i64>>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                std::make_shared<FilterOp>(scan, filter),
                std::vector<ExprPtr>{std::make_shared<GetCol>(2)},
                std::vector<std::shared_ptr<Aggr>>{min_url, min_title, cnt, cnt_dist},
                std::vector<ExprPtr>{
                    std::make_shared<GetCol>(1), std::make_shared<GetCol>(0),
                    std::make_shared<GetCol>(2), std::make_shared<GetCol>(3)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{3, false}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        // case 23: SELECT * — skipped (requires all columns)
        case 24: {
            // SELECT SearchPhrase FROM hits WHERE SearchPhrase <> ''
            // ORDER BY EventTime LIMIT 10
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path, std::vector<ui64>{kSearchPhrase, kEventTime});
            std::shared_ptr<BinExpr> filter = std::make_shared<BinExpr>(
                std::make_shared<GetCol>(0),
                std::make_shared<ConstValString>(""), kOp::kNe);
            std::shared_ptr<OrderByOp> ord = std::make_shared<OrderByOp>(
                std::make_shared<FilterOp>(scan, filter),
                std::vector<OrderByOp::SortKey>{{1, true}});
            std::shared_ptr<LimitOp> lim = std::make_shared<LimitOp>(ord, 10);
            ProjectOp proj(lim, {0});
            writer.ExecBatchCSV(proj.Next());
            break;
        }
        case 25: {
            // SELECT SearchPhrase FROM hits WHERE SearchPhrase <> ''
            // ORDER BY SearchPhrase LIMIT 10
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path, std::vector<ui64>{kSearchPhrase});
            std::shared_ptr<BinExpr> filter = std::make_shared<BinExpr>(
                std::make_shared<GetCol>(0),
                std::make_shared<ConstValString>(""), kOp::kNe);
            std::shared_ptr<OrderByOp> ord = std::make_shared<OrderByOp>(
                std::make_shared<FilterOp>(scan, filter),
                std::vector<OrderByOp::SortKey>{{0, true}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 26: {
            // SELECT SearchPhrase FROM hits WHERE SearchPhrase <> ''
            // ORDER BY EventTime, SearchPhrase LIMIT 10
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path, std::vector<ui64>{kSearchPhrase, kEventTime});
            std::shared_ptr<BinExpr> filter = std::make_shared<BinExpr>(
                std::make_shared<GetCol>(0),
                std::make_shared<ConstValString>(""), kOp::kNe);
            std::shared_ptr<OrderByOp> ord = std::make_shared<OrderByOp>(
                std::make_shared<FilterOp>(scan, filter),
                std::vector<OrderByOp::SortKey>{{1, true}, {0, true}});
            std::shared_ptr<LimitOp> lim = std::make_shared<LimitOp>(ord, 10);
            ProjectOp proj(lim, {0});
            writer.ExecBatchCSV(proj.Next());
            break;
        }
        case 27: {
            // SELECT CounterID, AVG(length(URL)) AS l, COUNT(*) AS c FROM hits
            // WHERE URL <> '' GROUP BY CounterID
            // HAVING COUNT(*) > 100000 ORDER BY l DESC LIMIT 25
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path, std::vector<ui64>{kCounterID, kURL});
            std::shared_ptr<BinExpr> url_filter = std::make_shared<BinExpr>(
                std::make_shared<GetCol>(1),
                std::make_shared<ConstValString>(""), kOp::kNe);
            std::shared_ptr<AggrAvg<i64>> avg = std::make_shared<AggrAvg<i64>>();
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                std::make_shared<FilterOp>(scan, url_filter),
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)},
                std::vector<std::shared_ptr<Aggr>>{avg, cnt},
                std::vector<ExprPtr>{
                    std::make_shared<LengthExpr>(std::make_shared<GetCol>(1)),
                    std::make_shared<GetCol>(0)});
            std::shared_ptr<BinExpr> having = std::make_shared<BinExpr>(
                std::make_shared<GetCol>(2),
                std::make_shared<СonstVal<i64>>(100000LL), kOp::kGs);
            std::shared_ptr<OrderByOp> ord = std::make_shared<OrderByOp>(
                std::make_shared<FilterOp>(grp, having),
                std::vector<OrderByOp::SortKey>{{1, false}});
            LimitOp lim(ord, 25);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        // case 28: REGEXP_REPLACE — skipped
        case 29: {
            // SELECT SUM(ResolutionWidth), SUM(ResolutionWidth+1), ...,
            //        SUM(ResolutionWidth+89) FROM hits
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path, std::vector<ui64>{kResolutionWidth});
            std::vector<std::shared_ptr<Aggr>> aggrs;
            std::vector<ExprPtr> agg_exprs;
            aggrs.reserve(90);
            agg_exprs.reserve(90);
            for (i16 k = 0; k < 90; ++k) {
                aggrs.push_back(std::make_shared<AggrSum<i16>>());
                agg_exprs.push_back(std::make_shared<BinExpr>(
                    std::make_shared<GetCol>(0),
                    std::make_shared<СonstVal<i16>>(k), kOp::kAdd));
            }
            GlobalAggrOp aggr(scan, std::move(aggrs), std::move(agg_exprs));
            writer.ExecBatchCSV(aggr.Next());
            break;
        }
        case 30: {
            // SELECT SearchEngineID, ClientIP, COUNT(*) AS c,
            //        SUM(IsRefresh), AVG(ResolutionWidth) FROM hits
            // WHERE SearchPhrase <> ''
            // GROUP BY SearchEngineID, ClientIP ORDER BY c DESC LIMIT 10
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path,
                std::vector<ui64>{kSearchEngineID, kClientIP, kIsRefresh,
                                   kResolutionWidth, kSearchPhrase});
            std::shared_ptr<BinExpr> filter = std::make_shared<BinExpr>(
                std::make_shared<GetCol>(4),
                std::make_shared<ConstValString>(""), kOp::kNe);
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<AggrSum<i16>> sum_ref = std::make_shared<AggrSum<i16>>();
            std::shared_ptr<AggrAvg<i16>> avg_res = std::make_shared<AggrAvg<i16>>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                std::make_shared<FilterOp>(scan, filter),
                std::vector<ExprPtr>{
                    std::make_shared<GetCol>(0), std::make_shared<GetCol>(1)},
                std::vector<std::shared_ptr<Aggr>>{cnt, sum_ref, avg_res},
                std::vector<ExprPtr>{
                    std::make_shared<GetCol>(0), std::make_shared<GetCol>(2),
                    std::make_shared<GetCol>(3)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{2, false}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 31: {
            // SELECT WatchID, ClientIP, COUNT(*) AS c,
            //        SUM(IsRefresh), AVG(ResolutionWidth) FROM hits
            // WHERE SearchPhrase <> ''
            // GROUP BY WatchID, ClientIP ORDER BY c DESC LIMIT 10
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path,
                std::vector<ui64>{kWatchID, kClientIP, kIsRefresh,
                                   kResolutionWidth, kSearchPhrase});
            std::shared_ptr<BinExpr> filter = std::make_shared<BinExpr>(
                std::make_shared<GetCol>(4),
                std::make_shared<ConstValString>(""), kOp::kNe);
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<AggrSum<i16>> sum_ref = std::make_shared<AggrSum<i16>>();
            std::shared_ptr<AggrAvg<i16>> avg_res = std::make_shared<AggrAvg<i16>>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                std::make_shared<FilterOp>(scan, filter),
                std::vector<ExprPtr>{
                    std::make_shared<GetCol>(0), std::make_shared<GetCol>(1)},
                std::vector<std::shared_ptr<Aggr>>{cnt, sum_ref, avg_res},
                std::vector<ExprPtr>{
                    std::make_shared<GetCol>(0), std::make_shared<GetCol>(2),
                    std::make_shared<GetCol>(3)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{2, false}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 32: {
            // SELECT WatchID, ClientIP, COUNT(*) AS c,
            //        SUM(IsRefresh), AVG(ResolutionWidth) FROM hits
            // GROUP BY WatchID, ClientIP ORDER BY c DESC LIMIT 10
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path,
                std::vector<ui64>{kWatchID, kClientIP, kIsRefresh, kResolutionWidth});
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<AggrSum<i16>> sum_ref = std::make_shared<AggrSum<i16>>();
            std::shared_ptr<AggrAvg<i16>> avg_res = std::make_shared<AggrAvg<i16>>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                scan,
                std::vector<ExprPtr>{
                    std::make_shared<GetCol>(0), std::make_shared<GetCol>(1)},
                std::vector<std::shared_ptr<Aggr>>{cnt, sum_ref, avg_res},
                std::vector<ExprPtr>{
                    std::make_shared<GetCol>(0), std::make_shared<GetCol>(2),
                    std::make_shared<GetCol>(3)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{2, false}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 33: {
            // SELECT URL, COUNT(*) AS c FROM hits
            // GROUP BY URL ORDER BY c DESC LIMIT 10
            std::shared_ptr<ScanOp> scan =
                std::make_shared<ScanOp>(isq_path, std::vector<ui64>{kURL});
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                scan,
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)},
                std::vector<std::shared_ptr<Aggr>>{cnt},
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{1, false}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 34: {
            // SELECT 1, URL, COUNT(*) AS c FROM hits
            // GROUP BY 1, URL ORDER BY c DESC LIMIT 10
            std::shared_ptr<ScanOp> scan =
                std::make_shared<ScanOp>(isq_path, std::vector<ui64>{kURL});
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                scan,
                std::vector<ExprPtr>{
                    std::make_shared<СonstVal<i32>>(1),
                    std::make_shared<GetCol>(0)},
                std::vector<std::shared_ptr<Aggr>>{cnt},
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{2, false}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 35: {
            // SELECT ClientIP, ClientIP-1, ClientIP-2, ClientIP-3, COUNT(*) AS c
            // FROM hits
            // GROUP BY ClientIP, ClientIP-1, ClientIP-2, ClientIP-3
            // ORDER BY c DESC LIMIT 10
            std::shared_ptr<ScanOp> scan =
                std::make_shared<ScanOp>(isq_path, std::vector<ui64>{kClientIP});
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                scan,
                std::vector<ExprPtr>{
                    std::make_shared<GetCol>(0),
                    std::make_shared<BinExpr>(
                        std::make_shared<GetCol>(0),
                        std::make_shared<СonstVal<i32>>(1), kOp::kSub),
                    std::make_shared<BinExpr>(
                        std::make_shared<GetCol>(0),
                        std::make_shared<СonstVal<i32>>(2), kOp::kSub),
                    std::make_shared<BinExpr>(
                        std::make_shared<GetCol>(0),
                        std::make_shared<СonstVal<i32>>(3), kOp::kSub)},
                std::vector<std::shared_ptr<Aggr>>{cnt},
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{4, false}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 36: {
            // SELECT URL, COUNT(*) AS PageViews FROM hits
            // WHERE CounterID=62 AND EventDate>='2013-07-01'
            //   AND EventDate<='2013-07-31' AND DontCountHits=0
            //   AND IsRefresh=0 AND URL<>''
            // GROUP BY URL ORDER BY PageViews DESC LIMIT 10
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path,
                std::vector<ui64>{kCounterID, kEventDate, kDontCountHits,
                                   kIsRefresh, kURL});
            std::shared_ptr<BinExpr> filter = std::make_shared<BinExpr>(
                std::make_shared<BinExpr>(
                    std::make_shared<BinExpr>(
                        std::make_shared<BinExpr>(
                            std::make_shared<BinExpr>(
                                std::make_shared<GetCol>(0),
                                std::make_shared<СonstVal<i32>>(62), kOp::kEq),
                            std::make_shared<BinExpr>(
                                std::make_shared<GetCol>(1),
                                std::make_shared<СonstVal<ISQDate>>(ISQDate{2013,7,1}),
                                kOp::kGe),
                            kOp::kAnd),
                        std::make_shared<BinExpr>(
                            std::make_shared<GetCol>(1),
                            std::make_shared<СonstVal<ISQDate>>(ISQDate{2013,7,31}),
                            kOp::kLe),
                        kOp::kAnd),
                    std::make_shared<BinExpr>(
                        std::make_shared<BinExpr>(
                            std::make_shared<GetCol>(2),
                            std::make_shared<СonstVal<i16>>(0), kOp::kEq),
                        std::make_shared<BinExpr>(
                            std::make_shared<GetCol>(3),
                            std::make_shared<СonstVal<i16>>(0), kOp::kEq),
                        kOp::kAnd),
                    kOp::kAnd),
                std::make_shared<BinExpr>(
                    std::make_shared<GetCol>(4),
                    std::make_shared<ConstValString>(""), kOp::kNe),
                kOp::kAnd);
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                std::make_shared<FilterOp>(scan, filter),
                std::vector<ExprPtr>{std::make_shared<GetCol>(4)},
                std::vector<std::shared_ptr<Aggr>>{cnt},
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{1, false}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 37: {
            // SELECT Title, COUNT(*) AS PageViews FROM hits
            // WHERE CounterID=62 AND EventDate>='2013-07-01'
            //   AND EventDate<='2013-07-31' AND DontCountHits=0
            //   AND IsRefresh=0 AND Title<>''
            // GROUP BY Title ORDER BY PageViews DESC LIMIT 10
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path,
                std::vector<ui64>{kCounterID, kEventDate, kDontCountHits,
                                   kIsRefresh, kTitle});
            std::shared_ptr<BinExpr> filter = std::make_shared<BinExpr>(
                std::make_shared<BinExpr>(
                    std::make_shared<BinExpr>(
                        std::make_shared<BinExpr>(
                            std::make_shared<BinExpr>(
                                std::make_shared<GetCol>(0),
                                std::make_shared<СonstVal<i32>>(62), kOp::kEq),
                            std::make_shared<BinExpr>(
                                std::make_shared<GetCol>(1),
                                std::make_shared<СonstVal<ISQDate>>(ISQDate{2013,7,1}),
                                kOp::kGe),
                            kOp::kAnd),
                        std::make_shared<BinExpr>(
                            std::make_shared<GetCol>(1),
                            std::make_shared<СonstVal<ISQDate>>(ISQDate{2013,7,31}),
                            kOp::kLe),
                        kOp::kAnd),
                    std::make_shared<BinExpr>(
                        std::make_shared<BinExpr>(
                            std::make_shared<GetCol>(2),
                            std::make_shared<СonstVal<i16>>(0), kOp::kEq),
                        std::make_shared<BinExpr>(
                            std::make_shared<GetCol>(3),
                            std::make_shared<СonstVal<i16>>(0), kOp::kEq),
                        kOp::kAnd),
                    kOp::kAnd),
                std::make_shared<BinExpr>(
                    std::make_shared<GetCol>(4),
                    std::make_shared<ConstValString>(""), kOp::kNe),
                kOp::kAnd);
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                std::make_shared<FilterOp>(scan, filter),
                std::vector<ExprPtr>{std::make_shared<GetCol>(4)},
                std::vector<std::shared_ptr<Aggr>>{cnt},
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{1, false}});
            LimitOp lim(ord, 10);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 38: {
            // SELECT URL, COUNT(*) AS PageViews FROM hits
            // WHERE CounterID=62 AND EventDate>='2013-07-01'
            //   AND EventDate<='2013-07-31' AND IsRefresh=0
            //   AND IsLink<>0 AND IsDownload=0
            // GROUP BY URL ORDER BY PageViews DESC LIMIT 10 OFFSET 1000
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path,
                std::vector<ui64>{kCounterID, kEventDate, kIsRefresh,
                                   kIsLink, kIsDownload, kURL});
            std::shared_ptr<BinExpr> filter = std::make_shared<BinExpr>(
                std::make_shared<BinExpr>(
                    std::make_shared<BinExpr>(
                        std::make_shared<BinExpr>(
                            std::make_shared<GetCol>(0),
                            std::make_shared<СonstVal<i32>>(62), kOp::kEq),
                        std::make_shared<BinExpr>(
                            std::make_shared<GetCol>(1),
                            std::make_shared<СonstVal<ISQDate>>(ISQDate{2013,7,1}),
                            kOp::kGe),
                        kOp::kAnd),
                    std::make_shared<BinExpr>(
                        std::make_shared<GetCol>(1),
                        std::make_shared<СonstVal<ISQDate>>(ISQDate{2013,7,31}),
                        kOp::kLe),
                    kOp::kAnd),
                std::make_shared<BinExpr>(
                    std::make_shared<BinExpr>(
                        std::make_shared<GetCol>(2),
                        std::make_shared<СonstVal<i16>>(0), kOp::kEq),
                    std::make_shared<BinExpr>(
                        std::make_shared<BinExpr>(
                            std::make_shared<GetCol>(3),
                            std::make_shared<СonstVal<i16>>(0), kOp::kNe),
                        std::make_shared<BinExpr>(
                            std::make_shared<GetCol>(4),
                            std::make_shared<СonstVal<i16>>(0), kOp::kEq),
                        kOp::kAnd),
                    kOp::kAnd),
                kOp::kAnd);
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                std::make_shared<FilterOp>(scan, filter),
                std::vector<ExprPtr>{std::make_shared<GetCol>(5)},
                std::vector<std::shared_ptr<Aggr>>{cnt},
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{1, false}});
            LimitOp lim(ord, 10, 1000);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        // case 39: CASE WHEN — skipped
        case 40: {
            // SELECT URLHash, EventDate, COUNT(*) AS PageViews FROM hits
            // WHERE CounterID=62 AND EventDate>='2013-07-01'
            //   AND EventDate<='2013-07-31' AND IsRefresh=0
            //   AND TraficSourceID IN (-1,6)
            //   AND RefererHash=3594120000172545465
            // GROUP BY URLHash, EventDate ORDER BY PageViews DESC LIMIT 10 OFFSET 100
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path,
                std::vector<ui64>{kCounterID, kEventDate, kIsRefresh,
                                   kTraficSourceID, kRefererHash, kURLHash});
            std::shared_ptr<BinExpr> in_src = std::make_shared<BinExpr>(
                std::make_shared<BinExpr>(
                    std::make_shared<GetCol>(3),
                    std::make_shared<СonstVal<i16>>(-1), kOp::kEq),
                std::make_shared<BinExpr>(
                    std::make_shared<GetCol>(3),
                    std::make_shared<СonstVal<i16>>(6), kOp::kEq),
                kOp::kOr);
            std::shared_ptr<BinExpr> filter = std::make_shared<BinExpr>(
                std::make_shared<BinExpr>(
                    std::make_shared<BinExpr>(
                        std::make_shared<BinExpr>(
                            std::make_shared<BinExpr>(
                                std::make_shared<GetCol>(0),
                                std::make_shared<СonstVal<i32>>(62), kOp::kEq),
                            std::make_shared<BinExpr>(
                                std::make_shared<GetCol>(1),
                                std::make_shared<СonstVal<ISQDate>>(ISQDate{2013,7,1}),
                                kOp::kGe),
                            kOp::kAnd),
                        std::make_shared<BinExpr>(
                            std::make_shared<GetCol>(1),
                            std::make_shared<СonstVal<ISQDate>>(ISQDate{2013,7,31}),
                            kOp::kLe),
                        kOp::kAnd),
                    std::make_shared<BinExpr>(
                        std::make_shared<BinExpr>(
                            std::make_shared<GetCol>(2),
                            std::make_shared<СonstVal<i16>>(0), kOp::kEq),
                        in_src,
                        kOp::kAnd),
                    kOp::kAnd),
                std::make_shared<BinExpr>(
                    std::make_shared<GetCol>(4),
                    std::make_shared<СonstVal<i64>>(3594120000172545465LL),
                    kOp::kEq),
                kOp::kAnd);
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                std::make_shared<FilterOp>(scan, filter),
                std::vector<ExprPtr>{
                    std::make_shared<GetCol>(5), std::make_shared<GetCol>(1)},
                std::vector<std::shared_ptr<Aggr>>{cnt},
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{2, false}});
            LimitOp lim(ord, 10, 100);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        case 41: {
            // SELECT WindowClientWidth, WindowClientHeight,
            //        COUNT(*) AS PageViews FROM hits
            // WHERE CounterID=62 AND EventDate>='2013-07-01'
            //   AND EventDate<='2013-07-31' AND IsRefresh=0
            //   AND DontCountHits=0 AND URLHash=2868770270353813622
            // GROUP BY WindowClientWidth, WindowClientHeight
            // ORDER BY PageViews DESC LIMIT 10 OFFSET 10000
            std::shared_ptr<ScanOp> scan = std::make_shared<ScanOp>(
                isq_path,
                std::vector<ui64>{kCounterID, kEventDate, kIsRefresh,
                                   kDontCountHits, kURLHash,
                                   kWindowClientWidth, kWindowClientHeight});
            std::shared_ptr<BinExpr> filter = std::make_shared<BinExpr>(
                std::make_shared<BinExpr>(
                    std::make_shared<BinExpr>(
                        std::make_shared<BinExpr>(
                            std::make_shared<BinExpr>(
                                std::make_shared<GetCol>(0),
                                std::make_shared<СonstVal<i32>>(62), kOp::kEq),
                            std::make_shared<BinExpr>(
                                std::make_shared<GetCol>(1),
                                std::make_shared<СonstVal<ISQDate>>(ISQDate{2013,7,1}),
                                kOp::kGe),
                            kOp::kAnd),
                        std::make_shared<BinExpr>(
                            std::make_shared<GetCol>(1),
                            std::make_shared<СonstVal<ISQDate>>(ISQDate{2013,7,31}),
                            kOp::kLe),
                        kOp::kAnd),
                    std::make_shared<BinExpr>(
                        std::make_shared<BinExpr>(
                            std::make_shared<GetCol>(2),
                            std::make_shared<СonstVal<i16>>(0), kOp::kEq),
                        std::make_shared<BinExpr>(
                            std::make_shared<GetCol>(3),
                            std::make_shared<СonstVal<i16>>(0), kOp::kEq),
                        kOp::kAnd),
                    kOp::kAnd),
                std::make_shared<BinExpr>(
                    std::make_shared<GetCol>(4),
                    std::make_shared<СonstVal<i64>>(2868770270353813622LL),
                    kOp::kEq),
                kOp::kAnd);
            std::shared_ptr<AggrCount> cnt = std::make_shared<AggrCount>();
            std::shared_ptr<GroupByOp> grp = std::make_shared<GroupByOp>(
                std::make_shared<FilterOp>(scan, filter),
                std::vector<ExprPtr>{
                    std::make_shared<GetCol>(5), std::make_shared<GetCol>(6)},
                std::vector<std::shared_ptr<Aggr>>{cnt},
                std::vector<ExprPtr>{std::make_shared<GetCol>(0)});
            std::shared_ptr<OrderByOp> ord =
                std::make_shared<OrderByOp>(grp, std::vector<OrderByOp::SortKey>{{2, false}});
            LimitOp lim(ord, 10, 10000);
            writer.ExecBatchCSV(lim.Next());
            break;
        }
        // case 42: DATE_FORMAT — skipped
        default:
            break;
    }
}

static int ModeConvert(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Not enough arguments: expected at least 4\n"
                     "Please restart the program with the first argument as "
                     "\"-h\" to open the user manual.\n";
        return 1;
    }
    ReaderCSV(argv[2], argv[3], argv[4]);
    return 0;
}

static int ModeQuery(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Usage: hits_bench query QUERY_NUM INPUT_ISQ OUTPUT_CSV\n";
        return 1;
    }
    int query_num = std::stoi(argv[2]);
    std::string isq_path = argv[3];
    std::string out_path = argv[4];

    Writer writer(out_path);
    std::chrono::time_point<std::chrono::high_resolution_clock> t0 =
        std::chrono::high_resolution_clock::now();
    RunQuery(query_num, isq_path, writer);
    std::chrono::time_point<std::chrono::high_resolution_clock> t1 =
        std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::cerr << "Query " << query_num << ": " << ms << " ms\n";
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Not enough arguments: expected at least 1\n"
                     "Please restart the program with the first argument as "
                     "\"-h\" to open the user manual.\n";
        return 1;
    }

    std::string mode = std::string(argv[1]);
    if (mode == "-c") {
        return ModeConvert(argc, argv);
    }
    if (mode == "-q") {
        return ModeQuery(argc, argv);
    }
    if (mode == "-h") {
        std::cout << help_manual;
        return 0;
    }
    if (mode == "-l") {
        std::cout << "List of supported queries:\n\n";
        ui16 i = 0;
        for (std::string& query : queries) {
            std::cout << "(" << i++ << ") " << query << "\n\n";
        }
        return 0;
    }
    std::cerr << "Unknown mode: " << argv[1]
              << "\n"
                 "Please restart the program with the first argument as \"-h\" "
                 "to open the user manual.\n";
    return 1;
}
