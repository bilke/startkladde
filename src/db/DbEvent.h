#ifndef DBEVENT_H_
#define DBEVENT_H_

#include <QString>

#include "src/db/dbId.h"

class DbEvent
{
	public:
		// TODO replace with templates
		enum Table { tableNone, tableAll, tablePeople, tableFlights, tableLaunchMethods, tablePlanes };
		enum Type { typeNone, typeAdd, typeDelete, typeChange, typeRefresh };

		DbEvent ();
		DbEvent (Type type, Table table, dbId);

		QString toString ();
		static QString typeString (Type type);
		static QString tableString (Table table);

		// Hack for until we have replaced Table with templates
		template<class T> static Table getTable () { return tableNone; } // TODO remove?
//		template<class T> static Table getTable ();

		Type type;
		Table table;
		dbId id;
};

#endif
