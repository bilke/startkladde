#ifndef MONITORDIALOG_H
#define MONITORDIALOG_H

#include "src/gui/SkDialog.h"

#include "ui_MonitorDialog.h"

class SignalOperationMonitor;

/**
 * When using this class, beware of race conditions: when the monitor emits end
 * before the signal is connected, the signal may never be received. The
 * #monitor method is properly synchronized and is currently the only method
 * that should be used (without further work). That's why the constructor is
 * private.
 */
class MonitorDialog: public SkDialog<Ui::MonitorDialogClass>
{
    Q_OBJECT

	public:
		~MonitorDialog ();
		static void monitor (SignalOperationMonitor &monitor, const QString &title, QWidget *parent);

	protected slots:
		void progress (int progress, int maxProgress);
		void status (QString status);
		virtual void reject ();

	protected:
		MonitorDialog (SignalOperationMonitor &monitor, QWidget *parent=NULL);

	private:
		SignalOperationMonitor &theMonitor;
};

#endif // MONITORDIALOG_H
