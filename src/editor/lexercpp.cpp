#include "lexercpp.h"

inline bool LexerCPP::isKeyword(const QString &lexem)
{
    return cKeywords.contains(lexem);
}

inline bool LexerCPP::isIdentifier(const QString &lexem)
{
    const QRegExp cRx("[A-Za-z_][A-Za-z0-9_]*");
    return cRx.exactMatch(lexem);
}

inline bool LexerCPP::isNumber(const QString &lexem)
{
    const QRegExp cRx("[0-9]+|[0-9]+[UuLl]?");
    return cRx.exactMatch(lexem);
}

inline bool LexerCPP::isFloatNumber(const QString &lexem)
{
    const QRegExp cRx("[0-9]+\\.[0-9]+");
    return cRx.exactMatch(lexem);
}

inline bool LexerCPP::isOperator(const QString &lexem)
{
    return cOperators.contains(lexem);
}

inline bool LexerCPP::isSymbolLiteral(const QString &lexem)
{
    const QRegExp cRx("'.'|'\.+'");
    return cRx.exactMatch(lexem);
}

inline bool LexerCPP::isStringLiteral(const QString &lexem)
{
    const QRegExp cRx("\".*\"");
    return cRx.exactMatch(lexem);
}

inline bool LexerCPP::isOneLineComment(const QString &lexem)
{
    const QRegExp cRx("\\/\\/.*");
    return cRx.exactMatch(lexem);
}

inline bool LexerCPP::isBlockComments(const QString &lexem)
{
    const QRegExp cRx("/\\*[\\s\\S]*\\*/");
    return cRx.exactMatch(lexem);
}

inline bool LexerCPP::isSpace(const QChar& sym)
{
    return cSpaces.contains(sym);
}

inline bool LexerCPP::isLetter(const QChar& sym)

{
    const QRegExp cRx("[A-Za-z]");
    return cRx.exactMatch(sym);
}

inline bool LexerCPP::isDigit(const QChar& sym)
{
    const QRegExp cRx("[0-9]");
    return cRx.exactMatch(sym);
}

inline bool LexerCPP::isQuote(const QChar& sym)
{
    return sym == '\'' || sym == '\"';
}

inline bool LexerCPP::isLexemEnd(const QChar& sym)
{
    return isSpace(sym) || isOperator(sym) || isQuote(sym);
}

inline void LexerCPP::addLexem()
{
    --mIndex;
    mTokensOnCurrentLine.append(Token(QString(mCurrentLexem), State(mState), mIndex - mCurrentLexem.size(), mIndex));
    mCurrentLexem.clear();
    mState = State::ST;
}

inline void LexerCPP::changeState(State state, QChar sym)
{
    mState = state;
    mCurrentLexem += sym;
}

void LexerCPP::clear()
{
    mIndex = 0;
    mCurrentLexem.clear();
    mState = State::ST;
}

bool LexerCPP::isLexerWasRunning() const
{
    return mWasRunning;
}

QVector<Token> LexerCPP::getTokens() const
{
    return mTokensOnCurrentLine;
}

void LexerCPP::handleStartState(const QChar &sym)
{
    if(isLetter(sym))
    {
        changeState(State::ID, sym);
    }
    else if(isDigit(sym))
    {
        changeState(State::NUM, sym);
    }
    else if(isOperator(QString(sym)))
    {
        changeState(State::OPER, sym);
    }
    else if(isQuote(sym))
    {
        changeState(State::LIT, sym);
    }
    else
    {
        changeState(State::UNDEF, sym);
    }
}

void LexerCPP::handleIdentifierState(const QChar &sym)
{
    if(isLexemEnd(sym))
    {
        addLexem();
    }
    else if(isIdentifier(mCurrentLexem + sym))
    {
        mCurrentLexem += sym;
        if(isKeyword(mCurrentLexem))
        {
            mState = State::KW;
        }
    }
    else
    {
        changeState(State::UNDEF, sym);
    }
}

void LexerCPP::handleKeywordState(const QChar &sym)
{
    if(isLexemEnd(sym))
    {
        addLexem();
    }
    else if(isKeyword(mCurrentLexem + sym))
    {
        mCurrentLexem += sym;
    }
    else if (isIdentifier(mCurrentLexem + sym))
    {
        changeState(State::ID, sym);
    }
    else
    {
        changeState(State::UNDEF, sym);
    }
}

void LexerCPP::handleNumberState(const QChar &sym)
{
    if(sym == cDot)
    {
        changeState(State::FNUM, sym);
    }
    else if(isLexemEnd(sym))
    {
        addLexem();
    }
    else if(isNumber(mCurrentLexem + sym))
    {
        mCurrentLexem += sym;
    }
    else
    {
        changeState(State::UNDEF, sym);
    }
}

void LexerCPP::handleFloatNumberState(const QChar &sym)
{
    if(isLexemEnd(sym))
    {
        addLexem();
    }
    else if(isFloatNumber(mCurrentLexem + sym))
    {
        mCurrentLexem += sym;
    }
    else
    {
        changeState(State::UNDEF, sym);
    }
}

void LexerCPP::handleOperatorState(const QChar &sym)
{
    if(isOneLineComment(mCurrentLexem + sym) || (mCurrentLexem + sym) == cBeginCommentBlock)
    {
        changeState(State::COM, sym);
    }
    else if(isSpace(sym) || !isOperator(mCurrentLexem + sym) || isQuote(sym))
    {
        addLexem();
    }
    else if(isOperator(mCurrentLexem + sym))
    {
        mCurrentLexem += sym;
    }
    else
    {
        changeState(State::UNDEF, sym);
    }
}

void LexerCPP::handleCommentState(const QChar &sym)
{
    if((isOneLineComment(mCurrentLexem) && sym == cNextLine) || isBlockComments(mCurrentLexem))
    {
        addLexem();
    }
    else
    {
        mCurrentLexem += sym;
    }
}

void LexerCPP::handleUndefinedState(const QChar &sym)
{
    if(isLexemEnd(sym))
    {
        addLexem();
    }
    else
    {
        mCurrentLexem += sym;
    }
}

void LexerCPP::handleLiteralState(const QChar &sym)
{
    if(isSymbolLiteral(mCurrentLexem) || isStringLiteral(mCurrentLexem))
    {
        addLexem();
    }
    else if(sym == cNextLine)
    {
        addLexem();
    }
    else
    {
        mCurrentLexem += sym;
    }
}

void LexerCPP::lexicalAnalysis(QString code)
{
    mWasRunning = true;
    mTokensOnCurrentLine.clear();
    QChar sym = 0;
    mIndex = 0;

    unsigned int end = static_cast<unsigned int>(code.size());

    while(mIndex < end)
    {
        sym = code[mIndex];
        ++mIndex;

        switch(mState)
        {
        case State::ST:
            if(isSpace(sym))
            {
                break;
            }
            handleStartState(sym);
            break;

        case State::ID:
            handleIdentifierState(sym);
            break;

        case State::KW:
            handleKeywordState(sym);
            break;

        case State::NUM:
            handleNumberState(sym);
            break;

        case State::FNUM:
            handleFloatNumberState(sym);
            break;

        case State::OPER:
            handleOperatorState(sym);
            break;

        case State::COM:
            handleCommentState(sym);
            break;

        case State::LIT:
            handleLiteralState(sym);
            break;

        case State::UNDEF:
            handleUndefinedState(sym);
            break;
        }
    }

    if(mState != State::ST)
    {
        ++mIndex;
        addLexem();
    }
}
