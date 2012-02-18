#include <unistd.h>

#include <iostream>
#include <iomanip>
#include <ios>
#include <string>
#include <QApplication>

#include "src/config/Settings.h"
#include "src/gui/windows/MainWindow.h"
#include "src/db/Database.h"
#include "src/db/interface/DefaultInterface.h"
#include "src/db/migration/Migrator.h"
#include "src/db/migration/MigrationFactory.h"
#include "src/db/schema/SchemaDumper.h"
#include "src/util/qString.h"
#include "src/db/interface/exceptions/SqlException.h"
#include "src/db/event/DbEvent.h" // For qRegisterMetaType
#include "src/net/TcpProxy.h" // remove
#include "src/version.h"

// For test_database
//#include "src/model/Plane.h"
//#include "src/model/Flight.h"
//#include "src/model/LaunchMethod.h"
//#include "src/model/Person.h"


// Testen des Wetterplugins
//#include "WeatherDialog.h"

//void display_help ()
//	/*
//	 * Displays a brief parameter information.
//	 */
//{
//	std::cout << "usage: startkladde [options]" << std::endl;
//	std::cout << "  options:" << std::endl;
//	Options::display_options ("    ");
//}


#include "src/concurrent/DefaultQThread.h"
#include "src/model/Person.h"
#include "src/model/Plane.h"
#include "src/db/interface/ThreadSafeInterface.h"

void ponder ()
{
//	std::cout << utf8 ("Pondering on thread %1: ").arg (QThread::currentThreadId ());
	std::cout << "Pondering: ";
	const int delay=200;

	std::cout << "[" << std::flush;

	for (int i=1; i<=10; ++i)
	{
		std::cout << i << std::flush;

		std::cout << "."; std::cout.flush(); DefaultQThread::msleep (delay);
		std::cout << "o"; std::cout.flush(); DefaultQThread::msleep (delay);
		std::cout << "O"; std::cout.flush(); DefaultQThread::msleep (delay);
		std::cout << "o"; std::cout.flush(); DefaultQThread::msleep (delay);
	}

	std::cout << "]" << std::endl;
}

void proxy_test ()
{
//	std::cout << utf8 ("Creating proxy on thread %1").arg (QThread::currentThreadId ()) << std::endl;
	std::cout << "Creating proxy" << std::endl;

	TcpProxy proxy;
//	proxy.open ("localhost", 3306);
	proxy.open ("damogran.local", 3307);

	while (true)
	{
//		ponder ();
		std::cout << "Press enter to terminat0r" << std::endl;

		char in[101];
		std::cin.getline (in, 100);
		proxy.close ();
	}
}

int test_database ()
{
//	DefaultInterface interface (Settings::instance ().databaseInfo);
	ThreadSafeInterface interface (Settings::instance ().databaseInfo);
	Database db (interface);

	try
	{
//		if (!interface.open ())
//		{
//			QSqlError error=interface.lastError ();
//			std::cout << "Database could not be opened" << std::endl;
//			std::cout << QString ("Type: %1, number: %2")
//				.arg (error.type ()).arg (error.number ()) << std::endl;
//			std::cout << QString ("Database text: %1").arg (error.databaseText ()) << std::endl;
//			std::cout << QString ("Driver text: %1").arg (error.driverText ()) << std::endl;
//
//			std::cout << "Database error: " << error.databaseText () << std::endl;
//
//			interface.close ();
//
//			return 1;
//		}

	//	std::cout << "Expect: 1" << std::endl;
	//	std::cout << interface.queryHasResult ("select id from people where id=1") << std::endl;
	//
	//	std::cout << "Expect: 0" << std::endl;
	//	std::cout << interface.queryHasResult ("select id from people where id=3") << std::endl;
	//
	//	std::cout << "Expect: caught exception" << std::endl;
	//	std::cout << interface.queryHasResult ("bam") << std::endl;

	//	std::cout << "Expect: OK" << std::endl;
	//	interface.executeQuery ("select 0");
	//	std::cout << "Expect: caught exception" << std::endl;
	//	interface.executeQuery ("bam");

	//	interface.close ();
	//	return 0;



	//	std::cout << std::endl;
	//	std::cout << "Get people>3" << std::endl;
	////	QList<Person> people=db.getObjects<Person> ("id>?", QList<QVariant> () << 3);
	//	QList<Person> people=db.getObjects<Person> (Query ("id>?").bind (3));
	//    foreach (const Person &person, people)
	//    	std::cout << person.toString () << std::endl;

	//    DefaultQThread::sleep (1);

	//    Person p;
	//    p.lastName=utf8 ("Müller");
	//    p.firstName="Busch";
	//    dbId newId=db.createObject (p);
	//
	//    db.deleteObject<Person> (newId-2);


	//    DefaultQThread::sleep (1);

		std::cout << std::endl;
		std::cout << "Get people" << std::endl;
		QList<Person> people=db.getObjects<Person> ();
		foreach (const Person &person, people)
			std::cout << person.toString () << std::endl;

		DefaultQThread::sleep (1);

		std::cout << std::endl;
		std::cout << "Get planes" << std::endl;
		QList<Plane> planes=db.getObjects<Plane> ();
		foreach (const Plane &plane, planes)
			std::cout << plane.toString () << std::endl;

		interface.close ();

	//	std::cout << std::endl;
	//	std::cout << "Get person 1" << std::endl;
	//	Person p=db.getObject<Person> (1);
	//	std::cout << p.toString () << std::endl;
	//
	//	std::cout << std::endl;
	//	std::cout << "Get person 3" << std::endl;
	//	p=db.getObject<Person> (3);
	//	std::cout << p.toString () << std::endl;



	//	std::cout << std::endl;
	//	std::cout << "Get people" << std::endl;
	//	QList<Person> people=db.getObjects<Person> ();
	//    foreach (const Person &person, people)
	//    	std::cout << person.toString ().toUtf8 () << std::endl;
	//
	//	std::cout << std::endl;
	//	std::cout << "Get planes" << std::endl;
	//	QList<Plane> planes=db.getObjects<Plane> ();
	//    foreach (const Plane &plane, planes)
	//    	std::cout << plane.toString ().toUtf8 () << std::endl;
	//
	//	std::cout << std::endl;
	//	std::cout << "Get flights" << std::endl;
	//	QList<Flight> flights=db.getObjects<Flight> ();
	//    foreach (const Flight &flight, flights)
	//    	std::cout << flight.toString ().toUtf8 () << std::endl;
	//
	//	std::cout << std::endl;
	//	std::cout << "Get prepared flights" << std::endl;
	//	flights=db.getPreparedFlights ();
	//    foreach (const Flight &flight, flights)
	//    	std::cout << flight.toString ().toUtf8 () << std::endl;
	//
	//	std::cout << std::endl;
	//	std::cout << "Get flights of 2010-01-23" << std::endl;
	//	flights=db.getFlightsDate (QDate (2010, 1, 23));
	//    foreach (const Flight &flight, flights)
	//    	std::cout << flight.toString ().toUtf8 () << std::endl;
	//
	//	std::cout << std::endl;
	//	std::cout << "Get launch methods" << std::endl;
	//	QList<LaunchMethod> launchMethods=db.getObjects<LaunchMethod> ();
	//    foreach (const LaunchMethod &launchMethod, launchMethods)
	//    	std::cout << launchMethod.toString ().toUtf8 () << std::endl;
	//
	//    std::cout << std::endl;
	//	std::cout << "List locations" << std::endl;
	//	std::cout << db.listLocations ().join (", ") << std::endl;
	//
	//    std::cout << std::endl;
	//	std::cout << "List accounting notes" << std::endl;
	//	std::cout << db.listAccountingNotes ().join (", ") << std::endl;
	//
	//    std::cout << std::endl;
	//	std::cout << "List clubs" << std::endl;
	//	std::cout << db.listClubs ().join (", ") << std::endl;
	//
	//    std::cout << std::endl;
	//	std::cout << "List plane types" << std::endl;
	//	std::cout << db.listPlaneTypes ().join (", ") << std::endl;
	}
	catch (SqlException &ex)
	{
		std::cout << ex.colorizedString () << std::endl;
		interface.close ();
		return 1;
	}

	return 0;
}

int showGui (QApplication &a, QTranslator *translator)
{
	//QApplication::setDesktopSettingsAware (FALSE); // I know better than the user

	// Put light.{la,so} to styles/
	//a.setStyle ("light, 3rd revision");
//	if (!style.isEmpty ()) a.setStyle (style);

	MainWindow w (NULL, translator);

	// Let the plugins initialize
	QThread::yieldCurrentThread ();

	if (Settings::instance ().noFullScreen)
		w.show ();
	else
		w.showMaximized ();

	int ret=a.exec();

	return ret;
}

int doStuff (const QStringList &nonOptions)
{
	Settings &s=Settings::instance ();

	// We don't need the ORM or thread safety, so we use a DefaultInterface.
	DefaultInterface db (s.databaseInfo);

	// Tests ahead
	bool ok=db.open ();

	if (!ok)
	{
		std::cout << "Database failed to open: " << db.lastError ().text () << std::endl;
		return 1;
	}

	if (nonOptions[0]=="db:up")
		Migrator (db).up ();
	else if (nonOptions[0]=="db:down")
		Migrator (db).down ();
	else if (nonOptions[0]=="db:migrate")
		Migrator (db).migrate ();
	else if (nonOptions[0]=="db:version")
		std::cout << "Version is " << Migrator (db).currentVersion () << std::endl;
	else if (nonOptions[0]=="db:load")
		Migrator (db).loadSchema ();
	else if (nonOptions[0]=="db:drop")
		Migrator (db).drop ();
	else if (nonOptions[0]=="db:clear")
		Migrator (db).clear ();
	else if (nonOptions[0]=="db:ensure_empty")
		return db.tableExists ()?1:0;
	else if (nonOptions[0]=="db:create")
		Migrator (db).create ();
//	else if (nonOptions[0]=="db:test")
//		test_database (db);
//	else if (nonOptions[0]=="db:fail") // TODO
//		db.executeQuery ("bam!");
	else if (nonOptions[0]=="db:migrations")
	{
		MigrationFactory factory;
		foreach (quint64 version, factory.availableVersions ())
			std::cout << version << " - " << factory.migrationName (version) << std::endl;
	}
	else if (nonOptions[0]=="db:dump")
	{
		Migrator m (db);
		if (!m.isCurrent ())
		{
			// TODO require --force switch if not current
			std::cerr << "Warning: the database is not current"  << std::endl;
		}

		SchemaDumper d (db);

		if (nonOptions.size ()>1)
		{
			QString filename=nonOptions[1];
			std::cout << QString ("Dumping schema to %1").arg (filename) << std::endl;
			d.dumpSchemaToFile (filename);
		}
		else
		{
			std::cout << d.dumpSchema () << std::endl;
		}
	}
	else
	{
		std::cout << "Unrecognized" << std::endl;
		return 1;
	}

	return 0;
}

void test ()
{
}

#include "src/plugin/info/InfoPlugin.h"
#include "src/plugin/factory/PluginFactory.h"

void plugins_test ()
{
	std::cout << "Begin plugin test" << std::endl;

	foreach (const InfoPlugin::Descriptor *descriptor, PluginFactory::getInstance ().getDescriptors<InfoPlugin> ())
	{
		std::cout << QString ("Registered plugin %1 (%2)").arg (descriptor->getName (), descriptor->getDescription ()) << std::endl;
		InfoPlugin *plugin=descriptor->create ();
		delete plugin;
	}

	std::cout << "End plugin test" << std::endl;
}

#include "src/concurrent/Waiter.h"
#include "src/model/LaunchMethod.h" //remove
#include "src/db/migrations/Migration_20100216135637_add_launch_methods.h"

int main (int argc, char **argv)
{
	QApplication a (argc, argv); // Always

    QTranslator translator;
    //translator.load ("startkladde_de");
    a.installTranslator (&translator);


	qApp->addLibraryPath (qApp->applicationDirPath () + "/plugins");

	// Event is used as parameters for signals emitted by tasks running on
	// a background thread. These connections must be queued, so the parameter
	// types must be registered.
	qRegisterMetaType<DbEvent> ("DbEvent");
	qRegisterMetaType<Query> ("Query");
	qRegisterMetaType<DatabaseInfo> ("DatabaseInfo");

	// For QSettings
	QCoreApplication::setOrganizationName ("startkladde");
	QCoreApplication::setOrganizationDomain("startkladde.sf.net");
	QCoreApplication::setApplicationName("startkladde");

	QStringList args;
	for (int i=0; i<argc; ++i) args << argv[i];

	if (args.size ()>=2 && args[1]=="--version")
	{
		std::cout << getVersion () << std::endl;

		return 0;
	}

	Settings::instance ().programPath=argv[0];
	QStringList nonOptions=Settings::instance ().readArgs (args);


	int ret=0;

	try
	{
		if (nonOptions.empty ())
		{
			ret=showGui (a, &translator);
		}
		else
		{
			if (nonOptions[0]=="test_db")
				ret=test_database ();
			else if (nonOptions[0]=="proxy")
				proxy_test ();
			else if (nonOptions[0]=="plugins")
				plugins_test ();
			else
				ret=doStuff (nonOptions);
		}
	}
	catch (SqlException &ex)
	{
		std::cout << ex.colorizedString () << std::endl;
		return 1;
	}

	return ret;
}
