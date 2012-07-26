/* @@@LICENSE
*
*      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */




#include "qtjsonabstract.h"
#include <QDebug>

#define ccstr(s) (s.toUtf8().data())
#define cstr(s) (s.toUtf8().constData())
#define check(o) ((o == 0) || is_error(o) ? 0 : o)

//static
QtJsonAbstractVariant QtJsonAbstractVariant::s_initObject = QtJsonAbstractVariant(true);

QtJsonAbstractVariant::QtJsonAbstractVariant(bool init)
: object(0)
{
	if (init)
		qRegisterMetaTypeStreamOperators<QtJsonAbstractVariant>("QtJsonAbstractVariant");
}

QtJsonAbstractVariant::QtJsonAbstractVariant()
{
	object = QtJsonAbstract::create();
}

QtJsonAbstractVariant::QtJsonAbstractVariant(QtJsonAbstract * p_obj)
{
	object = p_obj;
}

QtJsonAbstractVariant::QtJsonAbstractVariant(const QtJsonAbstract& obj)
{
	object = obj.clone();
}

QtJsonAbstractVariant::QtJsonAbstractVariant(const QtJsonAbstractVariant& c)
{
	if (c.object)
		object = c.object->clone();
}

QtJsonAbstractVariant::~QtJsonAbstractVariant()
{
	QtJsonAbstract::throwAway(object);
}

//static
QtJsonAbstract * QtJsonAbstract::jsonParse(const QString& s)
{
	json_object * obj = json_tokener_parse(s.toUtf8().constData());
	if (!check(obj))
	{
		obj = 0;
		return 0;
	}
	return new QtJsonAbstract(obj);
}

//static
QtJsonAbstract * QtJsonAbstract::jsonParseFromStream(QDataStream& stream)
{
	QString s;
	stream >> s;
	return jsonParse(s);
}

QtJsonAbstract * QtJsonAbstract::add(const QString& k, qint32 v)
{
	if (check(o))
		json_object_object_add(o,ccstr(k),json_object_new_int((int)v));
	return this;

}
QtJsonAbstract * QtJsonAbstract::add(const QString& k, qreal v)
{
	if (check(o))
		json_object_object_add(o,ccstr(k),json_object_new_double((double)v));
	return this;
}
QtJsonAbstract * QtJsonAbstract::add(const QString& k, const QString& v)
{
	if (check(o))
		json_object_object_add(o,ccstr(k),json_object_new_string(ccstr(v)));
	return this;
}

QtJsonAbstract * QtJsonAbstract::add(const QString& k, bool v)
{
	if (check(o))
		json_object_object_add(o,ccstr(k),json_object_new_boolean(v));
	return this;
}

QtJsonAbstract * QtJsonAbstract::add(const QString& k, const QList<QVariant>& a)
{
	return add(this,k,a);
}

QtJsonAbstract * QtJsonAbstract::add(const QString& k, const QtJsonAbstract& object)
{
	if (check(o))
		json_object_object_add(o,ccstr(k),json_object_get(object.o));
	return this;
}

//static
QtJsonAbstract * QtJsonAbstract::add(QtJsonAbstract * p_target,const QString& k, const QList<QVariant>& a)
{
	if (!p_target)
		return 0;
	if (!check(p_target->o))
		return p_target;

	json_object * array = _array(a);
	if (check(array))
		json_object_object_add(p_target->o,ccstr(k),array);
	return p_target;
}

//static
json_object * QtJsonAbstract::_array(const QList<QVariant>& a)
{
	json_object * array = json_object_new_array();
	for (QList<QVariant>::const_iterator it = a.constBegin();
			it != a.constEnd(); ++it)
	{
		if (it->userType() == qMetaTypeId<QtJsonAbstractVariant>())
		{
			json_object_array_add(array,
					json_object_get(it->value<QtJsonAbstractVariant>().object->o));
		}
		else
		{
			switch (it->type())
			{
			case QVariant::Bool:
				json_object_array_add(array,json_object_new_boolean(it->toBool()));
				break;
			case QVariant::Int:
			case QVariant::UInt:
				json_object_array_add(array,json_object_new_int(it->toInt()));
				break;
			case QVariant::Double:
				json_object_array_add(array,json_object_new_double(it->toDouble()));
				break;
			case QVariant::String:
				json_object_array_add(array,json_object_new_string(ccstr(it->toString())));
				break;
			case QVariant::List:
				json_object_array_add(array,(json_object *)(_array((const QVariantList&)*it)));
				break;
			default:
				qWarning() << __FUNCTION__ << " does not handle variant type: " << QString(it->typeName());
				break;
			}
		}
	}

	return array;
}

QVariant		QtJsonAbstract::extract(const QString& k) const
{
	if (!check(o))
		return QVariant();

	json_object * jo = json_object_object_get(o,ccstr(k));
	if (!check(jo))
		return QVariant();

	QVariant v;

	switch (json_object_get_type(jo))
	{
	case json_type_boolean:
		return QVariant(json_object_get_boolean(jo));
		break;
	case json_type_int:
		return QVariant(json_object_get_int(jo));
		break;
	case json_type_double:
		return QVariant(json_object_get_double(jo));
		break;
	case json_type_array:
		return _extractArray(jo);
		break;
	case json_type_string:
		return QVariant(QString(json_object_get_string(jo)));
		break;
	case json_type_object:
		v.setValue<QtJsonAbstractVariant>(
				QtJsonAbstractVariant(QtJsonAbstract(json_object_get(jo)))
				);
		return v;
		break;
	default:
		qWarning() << __FUNCTION__ << " does not handle json type: " << json_object_get_type(jo);
		break;
	}
	return v;
}

QList<QPair<QString,QVariant> >	QtJsonAbstract::extractAll() const
{
	QList<QPair<QString,QVariant> > r_list;
	if (!check(o))
		return r_list;
	if (json_object_is_type(o,json_type_object) == false)
		return r_list;

	json_object_object_foreach(o,key,val)
	{
		QString qk = QString(key);
		r_list.append(QPair<QString,QVariant>(qk,extract(qk)));
	}
	return r_list;
}

//static
QVariant QtJsonAbstract::_extractArray(json_object * p_o)
{
	if (!check(p_o))
		return QVariant();
	QVariantList list;
	QVariant v;
	for (int i=0;i<json_object_array_length(p_o);++i)
	{
		json_object * jo = json_object_array_get_idx(p_o,i);
		if (!check(jo))
			continue;

		switch (json_object_get_type(jo))
		{
		case json_type_boolean:
			list.append(QVariant(json_object_get_boolean(jo)));
			break;
		case json_type_int:
			list.append(QVariant(json_object_get_int(jo)));
			break;
		case json_type_double:
			list.append(QVariant(json_object_get_double(jo)));
			break;
		case json_type_array:
			list.append(_extractArray(jo));
			break;
		case json_type_string:
			list.append(QVariant(QString(json_object_get_string(jo))));
			break;
		case json_type_object:
			v.setValue<QtJsonAbstractVariant>(
					QtJsonAbstractVariant(QtJsonAbstract(json_object_get(jo)))
			);
			list.append(v);
			break;
		default:
			qWarning() << __FUNCTION__ << " does not handle json type: " << json_object_get_type(jo);
			break;
		}
	}
	return list;
}

QtJsonAbstract * QtJsonAbstract::clone() const
{
	return new QtJsonAbstract(json_object_get(o));
}

QString			QtJsonAbstract::toString()
{
	if (!check(o))
		return QString("{}");
	return QString(json_object_to_json_string(o));
}

//static
QtJsonAbstract * QtJsonAbstract::create()
{
	return new QtJsonAbstract(json_object_new_object());
}
//static
void QtJsonAbstract::throwAway(QtJsonAbstract * p_object)
{
	delete p_object;
}

QtJsonAbstract::~QtJsonAbstract()
{
	if (check(o))
		json_object_put(o);			//do not delete explicitly. see clone()
}

QDataStream& operator<<(QDataStream& stream, const  QtJsonAbstract& v)
{
	if (!check(v.o))
		stream << QString("<invalid>");
	else
		stream << QString(json_object_to_json_string(v.o));
	return stream;
}

QDataStream& operator>>(QDataStream& stream, QtJsonAbstract& v)
{
	if (check(v.o))
		json_object_put(v.o);
	QString s;
	stream >> s;
	v.o = json_tokener_parse(s.toUtf8().constData());
	return stream;
}

QDebug& operator<<(QDebug& dbg, const QtJsonAbstract& v)
{
	if (!check(v.o))
		dbg << QString("<invalid>");
	else
		dbg << QString(json_object_to_json_string(v.o));
	return dbg;
}

QDataStream& operator<<(QDataStream& stream, const  QtJsonAbstractVariant& v)
{
	if (!v.object)
		stream << QString("<invalid>");
	else
		stream << *(v.object);
	return stream;
}

QDataStream& operator>>(QDataStream& stream,QtJsonAbstractVariant& v)
{
	if (v.object)
		QtJsonAbstract::throwAway(v.object);
	v.object = QtJsonAbstract::jsonParseFromStream(stream);
	return stream;
}

QDebug& operator<<(QDebug& dbg, const QtJsonAbstractVariant& v)
{
	if (!v.object)
		dbg << QString("<invalid>");
	else
		dbg << *(v.object);
	return dbg;
}
