#include "PlaneLog.h"

#include <QSet>

#include "src/model/Flight.h"
#include "src/db/DataStorage.h"
#include "src/model/Plane.h"
#include "src/model/Person.h"
#include "src/text.h"

// ************************
// ** Entry construction **
// ************************

PlaneLog::Entry::Entry ():
	minPassengers (0), maxPassengers (0), numLandings (0), valid (true)
{
}

PlaneLog::Entry::~Entry ()
{
}

// **********************
// ** Entry formatting **
// **********************

QString PlaneLog::Entry::dateText () const
{
	return date.toString (Qt::LocaleDate);
}

QString PlaneLog::Entry::numPassengersString () const
{
	if (minPassengers==maxPassengers)
		return QString::number (minPassengers);
	else if (minPassengers==1 && maxPassengers==2)
		return "1/2";
	else
		// Should not happen: entries for non-gliders cannot be merged
		return QString ("%1-%2").arg (minPassengers).arg (maxPassengers);
}

QString PlaneLog::Entry::departureTimeText (bool noLetters) const
{
	return departureTime.to_string ("%H:%M", tz_utc, 0, noLetters);
}

QString PlaneLog::Entry::landingTimeText (bool noLetters) const
{
	return landingTime.to_string ("%H:%M", tz_utc, 0, noLetters);
}

QString PlaneLog::Entry::operationTimeText () const
{
	return operationTime.to_string ("%H:%M", tz_timespan);
}

// ********************
// ** Entry creation **
// ********************

/**
 * Create a log entry from a single flight
 */
PlaneLog::Entry PlaneLog::Entry::create (const Flight *flight, DataStorage &dataStorage)
{
	PlaneLog::Entry entry;

	Plane      *plane     =dataStorage.getNewObject<Plane     > (flight->planeId );
	Person     *pilot     =dataStorage.getNewObject<Person    > (flight->pilotId    );

	if (plane) entry.registration=plane->registration;
	entry.date=flight->effdatum ();
	if (pilot) entry.pilotName=pilot->formalName ();
	entry.minPassengers=entry.maxPassengers=flight->numPassengers ();
	entry.departureLocation=flight->departureLocation;
	entry.landingLocation=flight->landingLocation;
	entry.departureTime=flight->departureTime; // TODO: check flight mode
	entry.landingTime=flight->landingTime; // TODO: check flight mode
	entry.numLandings=flight->numLandings;
	entry.operationTime=flight->flightDuration (); // TODO: check flight mode
	entry.comments=flight->comments;

	entry.valid=flight->finished ();

	delete plane;
	delete pilot;

	return entry;
}

/**
 * Create an entry for a non-empty, sorted list of flights which we know can
 * be merged. All flights must be of the same plane and on the same date.
 */
PlaneLog::Entry PlaneLog::Entry::create (const QList<const Flight *> flights, DataStorage &dataStorage)
{
	assert (!flights.isEmpty ());

	PlaneLog::Entry entry;

	Plane      *plane     =dataStorage.getNewObject<Plane     > (flights.last ()->planeId );
	Person     *pilot     =dataStorage.getNewObject<Person    > (flights.last ()->pilotId    );

	// Values directly determined
	if (plane) entry.registration=plane->registration;
	entry.date=flights.last ()->effdatum ();
	if (pilot) entry.pilotName=pilot->formalName ();
	entry.departureLocation=flights.first ()->departureLocation;
	entry.landingLocation=flights.last ()->landingLocation;
	entry.departureTime=flights.first ()->departureTime;
	entry.landingTime=flights.last ()->landingTime;

	// Values determined from all flights
	entry.minPassengers=entry.maxPassengers=0;
	entry.numLandings=0;
	QStringList comments;
	entry.valid=true;

	int numTowFlights=0;

	foreach (const Flight *flight, flights)
	{
		int numPassengers=flight->numPassengers ();
		if (entry.minPassengers==0 || numPassengers<entry.minPassengers) entry.minPassengers=numPassengers;
		if (entry.maxPassengers==0 || numPassengers>entry.maxPassengers) entry.maxPassengers=numPassengers;

		entry.numLandings+=flight->numLandings;
		entry.operationTime.add_time (flight->flightDuration ()); // TODO: check flight mode
		if (!eintrag_ist_leer (flight->comments)) comments << flight->comments;
		if (!flight->finished ()) entry.valid=false;

		if (flight->isTowflight ()) ++numTowFlights;
	}

	if (numTowFlights==1)
		comments << "Schleppflug";
	else if (numTowFlights>1)
		comments << "Schleppflüge";

	entry.comments=comments.join ("; ");

	delete plane;
	delete pilot;

	return entry;
}


// ******************
// ** Construction **
// ******************

PlaneLog::PlaneLog (QObject *parent):
	QAbstractTableModel (parent)
{
}

PlaneLog::~PlaneLog ()
{
}


// **************
// ** Creation **
// **************

/**
 * Makes the log for one plane from a list of flights. The list may contain
 * flights of other planes.
 *
 * @param planeId
 * @param flights
 * @param dataStorage
 * @return
 */
PlaneLog *PlaneLog::createNew (dbId planeId, const QList<Flight> &flights, DataStorage &dataStorage)
{
	Plane *plane=dataStorage.getNewObject<Plane> (planeId);

	QList<const Flight *> interestingFlights;

	// Make a list of flights for this plane
	foreach (const Flight &flight, flights)
		if (flight.finished ())
			if (flight.planeId==planeId)
				interestingFlights.append (&flight);

	qSort (interestingFlights);

	// Iterate over all interesting flights, generating logbook entries.
	// Sometimes, we can generate one entry from several flights. These
	// flights are in entryFlights.
	PlaneLog *result=new PlaneLog ();

	QList<const Flight *> entryFlights;
	const Flight *previousFlight=NULL;
	foreach (const Flight *flight, interestingFlights)
	{
		assert (flight->finished ());

		// We accumulate in entryFlights as long as we can merge flights.
		// Then we create an entry, append it to the list and clear
		// entryFlights.
		if (previousFlight && !flight->collectiveLogEntryPossible (previousFlight, plane))
		{
			// No further merging
			result->entries.append (PlaneLog::Entry::create (entryFlights, dataStorage));
			entryFlights.clear ();
		}

		entryFlights.append (flight);
		previousFlight=flight;
	}
	result->entries.append (PlaneLog::Entry::create (entryFlights, dataStorage));

	delete plane;

	return result;
}

/**
 * Makes the logs for all pilots that have flights in a given flight list.
 *
 * @param flights
 * @param dataStorage
 * @return
 */
PlaneLog *PlaneLog::createNew (const QList<Flight> &flights, DataStorage &dataStorage)
{
	// TODO: should we consider tow flights here?

	QSet<dbId> planeIdSet;

	// Determine all planes which have flights
	foreach (const Flight &flight, flights)
		if (flight.finished ())
			planeIdSet.insert (flight.planeId);

	QList<dbId> planeIds=planeIdSet.toList ();
	planeIds.removeAll (0);

	// Make a list of the planes and sort it
	QList<Plane> planes;
	foreach (const dbId &id, planeIds)
	{
		try
		{
			planes.append (dataStorage.getObject<Plane> (id));
		}
		catch (...)
		{
			// TODO log error
		}
	}
	qSort (planes.begin (), planes.end (), Plane::clubAwareLessThan);

	PlaneLog *result=new PlaneLog ();
	foreach (const Plane &plane, planes)
	{
		PlaneLog *planeResult=createNew (plane.getId (), flights, dataStorage);
		result->entries+=planeResult->entries;
		delete planeResult;
	}

	return result;
}


// *********************************
// ** QAbstractTableModel methods **
// *********************************

int PlaneLog::rowCount (const QModelIndex &index) const
{
	if (index.isValid ())
		return 0;

	return entries.size ();
}

int PlaneLog::columnCount (const QModelIndex &index) const
{
	if (index.isValid ())
		return 0;

	return 10;
}

QVariant PlaneLog::data (const QModelIndex &index, int role) const
{
	const Entry &entry=entries[index.row ()];

	// TODO if invalid, add parentheses around name, passengers, 2*location,
	// 3*times, num landings

	if (role==Qt::DisplayRole)
	{
		switch (index.column ())
		{
			case 0: return entry.dateText ();
			case 1: return entry.pilotName;
			case 2: return entry.numPassengersString ();
			case 3: return entry.departureLocation;
			case 4: return entry.landingLocation;
			case 5: return entry.departureTimeText ();
			case 6: return entry.landingTimeText ();
			case 7: return entry.numLandings;
			case 8: return entry.operationTimeText ();
			case 9: return entry.comments;
			default: assert (false); return QVariant ();
		}
	}
	else
		return QVariant ();
}

QVariant PlaneLog::headerData (int section, Qt::Orientation orientation, int role) const
{
	if (role==Qt::DisplayRole)
	{
		if (orientation==Qt::Horizontal)
		{
			switch (section)
			{
				case 0: return "Datum"; break;
				case 1: return "Pilot"; break;
				case 2: return "Insassen"; break;
				case 3: return "Startort"; break;
				case 4: return "Zielort"; break;
				case 5: return "Startzeit"; break;
				case 6: return "Landezeit"; break;
				case 7: return "Landungen"; break;
				case 8: return "Betriebsdauer"; break;
				case 9: return "Bemerkungen"; break;
			}
		}
		else
		{
			return section+1;
		}
	}

	return QVariant ();
}
