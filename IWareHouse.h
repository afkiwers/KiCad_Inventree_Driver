/*
 *
 * Copyright (C) 2021 Andre Iwers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef INVENTREE_IWAREHOUSE_H
#define INVENTREE_IWAREHOUSE_H

#include <functional>
#include <cstdio>
#include <iostream>
#include <wx/string.h>

#include <iostream>
#include <iterator>
#include <map>
#include <vector>

using namespace std;

/**
 * This interface is shared between KiCad and the Warehouse -> In this case Inventree
 */
class IWareHouse {
public:
    enum Display {
        STATUS_BAR = 50,
        DIALOG,
        CONSOLE
    };

    enum AvailableOptions {
        PARTS_FILTERS = 100,
        CREDENTIALS,
    };

    virtual ~IWareHouse() = default;

    virtual bool connectToWarehouse(std::map<wxString, wxString> credentials) = 0;

    virtual void searchForParts(std::string searchTerm) = 0;

    virtual void getSelectedPartDetails(int listPos) = 0;

    // Callbacks
    virtual void CallbackForFoundParts(std::function<void(std::vector<wxString>)> f) = 0;

    virtual void
    CallbackForPartDetails(std::function<void(std::map<wxString, wxString>)> f) = 0;

    virtual void CallbackForStatusMessage(std::function<void(wxString, Display)> f) = 0;
};

#endif //INVENTREE_IWAREHOUSE_H