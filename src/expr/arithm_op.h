#include <type_traits>

#include "column/column_string.h"
#include "expr/expr.h"

struct ArithmOp {
    template <typename T>
    static ColPtr Exec(const Column<T>& left, const ColPtr& right, kOp op) {
        if constexpr (!std::is_arithmetic_v<T>) {
            SEND_MESSAGE("Date/Datetime arithmetic operation is not supported");
        } else {
            const Column<T>* r = static_cast<const Column<T>*>(right.get());
            std::vector<i64> result(left.Size());
            for (ui64 i = 0; i < left.Size(); ++i) {
                i64 li = static_cast<i64>(left.At(i));
                i64 ri = static_cast<i64>(r->At(i));
                switch (op) {
                    case kOp::kAdd:
                        result[i] = li + ri;
                        break;
                    case kOp::kSub:
                        result[i] = li - ri;
                        break;
                    case kOp::kMul:
                        result[i] = li * ri;
                        break;
                    case kOp::kDiv:
                        result[i] = li / ri;
                        break;
                    default:
                        SEND_MESSAGE("It's not an arithmetic operation");
                }
            }
            return std::make_shared<Column<i64>>(std::move(result));
        }
    }

    static ColPtr Exec(const ColumnString&, const ColPtr&, kOp) {
        SEND_MESSAGE("String arithmetic operation is not supported");
    }
};
