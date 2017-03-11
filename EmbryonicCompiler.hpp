

#pragma once

#include "Expr.hpp"

#include <functional>

namespace llast {

    namespace EmbryonicCompiler {
        void compile(const Expr *expr);
        int compileAndExecuteInt32Expr(const Expr *expr);
        float compileAndExecuteFloatExpr(const Expr *expr);
    };

}