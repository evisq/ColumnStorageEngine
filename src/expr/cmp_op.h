#include "column/column_string.h"
#include "expr.h"

struct CmpOp {
    template <typename T>
    static ColPtr Exec(const Column<T>& left, const ColPtr& right, Op op) {
        const Column<T>* r = static_cast<const Column<T>*>(right.get());
        std::vector<ui8> result(left.Size());
        for (ui64 i = 0; i < left.Size(); ++i) {
            switch (op) {
                case Op::kEq:
                    result[i] = left.At(i) == r->At(i);
                    break;
                case Op::kNe:
                    result[i] = left.At(i) != r->At(i);
                    break;
                case Op::kLs:
                    result[i] = left.At(i) < r->At(i);
                    break;
                case Op::kLe:
                    result[i] = left.At(i) <= r->At(i);
                    break;
                case Op::kGs:
                    result[i] = left.At(i) > r->At(i);
                    break;
                case Op::kGe:
                    result[i] = left.At(i) >= r->At(i);
                    break;
                default:
                    SEND_MESSAGE("It's not a comparison operation");
            }
        }
        return std::make_shared<Column<ui8>>(std::move(result));
    }

    static ColPtr Exec(const ColumnString& left, const ColPtr& right, Op op) {
        const ColumnString* r = static_cast<const ColumnString*>(right.get());
        std::vector<ui8> result(left.Size());
        for (ui64 i = 0; i < left.Size(); ++i) {
            switch (op) {
                case Op::kEq:
                    result[i] = left.At(i) == r->At(i);
                    break;
                case Op::kNe:
                    result[i] = left.At(i) != r->At(i);
                    break;
                case Op::kLs:
                    result[i] = left.At(i) < r->At(i);
                    break;
                case Op::kLe:
                    result[i] = left.At(i) <= r->At(i);
                    break;
                case Op::kGs:
                    result[i] = left.At(i) > r->At(i);
                    break;
                case Op::kGe:
                    result[i] = left.At(i) >= r->At(i);
                    break;
                default:
                    SEND_MESSAGE("It's not a comparison operation");
            }
        }
        return std::make_shared<Column<ui8>>(std::move(result));
    }
};