

#pragma once

#include "ast.hpp"

#include <functional>

namespace llast {

    namespace EmbryonicCompiler {
        void compileEmbryonically(const Expr *expr);
    };
}