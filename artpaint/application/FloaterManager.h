/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef	_FLOATER_MANAGER_H
#define	_FLOATER_MANAGER_H

#include <vector>
#include <Window.h>


using namespace std;

class FloaterManager {
public:
	static	void	ToggleFloaterVisibility();
	static	void	AddFloater(BWindow* floater);
	static	void	RemoveFloater(BWindow* floater);


private:
	static	bool	floatersVisible;
	static	vector<BWindow*>	floaterVector;
};


#endif	// _FLOATER_MANAGER_H
