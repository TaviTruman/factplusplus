/* This file is part of the FaCT++ DL reasoner
Copyright (C) 2003-2007 by Dmitry Tsarkov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "DataTypeCenter.h"
#include "DataReasoning.h"

DataTypeCenter :: ~DataTypeCenter ( void )
{
	for ( iterator p = begin(), p_end = end(); p < p_end; ++p )
		delete *p;
}

TDataType*
DataTypeCenter :: getTypeByName ( const std::string& name ) const
{
	// special case whatever you want here

	// go through all types and check the name
	for ( const_iterator p = begin(), p_end = end(); p < p_end; ++p )
		if ( name == (*p)->getType()->getName() )
			return *p;

	return NULL;
}

void
DataTypeCenter :: setLocked ( bool val )
{
	if ( val )
		return;

	for ( iterator p = begin(), p_end = end(); p < p_end; ++p )
		(*p)->setLocked(val);
}

void
DataTypeCenter :: initDataTypeReasoner ( DataTypeReasoner& DTReasoner ) const
{
	for ( const_iterator p = begin(), p_end = end(); p < p_end; ++p )
	{
		TDataEntry* type = (*p)->getType();
		if ( isValid(type->getBP()) )
			DTReasoner.registerDataType(type);
	}
}
