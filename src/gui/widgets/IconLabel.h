#ifndef ICONLABEL_H
#define ICONLABEL_H

#include <QtGui/QWidget>
#include <QStyle>

#include "ui_IconLabel.h"

class QString;
class QLabel;

/**
 * A widget that displays an icon next to a label
 *
 * The widget is composed of two labels in a horizontal layout. The first (left)
 * label (the icon label) displays an icon the second (right) label (the text
 * label) displays a text.
 */
class IconLabel: public QWidget
{
	Q_OBJECT

	public:
		using QWidget::show;

		IconLabel (QWidget *parent = NULL, Qt::WindowFlags f = 0);
		~IconLabel();

		void show (QStyle::StandardPixmap icon, const QString &text);
		void showWarning (const QString &text);
		void showCritical (const QString &text);
		void showInformation (const QString &text);
		void setIcon (QStyle::StandardPixmap icon);
		void setText (const QString &text);

		QLabel *textLabel ();

	private:
		Ui::IconLabelClass ui;
};

#endif