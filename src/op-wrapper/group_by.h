#pragma once

#include <boost/unordered/unordered_flat_map.hpp>
#include <climits>
#include <string>
#include <string_view>
#include <vector>

#include "aggr/aggr.h"
#include "column/col_ops.h"
#include "column/column_int_dt.h"
#include "column/column_string.h"
#include "expr/expr.h"
#include "op-wrapper/op.h"

class GroupByOp : public Op {
public:
    GroupByOp(OpPtr op, std::vector<ExprPtr>&& key_exprs,
              std::vector<std::shared_ptr<Aggr>>&& aggr_protos,
              std::vector<ExprPtr>&& agg_exprs, ui64 limit = ULLONG_MAX,
              ui64 offset = 0)
        : op_(std::move(op)),
          key_exprs_(std::move(key_exprs)),
          aggr_protos_(std::move(aggr_protos)),
          agg_exprs_(std::move(agg_exprs)),
          limit_(limit),
          offset_(offset) {}

    bool HasNext() override { return !done_; }

    Batch Next() override {
        const ui64 cap = CalcCap();
        while (op_->HasNext()) {
            Batch batch = op_->Next();
            const std::vector<ui8>& mask = batch.GetMask();

            std::vector<ColPtr> key_cols, agg_cols;
            for (ExprPtr& e : key_exprs_) key_cols.push_back(e->Eval(batch));
            for (ExprPtr& e : agg_exprs_) agg_cols.push_back(e->Eval(batch));
            if (key_col_protos_.empty()) {
                key_col_protos_ = key_cols;
            }

            std::string key;
            for (ui64 row = 0; row < batch.CntRows(); ++row) {
                if (!mask[row]) continue;
                key.clear();
                for (const ColPtr& kc : key_cols) AppendKeyCell(key, kc, row);

                auto it = groups_.find(key);
                if (it == groups_.end()) {
                    if (cap != ULLONG_MAX && groups_.size() >= cap) continue;
                    order_.push_back(key);
                    auto [new_it, _] = groups_.emplace(
                        key, std::vector<std::shared_ptr<Aggr>>{});
                    it = new_it;
                    std::vector<ColPtr> snap;
                    snap.reserve(key_cols.size());
                    for (const ColPtr& kc : key_cols)
                        snap.push_back(ColOps::SliceRow(kc, row));
                    order_keys_.push_back(std::move(snap));
                    for (std::shared_ptr<Aggr>& p : aggr_protos_)
                        it->second.push_back(p->Clone());
                }
                for (ui64 i = 0; i < agg_cols.size(); ++i)
                    it->second[i]->UpdateRow(agg_cols[i], row);
            }
        }
        done_ = true;
        return BuildResult();
    }

private:
    ui64 CalcCap() const {
        if (limit_ == ULLONG_MAX || offset_ > ULLONG_MAX - limit_) {
            return ULLONG_MAX;
        }
        return offset_ + limit_;
    }

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
                IsqDate v =
                    static_cast<const Column<IsqDate>*>(col.get())->At(row);
                key.append(reinterpret_cast<const char*>(&v), sizeof(v));
                break;
            }
            case ColType::kDatetime: {
                IsqDatetime v =
                    static_cast<const Column<IsqDatetime>*>(col.get())->At(row);
                key.append(reinterpret_cast<const char*>(&v), sizeof(v));
                break;
            }
            case ColType::kString: {
                std::string_view sv =
                    static_cast<const ColumnString*>(col.get())->At(row);
                ui32 len = static_cast<ui32>(sv.size());
                key.append(reinterpret_cast<const char*>(&len), 4);
                key.append(sv.data(), len);
                break;
            }
        }
    }

    ColPtr ConcatAggrCol(ui64 ind, ui64 start, ui64 count) {
        if (count == 0) return ColOps::EmptyLike(aggr_protos_[ind]->Result());
        std::vector<ColPtr> parts(count);
        for (ui64 i = start; i < start + count; ++i) {
            parts[i - start] = groups_.at(order_[i])[ind]->Result();
        }
        switch (parts[0]->GetType()) {
            case ColType::kInt8: {
                std::vector<i8> vals;
                vals.reserve(count);
                for (const ColPtr& p : parts)
                    vals.push_back(
                        static_cast<const Column<i8>*>(p.get())->At(0));
                return std::make_shared<Column<i8>>(std::move(vals));
            }
            case ColType::kInt16: {
                std::vector<i16> vals;
                vals.reserve(count);
                for (const ColPtr& p : parts)
                    vals.push_back(
                        static_cast<const Column<i16>*>(p.get())->At(0));
                return std::make_shared<Column<i16>>(std::move(vals));
            }
            case ColType::kInt32: {
                std::vector<i32> vals;
                vals.reserve(count);
                for (const ColPtr& p : parts)
                    vals.push_back(
                        static_cast<const Column<i32>*>(p.get())->At(0));
                return std::make_shared<Column<i32>>(std::move(vals));
            }
            case ColType::kInt64: {
                std::vector<i64> vals;
                vals.reserve(count);
                for (const ColPtr& p : parts)
                    vals.push_back(
                        static_cast<const Column<i64>*>(p.get())->At(0));
                return std::make_shared<Column<i64>>(std::move(vals));
            }
            case ColType::kDate: {
                std::vector<IsqDate> vals;
                vals.reserve(count);
                for (const ColPtr& p : parts)
                    vals.push_back(
                        static_cast<const Column<IsqDate>*>(p.get())->At(0));
                return std::make_shared<Column<IsqDate>>(std::move(vals));
            }
            case ColType::kDatetime: {
                std::vector<IsqDatetime> vals;
                vals.reserve(count);
                for (const ColPtr& p : parts)
                    vals.push_back(
                        static_cast<const Column<IsqDatetime>*>(p.get())->At(
                            0));
                return std::make_shared<Column<IsqDatetime>>(std::move(vals));
            }
            case ColType::kString: {
                std::vector<char> data;
                std::vector<ui64> offsets;
                offsets.reserve(count + 1);
                offsets.push_back(0);
                for (const ColPtr& p : parts) {
                    std::string_view sv =
                        static_cast<const ColumnString*>(p.get())->At(0);
                    data.insert(data.end(), sv.begin(), sv.end());
                    offsets.push_back(data.size());
                }
                return std::make_shared<ColumnString>(std::move(data),
                                                      std::move(offsets));
            }
        }
        return nullptr;
    }

    Batch BuildResult() {
        const ui64 n = order_.size();
        const ui64 start = std::min(offset_, n);
        const ui64 available = n - start;
        const ui64 count =
            (limit_ == ULLONG_MAX) ? available : std::min(limit_, available);
        std::vector<ColPtr> result_cols;
        result_cols.reserve(key_exprs_.size() + aggr_protos_.size());
        for (ui64 i = 0; i < key_exprs_.size(); ++i) {
            if (count == 0) {
                if (i < key_col_protos_.size()) {
                    result_cols.push_back(
                        ColOps::EmptyLike(key_col_protos_[i]));
                }
            } else {
                result_cols.push_back(
                    ColOps::ConcatCol(order_keys_, i, start, count));
            }
        }
        for (ui64 i = 0; i < aggr_protos_.size(); ++i)
            result_cols.push_back(ConcatAggrCol(i, start, count));
        return Batch(std::move(result_cols), count);
    }

    OpPtr op_;
    std::vector<ExprPtr> key_exprs_;
    std::vector<std::shared_ptr<Aggr>> aggr_protos_;
    std::vector<ExprPtr> agg_exprs_;
    ui64 limit_;
    ui64 offset_;
    boost::unordered_flat_map<std::string, std::vector<std::shared_ptr<Aggr>>>
        groups_;
    std::vector<ColPtr> key_col_protos_;
    std::vector<std::string> order_;
    std::vector<std::vector<ColPtr>> order_keys_;
    bool done_ = false;
};
