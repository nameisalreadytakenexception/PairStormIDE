#ifndef METHODSPARTSDEFINITIONGETTERS_H
#define METHODSPARTSDEFINITIONGETTERS_H

#include<QString>
#include<QTextCursor>

struct MethodDefinitionPattern
{
    QString functionDataType;
    QString fucntionName;
    QString functionParametrs;
};

QString removeExtension(QString str, const int extensionCharacterCount);
QString createMethodDefinitionBones(const QString &dataType,  const QString &className,
                                    const QString &methodName,const QString &methodParams);
QString createFilePath(const QString &rootPath, const QString &file);

QString getTextByCursor(QTextCursor cursor);
QString getClassNameForMethodDefinition(QTextCursor cursor);
MethodDefinitionPattern getMethodDefinitionPattern(const QString &difinition);

bool isValidMethodInitialization(QTextCursor cursor);
bool definitionExists(const QString documentText, QTextCursor cursor);
bool isFileWithExtension(const QString &fileName, const QString &extenion);

#endif // METHODSPARTSDEFINITIONGETTERS_H
