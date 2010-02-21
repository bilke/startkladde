#ifndef TEXT_H_
#define TEXT_H_

#include <iostream>

#include <QSet>
#include <QDateTime>
#include <QString>
#include <QColor>

// TODO split

extern const QString whitespace;

bool eintrag_ist_leer (QString eintrag);
bool eintraege_sind_leer (QString eintrag1, QString eintrag2);
bool airfieldEntryCanBeChanged (QString airfield);
void replace_substring (QString &, const QString&, const QString&);
void replace_tabs (QString &s);
QString simplify_club_name (const QString s);
bool club_name_identical (const QString s1, const QString s2);
QString concatenate_comments (const QString& s1, const QString& s2);
QString string_or_none (const QString& text);
bool parameter_matches (char *parameter, char *val1, char *val2);
void trim (QStringList &strings);
QString get_environment (const QString& name);
QString make_string (const QSet<QString> s, const QString &separator=",");
void split_string (QString &string1, QString &string2, QString separator, QString text);

QString firstToUpper (const QString &text);
QString firstToLower (const QString &text);

std::ostream &operator<< (std::ostream &s, const QColor &c);
std::ostream &operator<< (std::ostream &s, const QByteArray &ba);

#endif

