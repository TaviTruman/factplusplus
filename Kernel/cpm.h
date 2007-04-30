/* This file is part of the FaCT++ DL reasoner
Copyright (C) 2006 by Dmitry Tsarkov

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

#ifndef _CPM_H
#define _CPM_H

#include "globaldef.h"
#include "cppi.h"

/// console-based progress monitor
class ConsoleProgressMonitor: public TProgressMonitor
{
protected:
		/// real indication
	CPPI ind;

public:
		/// empty c'tor
	ConsoleProgressMonitor ( void ) {}
		/// empty d'tor
	virtual ~ConsoleProgressMonitor ( void ) {}

	// interface

		/// informs about beginning of classification with number of concepts to be classified
	virtual void setClassificationStarted ( unsigned int nConcepts ) { ind.setLimit(nConcepts); }
		/// informs about beginning of classification of a given CONCEPT
	virtual void setCurrentClass ( const char* name ATTR_UNUSED ) { ind.incIndicator(); }
}; // ConsoleProgressMonitor

#endif
