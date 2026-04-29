#pragma once 
#include <cwctype>
#include "antlr4-runtime.h"

namespace ams { // FIX: Wrap in namespace to match main.cpp

class CaseCaptilizeInputStream : public antlr4::ANTLRInputStream {
private:
    bool _inString = false;
    bool _inComment = false;

public:
    using antlr4::ANTLRInputStream::ANTLRInputStream;

    // LA (Look Ahead) is called by the lexer to see the next characters
    size_t LA(ssize_t i) override {
        size_t character = antlr4::ANTLRInputStream::LA(i);

        // 1. EOF or Null check
        if (character == antlr4::IntStream::EOF || character == 0) {
            return character;
        }

        // 2. State Detection for Strings
        // If we hit a quote, toggle the string state
        if (character == '"') {
            _inString = !_inString;
            return character;
        }

        // 3. State Detection for Comments (AMS uses # for comments)
        if (character == '#') {
            _inComment = true;
            return character;
        }
        
        // Reset comment state on New Line
        if (character == '\n' || character == '\r') {
            _inComment = false;
            return character;
        }

        // 4. Logic: If inside a string OR a comment, DO NOT capitalize.
        if (_inString || _inComment) {
            return character;
        }

        // 5. Otherwise, capitalize for Case-Independent Keywords
        // This makes 'source', 'SOURCE', and 'sOuRcE' identical to the Lexer
        return static_cast<size_t>(std::toupper(static_cast<int>(character)));
    }

    void reset() override {
        antlr4::ANTLRInputStream::reset();
        _inString = false;
        _inComment = false;
    }
};

} // namespace ams