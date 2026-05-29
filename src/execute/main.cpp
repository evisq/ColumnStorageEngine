#include <chrono>
#include <iostream>
#include <string>

#include "execute/queries.h"
#include "execute/query_registry.h"
#include "reader/csv_reader.h"
#include "writer/writer.h"

static int ModeConvert(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Not enough arguments: expected at least 4\n"
                     "Please restart the program with the first argument as "
                     "\"-h\" to open the user manual.\n";
        return 1;
    }
    ConvertCsvToIsq(argv[2], argv[3], argv[4]);
    return 0;
}

static int ModeQuery(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Usage: hits_bench query QUERY_NUM INPUT_ISQ OUTPUT_CSV\n";
        return 1;
    }

    const int query_num = std::stoi(argv[2]);
    const std::string isq_path = argv[3];
    const std::string out_path = argv[4];

    Writer writer(out_path);
    std::chrono::time_point<std::chrono::high_resolution_clock> t0 =
        std::chrono::high_resolution_clock::now();

    OpPtr plan = BuildQueryPlan(query_num, isq_path);
    if (!plan) {
        std::cerr << "Query " << query_num << " is not implemented\n";
        return 1;
    }
    while (plan->HasNext()) {
        writer.ExecBatchCsv(plan->Next());
    }

    std::chrono::time_point<std::chrono::high_resolution_clock> t1 =
        std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::cerr << "Query " << query_num << ": " << ms << " ms\n";
    return 0;
}

static int ModeList() {
    std::cout << "List of supported queries:\n\n";
    ui16 i = 0;
    for (std::string& query : queries) {
        std::cout << "(" << i++ << ") " << query << "\n\n";
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Not enough arguments: expected at least 1\n"
                     "Please restart the program with the first argument as "
                     "\"-h\" to open the user manual.\n";
        return 1;
    }

    const std::string mode = argv[1];
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
        return ModeList();
    }

    std::cerr << "Unknown mode: " << argv[1]
              << "\n"
                 "Please restart the program with the first argument as \"-h\" "
                 "to open the user manual.\n";
    return 1;
}
