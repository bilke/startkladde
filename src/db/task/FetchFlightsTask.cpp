/*
 * FetchFlightsTask.cpp
 *
 *  Created on: Sep 6, 2009
 *      Author: deffi
 */

#include "FetchFlightsTask.h"

#include "src/db/DataStorage.h"

FetchFlightsTask::FetchFlightsTask (DataStorage &dataStorage, QDate date):
	dataStorage (dataStorage),
	date (date)
{
}

FetchFlightsTask::~FetchFlightsTask ()
{
}

QString FetchFlightsTask::toString () const
{
	return QString::fromUtf8 ("Flüge für %1 abrufen").arg (date.toString ());
}

bool FetchFlightsTask::run ()
{
	bool success;
	QString message;
	bool completed=dataStorage.fetchFlightsDate (this, date, &success, &message);
	setSuccess (success);
	setMessage (message);
	return completed;
}
