
#pragma once

#include "AST.hpp"

namespace llast {


    namespace ExprRunner {
        /** Just attempts to compile expr and discards any results. Used by to see if expr contains any
         * conditions that can throw CompileException.  TODO:  remove from public API. */
        void compile(unique_ptr<const Expr> expr);

        float runFloatExpr(unique_ptr<const Expr> expr);
        int runInt32Expr(unique_ptr<const Expr> expr);
    }
}

