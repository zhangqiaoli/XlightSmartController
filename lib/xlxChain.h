/**
 * xlxChain.h - Xlight Chain Object Library - This library creates the linkedlists in working memory (Rules, Schedule, Scenerio etc)
 *
 * Created by Baoshi Sun <bs.sun@datatellit.com>
 * Copyright (C) 2015-2016 DTIT
 * Full contributor list:
 *
 * Documentation:
 * Support Forum:
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Created by Baoshi Sun <bs.sun@datatellit.com>
 *
 * DESCRIPTION
 *
 * ToDo:
 * 1.
**/

#include "xliCommon.h"
#include "LinkedList.h"

//------------------------------------------------------------------
// Chain Class, inherited from Arduino LinkedList base class
// Keep all member functions inside of this header file
//------------------------------------------------------------------
template <typename T>
class ChainClass : public LinkedList<T>
{
public:
 	virtual ListNode<T>* search(uint8_t uid);

private:
};

//------------------------------------------------------------------
// Member Functions
//------------------------------------------------------------------

template<typename T>
ListNode<T>* ChainClass<T>::search(uint8_t uid)
{
  ListNode<T> *tmp = LinkedList<T>::root;
  while(tmp!=false)
	{
		if(tmp->data.uid == uid) {
      return tmp;
    }
		tmp=tmp->next;
	}
  return NULL;
}
