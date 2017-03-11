

#pragma once

#include "Expr.hpp"

#include <functional>

namespace llast {

    namespace EmbryonicCompiler {
        int compileAndExecute(const Expr *expr);
    };

}