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




#ifndef QTJSONABSTRACT_H_
#define QTJSONABSTRACT_H_

#include <QString>
#include <cjson/json.h>
#include <QList>
#include <QVariant>
#include <QDataStream>
#include <QDebug>

class QtJsonAbstract
{

public:

	QtJsonAbstract * add(const QString& k, qint32 v);
	QtJsonAbstract * add(const QString& k, qreal v);
	QtJsonAbstract * add(const QString& k, bool v);
	QtJsonAbstract * add(const QString& k, const QString& v);
	QtJsonAbstract * add(const QString& k, const QList<QVariant>& a);
	QtJsonAbstract * add(const QString& k, const QtJsonAbstract& object);
	static QtJsonAbstract * add(QtJsonAbstract * p_target,const QString& k, const QList<QVariant>& a);

	QVariant		extract(const QString& k) const;
	QList<QPair<QString,QVariant> >	extractAll() const;
	typedef QList<QPair<QString,QVariant> > ContentsList;
	typedef QList<QPair<QString,QVariant> >::iterator ContentsListIterator;
	typedef QList<QPair<QString,QVariant> >::const_iterator ContentsListConstIterator;

	QtJsonAbstract * clone() const;
	QString			toString();

	static QtJsonAbstract * create();
	static QtJsonAbstract * jsonParse(const QString& s);
	static QtJsonAbstract * jsonParseFromStream(QDataStream& stream);

	static void throwAway(QtJsonAbstract * p_object);

	mutable json_object * o;

	friend QDataStream& operator<<(QDataStream &, const  QtJsonAbstract& );
	friend QDataStream& operator>>(QDataStream &, QtJsonAbstract& );
	friend QDebug& operator<<(QDebug& , const QtJsonAbstract& );

private:
	QtJsonAbstract() : o(0) {}
	QtJsonAbstract(json_object * obj) : o(obj) {}
	QtJsonAbstract(const QtJsonAbstract& c);
	QtJsonAbstract& operator=(const QtJsonAbstract& c);
	~QtJsonAbstract();

	static json_object * _array(const QList<QVariant>& a);
	static QVariant _extractArray(json_object * p_o);
};

QDataStream& operator<<(QDataStream &, const  QtJsonAbstract& );
QDataStream& operator>>(QDataStream &, QtJsonAbstract& );

QDebug& operator<<(QDebug& , const QtJsonAbstract& );

class QtJsonAbstractVariant
{
public:

	QtJsonAbstractVariant();
	QtJsonAbstractVariant(const QtJsonAbstract& obj);
	QtJsonAbstractVariant(QtJsonAbstract * p_obj);
	QtJsonAbstractVariant(const QtJsonAbstractVariant& c);
	~QtJsonAbstractVariant();

	mutable QtJsonAbstract * object;

	friend QDataStream& operator<<(QDataStream &, const  QtJsonAbstractVariant& );
	friend QDataStream& operator>>(QDataStream &, QtJsonAbstractVariant& );
	friend QDebug& operator<<(QDebug& dbg, const QtJsonAbstractVariant& );

	static const QtJsonAbstractVariant& initMetaType() {
		return s_initObject;
	}

private:
	QtJsonAbstractVariant(bool init);
	static QtJsonAbstractVariant s_initObject;

};
QDataStream& operator<<(QDataStream &, const  QtJsonAbstractVariant& );
QDataStream& operator>>(QDataStream &, QtJsonAbstractVariant& );
QDebug& operator<<(QDebug& , const QtJsonAbstractVariant& );

Q_DECLARE_METATYPE(QtJsonAbstractVariant)

static const QtJsonAbstractVariant initVariantTypeObject = QtJsonAbstractVariant::initMetaType();

#endif /* QTJSONABSTRACT_H_ */
