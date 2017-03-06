
#include "ast.hpp"

namespace llast {

    std::string to_string(ExpressionKind expressionType) {
        switch (expressionType) {
            case ExpressionKind::Binary:
                return "Binary";
            case ExpressionKind::Invoke:
                return "Invoke";
            case ExpressionKind::VariableRef:
                return "Variable";
            case ExpressionKind::AssignVariable:
                return "AssignVariable";
            case ExpressionKind::Conditional:
                return "Conditional";
            case ExpressionKind::Switch:
                return "Switch";
            case ExpressionKind::Block:
                return "Block";
            case ExpressionKind::LiteralInt32:
                return "LiteralInt";
            default:
                throw UnhandledSwitchCase();
        }
    }


    std::string to_string(OperationKind type) {
        switch (type) {
            case OperationKind::Add:
                return "Add";
            case OperationKind::Sub:
                return "Sub";
            case OperationKind::Mul:
                return "Mul";
            case OperationKind::Div:
                return "Div";
            default:
                throw UnhandledSwitchCase();
        }
    }
    
    std::string to_string(DataType dataType) {
        switch (dataType) {
            case DataType::Void:
                return "void";
            case DataType::Bool:
                return "Bool";
            case DataType::Int32:
                return "Int32";
            case DataType::Pointer:
                return "Pointer";
            default:
                throw UnhandledSwitchCase();
        }
    }


}