#include "parser.h"

#include <fstream>

#include "util/assert.h"

void GetLineVString(VString &vec, std::ifstream &file) {
    vec.clear();
    std::string line;
    if (!std::getline(file, line)) return;
    if (!line.empty() && line.back() == '\r') line.pop_back();

    vec.push_back("");
    int in_quotes = 0;
    while (true) {
        for (char c : line) {
            if (c == '"') {
                ++in_quotes;
                if ((in_quotes % 2 == 1) && in_quotes > 1) {
                    vec.back().push_back('"');
                }
            } else if ((in_quotes % 2 == 0) && c == ',') {
                in_quotes = 0;
                vec.push_back("");
            } else {
                vec.back().push_back(c);
            }
        }
        if ((in_quotes % 2 == 0) || !std::getline(file, line)) {
            break;
        }
        if (!line.empty() && line.back() == '\r') line.pop_back();
        vec.back().push_back('\n');
    }
}

std::vector<ColScheme> ParseScheme(std::string_view path) {
    std::ifstream f((std::string(path)));
    ASSERT_WITH_MESSAGE(f.is_open(),
                        "CSV Scheme file is not opening: " + std::string(path));
    std::vector<ColScheme> scheme;
    VString row;
    while (f.good()) {
        GetLineVString(row, f);
        if (row.size() < 2 || row[0].empty()) continue;
        scheme.push_back({row[0], StrToColType(row[1])});
    }
    return scheme;
}
