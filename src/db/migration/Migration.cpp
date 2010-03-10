/*
 * TODO
 *   - migration descriptions (for display in progress dialog instead of name)
 *   - method that returns whether a migration leads to data loss
 *
 */

#include "Migration.h"

#include "src/db/Database.h"
#include "src/db/result/Result.h"

QString Migration::dataTypeBinary    () { return Interface::dataTypeBinary    (); }
QString Migration::dataTypeBoolean   () { return Interface::dataTypeBoolean   (); }
QString Migration::dataTypeDate      () { return Interface::dataTypeDate      (); }
QString Migration::dataTypeDatetime  () { return Interface::dataTypeDatetime  (); }
QString Migration::dataTypeDecimal   () { return Interface::dataTypeDecimal   (); }
QString Migration::dataTypeFloat     () { return Interface::dataTypeFloat     (); }
QString Migration::dataTypeInteger   () { return Interface::dataTypeInteger   (); }
QString Migration::dataTypeString    () { return Interface::dataTypeString    (); }
QString Migration::dataTypeText      () { return Interface::dataTypeText      (); }
QString Migration::dataTypeTime      () { return Interface::dataTypeTime      (); }
QString Migration::dataTypeTimestamp () { return Interface::dataTypeTimestamp (); }
QString Migration::dataTypeCharacter () { return Interface::dataTypeCharacter (); }
QString Migration::dataTypeId        () { return Interface::dataTypeId        (); }

Migration::Migration (Interface &interface):
	interface (interface)
{

}

Migration::~Migration ()
{
}

/** Forwards to database#transaction */
void Migration::transaction ()
{
	interface.transaction ();
}

/** Forwards to database#commit */
void Migration::commit ()
{
	interface.commit ();
}

/** Forwards to database#rollback */
void Migration::rollback ()
{
	interface.rollback ();
}

/** Forwards to database#executeQuery */
void Migration::executeQuery (const Query &query)
{
	interface.executeQuery (query);
}

/** Forwards to database#executeQueryResult */
QSharedPointer<Result> Migration::executeQueryResult (const Query &query, bool forwardOnly)
{
	return interface.executeQueryResult (query, forwardOnly);
}

/** Forwards to database#createTable */
void Migration::createTable (const QString &name, bool skipIfExists)
{
	interface.createTable (name, skipIfExists);
}

/** Forwards to database#createTableLike */
void Migration::createTableLike (const QString &like, const QString &name, bool skipIfExists)
{
	interface.createTableLike (like, name, skipIfExists);
}

/** Forwards to database#dropTable */
void Migration::dropTable (const QString &name)
{
	interface.dropTable (name);
}

/** Forwards to database#renameTable */
void Migration::renameTable (const QString &oldName, const QString &newName)
{
	interface.renameTable (oldName, newName);
}


/** Forwards to database#addColumn */
void Migration::addColumn (const QString &table, const QString &name, const QString &type, const QString &extraSpecification, bool skipIfExists)
{
	interface.addColumn (table, name, type, extraSpecification, skipIfExists);
}

/** Forwards to database#changeColumnType */
void Migration::changeColumnType (const QString &table, const QString &name, const QString &type, const QString &extraSpecification)
{
	interface.changeColumnType (table, name, type, extraSpecification);
}

/** Forwards to database#dropColumn */
void Migration::dropColumn (const QString &table, const QString &name, bool skipIfNotExists)
{
	interface.dropColumn (table, name, skipIfNotExists);
}

/** Forwards to database#renameColumn */
void Migration::renameColumn (const QString &table, const QString &oldName, const QString &newName, const QString &type, const QString &extraSpecification)
{
	interface.renameColumn (table, oldName, newName, type, extraSpecification);
}


/** Forwards to database#updateColumnValues */
void Migration::updateColumnValues (const QString &tableName, const QString &columnName, const QVariant &oldValue, const QVariant &newValue)
{
	interface.updateColumnValues (tableName, columnName, oldValue, newValue);
}
