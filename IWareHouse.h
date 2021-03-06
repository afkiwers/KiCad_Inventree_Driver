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

    enum WareHouseOptions {
        _PART_PARAMETER_FILTER = 100,
        _CREDENTIALS,
        _SERVER_SETTINGS,
        _ADD_PART_TO_WAREHOUSE
    };

    virtual ~IWareHouse() = default;

    /*
     * To tell various drivers apart, each driver must store it's assigned driver ID internally
     * and return it with the search results
     * */
    virtual bool connectToWarehouse(std::map<wxString, wxString> args, int driverID) = 0;

    /*
     * If several drivers are used and this warehouse does not have a part which is available from
     * the Mouser or DigiKey driver, This function will take the parameter and create one.
     * */
    virtual bool addPartToWareHouse(std::map<wxString, wxString> parameters) = 0;

    /*
     * A short description of the driver which will be shown to the user
     * */
    virtual wxString wareHouseShortDescription() = 0;

    /*
     * The version of the driver which will be shown to the user
     * */
    virtual wxString driverVersion() = 0;

    virtual void searchWareHouseForParts(std::string searchTerm) = 0;

    virtual void getSelectedPartParameters(int listPos) = 0;

    virtual std::map<wxString, std::vector<wxString>> Filters() = 0;

    /*
    * This method is reports back the drivers capabilities and requirements such as filters or credentials etc.
    * */
    virtual std::vector<WareHouseOptions> wareHouseOptions() = 0;

    // Callbacks
    virtual void CallbackForFoundParts(std::function<void(std::vector<wxString>, int)> f) = 0;

    virtual void
    CallbackForPartDetails(std::function<void(std::map<wxString, wxString>, int)> f) = 0;

    virtual void CallbackForStatusMessage(
            std::function<void(const wxString &, const wxString &, Display)> f) = 0;
};

#endif //INVENTREE_IWAREHOUSE_H
