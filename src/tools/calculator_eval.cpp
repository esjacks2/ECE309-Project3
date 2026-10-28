#include "tools/calculator.h"
#include <stack>
#include <string>
#include <vector>

bool calc_evaluate(const std::vector<CalcToken>& tokens, double* out,
                   std::string* error) {
    using Kind = CalcToken::Kind;
    std::stack<double> values;
    std::stack<Kind> ops;
    auto precedence = [](Kind k) -> int {
        if (k == Kind::Plus || k == Kind::Minus) return 1;
        if (k == Kind::Times || k == Kind::Divide) return 2;
        return 0;
    };
    auto apply = [&]() -> bool {
        if (ops.empty() || values.size() < 2) {
            *error = "invalid expression";
            return false;
        }
        Kind k = ops.top();
        ops.pop();
        double right = values.top();
        values.pop();
        double left = values.top();
        values.pop();
        double result = 0;
        switch (k) {
            case Kind::Plus: result = left + right; break;
            case Kind::Minus: result = left - right; break;
            case Kind::Times: result = left * right; break;
            case Kind::Divide:
                if (right == 0) {
                    *error = "division by zero";
                    return false;
                }
                result = left / right;
                break;
            default:
                *error = "invalid operator";
                return false;
        }
        values.push(result);
        return true;
    };
    for (const auto& token : tokens) {
        Kind k = token.kind;
        if (k == Kind::Number) values.push(token.value);
        else if (k == Kind::LParen) ops.push(k);
        else if (k == Kind::RParen) {
            while (!ops.empty() && ops.top() != Kind::LParen)
                if (!apply()) return false;
            if (ops.empty()) {
                *error = "mismatched parentheses";
                return false;
            }
            ops.pop();
        } else {
            if (precedence(k) == 0) {
                *error = "invalid operator";
                return false;
            }
            while (!ops.empty() && ops.top() != Kind::LParen &&
                   precedence(ops.top()) >= precedence(k))
                if (!apply()) return false;
            ops.push(k);
        }
    }
    while (!ops.empty()) {
        if (ops.top() == Kind::LParen) {
            *error = "mismatched parentheses";
            return false;
        }
        if (!apply()) return false;
    }
    if (values.size() != 1) {
        *error = "invalid expression";
        return false;
    }
    *out = values.top();
    return true;
}
