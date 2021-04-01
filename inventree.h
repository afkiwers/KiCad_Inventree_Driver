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

#ifndef WAREHOUSE_INTERFACE_H
#define WAREHOUSE_INTERFACE_H

#include <functional>
#include <cstdio>
#include <utility>
#include <string>
#include <iostream>
#include <wx/string.h>
#include <wx/image.h>
#include <curl/curl.h>

#include "IWareHouse.h"

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/http_listener.h>   // HTTP server
#include <cpprest/json.h>            // JSON library
#include <cpprest/uri.h>             // URI library
#include <cpprest/ws_client.h>       // WebSocket client
#include <cpprest/containerstream.h> // Async streams backed by STL containers
#include <cpprest/interopstream.h> // Bridges for integrating Async streams with STL and WinRT streams
#include <cpprest/rawptrstream.h>           // Async streams backed by raw pointer to memory
#include <cpprest/producerconsumerstream.h> // Async streams for producer consumer scenarios

using namespace utility;              // Common utilities like string conversions
using namespace web;                  // Common features like URIs.
using namespace web::http;            // Common HTTP functionality
using namespace web::http::client;    // HTTP client features

using namespace web::json;            // JSON library
using namespace pplx;                 // PPL interfaces


/**
 * A structure to represent a part stock location from Inventree
 * The api responses with a JSON structure which is captured in this struct.
 */
struct STOCK_LOCATION {
    // this struct is a template of the api response when querying locations
    STOCK_LOCATION(int pk, int parent, int items, wxString url, wxString name,
                   wxString description, wxString pathstring) {
        m_pk = pk;
        m_parent = parent;
        m_items = items;
        m_url = url;
        m_name = name;
        m_description = description;
        m_pathstring = pathstring;
    }

    int m_pk;
    int m_parent;
    int m_items;
    wxString m_url;
    wxString m_name;
    wxString m_description;
    wxString m_pathstring;
};


/**
 * A structure to represent a part parameter template from Inventree
 * The api responses with a JSON structure which is captured in this struct.
 */
struct TEMPLATE_PARAMETER {
    // this struct
    TEMPLATE_PARAMETER(int pk, wxString name, wxString units) {
        m_pk = pk;
        m_name = name;
        m_units = units;
    }

    int m_pk;
    wxString m_name;
    wxString m_units;
};

/**
 * A structure to represent a part parameter from Inventree
 * The api responses with a JSON structure which is captured in this struct.
 */
struct PART_PARAMETER {
    /**
 * Search function to get the name
 * @param tp list with all available template parameters
 * @param pk primary key of template parameter
 */
    std::map<wxString, wxString> findTemplateName(std::vector<TEMPLATE_PARAMETER> tp, int pk) {
        std::map<wxString, wxString> map;

        for (auto &t : tp) {
            if (t.m_pk == pk) {
                map["name"] = t.m_name;
                map["units"] = t.m_units;
                break;
            }
        }

        return map;
    };

    // this struct is a template of the api response when querying locations
    PART_PARAMETER(int pk, int part, int template_pk, wxString data,
                   std::vector<TEMPLATE_PARAMETER> partTemplates) {
        // get name and units associated with template pk
        std::map<wxString, wxString> temp = findTemplateName(partTemplates, template_pk);
        m_template = temp["name"];
        m_units = temp["units"];

        m_pk = pk;
        m_part_pk = part;
        m_data = data;
    }

    int m_pk;
    int m_part_pk;
    wxString m_template;
    wxString m_data;

    wxString m_units;
};

/**
 * A structure to represent a part parameter from Inventree
 * The api responses with a JSON structure which is captured in this struct.
 */
struct PART_ATTRIBUTE {
    /**
    * Search function to get the name
    * @param tp list with all available template parameters
    * @param pk primary key of template parameter
    */
    std::map<wxString, wxString> findLocationDescription(std::vector<STOCK_LOCATION> sL, int pk) {
        std::map<wxString, wxString> map;

        for (auto &t : sL) {
            if (t.m_pk == pk) {
                map["name"] = t.m_name;
                map["description"] = t.m_description;
                break;
            }
        }

        return map;
    };

    // this struct is a template of the api response when querying locations
    PART_ATTRIBUTE(wxString name, wxString value, std::vector<STOCK_LOCATION> stockLocations) {
        m_name = name;
        m_value = value;

        try {
            if (name == "default_location") {
                std::map<wxString, wxString> loc = findLocationDescription(
                        std::move(stockLocations), atoi(value.c_str()));

                m_value = loc["name"] + " ->> " + loc["description"];
            } else if (name == "category") {
                //            std::map<wxString, wxString> loc =
                //                    findLocationDescription( stockLocations, atoi( value.c_str() ) );
                //
                //            m_value = loc["name"] + ":" + loc["description"];
            }
        }
        catch (...) {
            // do nothing...
        }
    }

    wxString m_name;
    wxString m_value;
};


/*!
  Downloads an image from a specified online source and saves it in a dummy file locally
  @param[in] url URL to image source
  @return bool returns true if file was downloaded and saved successfully
  @author Andre Iwers
  */
bool downloadImagesFile(std::string url);


/*!
  Callback function for downloadImagesFile(...)
  @author Andre Iwers
  */
size_t callbackFunctionWriteFile(void *ptr, size_t size, size_t nmemb, void *userdata);


/*! This Interface allows KiCAD to communicate with Inventree and open-source warehouse application
 * Inventree GitHub project can be found here:  https://github.com/inventree
 * */
class INVENTREE_DRIVER : public IWareHouse {
public:
    INVENTREE_DRIVER();

    virtual ~INVENTREE_DRIVER() override;

private:

    void CallbackForFoundParts(std::function<void(std::vector<wxString>)> f) override;

    void CallbackForPartDetails(std::function<void(std::map<wxString, wxString>)> f) override;

    void CallbackForStatusMessage(
            std::function<void(const wxString &, const wxString &, Display)> f) override;

    bool connectToWarehouse(std::map<wxString, wxString> credentials) override;

    std::map<wxString, wxString> getWareHouseDetails() override;

    void getAuthToken(const std::string &username, const std::string &password);

    void searchForParts(std::string searchTerm) override;

    void getSelectedPartDetails(int listPos) override;

    void getPartParameters(int jsonResponse);

    void getPartAttributes(int pk);

    void getInvenTreeVersion();

    void getAllParameterTemplates();

    void getAllStockLocations();

    bool visibleAttributes(const wxString &term);

    wxString formatNameString(wxString text);

    // general methods to evaluate server responses
    pplx::task<json::value> evaluateServerResponse(http_response response);

    json::value evaluateJSONResponse(pplx::task<json::value> jsonResponse);

    /*!
      Removes the quotation marks which are used by the API to embed string type data
      @param[in] str
      @return bool returns a trimmed wxString if there are quotation marks available. Otherwise, returns the same string as a wxString which was inputted.
      @author Andre Iwers
      */
    wxString removeQuotationMarks(std::string str);

    wxString m_apiToken;
    std::vector<web::json::value> m_foundParts;

    std::vector<TEMPLATE_PARAMETER> m_parameterTemplates;
    std::vector<STOCK_LOCATION> m_stockLocations;
    std::vector<PART_PARAMETER> m_partParameters;
    std::vector<PART_ATTRIBUTE> m_partAttributes;
    std::map<wxString, wxString> APIVersion;

    // URL to warehouse API
    std::string serverURL = "http://192.168.10.8:9080";
    std::string apiURL = serverURL + "/api/";


    std::function<void(std::vector<wxString>)> fCallbackDisplayFoundParts;
    std::function<void(std::map<wxString, wxString>)> fCallbackDisplayPartParameters;
    std::function<void(const wxString &, const wxString &,
                       IWareHouse::Display)> fCallbackDisplayStatusMessage;

};


#endif /* WAREHOUSE_INTERFACE_H */
