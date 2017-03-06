#pragma once

#include <string>
#include <iostream>

#define ARG_NOT_NULL(arg) if((arg) == nullptr) throw llast::InvalidArgumentException("##arg");

#ifdef LLAST_DEBUG
#define DEBUG_ASSERT(arg, reason) if(!(arg)) { throw llast::DebugAssertionFailedException( \
    std::string("Debug assertion failed!") + \
    std::string("\nFile       : ") + std::string(__FILE__) + \
    std::string("\nLine       : ") + std::to_string(__LINE__) + \
    std::string("\nExpression : ") + std::string(#arg) + \
    std::string("\nReason     : ") + std::string(reason)); }
#else
#define DEBUG_ASSERT(arg) //no op
#endif

namespace llast {

    class Exception {
        std::string message_;

    public:
        Exception(std::string message) : message_(message) {
            //TODO: store traceback/stacktrace/whatever.
            // http://stackoverflow.com/questions/3151779/how-its-better-to-invoke-gdb-from-program-to-print-its-stacktrace/4611112#4611112
        }

        virtual ~Exception() { }

        void dump() {
            std::cout << message_ << "\n";
        }
    };

    class FatalException : public Exception {
    public:
        FatalException(const std::string &message) : Exception(message) { }

        virtual ~FatalException() {

        }
    };

#ifdef LLAST_DEBUG
    class DebugAssertionFailedException : public FatalException {
    public:
        DebugAssertionFailedException(const std::string &message) : FatalException(message) {

        }
    };
#endif

    class InvalidArgumentException : public Exception {
    public:
        InvalidArgumentException(const std::string &argumentName)
                : Exception("Invalid value for argument " + argumentName) {

        }
    };

    class UnhandledSwitchCase : public FatalException {
    public:
        UnhandledSwitchCase() : FatalException("Ruh roh.  There was an unhandled switch case.") {}

        virtual ~UnhandledSwitchCase() {

        }
    };

    class InvalidStateException : public FatalException {
    public:
        InvalidStateException(const std::string &message) : FatalException(message) {

        }

        virtual ~InvalidStateException() {

        }

    };

    class CompileException : public Exception {
    public:
        CompileException(const std::string &message) : Exception(message) {

        }
    };

}