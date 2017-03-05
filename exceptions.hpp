#pragma once

#include <string>
#include <iostream>

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

    class InvalidArgumentException : public Exception {
    public:
        InvalidArgumentException(const std::string &argumentName)
                : Exception("Invalid value for argument " + argumentName) {

        }
    };

#define ARG_NOT_NULL(arg) if((arg) == nullptr) throw InvalidArgumentException("##arg");

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
}