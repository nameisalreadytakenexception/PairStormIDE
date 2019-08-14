#include "codeeditor.h"
#include "linenumberarea.h"
#include<QtGui>
#include<QTextCursor>
#include<QPainter>
#include<QTextCharFormat>
#include <QFontDatabase>
#include<QScrollBar>
#include<QMessageBox>
#include<iostream>
#include<QLabel>
#include "eventbuilder.h"
#include "usermessages.h"
#include "filemanager.h"
#include "utils.h"
// temp
#include <QDebug>

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
    mLineNumberArea = new LineNumberArea(this);
    mTimer = new QTimer;
    mLcpp = new LexerCPP();
    mTimer = new QTimer;
    mChangeManager = new ChangeManager(this->toPlainText().toUtf8().constData());
    //comment button
    mAddCommentButton = new AddCommentButton(this);
    mAddCommentButton->setText("+");
    mAddCommentButton->setVisible(false);
    mCurrentCommentLable = new QLabel(this);
    setMouseTracking(true);

     //comment text edit
  //  mAddCommentTextEdit = new AddCommentTextEdit;
   // mAddCommentTextEdit->setVisible(false);

    mCommentWidget = new CommentWidget;
    mCommentWidget->setVisible(false);
    //This signal is emitted when the text document needs an update of the specified rect.
    //If the text is scrolled, rect will cover the entire viewport area.
    //If the text is scrolled vertically, dy carries the amount of pixels the viewport was scrolled.

    connect(this,              &QPlainTextEdit::updateRequest,                  this, &CodeEditor::updateLineNumberArea);
    connect(this,              &QPlainTextEdit::cursorPositionChanged,          this, &CodeEditor::runLexer);
    connect(mTimer,            &QTimer::timeout,                                this, &CodeEditor::saveStateInTheHistory);
    connect(this,              &QPlainTextEdit::cursorPositionChanged,          this, &CodeEditor::highlighText);
    connect(this,              &QPlainTextEdit::cursorPositionChanged,          this, &CodeEditor::textChangedInTheOneLine);
    connect(mAddCommentButton, &AddCommentButton::addCommentButtonPressed ,     this, &CodeEditor::showCommentTextEdit);
    connect(mCommentWidget->getEditTab(), &AddCommentTextEdit::emptyComment,    this, &CodeEditor::emptyCommentWasAdded);
    connect(mCommentWidget->getEditTab(), &AddCommentTextEdit::notEmptyComment, this, &CodeEditor::notEmptyCommentWasAdded  );

    //connect(mCommentsVector[int], SIGNAL(AddCommentButtonPressed(int)), this, SLOT(showCommentTextEdit(int)));

    mTimer->start(CHANGE_SAVE_TIME);//save text by this time

    // start typing from correct position (in the first line it doesn't consider weight of lineCounter)
    //that's why we need to set this position
    updateLineNumberAreaWidth();

    //fonts and colors configurations
    mFont.setPointSize(mConfigParam.mFontSize);
    mFont.setFamily(mConfigParam.mTextStyle);
    mFont.setBold(false);
    mFont.setItalic(false);
    this->setFont(mFont);

    //set text highlighting color
    fmtLiteral.setForeground(mConfigParam.mStringsColor);
    fmtKeyword.setForeground(mConfigParam.mBasicLiteralsColor);
    fmtComment.setForeground(mConfigParam.mCommentColor);
    fmtRegular.setForeground(mConfigParam.mCodeTextColor);
}

void CodeEditor::runLexer()
{
    mLcpp->clear();
    mLcpp->lexicalAnalysis(document()->toPlainText());
    mTokens = mLcpp->getTokens();
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

bool CodeEditor::isChanged()
{
    return mBeginTextState != this->toPlainText();
}

void CodeEditor::setBeginTextState()
{
    mBeginTextState = this->toPlainText();
}

void CodeEditor::updateLineNumberAreaWidth()
{
    // reset start position for typing (according new linecounter position)
    setViewportMargins(getLineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)// rectangle of current block and Y-Axis changing
{
    if (dy)// when not all of the text is in the visible area (we scrolled it)
    {
        mLineNumberArea->scroll(0, dy);// we should scroll lines numbers in following direction
    }
    else
    {
        mLineNumberArea->update(0, 0, mLineNumberArea->width(), rect.height());//set position to the new block (area for line number)
    }
    if (rect.contains(viewport()->rect()))//when one covers other (text is under line counter)
        updateLineNumberAreaWidth();
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();//whole area inside widget's margins
    mLineNumberArea->setGeometry(QRect(0, 0, getLineNumberAreaWidth(), cr.height()));//set the same height as codeEditor for lineCouter
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(mLineNumberArea);
    painter.fillRect(event->rect(), mConfigParam.mLineCounterAreaColor);

    QTextBlock block = firstVisibleBlock();//area of first numeration block from linecounter
    int blockNumber = block.blockNumber();//get line number (start from 0)
    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());//top of currentblock 0
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());//bottom of current block                 -


    while (block.isValid())//we have blocks (have lines numbers)
    {
        QString number = QString::number(blockNumber + 1);
        painter.setPen(mConfigParam.mCodeTextColor);
        painter.drawText(0, top, mLineNumberArea->width(), fontMetrics().height(),//draw line count
                         Qt::AlignCenter, number);
        block = block.next();
        int temp = top;//save current top position
        top = bottom;//refresh the top bottom (next block top == this block bottom)
        bottom += bottom - temp;// bottom - temp = dy which is block height
        ++blockNumber;
    }

    int addedHight = this->verticalScrollBar()->sliderPosition()? 0 : TOP_UNUSED_PIXELS_HEIGHT;
    int height = bottom - top;
    for(auto &i :mCommentsVector)
    {
        i->setGeometry(this->width() - this->verticalScrollBar()->width() - height,
                       (i->getCurrentLine() - this->verticalScrollBar()->sliderPosition() - 1) * height + addedHight,
                       bottom - top,
                       bottom - top);
    }
    mLinesCount = blockNumber;
}

void CodeEditor::saveStateInTheHistory()
{
    std::string newFileState = this->toPlainText().toUtf8().constData();
    mChangeManager->writeChange(newFileState);
}

void CodeEditor::zoom(int val)
{
    val > 0 ?this->zoomIn(val):this->zoomOut(-val);
    mCurrentZoom+=val;
}

void CodeEditor::setZoom(int zoomVal)
{
    zoom(zoomVal - mCurrentZoom);
}

void CodeEditor::textChangedInTheOneLine()
{
    emit(textChangedInLine(this->textCursor().blockNumber() + 1));
}

void CodeEditor::showCommentTextEdit(int line)
{
    mCommentWidget->setWindowTitle("Comment to " + QString::number(line) + " line");
    mCommentWidget->setPosition(this, mAddCommentButton);
    mCommentWidget->setVisible(true);
}

void CodeEditor::emptyCommentWasAdded()
{
    //delete from the database if record exists(for the future)
    mCommentWidget->setVisible(false);
    for (int i = 0; i < mCommentsVector.size(); i++)
    {
        if (mCommentsVector[i]->getCurrentLine() == mAddCommentButton->getCurrentLine())
        {
            mCommentsVector[i]->setVisible(false);
            mCommentsVector.erase(mCommentsVector.begin() + i);
        }
    }
}

void CodeEditor::notEmptyCommentWasAdded()
{
    //write to the database (for the future)
    AddCommentButton *commentButton = new AddCommentButton(this);
    commentButton->setGeometry(mAddCommentButton->geometry());
    commentButton->setCurrentLine(mAddCommentButton->getCurrentLine());
    commentButton->setStyleSheet("background-color: #18CD3C");
    commentButton->setText("✔");
    commentButton->setVisible(true);

    mCommentsVector.push_back(commentButton);
    for(int i = 0; i<mCommentsVector.size() - 1; i++)
    {
        if(mCommentsVector[i]->getCurrentLine() == mAddCommentButton->getCurrentLine())
        {
            mCommentsVector[i]->setVisible(false);
            mCommentsVector.erase(mCommentsVector.begin() + i);
        }
    }
    mCommentsSet.insert(commentButton);

    connect(mCommentsVector.back(), &AddCommentButton::addCommentButtonPressed, this, &CodeEditor::showCommentTextEdit);
    mAddCommentButton->setVisible(false);
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

       if ((event->x() >= commentAreaLeftMargin) && (event->x() <= commentAreaRightMargin))//mouse inside comment block
       {
            int linesFromTheTop = event->y() / side;
            int currLine = linesFromTheTop + currSliderPos + 1;//because first block = 0
            mAddCommentButton->setCurrentLine(currLine);

            if (currLine <= mLinesCount)// check if the line exists
            {
                int commentBottonXpos = commentAreaLeftMargin + getLineNumberAreaWidth();
                int commentBottonYpos = linesFromTheTop * side;

                mAddCommentButton->setGeometry(commentBottonXpos, currSliderPos ? commentBottonYpos :
                                          commentBottonYpos + TOP_UNUSED_PIXELS_HEIGHT , side, side);
                mAddCommentButton->setVisible(true);
                //label for line showing
                mCurrentCommentLable->setGeometry(mAddCommentButton->x() - getLineNumberAreaWidth(), mAddCommentButton->y(),
                                                  getLineNumberAreaWidth(), mAddCommentButton->height());
                mCurrentCommentLable->setText(QString::number(mAddCommentButton->getCurrentLine()));
                mCurrentCommentLable->setVisible(true);
            }
       }
       else
       {
           mAddCommentButton->setVisible(false);
           mCurrentCommentLable->setVisible(false);
       }
    }
    QPlainTextEdit::mouseMoveEvent(event);
}

void CodeEditor::closeEvent(QCloseEvent *event)
{    
    if (!isChanged())
    {
        event->accept();
        return;
    }
    QMessageBox::StandardButton reply = QMessageBox::question
            (this,
             userMessages[UserMessages::PromptSaveTitle],
             userMessages[UserMessages::SaveQuestion],
             QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    // if user closes dialog event is ignored
    if (reply == QMessageBox::Cancel)
    {
        event->ignore();
        return;
    }
    // if user doesn't want to save changes
    if (reply == QMessageBox::No)
    {
        event->accept();
        return;
    }

    try
    {
        FileManager().writeToFile(getFileName(), toPlainText());
        setBeginTextState();
    }
    catch (const FileOpeningFailure&)
    {
        QMessageBox::warning(this, userMessages[UserMessages::ErrorTitle],
                userMessages[UserMessages::FileOpeningForSavingErrorMsg]);
        event->ignore();
        return;
    }

    emit closeDocEventOccured(this);
}

void formating(QTextCharFormat fmt, QTextCursor cursor,  Token token)
{
    cursor.setPosition(token.mBegin, QTextCursor::MoveAnchor);
    cursor.setPosition(token.mEnd, QTextCursor::KeepAnchor);
    cursor.setCharFormat(fmt);
}

void CodeEditor::highlighText()
{
    QTextCursor cursor = textCursor();
    for(const auto &i: mTokens)
    {
        switch(i.mType)
        {
        case(State::KW):
            formating(fmtKeyword, cursor, i);
            break;
        case(State::LIT):
            formating(fmtLiteral, cursor, i);
            break;
        case(State::COM):
            formating(fmtComment, cursor, i);
            break;
        default:
            formating(fmtRegular, cursor, i);
            break;
        }
    }
}

void CodeEditor::keyPressEvent(QKeyEvent *e)
{
    Event *pressEvent = EventBuilder::getEvent(e);
    (*pressEvent)(this, e);
}
