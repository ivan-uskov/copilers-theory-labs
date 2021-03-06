#include "stdafx.h"
#include "Tokenizer.h"
#include <sstream>
#include <algorithm>
#include <functional>

using namespace std;
using namespace std::placeholders;

namespace Lexer
{
    Tokenizer::Tokenizer(istream & input, ostream & errors)
        : m_input(input)
        , m_errors(errors)
    {
    }

    //public

    vector<Token> Tokenizer::getTokens() const
    {
        return m_tokens;
    }

    bool Tokenizer::scan()
    {
        bool success = true;
        m_tokenPatternProvider.initialize();

        while (!m_input.eof())
        {
            getline(m_input, m_currentLine);

            if (m_currentLine.size())
            {
                if (!parseLine())
                {
                    success = false;
                }

                ++m_lineCount;
            }
        }

        addEndToken();
        return success;
    }

    //private

    bool Tokenizer::parseLine()
    {
        m_charCount = 0;
        bool success = true;
        string::iterator it;

        trimLine();
        while ((it = m_currentLine.begin() + m_charCount) < m_currentLine.end())
        {
            auto baseCount = m_charCount;
            if (!parseAtom(it))
            {
                success = false;
            }

            if (baseCount == m_charCount)
            {
                addInvalidToken();
            }

            trimLine();
        }

        return success;
    }

    bool Tokenizer::parseAtom(std::string::iterator const& it)
    {
        return parseToken(it, m_currentLine.end() - it)      ||
               parseRegexTokens({ it, m_currentLine.end() });
    }

    bool Tokenizer::parseRegexTokens(string const& line)
    {
        auto isPatternMatches = bind(matchRegexPattern, _1, line);
        auto patterns = m_tokenPatternProvider.getRegexPatterns();

        auto it = find_if(patterns.begin(), patterns.end(), [&](TokenRegexPattern const& pattern) {
            auto result = matchRegexPattern(pattern.pattern, line);
            if (result.size())
            {
                addToken(pattern.type, result);
                m_charCount += result.size();
                return true;
            }

            return false;
        });

        return it < patterns.end();
    }

    bool Tokenizer::parseToken(string::iterator const& it, size_t availableSize)
    {
        bool success = false;
        auto isPatternMatches = bind(patternMatches, _1, it, availableSize);
        auto tokens = m_tokenPatternProvider.getPatterns();
        auto patternIt = find_if(tokens.begin(), tokens.end(), isPatternMatches);

        if (patternIt != tokens.end())
        {
            addTokenByPattern(*patternIt);
            success = true;
        }

        return success;
    }

    void Tokenizer::addTokenByPattern(TokenPattern const& pattern)
    {
        addToken(pattern.type, pattern.pattern);
        m_charCount += pattern.pattern.size();
    }

    void Tokenizer::addEndToken()
    {
        addToken(TokenType::END, "END");
    }

    void Tokenizer::addInvalidToken()
    {
        addToken(TokenType::Invalid);
        ++m_charCount;
    }

    void Tokenizer::addToken(TokenType const& type, string const& value)
    {
        m_tokens.push_back({
            type,
            value,
            m_lineCount + 1,
            m_charCount + 1
        });
    }

    void Tokenizer::trimLine()
    {
        while (m_charCount < m_currentLine.size() && (m_currentLine.begin()[m_charCount] == ' ' || m_currentLine.begin()[m_charCount] == '\t'))
        {
            ++m_charCount;
        }
    }
}
