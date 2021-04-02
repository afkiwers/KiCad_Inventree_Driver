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
        _STATUS_BAR = 50,
        _ERROR_DIALOG,
        _INFO_DIALOG,
        _CONSOLE
    };

    enum DriverOptions {
        _PART_PARAMETER_FILTER = 100,
        _CREDENTIALS
    };

    virtual ~IWareHouse() = default;

    /*
     * To tell various drivers apart, each driver must store it's assigned driver ID internally
     * and return it with the search results
     * */
    virtual bool connectToWarehouse(std::map<wxString, wxString> args, int driverID) = 0;

    virtual std::map<wxString, wxString> getWareHouseDetails() = 0;

    virtual void searchForParts(std::string searchTerm) = 0;

    virtual void getSelectedPartDetails(int listPos) = 0;

    virtual std::map<wxString, std::vector<wxString>> getFilters() = 0;

    /*
    * This method is reports back the drivers capabilities and requirements such as filters or credentials etc.
    * */
    virtual std::vector<DriverOptions> getDriverOptions() = 0;

    // Callbacks
    virtual void CallbackForFoundParts(std::function<void(std::vector<wxString>, int)> f) = 0;

    virtual void
    CallbackForPartDetails(std::function<void(std::map<wxString, wxString>)> f) = 0;

    virtual void CallbackForStatusMessage(
            std::function<void(const wxString &, const wxString &, Display)> f) = 0;
};

#endif //INVENTREE_IWAREHOUSE_H
