#include "Query.h"

#include <QVariant>
#include <QSqlQuery>
#include <QStringList>
#include <QString>

#include "src/io/AnsiColors.h"

// ******************
// ** Construction **
// ******************

Query::Query ()
{
}

/**
 * Note that we could write Query ("...") without this constructor, but we
 * couldn't write interface.executeQuery ("...") (therefore, we don't need
 * a constructor taking a char* and a QList<QVariant>.
 *
 * @param queryString
 * @return
 */
Query::Query (const char *queryString):
	queryString (queryString)
{
}

Query::Query (const QString &queryString):
	queryString (queryString)
{
}

Query::Query (const QString &queryString, const QList<QVariant> &bindValues):
	queryString (queryString), bindValues (bindValues)
{
}

Query::~Query ()
{
}


// ****************
// ** Properties **
// ****************

bool Query::isEmpty () const
{
	return queryString.isEmpty ();
}

QString Query::toString () const
{
	if (bindValues.isEmpty ())
		return queryString;
	else if (bindValues.size ()==1)
		return QString ("%1 [%2]").arg (queryString, bindValues[0].toString ());
	else
	{
		QString result (queryString);
		result += " [";

		bool first=true;
		foreach (const QVariant &value, bindValues)
		{
			if (!first) result+=", ";
			result+=value.toString ();
			first=false;
		}

		result+="]";
		return result;
	}
}

QString Query::colorizedString () const
{
	QString result;
	AnsiColors c;

	result += c.blue ();

	QStringList words=toString ().split (" ", QString::SkipEmptyParts);

	bool first=true;
	foreach (const QString &word, words)
	{
		if (!first) result+=" ";
		first=false;

		if (word.toUpper ()==word)
			result += c.bold ().cyan();
		else
			result += c.noBold ().blue();

		result+=word;
	}

//		return QString ("%1%2%3")
//			.arg (c.bold().blue())
//			.arg (toString ())
//			.arg (c.reset ())
		;

	result += c.reset ();

	return result;
}



// ****************
// ** Generation **
// ****************

// TODO rename selectDistinctValues
Query Query::selectDistinctColumns (const QString &table, const QString &column, bool excludeEmpty)
{
	// "select distinct column from table"
	QString queryString=QString ("SELECT DISTINCT %1 FROM %2").arg (column, table);

	// ..." where column!=''"
	if (excludeEmpty) queryString+=QString (" WHERE %1!=''").arg (column);

	return Query (queryString);
}

Query Query::selectDistinctColumns (const QStringList &tables, const QStringList &columns, bool excludeEmpty)
{
	QStringList parts;

	// TODO join queries (with potential bind values) instead of strings
	foreach (const QString &table, tables)
		foreach (const QString &column, columns)
			parts << selectDistinctColumns (table, column, excludeEmpty).queryString;

	return Query (parts.join (" UNION "));
}

Query Query::selectDistinctColumns (const QStringList &tables, const QString &column, bool excludeEmpty)
{
	return selectDistinctColumns (tables, QStringList (column), excludeEmpty);
}

Query Query::selectDistinctColumns (const QString &table, const QStringList &columns, bool excludeEmpty)
{
	return selectDistinctColumns (QStringList (table), columns, excludeEmpty);
}

Query Query::select (const QString &table, const QString &columns)
{
	return Query ("SELECT %1 FROM %2")
		.arg (columns, table);
}

Query Query::count (const QString &table)
{
	// TODO default case of count(table, condition)
	return Query ("SELECT COUNT(*) FROM %1")
		.arg (table);
}

Query Query::count (const QString &table, const Query &condition)
{
	if (condition.isEmpty ())
		return Query ("SELECT COUNT(*) FROM %1").arg (table);
	else
		return Query ("SELECT COUNT(*) FROM %1 WHERE ").arg (table)+condition;
}


// ******************
// ** Manipulation **
// ******************

Query Query::operator+ (const Query &other) const
{
	return Query (queryString+other.queryString, bindValues+other.bindValues);
}

Query &Query::operator+= (const Query &other)
{
	queryString += other.queryString;
	bindValues += other.bindValues;
	return *this;
}

Query Query::operator+ (const QString &string) const
{
	return Query (queryString+string, bindValues);
}

Query &Query::operator+= (const QString &string)
{
	queryString+=string;
	return *this;
}

Query &Query::condition (const Query &condition)
{
	if (!condition.isEmpty ())
	{
		queryString+=" WHERE ";
		(*this)+=condition;
	}

	return *this;
}


// *********
// ** Arg **
// *********

Query &Query::arg (const QString &a1, const QString &a2)
	{ queryString=queryString.arg (a1, a2); return *this; }
Query &Query::arg (const QString &a1, const QString &a2, const QString &a3)
	{ queryString=queryString.arg (a1, a2, a3); return *this; }
Query &Query::arg (const QString &a1, const QString &a2, const QString &a3, const QString &a4)
	{ queryString=queryString.arg (a1, a2, a3, a4); return *this; }
Query &Query::arg (const QString &a1, const QString &a2, const QString &a3, const QString &a4, const QString &a5)
	{ queryString=queryString.arg (a1, a2, a3, a4, a5); return *this; }
Query &Query::arg (const QString &a1, const QString &a2, const QString &a3, const QString &a4, const QString &a5, const QString &a6)
	{ queryString=queryString.arg (a1, a2, a3, a4, a5, a6); return *this; }
Query &Query::arg (const QString &a1, const QString &a2, const QString &a3, const QString &a4, const QString &a5, const QString &a6, const QString &a7)
	{ queryString=queryString.arg (a1, a2, a3, a4, a5, a6, a7); return *this; }
Query &Query::arg (const QString &a1, const QString &a2, const QString &a3, const QString &a4, const QString &a5, const QString &a6, const QString &a7, const QString &a8)
	{ queryString=queryString.arg (a1, a2, a3, a4, a5, a6, a7, a8); return *this; }
Query &Query::arg (const QString &a1, const QString &a2, const QString &a3, const QString &a4, const QString &a5, const QString &a6, const QString &a7, const QString &a8, const QString &a9)
	{ queryString=queryString.arg (a1, a2, a3, a4, a5, a6, a7, a8, a9); return *this; }

Query &Query::arg (const QString &a, int fieldWidth, const QChar &fillChar)
	{ queryString=queryString.arg (a, fieldWidth, fillChar); return *this; }
Query &Query::arg (QChar      a, int fieldWidth, const QChar &fillChar)
	{ queryString=queryString.arg (a, fieldWidth, fillChar); return *this; }
Query &Query::arg (char       a, int fieldWidth, const QChar &fillChar)
	{ queryString=queryString.arg (a, fieldWidth, fillChar); return *this; }
Query &Query::arg (int        a, int fieldWidth, int base, const QChar &fillChar)
	{ queryString=queryString.arg (a, fieldWidth, base, fillChar); return *this; }
Query &Query::arg (uint       a, int fieldWidth, int base, const QChar &fillChar)
	{ queryString=queryString.arg (a, fieldWidth, base, fillChar); return *this; }
Query &Query::arg (long       a, int fieldWidth, int base, const QChar &fillChar)
	{ queryString=queryString.arg (a, fieldWidth, base, fillChar); return *this; }
Query &Query::arg (ulong      a, int fieldWidth, int base, const QChar &fillChar)
	{ queryString=queryString.arg (a, fieldWidth, base, fillChar); return *this; }
Query &Query::arg (qlonglong  a, int fieldWidth, int base, const QChar &fillChar)
	{ queryString=queryString.arg (a, fieldWidth, base, fillChar); return *this; }
Query &Query::arg (qulonglong a, int fieldWidth, int base, const QChar &fillChar)
	{ queryString=queryString.arg (a, fieldWidth, base, fillChar); return *this; }
Query &Query::arg (short      a, int fieldWidth, int base, const QChar &fillChar)
	{ queryString=queryString.arg (a, fieldWidth, base, fillChar); return *this; }
Query &Query::arg (ushort     a, int fieldWidth, int base, const QChar &fillChar)
	{ queryString=queryString.arg (a, fieldWidth, base, fillChar); return *this; }
Query &Query::arg (double     a, int fieldWidth, char format, int precision, const QChar &fillChar)
	{ queryString=queryString.arg (a, fieldWidth, format, precision, fillChar); return *this; }


// **********
// ** Bind **
// **********

Query &Query::bind (const QVariant &v)
{
	bindValues.append (v);
	return *this;
}


// *************************
// ** QSqlQuery interface **
// *************************

bool Query::prepare (QSqlQuery &query) const
{
	return query.prepare (queryString);
}


/**
 * Adds the bind values to the given QSqlQuery
 *
 * Note that due to QSql limitations, this method may only be called from
 * the thread that created the QSqlQuery.
 *
 * @param query the query the bind values will be added to
 */
void Query::bindTo (QSqlQuery &query) const
{
	foreach (const QVariant &value, bindValues)
		query.addBindValue (value);
}