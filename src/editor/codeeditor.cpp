#include "linenumberarea.h"
#include "usermessages.h"
#include "eventbuilder.h"
#include "filemanager.h"
#include "codeeditor.h"
#include "keywords.h"
#include "utils.h"
#include<QtGui>
#include<QTextCursor>
#include<QPainter>
#include<QTextCharFormat>
#include<QFontDatabase>
#include<QScrollBar>
#include<QLineEdit>
#include<QMessageBox>
#include<iostream>
#include<QLabel>
#include"classgenerator.h"
#include"methodspartsdefinitiongetters.h"
#include"classgenerationliterals.h"
#include<QMenu>
#include <QVector>

CodeEditor::CodeEditor(QWidget *parent, const QString &fileName) : QPlainTextEdit(parent)
{
    mFileName = fileName;
    setLineWrapMode(QPlainTextEdit::NoWrap);// don't move cursor to the next line where it's out of visible scope
    this->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
    this->setTabStopDistance(TAB_SPACE * fontMetrics().width(QLatin1Char('0')));//set tab distance
    mCurrentZoom = 100;//in persents    
    mLinesCount = 1;
    mCode = document()->toPlainText();
    mCodeSize = 1;
    mHighlightingStart = 0;
    mStyle = mConfigParam.getIdeType();

    //read settings
    QString analizerFontSize = settings.value("editorFontSize").toString();
    QString analizerFontName = settings.value("editorFontName").toString();
    QString analizerStyle = settings.value("style").toString();
    mConfigParam.setConfigParams(analizerFontName,analizerFontSize,analizerStyle);

    //create objects connected to codeEditor
    mLineNumberArea = new LineNumberArea(this);
    mTimer = new QTimer;
    mLcpp = new LexerCPP();
    mTimer = new QTimer;
    QVector<Token> firstLine;
    mTokensList.append(firstLine);
    mChangeManager = new ChangeManager(this->toPlainText().toUtf8().constData());
    //comment button
    mAddCommentButton = new AddCommentButton(this);
    mAddCommentButton->setText("+");
    mAddCommentButton->setVisible(false);
    mCurrentCommentLable = new QLabel(this);
    setMouseTracking(true);

    //widget that shows the comment to each line
    mCommentWidget = new CommentWidget;
    mCommentWidget->setVisible(false);

    commentGetter = new CommentDb;

    mStartComments = commentGetter->getAllCommentsFromFile(getFileName());
    readAllCommentsFromDB(mStartComments);


    //This signal is emitted when the text document needs an update of the specified rect.
    //If the text is scrolled, rect will cover the entire viewport area.
    //If the text is scrolled vertically, dy carries the amount of pixels the viewport was scrolled.

    connect(this,                         &CodeEditor::linesWasSwapped,                    this, &CodeEditor::handleLinesSwap);
    connect(this,                         &CodeEditor::textChanged,                        this, &CodeEditor::textChangedInTheOneLine);
    connect(this,                         &CodeEditor::textChangedInLine,                  this, &CodeEditor::handleLineChange);
    connect(this,                         &CodeEditor::runHighlighter,                     this, &CodeEditor::highlightText);
    connect(this,                         &QPlainTextEdit::updateRequest,                  this, &CodeEditor::updateLineNumberArea);
    connect(mTimer,                       &QTimer::timeout,                                this, &CodeEditor::saveStateInTheHistory);
    connect(mAddCommentButton,            &AddCommentButton::addCommentButtonPressed,      this, &CodeEditor::showCommentTextEdit);
    connect(mCommentWidget->getEditTab(), &AddCommentTextEdit::emptyCommentWasSent,        this, &CodeEditor::emptyCommentWasAdded);
    connect(mCommentWidget->getEditTab(), &AddCommentTextEdit::notEmptyCommentWasSent,     this, &CodeEditor::notEmptyCommentWasAdded);
    connect(mCommentWidget->getEditTab(), &AddCommentTextEdit::commentWasDeleted,          this, &CodeEditor::deleteComment);
    connect(this,                         &CodeEditor::linesCountUpdated,                  this, &CodeEditor::changeCommentButtonsState);

    mTimer->start(CHANGE_SAVE_TIME);//save text by this time
    mLinesCountCurrent = 1;
    mLinesCountPrev = 1;

    // start typing from correct position (in the first line it doesn't consider weight of lineCounter)
    //that's why we need to set this position
    updateLineNumberAreaWidth();

    //fonts and colors configurations
    mFont.setPointSize(mConfigParam.mFontSize);
    mFont.setFamily(mConfigParam.mFontStyle);
    mFont.setBold(false);
    mFont.setItalic(false);
    this->setFont(mFont);

    //set text highlighting color
    setTextColors();

    //completer
    QStringList keywordsStringList;
    for (auto &i :cKeywords)
    {
        keywordsStringList.append(i);
    }

    mCompleter = new AutoCodeCompleter(keywordsStringList, this);
    mCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    mCompleter->setWidget(this);
}

CodeEditor::~CodeEditor()
{
    commentGetter->deleteCommentsFromDb(getFileName());
    commentGetter->addCommentsToDb(getAllCommentsToDB());
}

void CodeEditor::setTextColors()
{
    fmtUndefined.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    fmtUndefined.setUnderlineColor(mConfigParam.textColors.mWaveUnderlineColor);
    fmtLiteral.setForeground(mConfigParam.textColors.mStringsColor);
    fmtKeyword.setForeground(mConfigParam.textColors.mBasicLiteralsColor);
    fmtComment.setForeground(mConfigParam.textColors.mCommentColor);
    fmtRegular.setForeground(mConfigParam.textColors.mCodeTextColor);
}

void CodeEditor::setIdeType(const QString &ideType)
{
    mConfigParam.setIdeType(ideType);
    setTextColors();
}

void CodeEditor::writeDefinitionToSource()
{
    if (!isFileWithExtension(getFileName(), "h"))//if current file is not header, we can't define fucntion
    {
        return;
    }
    QTextCursor curs = this->textCursor();
    //get className::methodName or methodName if it's fucntion
    auto definePattern = getMethodDefinitionPattern(getTextByCursor(curs));
    auto className = getClassNameForMethodDefinition(curs);
    QString definitonTest = createMethodDefinitionBones(definePattern.mFunctionDataType,
                                                        className,
                                                        definePattern.mFucntionName,
                                                        definePattern.mFunctionParametrs);
    FileManager fileManager;
    if (fileManager.sourceFileByTheSameNameExists(getFileName()))
    {
        auto sourceFileName = removeExtension(getFileName(), headerExtension.length())//create source file path
                .append(sourceExtension);

        auto sourceFileText = fileManager.readFromFile(sourceFileName);//get content of this source file
        if (!definitionExists(sourceFileText, this->textCursor()))//check if definition already exists
        {
            auto sourceDocument =  getOpenedDocument(sourceFileName);
            if (!sourceDocument)//if file is not opened
            {
                //write method definition to source file and emit signal in order to open this file
                fileManager.writeToFile(sourceFileName, sourceFileText + "\n" + definitonTest);
                emit openDocument(sourceFileName);
            }
            else
            {
                 sourceDocument->setPlainText(sourceDocument->toPlainText() + "\n" + definitonTest);
            }
            QMessageBox::information(this, successDefinCreateTitle, successDefinCreateMessage);
        }
        else
        {
            QMessageBox::information(this, definitionExistsTitle, definitionExistsMessage);
        }
    }
}

QVector<Comment> CodeEditor::getStartComments() const
{
    return mStartComments;
}

CommentDb *CodeEditor::getCommentGetter() const
{
    return commentGetter;
}

void CodeEditor::contextMenuEvent(QContextMenuEvent *event)
{
    std::shared_ptr<QMenu>menu(this->createStandardContextMenu());
    QMenu *refactorItem = menu->addMenu("Refactor");

    QAction *addDefinitionAction = new QAction("Add definition", refactorItem);
    refactorItem->addAction(addDefinitionAction);

    addDefinitionAction->setEnabled(isValidMethodInitialization(this->textCursor()));
    connect(addDefinitionAction, &QAction::triggered, this, &CodeEditor::writeDefinitionToSource);
    menu->exec(event->globalPos());
}

void CodeEditor::setFontSize(const QString &fontSize)
{
    mConfigParam.setFontSize(fontSize);
    mFont.setPointSize(mConfigParam.mFontSize);
    this->setFont(mFont);
}

void CodeEditor::setFontStyle(const QString &fontStyle)
{    
    mConfigParam.setFontStyle(fontStyle);
    mFont.setFamily(mConfigParam.mFontStyle);
    this->setFont(mFont);
}

ConfigParams CodeEditor::getConfigParam()
{
    return mConfigParam;
}

void CodeEditor::setConfigParam(const ConfigParams &configParam)
{
    mConfigParam = configParam;
}

void CodeEditor::handleLinesSwap(const int firstLine, const int secondLine)
{
    QVector<Token> tmp = mTokensList[firstLine];
    mTokensList[firstLine] = mTokensList[secondLine];
    mTokensList[secondLine] = tmp;
}

void CodeEditor::addToIdentifiersList(QStringList &identifiersName, int line)
{
    for (auto j = 0; j < mTokensList[line].size(); ++j)
    {
        if(mTokensList[line][j].mType == State::ID)
        {
            identifiersName << mTokensList[line][j].mName;
        }
    }
}

void CodeEditor::handleLinesAddition(int changeStart, int lastLineWithChange, int lineDifference)
{
    QString changedCode;

    if (lineDifference > 0)
    {
        changeStart = lastLineWithChange - lineDifference;
        mTokensList.removeAt(changeStart);
    }

    mHighlightingStart = changeStart > 0 ? changeStart - 1 : changeStart;
    for (auto i = changeStart; i <= lastLineWithChange; ++i)
    {
        changedCode = document()->findBlockByLineNumber(i).text();
        mLcpp->lexicalAnalysis(changedCode);
        if (lineDifference)
        {
            mTokensList.insert(i, mLcpp->getTokens());
        }
        else
        {
            mTokensList[i] = mLcpp->getTokens();
        }
    }
}

void CodeEditor::handleLinesDelition(int lastLineWithChange, int lineDifference)
{
    QString changedCode;
    lineDifference = -lineDifference;
    changedCode = document()->findBlockByLineNumber(lastLineWithChange).text();

    mLcpp->lexicalAnalysis(changedCode);
    mHighlightingStart = lastLineWithChange;
    mTokensList[lastLineWithChange] = mLcpp->getTokens();

    for (auto i = lastLineWithChange + 1; i < lastLineWithChange + lineDifference + 1; ++i)
    {
        mTokensList.removeAt(lastLineWithChange + 1);
    }
}

void CodeEditor::getNamesOfIdentifiers()
{
    mIdentifiersNameList.clear();
    for (auto i = 0; i < mIdentifiersList.size(); ++i)
    {
        for (auto j = 0; j < mIdentifiersList[i].size(); ++j)
        {
            mIdentifiersNameList << mIdentifiersList[i][j];
        }
    }
}

void CodeEditor::handleLineChange(int lastLineWithChange)
{
    mLcpp->clear();

    int changeStart = lastLineWithChange;

    int currentLinesCount = document()->lineCount();
    int lineDifference = currentLinesCount - mLinesCount;
    mLinesCount = currentLinesCount;

    if (!mLcpp->isLexerWasRunning())
    {
        lastLineWithChange += lineDifference;
    }

    if (lineDifference >= 0)
    {
        handleLinesAddition(changeStart, lastLineWithChange, lineDifference);
    }
    else
    {
        handleLinesDelition(lastLineWithChange, lineDifference);
    }

    emit runHighlighter();
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

std::pair<const QString&, const QString&> CodeEditor::getChangedFileInfo()
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
    return mBeginTextState != QCryptographicHash::hash(this->toPlainText().toLatin1(),
                                                       QCryptographicHash::Sha256);
}

void CodeEditor::setBeginTextState()
{
    // when document is opened control sum is generated in order to have an opportunity
    // to check whether document was modified
    mBeginTextState = QCryptographicHash::hash(this->toPlainText().toLatin1(),
                                               QCryptographicHash::Sha256);
}

const QByteArray& CodeEditor::getBeginTextState() const
{
    return  mBeginTextState;
}

void CodeEditor::setTextState(const QByteArray &beginTextState)
{
    mBeginTextState = beginTextState;
}

void CodeEditor::updateLineNumberAreaWidth()
{
    // reset start position for typing (according new linecounter position)
    setViewportMargins(getLineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, const int dy)// rectangle of current block and Y-Axis changing
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
    {
        updateLineNumberAreaWidth();
    }
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();//whole area inside widget's margins
    mLineNumberArea->setGeometry(QRect(0, 0, getLineNumberAreaWidth(), cr.height()));//set the same height as codeEditor for lineCouter
}

void CodeEditor::specialAreasRepaintEvent(QPaintEvent *event)
{
    QPainter painter(mLineNumberArea);
    painter.fillRect(event->rect(), mConfigParam.textColors.mLineCounterAreaColor);

    QTextBlock block = firstVisibleBlock();//area of first numeration block from linecounter
    int blockNumber = block.blockNumber();//get line number (start from 0)
    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());//top of currentblock 0
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());//bottom of current block                 -

    while (block.isValid())//we have blocks (have lines numbers)
    {
        QString number = QString::number(blockNumber + 1);
        painter.setPen(mConfigParam.textColors.mCodeTextColor);
        painter.drawText(0, top, mLineNumberArea->width(), fontMetrics().height(),//draw line count
                         Qt::AlignCenter, number);
        block = block.next();
        int temp = top;//save current top position
        top = bottom;//refresh the top bottom (next block top == this block bottom)
        bottom += bottom - temp;// bottom - temp = dy which is block height
        ++blockNumber;
    }
    repaintButtonsArea(bottom, top, blockNumber);
}

void CodeEditor::repaintButtonsArea(const int bottom, const int top, const int blockNumber)
{
    int addedHight = this->verticalScrollBar()->sliderPosition() ? 0 : TOP_UNUSED_PIXELS_HEIGHT;
    int height = bottom - top;
    for (auto &i : mCommentsVector)
    {
        i->setGeometry(this->width() - this->verticalScrollBar()->width() - height,
                       (i->getCurrentLine() - this->verticalScrollBar()->sliderPosition() - 1) * height + addedHight,
                       bottom - top,
                       bottom - top);
        i->setVisible(true);
    }

    if (mLinesCountCurrent != blockNumber)//lines count changed
    {
        mLinesCountPrev = mLinesCountCurrent;
        mLinesCountCurrent = blockNumber;
        emit linesCountUpdated();
    }
}

void CodeEditor::saveStateInTheHistory()
{
    std::string newFileState = this->toPlainText().toUtf8().constData();
    mChangeManager->writeChange(newFileState);
}

void CodeEditor::zoom(const int val)
{
    if (val > 0)
    {
        this->zoomIn(val);
    }
    else
    {
        this->zoomOut(-val);
    }
    mCurrentZoom+=val;
}

void CodeEditor::setZoom(const int zoomVal)
{
    zoom(zoomVal - mCurrentZoom);
}

void CodeEditor::textChangedInTheOneLine()
{
    if (mCode != document()->toPlainText() || mStyle != mConfigParam.getIdeType())
    {
        mStyle = mConfigParam.getIdeType();
        mCode = document()->toPlainText();
        emit textChangedInLine(this->textCursor().blockNumber());
    }
}

AddCommentButton* CodeEditor::getCommentButtonByIndex(const int line)
{
    auto rCommentButton = std::find_if(mCommentsVector.begin(), mCommentsVector.end(),
                      [=](AddCommentButton *button) {return button->getCurrentLine() == line; });

    return rCommentButton == mCommentsVector.end() ? nullptr : *rCommentButton;
}

void CodeEditor::setNewAddedButtonSettings(AddCommentButton *commentButton)
{
    if (mStartComments.size())//if this is first comments set from DB
    {
        //in this case we already have comment in the button field, so we should read the data from it
        mCommentWidget->getEditTab()->setText(commentButton->getCommentString());
    }
    else
    {
        //if we add comment from commentwidget ui
        commentButton->setCommentString(mCommentWidget->getEditTab()->getText());
    }
    mCommentWidget->setViewText(1);//set text to view. because it doens't set automiticaly when we do it to the edit
    commentButton->setToolTip(mCommentWidget->getViewTab()->getText());
    mCommentWidget->setVisible(false);
}

CodeEditor* CodeEditor::getOpenedDocument(const QString &fileName)
{
    auto allWidgets = QApplication::allWidgets();
    for (auto widget: allWidgets)
    {
        auto doc = qobject_cast<CodeEditor*>(widget);

        if (doc && (doc->getFileName() == fileName))
        {
            return doc;
        }
    }
    return  nullptr;
}

void CodeEditor::readAllCommentsFromDB(QVector<Comment> comments)
{
    for(auto &i : comments)//go through all vector's elements from the DB
    {
        addButton(i.mLine, i.mText, i.mUser);
    }
}

QVector<Comment> CodeEditor::getAllCommentsToDB()
{
    QVector<Comment> comments;
    for (auto &i : mCommentsVector)
    {
        Comment comment;
        comment.mFile = getFileName();
        comment.mLine = i->getCurrentLine();
        comment.mText = i->getCommentString();
        comment.mUser = i->getUser();
        comments.push_back(comment);
    }
    return comments;
}

void CodeEditor::showCommentTextEdit(int line)
{
    mCommentWidget->setPosition(this, mAddCommentButton);
    mCommentWidget->setVisible(true);
    mCommentWidget->setCommentButtonGeometry(mAddCommentButton->geometry());
    mCommentWidget->setCommentLine(line);

    QString userName;
    auto commentButton = getCommentButtonByIndex(line);
    if (commentButton)//if comment button by passed in the parametr line which exists
    {
        mCommentWidget->getEditTab()->setText(commentButton->getCommentString());//set text from this button text
        userName = commentButton->getUser();
    }
    else
    {
        mCommentWidget->getEditTab()->setText("");
        userName = settings.value("UserName").toString();
    }
    mCommentWidget->setWindowTitle("Comment to " + QString::number(line) + " line by " + userName);
}

void CodeEditor::emptyCommentWasAdded()
{
    mCommentWidget->setVisible(false);
    auto commentButton = getCommentButtonByIndex(mCommentWidget->getCommentLine());
    if (commentButton)
    {
        removeButtomByValue(mCommentsVector, commentButton);
    }
}

void CodeEditor::notEmptyCommentWasAdded()
{
    if (commentButtonExists(mCommentWidget->getCommentLine()))//if button was existing, just reset text
    {
        auto commentButon = getCommentButtonByIndex(mCommentWidget->getCommentLine());
        commentButon->setUser(settings.value("UserName").toString());
        setNewAddedButtonSettings(commentButon);
    }
    else
    {
        addButton(mCommentWidget->getCommentLine(),
                  mCommentWidget->getEditTab()->getText(),
                  settings.value("UserName").toString());// create new button
    }
}

void CodeEditor::deleteComment()
{
    auto commentButon = getCommentButtonByIndex(mCommentWidget->getCommentLine());
    if (commentButon)
    {
        removeButtomByValue(mCommentsVector, commentButon);
    }
    mCommentWidget->setVisible(false);
}

void CodeEditor::changeCommentButtonsState()
{
    if (mStartComments.size())//if it's first time set from db
    {
        mStartComments.clear();//that means that we have read all data from the DB and don't need them in this vector anymore
        return;
    }

    int diff = mLinesCountCurrent - mLinesCountPrev;//number that keeps the difference between previous and current lines count
    int cursorLine = this->textCursor().blockNumber() + 1;//get the line where cursor is

    //get the lines of start and end changing
    int startLine = diff > 0 ? cursorLine - diff : cursorLine;
    int endLine = diff > 0 ? cursorLine : cursorLine - diff;

    removeButtons(mCommentsVector, cursorLine, startLine, endLine, diff);//remove all buttons if we deleted their lines

    rewriteButtonsLines(mCommentsVector,diff,startLine);//move position of buttons that need to be moved
}

LastRemoveKey CodeEditor::getLastRemomeKey() const
{
    return lastRemomeKey;
}

void CodeEditor::setLastRemomeKey(const LastRemoveKey &value)
{
    lastRemomeKey = value;
}

void CodeEditor::rewriteButtonsLines(QVector<AddCommentButton*> &commentV, const int diff, const int startLine)
{
    //move buttons' lines according to diff
    for (auto &i : commentV)
    {
        if (i->getCurrentLine() == startLine)
        {
            auto cursor = this->textCursor();
            cursor.movePosition(QTextCursor::PreviousCharacter);
            //check if cursor was in the start of comment line.
            //that is the only one situation when we consider this line as the line where we should move comment button
            if (cursor.atBlockStart())
            {
                setAnotherButtonLine(i, diff);
            }
        }
        else if (i->getCurrentLine() > startLine)
        {
            setAnotherButtonLine(i, diff);
        }
    }
}

void CodeEditor::setAnotherButtonLine(AddCommentButton *comment, const int diff)
{
    comment->setCurrentLine(comment->getCurrentLine() + diff);
}

bool CodeEditor::isInRangeIncludBoth(const int val, const int leftMargin, const int rightMargin)
{
    return val >= leftMargin && val <= rightMargin;
}

bool CodeEditor::isInRangeIncludLast(const int val, const int leftMargin, const int rightMargin)
{
    return val > leftMargin && val <= rightMargin;
}

void CodeEditor::addButton(const int line, const QString &comment, const QString &userName)
{
    AddCommentButton *commentButtonNew = new AddCommentButton(this);
    commentButtonNew->setGeometry(mCommentWidget->getCommentButtonGeometry());
    commentButtonNew->setCurrentLine(line);
    commentButtonNew->setCommentString(comment);
    commentButtonNew->setUser(userName);

    commentButtonNew->setStyleSheet("background-color: #18CD3C");
    commentButtonNew->setText("✔");
    commentButtonNew->setVisible(true);

    setNewAddedButtonSettings(commentButtonNew);

    mCommentsVector.push_back(commentButtonNew);
    connect(mCommentsVector.back(), &AddCommentButton::addCommentButtonPressed, this, &CodeEditor::showCommentTextEdit);
}

void CodeEditor::removeButtonByIndex(QVector<AddCommentButton*> &commentV, const int index)
{
    commentV[index]->setVisible(false);
    commentV.erase(commentV.begin() + index);
}

void CodeEditor::removeButtomByValue(QVector<AddCommentButton*> &commentV, AddCommentButton *commentButton)
{
    commentButton->setVisible(false);
    mCommentsVector.erase(std::remove(commentV.begin(), commentV.end(), commentButton), commentV.end());
}

bool CodeEditor::commentButtonExists(int line)
{
    auto rCommentButton = std::find_if(mCommentsVector.begin(), mCommentsVector.end(),
                      [=](AddCommentButton *button) {return button->getCurrentLine() == line; });

    return !(rCommentButton == mCommentsVector.end());

}

void CodeEditor::removeButtons(QVector<AddCommentButton*> &commentV, const int cursorLine,
                               const int startLine, const int endLine, const int diff)
{
    if (diff > 0)//we shouldn't remove button if the wasn't any removing
    {
        return;
    }

    for (int i = 0; i < commentV.size(); i++)
    {
        //if the last pressed remove button (delete or backspace) was delete we should check one more condition
        //because cursor position hasn't changed
        if (lastRemomeKey == LastRemoveKey::DEL)
        {
            //this statment happens when we're staying above the line where comment is and pressed Delete.
            //we havn't moved the cursor position, but diff became -1 therefore we can't check this as usually
            //using only startLine position
            if (cursorLine == startLine
                && cursorLine != commentV[i]->getCurrentLine())
            {
                continue;
            }
            if (isInRangeIncludBoth(commentV[i]->getCurrentLine(), startLine, endLine))
            {
                removeButtonByIndex(commentV, i);//otherwise delete this button (we delete whole line where comment button was)
            }
        }
        else
        {
            if (isInRangeIncludLast(commentV[i]->getCurrentLine(), startLine, endLine))
            {
                removeButtonByIndex(commentV, i);//the same deleting here
            }
        }
    }
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

            if (currLine <= mLinesCountCurrent)// check if the line exists
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
        emit closeDocEventOccured(this);
        event->accept();
        return;
    }
    QMessageBox::StandardButton reply = QMessageBox::question
            (this,
             userMessages[UserMessages::PromptSaveTitle],
            userMessages[UserMessages::SaveQuestion]
            + getFileName() + "?",
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
        emit closeDocEventOccured(this);
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

void formating(QTextCharFormat fmt, QTextCursor &cursor, Token token, int startingPosition)
{
    cursor.setPosition(startingPosition + token.mBegin, QTextCursor::MoveAnchor);
    cursor.setPosition(startingPosition + token.mEnd, QTextCursor::KeepAnchor);
    cursor.setCharFormat(fmt);
}

void CodeEditor::highlightText()
{
    QTextBlock block = document()->findBlockByLineNumber(mHighlightingStart);
    QTextCursor cursor(block);
    int start = cursor.blockNumber();
    int startingPosition = cursor.position();

    // Set cursor to end of visible area
    QPoint bottom_right(this->viewport()->width() - 1, this->viewport()->height() - 1);
    cursor = this->cursorForPosition(bottom_right);
    int lastVisibleLine = cursor.blockNumber();

    // Highlight visible area
    for (auto i = start; i <= lastVisibleLine; ++i)
    {
        if(i < mTokensList.size())
        {
            for(auto j = 0; j < mTokensList[i].size(); ++j)
            {
                switch(mTokensList[i][j].mType)
                {
                case(State::KW):
                    formating(fmtKeyword, cursor, mTokensList[i][j], startingPosition);
                    break;
                case(State::LIT):
                    formating(fmtLiteral, cursor, mTokensList[i][j], startingPosition);
                    break;
                case(State::COM):
                    formating(fmtComment, cursor, mTokensList[i][j], startingPosition);
                    break;
                case(State::UNDEF):
                    formating(fmtUndefined, cursor, mTokensList[i][j], startingPosition);
                    break;
                default:
                    formating(fmtRegular, cursor, mTokensList[i][j], startingPosition);
                    break;
                }
            }
        }

        // Move cursor to the next line
        cursor.setPosition(startingPosition);
        cursor.movePosition(QTextCursor::EndOfLine);
        startingPosition = cursor.position() + 1;
    }
}

void CodeEditor::keyPressEvent(QKeyEvent *e)
{
    Event *pressEvent = EventBuilder::getEvent(e);
    (*pressEvent)(this, e);
}
