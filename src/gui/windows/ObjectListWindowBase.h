#ifndef OBJECTLISTWINDOWBASE_H
#define OBJECTLISTWINDOWBASE_H

#include <QtGui/QMainWindow>

#include "ui_ObjectListWindowBase.h"

#include "src/db/DbManager.h" // Required for DbManager::State

/*
 * TODO: Menu View->Sort and hotkey
 */

class ObjectListWindowBase : public QMainWindow
{
    Q_OBJECT

	public:
		ObjectListWindowBase (DbManager &manager, QWidget *parent = 0);
		~ObjectListWindowBase ();

		virtual void requireEditPassword (const QString &password);

	public slots:
		virtual void on_actionNew_triggered ()=0;
		virtual void on_actionEdit_triggered ()=0;
		virtual void on_actionDelete_triggered ()=0;
		virtual void on_actionRefresh_triggered ()=0;
		virtual void on_actionClose_triggered ();

		virtual void on_table_activated (const QModelIndex &index)=0;

	protected slots:
		virtual void databaseStateChanged (DbManager::State state);

	protected:
		DbManager &manager;
		void keyPressEvent (QKeyEvent *e);
		Ui::ObjectListWindowBaseClass ui;

		bool allowEdit (QString message);

		bool editPasswordRequired;
		QString editPassword;
		bool editPasswordOk;
};

#endif // OBJECTLISTWINDOWBASE_H