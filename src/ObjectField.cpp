#include "ObjectField.h"

#include <stdlib.h>

#include "src/text.h"

void ObjectField::parse_text (const QString &text)
{
	switch (data_type)
	{
		case dt_string: current_string=text; break;
		case dt_db_id:
		{
			if (text.isEmpty () || text=="-")
				current_db_id=invalid_id;
			else
				current_db_id=text.toLongLong ();
		} break;
		case dt_bool: current_bool=(text.toInt()!=0); break;
		case dt_password: current_string=text; break;
		case dt_special: break;
	}
}

QString ObjectField::make_text () const
{
	switch (data_type)
	{
		case dt_string: return current_string; break;
		case dt_db_id:
		{
			if (id_invalid (current_db_id))
				return "";
			else
				return QString::number(current_db_id);
		} break;
		case dt_bool: return current_bool?"1":"0"; break;
		case dt_password: return current_string; break;
		case dt_special: return ""; break;
	}

	return "???";
}

QString ObjectField::make_text_safe () const
{
	if (data_type==dt_password)
		return "***";
	else
		return make_text ();
}

QString ObjectField::make_display_text () const
{
	switch (data_type)
	{
		case dt_string: return current_string; break;
		case dt_db_id:
		{
			if (id_invalid (current_db_id))
				return "-";
			else
				return QString::number(current_db_id);
		} break;
		case dt_bool: return current_bool?"Ja":"Nein"; break;
		case dt_password: return "***"; break;
		case dt_special: return ""; break;
	}

	return "???";
}

void ObjectField::clear ()
{
	switch (data_type)
	{
		case dt_string: current_string=""; break;
		case dt_db_id: current_db_id=invalid_id; break;
		case dt_bool: current_bool=false; break;
		case dt_password: current_string=""; break;
		case dt_special: break;
	}
}
