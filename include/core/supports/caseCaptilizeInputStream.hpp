#pragma once 
#include <cwctype>
#include "antlr4-runtime.h"

class CaseCaptilizeInputStream : public antlr4::ANTLRInputStream {
private:
    bool _inString = false;
    bool _inComment = false;

public:
    using antlr4::ANTLRInputStream::ANTLRInputStream;

    size_t LA(ssize_t i) override {
        const size_t character = antlr4::ANTLRInputStream::LA(i);

        // EOF or Null check
        if (character == antlr4::IntStream::EOF || character == 0) {
            return character;
        }

        // 2. Detect String Literals (starts and ends with ")
        if (!_inComment && character == '"') {
            _inString = !_inString;
            return character;
        }

        // 3. Logic: If inside a string OR a comment, DO NOT capitalize.
        if (_inString || _inComment) {
            return character;
        }

        // 4. Otherwise, capitalize for Case-Independent Keywords
        return static_cast<size_t>(std::toupper(static_cast<int>(character)));
    }

    // Ensure state resets if the stream is reused
    void reset() override {
        antlr4::ANTLRInputStream::reset();
        _inString = false;
        _inComment = false;
    }
};