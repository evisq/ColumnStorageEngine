#pragma once

#include <string_view>
#include <vector>

#include "column/column_int_dt.h"
#include "column/column_string.h"
#include "expr/expr.h"

class CaseWhenExpr : public Expr {
public:
    explicit CaseWhenExpr(std::vector<std::pair<ExprPtr, ExprPtr>>&& cases)
        : cases_(std::move(cases)) {
        ASSERT_WITH_MESSAGE(!cases_.empty(), "CASE WHEN must have cases");
    }

    ColPtr Eval(const Batch& batch) const override {
        const ui64 n = batch.CntRows();
        std::vector<i32> case_num(n, -1);
        for (size_t ind = 0; ind < cases_.size(); ++ind) {
            ColPtr bool_col = cases_[ind].first->Eval(batch);
            const Column<ui8>* mask =
                static_cast<const Column<ui8>*>(bool_col.get());
            for (ui64 i = 0; i < n; ++i) {
                if (case_num[i] == -1 && mask->At(i)) {
                    case_num[i] = static_cast<i32>(ind);
                }
            }
        }
        std::vector<ColPtr> val_cols(cases_.size());
        for (size_t i = 0; i < cases_.size(); ++i) {
            val_cols[i] = cases_[i].second->Eval(batch);
        }
        switch (val_cols[0]->GetType()) {
            case ColType::kInt8: {
                std::vector<i8> result(n);
                for (ui64 i = 0; i < n; ++i) {
                    result[i] =
                        (case_num[i] < 0 ? 0
                                         : static_cast<const Column<i8>*>(
                                               val_cols[case_num[i]].get())
                                               ->At(i));
                }
                return std::make_shared<Column<i8>>(std::move(result));
            }
            case ColType::kInt16: {
                std::vector<i16> result(n);
                for (ui64 i = 0; i < n; ++i) {
                    result[i] =
                        (case_num[i] < 0 ? 0
                                         : static_cast<const Column<i16>*>(
                                               val_cols[case_num[i]].get())
                                               ->At(i));
                }
                return std::make_shared<Column<i16>>(std::move(result));
            }
            case ColType::kInt32: {
                std::vector<i32> result(n);
                for (ui64 i = 0; i < n; ++i) {
                    result[i] =
                        (case_num[i] < 0 ? 0
                                         : static_cast<const Column<i32>*>(
                                               val_cols[case_num[i]].get())
                                               ->At(i));
                }
                return std::make_shared<Column<i32>>(std::move(result));
            }
            case ColType::kInt64: {
                std::vector<i64> result(n);
                for (ui64 i = 0; i < n; ++i) {
                    result[i] =
                        (case_num[i] < 0 ? 0
                                         : static_cast<const Column<i64>*>(
                                               val_cols[case_num[i]].get())
                                               ->At(i));
                }
                return std::make_shared<Column<i64>>(std::move(result));
            }
            case ColType::kDate: {
                std::vector<IsqDate> result(n);
                for (ui64 i = 0; i < n; ++i) {
                    result[i] =
                        (case_num[i] < 0 ? IsqDate{}
                                         : static_cast<const Column<IsqDate>*>(
                                               val_cols[case_num[i]].get())
                                               ->At(i));
                }
                return std::make_shared<Column<IsqDate>>(std::move(result));
            }
            case ColType::kDatetime: {
                std::vector<IsqDatetime> result(n);
                for (ui64 i = 0; i < n; ++i) {
                    result[i] = (case_num[i] < 0
                                     ? IsqDatetime{}
                                     : static_cast<const Column<IsqDatetime>*>(
                                           val_cols[case_num[i]].get())
                                           ->At(i));
                }
                return std::make_shared<Column<IsqDatetime>>(std::move(result));
            }
            case ColType::kString: {
                std::vector<char> data;
                std::vector<ui64> offsets;
                offsets.reserve(n + 1);
                offsets.push_back(0);
                for (ui64 i = 0; i < n; ++i) {
                    std::string_view sv =
                        (case_num[i] < 0 ? std::string_view{}
                                         : static_cast<const ColumnString*>(
                                               val_cols[case_num[i]].get())
                                               ->At(i));
                    data.insert(data.end(), sv.begin(), sv.end());
                    offsets.push_back(data.size());
                }
                return std::make_shared<ColumnString>(std::move(data),
                                                      std::move(offsets));
            }
        }
        return nullptr;
    }

private:
    std::vector<std::pair<ExprPtr, ExprPtr>> cases_;
};
