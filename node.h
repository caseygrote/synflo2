/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#ifdef NOT_USERSPACE
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo.h>
#include <sifteo/menu.h>

namespace Sifteo {

	class Node{
public: 
	Node(){};
	Node(MenuItem* itemList, MenuAssets* assetList, unsigned levelInt){
		items = itemList;
		level = levelInt;
		assets = assetList;
	}

	void setChildren(Node* kiddies){
		children = kiddies;
	}

	Node* getChildren(){
		return children;
	}

	void setAssets(MenuAssets* assetList){
		assets = assetList;
	}

	MenuAssets* getAssets(){
		return assets;
	}

	void setMenu(MenuItem* itemList){
		items = itemList;
	}

	MenuItem* getMenu(){
		return items;
	}

	unsigned getLevel(){
		return level;
	}

private:
	MenuItem* items;
	MenuAssets* assets;
	unsigned level;
	Node* children;
};

}
