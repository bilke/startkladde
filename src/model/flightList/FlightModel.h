/*
 * FlightModel.h
 *
 *  Created on: Aug 30, 2009
 *      Author: mherrman
 */

#ifndef FLIGHTMODEL_H_
#define FLIGHTMODEL_H_

#include <QVariant>

#include "src/model/objectList/ObjectModel.h"

#include "src/model/Flight.h"
#include "src/db/DataStorage.h"
#include "src/model/Person.h"
#include "src/model/LaunchType.h"
#include "src/model/objectList/ColumnInfo.h"

/**
 * Unlike the models for Person, Plane and LaunchType, this is not part of the
 * respective class because it is much more complex.
 */
class FlightModel: public ObjectModel<Flight>, public ColumnInfo
{
	public:
		FlightModel (DataStorage &dataStorage);
		virtual ~FlightModel ();

		virtual int columnCount () const;
		virtual QVariant displayHeaderData (int column) const;
		virtual QVariant data (const Flight &flight, int column, int role=Qt::DisplayRole) const;

		virtual int launchButtonColumn () const { return 6; }
		virtual int landButtonColumn () const { return 7; }
		virtual int durationColumn () const { return 8; }

		// ColumnNames methods
		virtual QString columnName (int columnIndex) const;

	protected:
		virtual QVariant registrationData (const Flight &flight, int role) const;
		virtual QVariant planeTypeData (const Flight &flight, int role) const;
		virtual QVariant pilotData (const Flight &flight, int role) const;
		virtual QVariant copilotData (const Flight &flight, int role) const;
		virtual QVariant launchTypeData (const Flight &flight, int role) const;
		virtual QVariant launchTimeData (const Flight &flight, int role) const;
		virtual QVariant landingTimeData (const Flight &flight, int role) const;
		virtual QVariant durationData (const Flight &flight, int role) const;

	private:
		DataStorage &dataStorage;
};

#endif /* FLIGHTMODEL_H_ */
