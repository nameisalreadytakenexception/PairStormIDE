#include "codeeditor.h"
#include "linenumberarea.h"
#include<QtGui>
#include<QDebug>
#include<QTextCursor>
#include<QPainter>
#include <QFontDatabase>
#include<QScrollBar>
#include<QMessageBox>
#include<iostream>

#define TAB_SPACE 4

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    setLineWrapMode(QPlainTextEdit::NoWrap);// don't move cursor to the next line where it's out of visible scope
    this->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
    this->setTabStopDistance(TAB_SPACE * fontMetrics().width(QLatin1Char('0')));//set tab distance
    mCurrentZoom = 100;//in persents

    //read settings
    QSettings settings(QApplication::organizationName(), QApplication::applicationName());
    QString analizerFontSize = settings.value("analizerFontSize").toString();
    QString analizerFontName = settings.value("analizerFontName").toString();
    QString analizerStyle = settings.value("analizerStyle").toString();
    mConfigParam.setConfigParams(analizerFontName,analizerFontSize,analizerStyle);

    //create objects connected to codeEditor
    lineNumberArea = new LineNumberArea(this);
    mTimer = new QTimer;
    mHcpp = new Highlightercpp(document());
    mLcpp = new LexerCPP();
    mTimer = new QTimer;
    mChangeManager = new ChangeManager(this->toPlainText().toUtf8().constData());
    //comment button
    mAddCommentButton = new AddCommentButton(this);
    mAddCommentButton->setText("+");
    mAddCommentButton->setVisible(false);
    setMouseTracking(true);

    //This signal is emitted when the text document needs an update of the specified rect.
    //If the text is scrolled, rect will cover the entire viewport area.
    //If the text is scrolled vertically, dy carries the amount of pixels the viewport was scrolled.
    connect(this,  SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
    connect(this,  SIGNAL(cursorPositionChanged()), this, SLOT(runLexerAndHighlight()));
    connect(this,  SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));
    connect(mTimer, SIGNAL(timeout()), this, SLOT(saveStateInTheHistory()));
    connect(this,  SIGNAL(textChanged()), this, SLOT(changesAppeared()));

    mTimer->start(CHANGE_SAVE_TIME);//save text by this time

    // start typing from correct position (in the first line it doesn't consider weight of lineCounter)
    //that's why we need to set this position
    updateLineNumberAreaWidth();
    highlightCurrentLine();
    //this->setTabStopDistance(TAB_SPACE * fontMetrics().width(QLatin1Char('0')));//set tab distance
    //fonts and colors configurations
    mFont.setPointSize(mConfigParam.mFontSize);
    mFont.setFamily(mConfigParam.mTextStyle);
    mFont.setBold(false);
    mFont.setItalic(false);
    this->setFont(mFont);
}

void CodeEditor::runLexerAndHighlight()
{
    //run lexer
    mLcpp->clear();
    mLcpp->lexicalAnalysis(document()->toPlainText());
    mTokens = mLcpp->getTokens();
    //run highlight
    mHcpp->setData(mTokens);
    mHcpp->setText(this->document()->toPlainText());
    for(int i = 0; i < mHcpp->mLines.size(); i++)
    {
        mHcpp->highlightBlock(mHcpp->mLines[i]);
    }

    for(auto it = mTokens.begin(); it < mTokens.end(); ++it)
       // qDebug() << it->name << " "  << it->begin << " " << it->end << " " << it->linesCount << '\n';
    mLexer.lexicalAnalysis(toPlainText());
    mTokens = mLexer.getTokens();
}

int CodeEditor::getLineNumberAreaWidth()
{
    int digits = 1;
    int currLineNumber = qMax(1, blockCount());
    while (currLineNumber >= 10)
    {
        currLineNumber /= 10;
        ++digits;
    }
    return fontMetrics().averageCharWidth() * digits;// wight of one symbol(in our case number) * count of digits
}

QString &CodeEditor::getFileName()
{
    return mFileName;
}

void CodeEditor::setFileName(const QString &fileName)
{
    this->mFileName = fileName;
}

std::pair<const QString &, const QString &> CodeEditor::getChangedFileInfo()
{
    return std::make_pair(this->toPlainText(), mFileName);
}

void CodeEditor::undo()
{
    QString text = QString::fromStdString(this->mChangeManager->undo());
    this->document()->setPlainText(text);

    QTextCursor cursor(this->document());
    cursor.setPosition(mChangeManager->getCursorPosPrev());

    this->setTextCursor(cursor);
}

void CodeEditor::redo()
{
    QString text = QString::fromStdString(this->mChangeManager->redo());
    this->document()->setPlainText(text);

    QTextCursor cursor(this->document());
    cursor.setPosition(mChangeManager->getCursorPosNext());

    this->setTextCursor(cursor);
}

void CodeEditor::updateLineNumberAreaWidth()
{
    // reset start position for typing (according new linecounter position)
    setViewportMargins(getLineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)// rectangle of current block and Y-Axis changing
{
    if(dy)// when not all of the text is in the visible area (we scrolled it)
    {
        lineNumberArea->scroll(0, dy);// we should scroll lines numbers in following direction
    }
    else
    {
        lineNumberArea->update(0, 0, lineNumberArea->width(), rect.height());//set position to the new block (area for line number)
    }
    if(rect.contains(viewport()->rect()))//when one covers other (text is under line counter)
        updateLineNumberAreaWidth();
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();//whole area inside widget's margins
    lineNumberArea->setGeometry(QRect(0, 0, getLineNumberAreaWidth(), cr.height()));//set the same height as codeEditor for lineCouter
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    QTextEdit::ExtraSelection selection;

    QColor lineColor = mConfigParam.mCurrentLineColor;
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    extraSelections.append(selection);
    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), mConfigParam.mLineCounterAreaColor);

    QTextBlock block = firstVisibleBlock();//area of first numeration block from linecounter
    int blockNumber = block.blockNumber();//get line number (start from 0)
    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());//top of currentblock 0
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());//bottom of current block                 -

    while (block.isValid())//we have blocks (have lines numbers)
    {
        QString number = QString::number(blockNumber + 1);
        painter.setPen(mConfigParam.mCodeTextColor);
        painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),//draw line count
                         Qt::AlignCenter, number);
        block = block.next();
        int temp = top;//save current top position
        top = bottom;//refresh the top bottom (next block top == this block bottom)
        bottom += bottom - temp;// bottom - temp = dy which is block height
        ++blockNumber;
    }
    mLinesCount = blockNumber;
}

void CodeEditor::saveStateInTheHistory()
{
    std::string newFileState = this->toPlainText().toUtf8().constData();
    mChangeManager->writeChange(newFileState);
}

void CodeEditor::mouseMoveEvent(QMouseEvent *event)
{
    if (event->button() == Qt::NoButton)
    {
        QTextBlock block = this->firstVisibleBlock();

        int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());//top of currentblock 0
        int bottom = top + static_cast<int>(blockBoundingRect(block).height());
        int side = bottom - top;// size of each side of comment button

       // side of rectancge where our bottom will be. X-0 && Y-0 start from the left top, so (top < bottom)
        const QSize buttonSize = QSize(side, side);
        mAddCommentButton->setFixedSize(buttonSize);

        auto currSliderPos = this->verticalScrollBar()->sliderPosition();

        int commentAreaRightMargin = this->width() - this->verticalScrollBar()->width() - getLineNumberAreaWidth();
        int commentAreaLeftMargin = commentAreaRightMargin - side;

       if((event->x() >= commentAreaLeftMargin) && (event->x() <= commentAreaRightMargin))//mouse inside comment block
       {
            int linesFromTheTop = event->y() / side;
            int currLine = linesFromTheTop + currSliderPos + 1;//because first block = 0
            mAddCommentButton->setCurrentLine(currLine);
            int commentBottonYpos = linesFromTheTop * side;// get Y pos for bottom

            if(currLine <= mLinesCount)// check if the line exists
            {
                int commentBottonXpos = commentAreaLeftMargin + getLineNumberAreaWidth();

                if(currSliderPos)// if we scrolled
                {
                    mAddCommentButton->setGeometry(commentBottonXpos, commentBottonYpos, side, side);
                }
                else
                {
                    //if text wasn't scrolled, we have to add these pixels in order to right drawing level
                    mAddCommentButton->setGeometry(commentBottonXpos,
                                                   commentBottonYpos + TOP_UNUSED_PIXELS_HEIGHT, side, side);
                }
                mAddCommentButton->setVisible(true);
            }
       }
       else
       {
           mAddCommentButton->setVisible(false);
       }
    }
    QPlainTextEdit::mouseMoveEvent(event);
}
