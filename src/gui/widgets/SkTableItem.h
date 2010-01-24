#ifndef _SkTableItem_h
#define _SkTableItem_h

#include <cstdio>
#include <QString>

#include "src/dataTypes.h"
#include "src/db/dbTypes.h"

#include <QTableWidgetItem>

// TODO probably remove after FlightTable has been removed
class SkTableItem:public QTableWidgetItem
{
	public:
		SkTableItem ();
		SkTableItem (const QString &, QColor);
		SkTableItem (const char *, QColor);

		void set_data (void *d) { data=d; }
		void *get_data () { return data; }

		void set_id (db_id i) { data_id=i; }
		db_id id () { return data_id; }

	private:
		void init ();
		void init (QColor bg);

		QColor background;
		db_id data_id;
		void *data;
};

#endif

