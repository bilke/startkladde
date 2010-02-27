/*
 * Implementation notes:
 *   - All accesses to internal data, even single values, must be protected by
 *     dataMutex. Use:
 *     - synchronizedReturn (dataMutex, value) if just a value is returned
 *     - synchronized (dataMutex) { ... } if this doesn't cause warnings about
 *       control reaching the end of the function (in methods which return a
 *       value)
 *     - QMutexLocker (&dataMutex) otherwise
 *     - dataMutex.lock () only when there's good reason (and document the
 *       reason)
 */

// FIXME all methods accessing the database:
//   - error handling


/*
 * Cache.cpp
 *
 *  Created on: 26.02.2010
 *      Author: Martin Herrmann
 */

#include "Cache.h"

#include <QSet>

#include "src/model/Flight.h"
#include "src/model/LaunchMethod.h"
#include "src/model/Person.h"
#include "src/model/Plane.h"

#include "src/db/Database.h"
#include "src/model/objectList/EntityList.h"
#include "src/concurrent/synchronized.h"


namespace Db
{
	namespace Cache
	{
		Cache::Cache (Database &db):
			db (db),
			flightsToday (new EntityList<Flight> ()),
			flightsOther (new EntityList<Flight> ()),
			preparedFlights (new EntityList<Flight> ())
		{
		}

		Cache::~Cache ()
		{
		}


		// ****************
		// ** Properties **
		// ****************

		QDate Cache::getTodayDate ()
		{
			synchronizedReturn (dataMutex, todayDate);
		}

		QDate Cache::getOtherDate ()
		{
			synchronizedReturn (dataMutex, otherDate);
		}


		// ******************
		// ** Object lists **
		// ******************

		QList<Plane> Cache::getPlanes ()
		{
			synchronizedReturn (dataMutex, planes);
		}

		QList<Person> Cache::getPeople ()
		{
			synchronizedReturn (dataMutex, people);
		}

		QList<LaunchMethod> Cache::getLaunchMethods ()
		{
			synchronizedReturn (dataMutex, launchMethods);
		}

		QList<Flight> Cache::getFlightsToday ()
		{
			// TODO handle date change?
			synchronizedReturn (dataMutex, flightsToday->getList ());
		}

		QList<Flight> Cache::getFlightsOther ()
		{
			synchronizedReturn (dataMutex, flightsOther->getList ());
		}

		QList<Flight> Cache::getPreparedFlights ()
		{
			synchronizedReturn (dataMutex, preparedFlights->getList ());
		}

		template<class T> const QList<T> Cache::getObjects () const
		{
			// TODO the cast should be in the non-const method
			// TODO: make a copy of the list (synchronized) because the lists
			// have to be locked for concurrent access
			Cache *nonConstCache=const_cast<Cache *> (this);
			return *nonConstCache->objectList<T> ();
		}

		template<> QList<Plane       > *Cache::objectList<Plane       > () { return &planes       ; }
		template<> QList<Person      > *Cache::objectList<Person      > () { return &people       ; }
		template<> QList<LaunchMethod> *Cache::objectList<LaunchMethod> () { return &launchMethods; }



		// ************************
		// ** Individual objects **
		// ************************

		/**
		 * Gets an object from the cache; the database is not accessed.
		 *
		 * @param id the ID of the object
		 * @return a copy of the object
		 * @throw NotFoundException if the object is not found or id is invalid
		 */
		template<class T> T Cache::getObject (dbId id)
		{
			if (idInvalid (id)) throw NotFoundException (id);

			// TODO use a hash/map
			synchronized (dataMutex)
				foreach (const T &object, *objectList<T> ())
					if (object.getId ()==id)
						return T (object);

			throw NotFoundException (id);
		}

		// Different specialization for Flight
		template<> Flight Cache::getObject (dbId id)
		{
			// TODO use EntityList methods
			synchronized (dataMutex)
			{
				if (todayDate.isValid ())
					foreach (const Flight &flight, flightsToday->getList ())
						if (flight.getId ()==id)
							return Flight (flight);

				if (otherDate.isValid ())
					foreach (const Flight &flight, flightsOther->getList ())
						if (flight.getId ()==id)
								return Flight (flight);

				foreach (const Flight &flight, preparedFlights->getList ())
					if (flight.getId ()==id)
						return Flight (flight);
			}

			throw NotFoundException (id);
		}

		/**
		 * Gets an object from the cache; the database is not accessed.
		 *
		 * @param id the ID of the object
		 * @return a newly allocated copy of the object (the caller takes
		 *         ownership) or NULL the object is not found or id is invalid
		 */
		template<class T> T* Cache::getNewObject (dbId id)
		{
			try
			{
				return new T (getObject<T> (id));
			}
			catch (NotFoundException &ex)
			{
				return NULL;
			}
		}

		/**
		 * Determines if an object exists in the cache; the database is not
		 * accessed.
		 *
		 * @param id the ID of the object
		 * @return true if the object exists, or false if not
		 */
		template<class T> bool Cache::objectExists (dbId id)
		{
			// TODO use hash/map
			synchronized (dataMutex)
				foreach (const T &object, *objectList<T> ())
					if (object.getId ()==id)
						return true;

			return false;
		}

		// TODO template
		QList<Person> Cache::getPeople (const QList<dbId> &ids)
		{
			QList<Person> result;

			foreach (dbId id, ids)
				result.append (getObject<Person> (id));

			return result;
		}

		// *** Object lookup
		dbId Cache::getPlaneIdByRegistration (const QString &registration)
		{
			// TODO use hash/map
			synchronized (dataMutex)
				foreach (const Plane &plane, planes)
					if (plane.registration.toLower ()==registration.toLower ())
						return plane.getId ();

			return invalidId;
		}

		QList<dbId> Cache::getPersonIdsByName (const QString &firstName, const QString &lastName)
		{
			// TODO use hash/map
			QList<dbId> result;

			synchronized (dataMutex)
				foreach (const Person &person, people)
					if (person.firstName.toLower ()==firstName.toLower () && person.lastName.toLower ()==lastName.toLower ())
						result.append (person.getId ());

			return result;
		}

		/**
		 * Returns the ID of the person with the given first and last name
		 * (case insensitively) if there is exactly one such person, or an
		 * invalid id if threre are multiple or no such people
		 *
		 * @param firstName the first name of the person (the case is ignored)
		 * @param lastName the last name of the person (the case is ignored)
		 * @return the ID of the person, or an invalid ID
		 */
		dbId Cache::getUniquePersonIdByName (const QString &firstName, const QString &lastName)
		{
			// TODO use hash/map
			// TODO: instead of getting all matching people, we could stop on the first
			// duplicate.
			const QList<dbId> personIds=getPersonIdsByName (firstName, lastName);

			if (personIds.size ()==1)
				return personIds.at (0);
			else
				return invalidId;
		}

		QList<dbId> Cache::getPersonIdsByLastName (const QString &lastName)
		{
			// TODO use hash/map
			QList<dbId> result;

			synchronized (dataMutex)
				foreach (const Person &person, people)
					if (person.lastName.toLower ()==lastName.toLower ())
						result.append (person.getId ());

			return result;
		}

		QList<dbId> Cache::getPersonIdsByFirstName (const QString &firstName)
		{
			// TODO use hash/map
			QList<dbId> result;

			synchronized (dataMutex)
				foreach (const Person &person, people)
					if (person.firstName.toLower ()==firstName.toLower ())
						result.append (person.getId ());

			return result;
		}

		dbId Cache::getLaunchMethodByType (LaunchMethod::Type type)
		{
			// TODO use hash/map
			synchronized (dataMutex)
				foreach (const LaunchMethod &launchMethod, launchMethods)
					if (launchMethod.type==type)
						return launchMethod.getId ();

			return invalidId;
		}


		// *****************
		// ** Object data **
		// *****************

		QStringList Cache::getPlaneRegistrations ()
		{
			// TODO cache
			QStringList result;

			synchronized (dataMutex)
				foreach (const Plane &plane, planes)
					result.append (plane.registration);

			result.sort ();
			return result;
		}

		QStringList Cache::getPersonFirstNames ()
		{
			// TODO cache
			// TODO case insensitive unique
			QSet<QString> firstNames;

			synchronized (dataMutex)
				foreach (const Person &person, people)
					firstNames.insert (person.firstName);

			// TODO sort case insensitively
			QStringList result=QStringList::fromSet (firstNames);
			result.sort ();
			return result;
		}

		QStringList Cache::getPersonFirstNames (const QString &lastName)
		{
			// TODO cache
			// TODO case insensitive unique
			QSet<QString> firstNames;

			synchronized (dataMutex)
				foreach (const Person &person, people)
					if (person.lastName.toLower ()==lastName.toLower ())
						firstNames.insert (person.firstName);

			// TODO sort case insensitively
			QStringList result=QStringList::fromSet (firstNames);
			result.sort ();
			return result;
		}

		QStringList Cache::getPersonLastNames ()
		{
			// TODO cache
			// TODO case insensitive unique
			QSet<QString> lastNames;

			synchronized (dataMutex)
				foreach (const Person &person, people)
					lastNames.insert (person.lastName);

			// TODO sort case insensitively
			QStringList result=QStringList::fromSet (lastNames);
			result.sort ();
			return result;
		}

		QStringList Cache::getPersonLastNames (const QString &firstName)
		{
			// TODO cache
			// TODO case insensitive unique
			QSet<QString> lastNames;

			synchronized (dataMutex)
				foreach (const Person &person, people)
					if (person.firstName.toLower ()==firstName.toLower ())
						lastNames.insert (person.lastName);

			// TODO sort case insensitively
			QStringList result=QStringList::fromSet (lastNames);
			result.sort ();
			return result;
		}

		QStringList Cache::getLocations ()
		{
			synchronizedReturn (dataMutex, locations);
		}

		QStringList Cache::getAccountingNotes ()
		{
			synchronizedReturn (dataMutex, accountingNotes);
		}

		QStringList Cache::getPlaneTypes ()
		{
			synchronizedReturn (dataMutex, planeTypes);
		}

		QStringList Cache::getClubs ()
		{
			synchronizedReturn (dataMutex, clubs);
		}


		dbId Cache::planeFlying (dbId id)
		{
			synchronized (dataMutex)
			{
				// Only use the flights of today
				foreach (const Flight &flight, flightsToday->getList ())
				{
					if (
						(flight.isFlying         () && flight.planeId==id) ||
						(flight.isTowplaneFlying () && flight.towplaneId==id))
						return flight.getId ();
				}
			}

			return invalidId;
		}

		dbId Cache::personFlying (dbId id)
		{
			synchronized (dataMutex)
			{
				// Only use the flights of today
				foreach (const Flight &flight, flightsToday->getList ())
				{
					if (
						(flight.isFlying         () && flight.pilotId    ==id) ||
						(flight.isFlying         () && flight.copilotId  ==id) ||
						(flight.isTowplaneFlying () && flight.towpilotId ==id))
						return flight.getId ();
				}
			}

			return invalidId;
		}


		// *************
		// ** Reading **
		// *************

		// TODO allow canceling (old OperationMonitor)
		int Cache::refreshPlanes ()
		{
			QList<Plane> newPlanes=db.getObjects<Plane> ();
			synchronized (dataMutex) planes=newPlanes;
			// TODO rebuild hashes

			return 0;
		}

		int Cache::refreshPeople ()
		{
			QList<Person> newPeople=db.getObjects<Person> ();
			synchronized (dataMutex) people=newPeople;
			// TODO rebuild hashes

			return 0;
		}

		int Cache::refreshLaunchMethods ()
		{
			QList<LaunchMethod> newLaunchMethods=db.getObjects<LaunchMethod> ();
			synchronized (dataMutex) launchMethods=newLaunchMethods;
			// TODO rebuild hashes

			return 0;
		}


#define copyList(T, source, target) do \
{ \
	(target).clear (); \
	foreach (T it, (source)) \
		(target).append (it); \
} while (0)

		int Cache::refreshFlightsToday ()
		{
			QDate today=QDate::currentDate ();

			QList<Flight> newFlights=db.getFlightsDate (today);

			synchronized (dataMutex)
			{
				todayDate=today;
				// TODO better copying
				copyList (Flight, newFlights, *flightsToday);
//				(*flightsToday)=newFlights;
			}

			return 0;
		}

		int Cache::refreshFlightsOther ()
		{
			if (otherDate.isNull ()) return 0;

			QList<Flight> newFlights=db.getFlightsDate (otherDate);
			synchronized (dataMutex)
			{
				// TODO better copying
				copyList (Flight, newFlights, *flightsOther);
//				(*flightsOther)=newFlights;
			}

			return 0;
		}

		int Cache::fetchFlightsOther (QDate date)
		{
			if (otherDate.isNull ()) return 0;

			QList<Flight> newFlights=db.getFlightsDate (date);

			synchronized (dataMutex)
			{
				otherDate=date;
				// TODO better copying
				copyList (Flight, newFlights, *flightsOther);
//				(*flightsOther)=newFlights;
			}

			return 0;
		}

		int Cache::refreshPreparedFlights ()
		{
			QList<Flight> newFlights=db.getPreparedFlights ();
			synchronized (dataMutex)
			{
				// TODO better copying
				copyList (Flight, newFlights, *preparedFlights);
//				(*preparedFlights)=newFlights;
			}

			return 0;
		}

		int Cache::refreshLocations ()
		{
			QStringList newLocations=db.listLocations ();
			synchronized (dataMutex) locations=newLocations;

			return 0;
		}

		int Cache::refreshAccountingNotes ()
		{
			QStringList newAccountingNotes=db.listAccountingNotes ();
			synchronized (dataMutex) accountingNotes=newAccountingNotes;

			return 0;
		}

		bool Cache::refreshAll ()
		{
			// Refresh planes and people before refreshing flights!
			refreshPlanes ();
			refreshPeople ();
			refreshLaunchMethods ();
			refreshFlightsToday ();
			refreshFlightsOther ();
			refreshPreparedFlights ();
			refreshLocations ();
			refreshAccountingNotes ();
			return true;

			// Old:
			// if (monitor->isCanceled ()) return false;
			// monitor->progress (3, 7);
			// monitor->status ("Flüge aktualisieren")
			// refreshFlights ();
		}


		// *********************
		// ** Change handling **
		// *********************

		// This template is specialized for T==Flight
		template<class T> void Cache::objectAdded (const T &object)
		{
			// Add the object to the cache
			synchronized (dataMutex) objectList<T> ()->append (object);
			// TODO update hashes
		}

		template<> void Cache::objectAdded<Flight> (const Flight &flight)
		{
			synchronized (dataMutex)
			{
				if (flight.isPrepared ())
					preparedFlights->append (flight);
				else if (flight.effdatum ()==todayDate)
					flightsToday->append (flight);
				else if (flight.effdatum ()==otherDate)
					// If otherDate is the same as today, this is not reached.
					flightsOther->append (flight);
				//else
				//	we're not interested in this flight
			}
		}

		// This template is specialized for T==Flight
		template<class T> void Cache::objectDeleted (dbId id)
		{
			// Remove the object from the cache
			// TODO use EntityList methods
			synchronized (dataMutex)
			{
				QMutableListIterator<T> it (*(objectList<T> ()));
				while (it.hasNext ())
					if (it.next ().getId ()==id)
						it.remove ();
			}
		}

		template<> void Cache::objectDeleted<Flight> (dbId id)
		{
			// If any of the lists contain this flight, remove it
			preparedFlights->removeById (id);
			flightsToday->removeById (id);
			flightsOther->removeById (id);
		}

		// This template is specialized for T==Flight
		template<class T> void Cache::objectUpdated (const T &object)
		{
			// TODO if the object is not in the cache, add it and log an error
			// TODO use EntityList methods

			synchronized (dataMutex)
			{
				// Update the cache
				QMutableListIterator<T> it (*(objectList<T> ()));

				while (it.hasNext ())
					if (it.next ().getId ()==object.getId ())
						it.setValue (object);
			}
		}

		template<> void Cache::objectUpdated<Flight> (const Flight &flight)
		{
			// If the date or the prepared status of a flight changed, we may have to
			// relocate it to a different list. If the date is changed, it may not be
			// on any list at all any more; or it may not have been on any list before
			// (although the UI does not provide a way to modify a flight that is not
			// on one of these lists, but something like that may well be added, and
			// even if not, we'd still have to handle this case).

			// Determine which list the flight should be in (or none). Replace it if
			// it already exists, add it if not, and remove it from the other lists.

			synchronized (dataMutex)
			{
				if (flight.isPrepared ())
				{
					preparedFlights->replaceOrAdd (flight.getId (), flight);
					flightsToday->removeById (flight.getId ());
					flightsOther->removeById (flight.getId ());
				}
				else if (flight.effdatum ()==todayDate)
				{
					preparedFlights->removeById (flight.getId ());
					flightsToday->replaceOrAdd (flight.getId (), flight);
					flightsOther->removeById (flight.getId ());
				}
				else if (flight.effdatum ()==otherDate)
				{
					// If otherDate is the same as today, this is not reached.
					preparedFlights->removeById (flight.getId ());
					flightsToday->removeById (flight.getId ());
					flightsOther->replaceOrAdd (flight.getId (), flight);
				}
				else
				{
					// The flight should not be on any list - remove it from all lists
					preparedFlights->removeById (flight.getId ());
					flightsToday->removeById (flight.getId ());
					flightsOther->removeById (flight.getId ());
				}
			}
		}


		// ****************************
		// ** Template instantiation **
		// ****************************

		// Instantiate the get method templates
		template Flight       Cache::getObject (dbId id);
		template Plane        Cache::getObject (dbId id);
		template Person       Cache::getObject (dbId id);
		template LaunchMethod Cache::getObject (dbId id);

		template Flight       *Cache::getNewObject (dbId id);
		template Plane        *Cache::getNewObject (dbId id);
		template Person       *Cache::getNewObject (dbId id);
		template LaunchMethod *Cache::getNewObject (dbId id);


		// Instantiate the change method templates (not for Flight - specialized)
		template void Cache::objectAdded<Plane       > (const Plane        &object);
		template void Cache::objectAdded<Person      > (const Person       &object);
		template void Cache::objectAdded<LaunchMethod> (const LaunchMethod &object);

		template void Cache::objectDeleted<Plane       > (dbId id);
		template void Cache::objectDeleted<Person      > (dbId id);
		template void Cache::objectDeleted<LaunchMethod> (dbId id);

		template void Cache::objectUpdated<Plane       > (const Plane        &plane );
		template void Cache::objectUpdated<Person      > (const Person       &flight);
		template void Cache::objectUpdated<LaunchMethod> (const LaunchMethod &flight);
	}
}
