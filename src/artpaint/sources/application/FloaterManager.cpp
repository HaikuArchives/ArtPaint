/* 

	Filename:	FloaterManager.cpp
	Contents:	FloaterManager-class definition
	Author:		Heikki Suhonen
	
*/


#include <algorithm>

#include "FloaterManager.h"

vector<BWindow*> FloaterManager::floaterVector;

bool FloaterManager::floatersVisible = true;


void FloaterManager::ToggleFloaterVisibility()
{
	floatersVisible = !floatersVisible;
	
	vector<BWindow*>::iterator floater_iterator = floaterVector.begin();
	
	if (floatersVisible) {
		while (floater_iterator < floaterVector.end()) {
			(*floater_iterator)->Lock();
			if ((*floater_iterator)->IsHidden()) {
				(*floater_iterator)->Show();
			}
			(*floater_iterator)->Unlock();
			floater_iterator++;
		}
	}
	else {
		while (floater_iterator < floaterVector.end()) {
			(*floater_iterator)->Lock();
			if ((*floater_iterator)->IsHidden() == false) {
				(*floater_iterator)->Hide();
			}
			(*floater_iterator)->Unlock();
			floater_iterator++;
		}
	}

}



void FloaterManager::AddFloater(BWindow *floater)
{
	vector<BWindow*>::iterator iter 
				= find(floaterVector.begin(),floaterVector.end(),floater);
				
	if (iter == floaterVector.end()) {
		floaterVector.push_back(floater);			
	}
}


void FloaterManager::RemoveFloater(BWindow *floater)
{
	vector<BWindow*>::iterator iter
				= find(floaterVector.begin(),floaterVector.end(),floater);

	if (iter < floaterVector.end()) {
		floaterVector.erase(iter);
	}	
}