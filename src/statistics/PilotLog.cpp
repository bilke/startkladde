#include "PilotLog.h"

#include "src/model/LaunchType.h"

PilotLogEntry::PilotLogEntry ()
{
	invalid=false;
}

QString PilotLogEntry::tag_string () const
{
	return tag.toString ("yyyy-MM-dd");
}

QString PilotLogEntry::zeit_start_string (bool no_letters) const
{
	return zeit_start.to_string ("%H:%M", tz_utc, 0, no_letters);
}

QString PilotLogEntry::zeit_landung_string (bool no_letters) const
{
	return zeit_landung.to_string ("%H:%M", tz_utc, 0, no_letters);
}

QString PilotLogEntry::flugdauer_string () const
{
	return flugdauer.to_string ("%H:%M", tz_timespan);
}



void makePilotLogPerson (QList<PilotLogEntry *> &fb, Database *db, QDate date, Person *person, QList<Flight *> &flights, PilotLogEntry::flight_instructor_mode fim)
	// flights may contain flights which don't belong to the person
	// TODO pass list of planes here?
	// TODO this is slow because it needs to query the database for persons
	// TODO startart dito
	// pass an invalid/null date to ignore.
{
	// TODO! this should use flight_data

	QList<Flight *> interesting_flights;

	// We use only flights where both the person and the date matches. Make a
	// list of these flights ("interesting flights").
	foreach (Flight *flight, flights)
	{
		// First condition: person matches.
		// This means that the person given is the pilot, or (in case of
		// certain flight instructor modes, the flight instructor, which is the
		// copilot).
		bool person_match=false;
		if (flight->pilot==person->id) person_match=true;
		else if (fim==PilotLogEntry::fim_loose && flight->begleiter==person->id) person_match=true;
		else if (fim==PilotLogEntry::fim_strict && flight->begleiter==person->id && flight->flugtyp==ftTraining2) person_match=true;

		// Second condition: date matches.
		// This means that, if the date is given, it must match the flight's
		// effective date.
		bool date_match=false;
		if (!date.isValid ()) date_match=true;
		else if (flight->effdatum ()==date) date_match=true;

		if (person_match && date_match)
			interesting_flights.append (flight);
	}
	qSort (interesting_flights);

	// Iterate over all interesting flights, generating logbook entries.

	QListIterator<Flight *> flight (interesting_flights);
	while (flight.hasNext ())
	{
		Flight *f=flight.next ();

		// TODO Move to PilotLogEntry class
		PilotLogEntry *fb_entry=new PilotLogEntry;

		// Get additional data
		// TODO error checking
		Plane fz; db->get_plane (&fz, f->flugzeug);

		// The person we are checking may either be pilot (regular) or copilot
		// (flight instructor). Thus, both of these may be a different person.

		Person pilot, begleiter;

		// If the pilot is the person we're checking, copy it. If not, get it
		// from the database.
		if (f->pilot==person->id)
			pilot=*person;
		else
			// TODO error checking
			db->get_person (&pilot, f->pilot);

		// Same for copilot
		if (f->begleiter==person->id)
			begleiter=*person;
		else
			// TODO error checking
			db->get_person (&begleiter, f->begleiter);

		LaunchType sa; db->get_startart (&sa, f->startart);

		fb_entry->tag=f->effdatum ();
		fb_entry->muster=fz.typ;
		fb_entry->registration=fz.registration;
		fb_entry->flugzeugfuehrer=pilot.name ();
		fb_entry->begleiter=begleiter.name ();
		fb_entry->startart=sa.get_logbook_string ();
		fb_entry->ort_start=f->startort;
		fb_entry->ort_landung=f->zielort;
		fb_entry->zeit_start=f->startzeit;
		fb_entry->zeit_landung=f->landezeit;
		fb_entry->flugdauer=f->flugdauer ();
		fb_entry->bemerkung=f->bemerkungen;

		if (!f->finished ())
		{
			fb_entry->invalid=true;
		}

		fb.append (fb_entry);
	}
}

void makePilotLogsDay (QList<PilotLogEntry *> &fb, Database *db, QDate date)
	// Make all pilot logs for one day
{
	// TODO error handling

	QList<Person *> persons;
	// Find out which persons had flights today
	db->list_persons_date (persons, &date);

	// Sort the persons
	// TODO this uses manual selection sort. Better use the heap sort provided
	// by QPtrList.
	QList<Person *> sorted_persons;
	while (!persons.isEmpty ())
	{
		Person *smallest=NULL;
		// Find the smallest element.
		foreach (Person *person, persons)
		{
			if (!smallest)
				// No smallest entry set yet (first element in list)
				smallest=person;
			else if (person->nachname<smallest->nachname)
				// Last name smaller
				smallest=person;
			else if (person->nachname==smallest->nachname && person->vorname<smallest->vorname)
				// Last name identical, first name smaller
				smallest=person;
		}
		sorted_persons.append (smallest);
		persons.remove (smallest);
	}

	QList<Flight *> flights;
	// We need all flights of that date anyway. For speed, we don't query the
	// database for each person but retrieve all flights here.
	db->list_flights_date (flights, &date);

	foreach (Person *person, sorted_persons)
	{
		// TODO emit progress
		makePilotLogPerson (fb, db, date, person, flights);
	}

	foreach (Flight *f, flights) delete f;
	foreach (Person *p, sorted_persons) delete p;
}

