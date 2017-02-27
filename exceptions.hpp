#pragma once

namespace llast {

    class Exception {
        std::string message_;
    public:
        Exception(std::string message) : message_(message) {
            //TODO: store traceback/stacktrace/whatever.
            // http://stackoverflow.com/questions/3151779/how-its-better-to-invoke-gdb-from-program-to-print-its-stacktrace/4611112#4611112
        }

        void dump() {
            std::cout << message_ << "\n";
        }
    };

    class FatalException : Exception {
    public:
        FatalException(const std::string &message) : Exception(message) {}

    };

    class UnhandledSwitchCase : FatalException {
    public:
        UnhandledSwitchCase() : FatalException("Ruh roh.  There was an unhandled switch case.") {}
    };

}