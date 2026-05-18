#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include "aggr/aggr_avg.h"
#include "aggr/aggr_count.h"
#include "aggr/aggr_count_distinct.h"
#include "aggr/aggr_minmax.h"
#include "aggr/aggr_sum.h"
#include "column/col_util.h"
#include "column/column_int_dt.h"
#include "column/column_string.h"
#include "execute/execute.h"
#include "expr/bin_expr.h"
#include "reader/reader_CSV.h"
#include "reader/reader_ISQ.h"

static constexpr ui64 kEventDate       = 5;
static constexpr ui64 kUserID          = 9;
static constexpr ui64 kResolutionWidth = 20;
static constexpr ui64 kSearchPhrase    = 39;
static constexpr ui64 kAdvEngineID     = 40;

static void WriteResult(std::ostream& out, const ColPtr& col) {
    switch (col->GetType()) {
        case ColType::kInt8:
            out << static_cast<int>(static_cast<Column<i8>*>(col.get())->At(0));
            break;
        case ColType::kInt16:
            out << static_cast<Column<i16>*>(col.get())->At(0);
            break;
        case ColType::kInt32:
            out << static_cast<Column<i32>*>(col.get())->At(0);
            break;
        case ColType::kInt64:
            out << static_cast<Column<i64>*>(col.get())->At(0);
            break;
        case ColType::kDate: {
            auto d = static_cast<Column<ISQDate>*>(col.get())->At(0);
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
                          (int)d.year, (int)d.month, (int)d.day);
            out << buf;
            break;
        }
        case ColType::kDatetime: {
            auto d = static_cast<Column<ISQDatetime>*>(col.get())->At(0);
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                          (int)d.year, (int)d.month, (int)d.day,
                          (int)d.hour, (int)d.minute, (int)d.second);
            out << buf;
            break;
        }
        case ColType::kString:
            out << static_cast<ColumnString*>(col.get())->At(0);
            break;
    }
}

static void RunQuery(int query_num, const std::string& isq_path, std::ostream& out) {
    switch (query_num) {
        case 0: {
            ReaderISQ reader(isq_path, {0});
            auto cnt = std::make_shared<AggrCount>();
            Execute(reader, nullptr, {cnt}, {0});
            WriteResult(out, cnt->Result());
            out << "\n";
            break;
        }
        case 1: {
            ReaderISQ reader(isq_path, {kAdvEngineID});
            auto filter = std::make_shared<BinExpr>(
                std::make_shared<GetCol>(0),
                std::make_shared<СonstVal<i16>>(0),
                Op::kNe);
            auto cnt = std::make_shared<AggrCount>();
            Execute(reader, filter, {cnt}, {0});
            WriteResult(out, cnt->Result());
            out << "\n";
            break;
        }
        case 2: {
            ReaderISQ reader(isq_path, {kAdvEngineID, kResolutionWidth});
            auto sum_adv = std::make_shared<AggrSum<i16>>();
            auto cnt     = std::make_shared<AggrCount>();
            auto avg_res = std::make_shared<AggrAvg<i16>>();
            Execute(reader, nullptr, {sum_adv, cnt, avg_res}, {0, 0, 1});
            WriteResult(out, sum_adv->Result()); out << ",";
            WriteResult(out, cnt->Result());     out << ",";
            WriteResult(out, avg_res->Result()); out << "\n";
            break;
        }
        case 3: {
            ReaderISQ reader(isq_path, {kUserID});
            auto avg = std::make_shared<AggrAvg<i64>>();
            Execute(reader, nullptr, {avg}, {0});
            WriteResult(out, avg->Result());
            out << "\n";
            break;
        }
        case 4: {
            ReaderISQ reader(isq_path, {kUserID});
            auto cnt_dist = std::make_shared<AggrCountDistinct<i64>>();
            Execute(reader, nullptr, {cnt_dist}, {0});
            WriteResult(out, cnt_dist->Result());
            out << "\n";
            break;
        }
        case 5: {
            ReaderISQ reader(isq_path, {kSearchPhrase});
            auto cnt_dist = std::make_shared<AggrCountDistinctString>();
            Execute(reader, nullptr, {cnt_dist}, {0});
            WriteResult(out, cnt_dist->Result());
            out << "\n";
            break;
        }
        case 6: {
            ReaderISQ reader(isq_path, {kEventDate});
            auto mn = std::make_shared<AggrMin<ISQDate>>();
            auto mx = std::make_shared<AggrMax<ISQDate>>();
            Execute(reader, nullptr, {mn, mx}, {0, 0});
            WriteResult(out, mn->Result()); out << ",";
            WriteResult(out, mx->Result()); out << "\n";
            break;
        }
        default:
            break;
    }
}

static int ModeConvert(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Usage: hits_bench convert INPUT_CSV SCHEME_CSV OUTPUT_ISQ\n";
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
    std::string isq_path  = argv[3];
    std::string out_path  = argv[4];

    std::ofstream out_file(out_path);
    if (!out_file.is_open()) {
        std::cerr << "Cannot open output file: " << out_path << "\n";
        return 1;
    }
    RunQuery(query_num, isq_path, out_file);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage:\n"
                  << "  hits_bench convert INPUT_CSV SCHEME_CSV OUTPUT_ISQ\n"
                  << "  hits_bench query   QUERY_NUM INPUT_ISQ  OUTPUT_CSV\n";
        return 1;
    }

    std::string mode = argv[1];
    if (mode == "convert") return ModeConvert(argc, argv);
    if (mode == "query")   return ModeQuery(argc, argv);

    std::cerr << "Unknown mode: " << mode << "\n";
    return 1;
}
