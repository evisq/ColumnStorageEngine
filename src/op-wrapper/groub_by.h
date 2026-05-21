#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "aggr/aggr.h"
#include "column/column_int_dt.h"
#include "column/column_string.h"
#include "expr/expr.h"
#include "op.h"

class GroupByOp : public Op {
public:
    GroupByOp(OpPtr op,
              std::vector<ExprPtr> key_exprs,
              std::vector<std::shared_ptr<Aggr>> aggr_protos,
              std::vector<ExprPtr> agg_exprs)
        : op_(std::move(op)),
          key_exprs_(std::move(key_exprs)),
          aggr_protos_(std::move(aggr_protos)),
          agg_exprs_(std::move(agg_exprs)) {}

    bool HasNext() override { return !done_; }

    Batch Next() override {
        while (op_->HasNext()) {
            Batch batch = op_->Next();
            const std::vector<ui8>& mask = batch.GetMask();

            std::vector<ColPtr> key_cols, agg_cols;
            for (ExprPtr& e : key_exprs_) key_cols.push_back(e->Eval(batch));
            for (ExprPtr& e : agg_exprs_) agg_cols.push_back(e->Eval(batch));

            std::string key;
            for (ui64 row = 0; row < batch.CntRows(); ++row) {
                if (!mask[row]) continue;

                key.clear();
                for (const ColPtr& kc : key_cols) AppendKeyCell(key, kc, row);

                auto [it, inserted] = groups_.emplace(
                    key, std::vector<std::shared_ptr<Aggr>>{});
                if (inserted) {
                    order_.push_back(key);
                    std::vector<ColPtr> snap;
                    snap.reserve(key_cols.size());
                    for (const ColPtr& kc : key_cols) snap.push_back(SliceRow(kc, row));
                    order_keys_.push_back(std::move(snap));
                    for (std::shared_ptr<Aggr>& p : aggr_protos_) it->second.push_back(p->Clone());
                }
                for (ui64 a = 0; a < agg_cols.size(); ++a)
                    it->second[a]->UpdateRow(agg_cols[a], row);
            }
        }
        done_ = true;
        return BuildResult();
    }

private:
    static void AppendKeyCell(std::string& key, const ColPtr& col, ui64 row) {
        switch (col->GetType()) {
            case ColType::kInt8: {
                i8 v = static_cast<const Column<i8>*>(col.get())->At(row);
                key.append(reinterpret_cast<const char*>(&v), sizeof(v));
                break;
            }
            case ColType::kInt16: {
                i16 v = static_cast<const Column<i16>*>(col.get())->At(row);
                key.append(reinterpret_cast<const char*>(&v), sizeof(v));
                break;
            }
            case ColType::kInt32: {
                i32 v = static_cast<const Column<i32>*>(col.get())->At(row);
                key.append(reinterpret_cast<const char*>(&v), sizeof(v));
                break;
            }
            case ColType::kInt64: {
                i64 v = static_cast<const Column<i64>*>(col.get())->At(row);
                key.append(reinterpret_cast<const char*>(&v), sizeof(v));
                break;
            }
            case ColType::kDate: {
                ISQDate v = static_cast<const Column<ISQDate>*>(col.get())->At(row);
                key.append(reinterpret_cast<const char*>(&v), sizeof(v));
                break;
            }
            case ColType::kDatetime: {
                ISQDatetime v = static_cast<const Column<ISQDatetime>*>(col.get())->At(row);
                key.append(reinterpret_cast<const char*>(&v), sizeof(v));
                break;
            }
            case ColType::kString: {
                std::string_view sv = static_cast<const ColumnString*>(col.get())->At(row);
                ui32 len = static_cast<ui32>(sv.size());
                key.append(reinterpret_cast<const char*>(&len), 4);
                key.append(sv.data(), len);
                break;
            }
        }
    }

    static ColPtr SliceRow(const ColPtr& col, ui64 row) {
        switch (col->GetType()) {
            case ColType::kInt8:
                return std::make_shared<Column<i8>>(
                    std::vector<i8>{static_cast<const Column<i8>*>(col.get())->At(row)});
            case ColType::kInt16:
                return std::make_shared<Column<i16>>(
                    std::vector<i16>{static_cast<const Column<i16>*>(col.get())->At(row)});
            case ColType::kInt32:
                return std::make_shared<Column<i32>>(
                    std::vector<i32>{static_cast<const Column<i32>*>(col.get())->At(row)});
            case ColType::kInt64:
                return std::make_shared<Column<i64>>(
                    std::vector<i64>{static_cast<const Column<i64>*>(col.get())->At(row)});
            case ColType::kDate:
                return std::make_shared<Column<ISQDate>>(
                    std::vector<ISQDate>{static_cast<const Column<ISQDate>*>(col.get())->At(row)});
            case ColType::kDatetime:
                return std::make_shared<Column<ISQDatetime>>(
                    std::vector<ISQDatetime>{static_cast<const Column<ISQDatetime>*>(col.get())->At(row)});
            case ColType::kString: {
                std::string_view sv = static_cast<const ColumnString*>(col.get())->At(row);
                std::vector<char> data(sv.begin(), sv.end());
                std::vector<ui64> offsets = {0, sv.size()};
                return std::make_shared<ColumnString>(std::move(data), std::move(offsets));
            }
        }
        return nullptr;
    }

    static ColPtr ConcatKeyCol(const std::vector<std::vector<ColPtr>>& order_keys, ui64 k) {
        if (order_keys.empty()) return nullptr;
        const ui64 n = order_keys.size();
        switch (order_keys[0][k]->GetType()) {
            case ColType::kInt8: {
                std::vector<i8> vals;
                vals.reserve(n);
                for (const std::vector<ColPtr>& g : order_keys)
                    vals.push_back(static_cast<const Column<i8>*>(g[k].get())->At(0));
                return std::make_shared<Column<i8>>(std::move(vals));
            }
            case ColType::kInt16: {
                std::vector<i16> vals;
                vals.reserve(n);
                for (const std::vector<ColPtr>& g : order_keys)
                    vals.push_back(static_cast<const Column<i16>*>(g[k].get())->At(0));
                return std::make_shared<Column<i16>>(std::move(vals));
            }
            case ColType::kInt32: {
                std::vector<i32> vals;
                vals.reserve(n);
                for (const std::vector<ColPtr>& g : order_keys)
                    vals.push_back(static_cast<const Column<i32>*>(g[k].get())->At(0));
                return std::make_shared<Column<i32>>(std::move(vals));
            }
            case ColType::kInt64: {
                std::vector<i64> vals;
                vals.reserve(n);
                for (const std::vector<ColPtr>& g : order_keys)
                    vals.push_back(static_cast<const Column<i64>*>(g[k].get())->At(0));
                return std::make_shared<Column<i64>>(std::move(vals));
            }
            case ColType::kDate: {
                std::vector<ISQDate> vals;
                vals.reserve(n);
                for (const std::vector<ColPtr>& g : order_keys)
                    vals.push_back(static_cast<const Column<ISQDate>*>(g[k].get())->At(0));
                return std::make_shared<Column<ISQDate>>(std::move(vals));
            }
            case ColType::kDatetime: {
                std::vector<ISQDatetime> vals;
                vals.reserve(n);
                for (const std::vector<ColPtr>& g : order_keys)
                    vals.push_back(static_cast<const Column<ISQDatetime>*>(g[k].get())->At(0));
                return std::make_shared<Column<ISQDatetime>>(std::move(vals));
            }
            case ColType::kString: {
                std::vector<char> data;
                std::vector<ui64> offsets;
                offsets.reserve(n + 1);
                offsets.push_back(0);
                for (const std::vector<ColPtr>& g : order_keys) {
                    std::string_view sv = static_cast<const ColumnString*>(g[k].get())->At(0);
                    data.insert(data.end(), sv.begin(), sv.end());
                    offsets.push_back(data.size());
                }
                return std::make_shared<ColumnString>(std::move(data), std::move(offsets));
            }
        }
        return nullptr;
    }

    ColPtr ConcatAggrCol(ui64 a) {
        if (order_.empty()) return nullptr;
        const ui64 n = order_.size();
        std::vector<ColPtr> parts;
        parts.reserve(n);
        for (const std::string& key : order_)
            parts.push_back(groups_.at(key)[a]->Result());
        switch (parts[0]->GetType()) {
            case ColType::kInt8: {
                std::vector<i8> vals;
                vals.reserve(n);
                for (const ColPtr& p : parts)
                    vals.push_back(static_cast<const Column<i8>*>(p.get())->At(0));
                return std::make_shared<Column<i8>>(std::move(vals));
            }
            case ColType::kInt16: {
                std::vector<i16> vals;
                vals.reserve(n);
                for (const ColPtr& p : parts)
                    vals.push_back(static_cast<const Column<i16>*>(p.get())->At(0));
                return std::make_shared<Column<i16>>(std::move(vals));
            }
            case ColType::kInt32: {
                std::vector<i32> vals;
                vals.reserve(n);
                for (const ColPtr& p : parts)
                    vals.push_back(static_cast<const Column<i32>*>(p.get())->At(0));
                return std::make_shared<Column<i32>>(std::move(vals));
            }
            case ColType::kInt64: {
                std::vector<i64> vals;
                vals.reserve(n);
                for (const ColPtr& p : parts)
                    vals.push_back(static_cast<const Column<i64>*>(p.get())->At(0));
                return std::make_shared<Column<i64>>(std::move(vals));
            }
            case ColType::kDate: {
                std::vector<ISQDate> vals;
                vals.reserve(n);
                for (const ColPtr& p : parts)
                    vals.push_back(static_cast<const Column<ISQDate>*>(p.get())->At(0));
                return std::make_shared<Column<ISQDate>>(std::move(vals));
            }
            case ColType::kDatetime: {
                std::vector<ISQDatetime> vals;
                vals.reserve(n);
                for (const ColPtr& p : parts)
                    vals.push_back(static_cast<const Column<ISQDatetime>*>(p.get())->At(0));
                return std::make_shared<Column<ISQDatetime>>(std::move(vals));
            }
            case ColType::kString: {
                std::vector<char> data;
                std::vector<ui64> offsets;
                offsets.reserve(n + 1);
                offsets.push_back(0);
                for (const ColPtr& p : parts) {
                    std::string_view sv = static_cast<const ColumnString*>(p.get())->At(0);
                    data.insert(data.end(), sv.begin(), sv.end());
                    offsets.push_back(data.size());
                }
                return std::make_shared<ColumnString>(std::move(data), std::move(offsets));
            }
        }
        return nullptr;
    }

    Batch BuildResult() {
        const ui64 n = order_.size();
        std::vector<ColPtr> result_cols;
        result_cols.reserve(key_exprs_.size() + aggr_protos_.size());
        for (ui64 k = 0; k < key_exprs_.size(); ++k)
            result_cols.push_back(ConcatKeyCol(order_keys_, k));
        for (ui64 a = 0; a < aggr_protos_.size(); ++a)
            result_cols.push_back(ConcatAggrCol(a));
        return Batch(std::move(result_cols), n);
    }

    OpPtr op_;
    std::vector<ExprPtr> key_exprs_;
    std::vector<std::shared_ptr<Aggr>> aggr_protos_;
    std::vector<ExprPtr> agg_exprs_;
    std::unordered_map<std::string, std::vector<std::shared_ptr<Aggr>>> groups_;
    std::vector<std::string> order_;
    std::vector<std::vector<ColPtr>> order_keys_;
    bool done_ = false;
};
