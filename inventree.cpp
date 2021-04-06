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

#include "inventree.h"
# include "IWareHouse.h"

#if defined(__linux__) || defined(__APPLE__)
extern "C"
{
INVENTREE_DRIVER *allocator() {
    return new INVENTREE_DRIVER();
}

void deleter(INVENTREE_DRIVER *ptr) {
    delete ptr;
}
}
#endif

#ifdef WIN32
extern "C"
{
    __declspec (dllexport) INVENTREE_DRIVER *allocator()
    {
        return new INVENTREE_DRIVER();
    }

    __declspec (dllexport) void deleter(INVENTREE_DRIVER *ptr)
    {
        delete ptr;
    }
}
#endif

INVENTREE_DRIVER::INVENTREE_DRIVER() = default;

INVENTREE_DRIVER::~INVENTREE_DRIVER() = default;

bool INVENTREE_DRIVER::connectToWarehouse(std::map<wxString, wxString> args, int driverID) {
    std::cout << "connectToWarehouse" << std::endl;

    // save assigned driver ID, this ID is assigned randomly by the caller
    m_driverID = driverID;

    if (args.empty()) {

//        fCallbackDisplayStatusMessage(
//                "No credentials defined! Inventree needs credentials to allow access to the "
//                "database",
//                "Initialisation Error", IWareHouse::Display::_ERROR_DIALOG);

        return false;
    }

    // construct server url
    m_ServerURL = wxString::Format("%s:%s", args["server_url"], args["server_port"]).ToStdString();
    m_ApiURL = m_ServerURL + "/api/";

    // request auth token from warehouse API
    wxString username = args["username"];
    wxString password = args["password"];

    getInvenTreeVersion();

    getAuthToken(username.ToStdString(), password.ToStdString());

    if (m_apiToken.empty()) {
        return false;
    }

    return true;
}

void INVENTREE_DRIVER::getSelectedPartParameters(int listPos) {
    std::map<wxString, wxString> attributes;

    // Create iterator pointing to first element of list
    auto it = m_foundParts.begin();

    // Advance the iterator by listPos positions,
    std::advance(it, listPos);

    // Now iterator it points to listPos element -> assign to a new variable
    json::value part = *it;

    try {
        int pk = part[U("pk")].as_integer();

        // query attributes and parameters
        getPartAttributes(pk);
        getPartParameters(pk);

        // download image from inventree
        if (!downloadImagesFile(m_ServerURL + part[U("image")].as_string())) {
            std::cout << "!! Failed to download file:";
        } else {
            std::cout << "Load images from file..." << std::endl;
        }

        // map received data in vector
        std::map<wxString, wxString> params;
        for (const auto &p : m_partParameters) {
            params[formatNameString(p.m_template)] = p.m_data + " " + p.m_units;
        }

        for (const auto &a : m_partAttributes) {
            if (visibleAttributes(a.m_name))
                params[formatNameString(a.m_name)] = a.m_value;
        }

        fCallbackDisplayPartParameters(params, m_driverID);
    }
    catch (...) {
        std::cout << "Error occurred" << std::endl;
    }
}

std::vector<IWareHouse::WareHouseOptions> INVENTREE_DRIVER::wareHouseOptions() {
    std::vector<IWareHouse::WareHouseOptions> options;

    options.push_back(IWareHouse::WareHouseOptions::_CREDENTIALS);
    options.push_back(IWareHouse::WareHouseOptions::_PART_PARAMETER_FILTER);
    options.push_back(IWareHouse::WareHouseOptions::_SERVER_SETTINGS);
    options.push_back(IWareHouse::WareHouseOptions::_ADD_PART_TO_WAREHOUSE);

    return options;
}

std::map<wxString, std::vector<wxString>> INVENTREE_DRIVER::Filters() {

    // TODO: This is just a mockup and needs to be coded properly
    std::map<wxString, std::vector<wxString>> filters;

    std::vector<wxString> values = {"value_1", "value_2", "value_3", "value_4", "value_5", "value_6"};

    filters["filter1"] = values;
    filters["filter2"] = values;
    filters["filter3"] = values;
    filters["filter4"] = values;
    filters["filter5"] = values;
    filters["filter6"] = values;
    filters["filter7"] = values;

    return filters;

}

/***** Inventree HTTP requests ********/
void INVENTREE_DRIVER::getInvenTreeVersion() {
    std::cout << "getInvenTreeVersion" << std::endl;

    // create request, and add header information
    web::http::http_request req(methods::GET);
    req.headers().add(header_names::content_type, http::details::mime_types::application_json);

    // create http client
    http_client client(m_ApiURL);

    client.request(req)
            .then([=](http_response response) {
                // evaluate server response
                return evaluateServerResponse(std::move(response));
            })
            .then([=](pplx::task<json::value> jsonResponse) {
                try {
                    // evaluate JSON response
                    json::value obj = evaluateJSONResponse(std::move(jsonResponse));

                    if (!obj.is_null()) {
                        // add new results to vector
                        for (auto &attr : obj.as_object()) {
                            APIVersion[attr.first] = removeQuotationMarks(attr.second.serialize());
                        }
                    }
                }
                catch (http_exception const &e) {
//                    fCallbackDisplayStatusMessage(e.what(), "getInvenTreeVersion()",
//                                                  IWareHouse::Display::_ERROR_DIALOG);
                }
            })
            .wait();
}

void INVENTREE_DRIVER::getAuthToken(const std::string &username, const std::string &password) {
    web::credentials cred(username, password); // WinHTTP requires non-empty password

    std::cout << "getAuthToken" << std::endl;

    http_client_config client_config;
    client_config.set_credentials(cred);

    // create http client with base url
    http_client client(m_ApiURL + "user/token/", client_config);

    // add request type
    web::http::http_request req(methods::GET);

    // create request, and add header information
    req.headers().add(header_names::content_type, http::details::mime_types::application_json);

    client.request(req)
            .then([=](http_response response) {

                // evaluate server response
                return evaluateServerResponse(std::move(response));
            })
            .then([=](pplx::task<json::value> jsonResponse) {
                try {
                    // evaluate JSON response
                    json::value obj = evaluateJSONResponse(std::move(jsonResponse));

                    if (obj.size()) {
                        m_apiToken = obj[U("token")].as_string();

                        getAllParameterTemplates();

                        getAllStockLocations();

//                        fCallbackDisplayStatusMessage("Connected to InvenTree as: " + username,
//                                                      "Version: " + APIVersion["version"],
//                                                      IWareHouse::Display::_STATUS_BAR);

                    } else {
                        std::cout << "Empty token response..." << std::endl;
                    }
                }
                catch (http_exception const &e) {
//                    fCallbackDisplayStatusMessage(e.what(), "Server Message",
//                                                  IWareHouse::Display::_ERROR_DIALOG);
                }

            })
            .wait();
}

void INVENTREE_DRIVER::searchWareHouseForParts(std::string searchTerm) {
    std::cout << "searchWareHouseForParts" << std::endl;

    // create request, and add header information
    web::http::http_request req(methods::GET);
    req.headers().add(header_names::content_type, http::details::mime_types::application_json);
    req.headers().add("Authorization", "Token " + m_apiToken);
    req.set_request_uri("?search=" + uri::encode_data_string(searchTerm));

    // create http client
    http_client client(m_ApiURL + "part/");

    client.request(req)
            .then([=](http_response response) {
                // evaluate server response
                return evaluateServerResponse(std::move(response));
            })
            .then([=](pplx::task<json::value> jsonResponse) {
                try {
                    // evaluate JSON response
                    json::value obj = evaluateJSONResponse(std::move(jsonResponse));

                    //empty vector to save new results
                    m_foundParts.clear();
                    std::vector<wxString> foundParts;

                    if (!obj.is_null()) {
                        // add new results to vector
                        for (auto &part : obj.as_array()) {
                            m_foundParts.emplace_back(part);
                            foundParts.emplace_back(part.at(U("description")).as_string());
                        }
                    }

                    fCallbackDisplayFoundParts(foundParts, m_driverID);

                }
                catch (http_exception const &e) {
//                    fCallbackDisplayStatusMessage(e.what(), "searchWareHouseForParts()",
//                                                  IWareHouse::Display::_ERROR_DIALOG);

                    // clear parts and update connection status msg
                    m_foundParts.clear();
                }
            })
            .wait();
}

void INVENTREE_DRIVER::getAllParameterTemplates() {
    std::cout << "getAllParameterTemplates" << std::endl;

    // create request, and add header information
    web::http::http_request req(methods::GET);
    req.headers().add(header_names::content_type, http::details::mime_types::application_json);
    req.headers().add("Authorization", "Token " + m_apiToken);

    // create http client
    http_client client(m_ApiURL + "part/parameter/template/");

    client.request(req)
            .then([=](http_response response) {
                // evaluate server response
                return evaluateServerResponse(std::move(response));
            })
            .then([=](pplx::task<json::value> jsonResponse) {
                try {
                    // evaluate JSON response
                    json::value obj = evaluateJSONResponse(std::move(jsonResponse));

                    if (!obj.is_null()) {
                        // convert json object to array
                        json::array templates = obj.as_array();

                        // clear templates
                        m_parameterTemplates.clear();

                        int pk = -1;
                        std::string name;
                        std::string units;

                        // map received templates in global vector for later use
                        for (const auto &iter : templates) {
                            for (const auto &temp : iter.as_object()) {
                                auto &propertyName = temp.first;
                                auto &propertyValue = temp.second;

                                if (propertyName == "pk")
                                    pk = stoi(propertyValue.serialize());

                                if (propertyName == "name")
                                    name = propertyValue.serialize();

                                if (propertyName == "units")
                                    units = propertyValue.serialize();
                            }

                            m_parameterTemplates.emplace_back(
                                    TEMPLATE_PARAMETER(pk, removeQuotationMarks(name),
                                                       removeQuotationMarks(units)));
                        }

                        std::cout << m_parameterTemplates.size() << " template(s) received"
                                  << std::endl;
                    } else {
                        // clear parts and update connection status msg
                        m_foundParts.clear();
                    }
                }
                catch (http_exception const &e) {
//                    fCallbackDisplayStatusMessage(e.what(), "getAllParameterTemplates()",
//                                                  IWareHouse::Display::_ERROR_DIALOG);

                    // clear parts and update connection status msg
                    m_foundParts.clear();
                }
            })
            .wait();
}

void INVENTREE_DRIVER::getAllStockLocations() {
    std::cout << "getAllStockLocations" << std::endl;

    // create request, and add header information
    web::http::http_request req(methods::GET);
    req.headers().add(header_names::content_type, http::details::mime_types::application_json);
    req.headers().add("Authorization", "Token " + m_apiToken);

    // create http client
    http_client client(m_ApiURL + "stock/location/");

    client.request(req)
            .then([=](http_response response) {
                // evaluate server response
                return evaluateServerResponse(std::move(response));
            })
            .then([=](pplx::task<json::value> jsonResponse) {
                try {
                    // evaluate JSON response
                    json::value obj = evaluateJSONResponse(std::move(jsonResponse));

                    if (!obj.is_null()) {
                        // convert json object to array
                        json::array templates = obj.as_array();

                        // clear templates
                        m_stockLocations.clear();

                        int pk = -1;
                        int parent = -1;
                        int items = -1;
                        std::string url;
                        std::string name;
                        std::string description;
                        std::string pathstring;

                        // map received templates in global vector for later use
                        for (const auto &iter : templates) {
                            for (const auto &temp : iter.as_object()) {
                                auto &propertyName = temp.first;
                                auto &propertyValue = temp.second;

                                if (propertyName == "pk")
                                    pk = stoi(propertyValue.serialize());

                                if (propertyName == "parent") {
                                    try {
                                        parent = stoi(propertyValue.serialize());
                                    }
                                    catch (...) {
                                        // do nothing if parent is not a number
                                    }
                                }

                                if (propertyName == "items")
                                    items = stoi(propertyValue.serialize());


                                if (propertyName == "url")
                                    url = propertyValue.serialize();

                                if (propertyName == "name")
                                    name = propertyValue.serialize();

                                if (propertyName == "description")
                                    description = propertyValue.serialize();

                                if (propertyName == "pathstring")
                                    pathstring = propertyValue.serialize();
                            }

                            m_stockLocations.emplace_back(STOCK_LOCATION(
                                    pk, parent, items, url, removeQuotationMarks(name),
                                    removeQuotationMarks(description),
                                    removeQuotationMarks(pathstring)));
                        }

                        std::cout << m_stockLocations.size() << " location(s) received"
                                  << std::endl;
                    } else {
                        // clear parts and update connection status msg
                        m_foundParts.clear();
                    }
                }
                catch (http_exception const &e) {
//                    fCallbackDisplayStatusMessage(e.what(), "getAllStockLocations()",
//                                                  IWareHouse::Display::_ERROR_DIALOG);

                    // clear parts and update connection status msg
                    m_foundParts.clear();
                }
            })
            .wait();
}

void INVENTREE_DRIVER::getPartAttributes(int pk) {
    std::cout << "getPartAttributes" << std::endl;

    // create request, and add header information
    web::http::http_request req(methods::GET);
    req.headers().add(header_names::content_type, http::details::mime_types::application_json);
    req.headers().add("Authorization", "Token " + m_apiToken);

    // create http client
    http_client client(m_ApiURL + "part/" + std::to_string(pk) + "/");

    client.request(req)
            .then([=](http_response response) {
                // evaluate server response
                return evaluateServerResponse(std::move(response));
            })
            .then([=](pplx::task<json::value> jsonResponse) {
                try {
                    // evaluate JSON response
                    json::value obj = evaluateJSONResponse(std::move(jsonResponse));
                    m_partAttributes.clear();

                    if (!obj.is_null()) {
                        // extract attributes and store in vector
                        for (auto &attr : obj.as_object()) {
                            auto &propertyName = attr.first;
                            auto &propertyValue = attr.second;

                            m_partAttributes.emplace_back(PART_ATTRIBUTE(
                                    removeQuotationMarks(propertyName),
                                    removeQuotationMarks(propertyValue.serialize()),
                                    m_stockLocations));
                        }
                    }
                }
                catch (http_exception const &e) {
                    m_foundParts.clear();

//                    fCallbackDisplayStatusMessage(e.what(), "getPartAttributes()",
//                                                  IWareHouse::Display::_ERROR_DIALOG);
                }
            })
            .wait();
}

void INVENTREE_DRIVER::getPartParameters(int pk) {
    std::cout << "getPartParameters" << std::endl;

    // create request, and add header information
    web::http::http_request req(methods::GET);
    req.headers().add(header_names::content_type, http::details::mime_types::application_json);
    req.headers().add("Authorization", "Token " + m_apiToken);
    req.set_request_uri("?part=" + std::to_string(pk));

    // create http client
    http_client client(m_ApiURL + "part/parameter/");

    client.request(req)
            .then([=](http_response response) {
                // evaluate server response
                return evaluateServerResponse(std::move(response));
            })
            .then([=](pplx::task<json::value> jsonResponse) {
                try {
                    // evaluate JSON response
                    json::value obj = evaluateJSONResponse(std::move(jsonResponse));
                    m_partParameters.clear();

                    if (!obj.is_null()) {
                        // extract attributes and store in vector
                        for (auto &param : obj.as_array()) {
                            int _pk = 1;
                            int _part = 1;
                            int _template = 1;
                            std::string _data;

                            for (const auto &field : param.as_object()) {
                                auto &propertyName = field.first;
                                auto &propertyValue = field.second;

                                if (propertyName == "pk")
                                    _pk = std::stoi(propertyValue.serialize());

                                if (propertyName == "part")
                                    _part = std::stoi(propertyValue.serialize());

                                if (propertyName == "template")
                                    _template = std::stoi(propertyValue.serialize());

                                if (propertyName == "data") {
                                    // remove " charcater from api response
                                    std::string buf = propertyValue.serialize();
                                    buf.erase(buf.begin());
                                    buf.erase(buf.end() - 1);
                                    _data = buf;
                                }
                            }

                            m_partParameters.emplace_back(PART_PARAMETER(
                                    _pk, _part, _template, removeQuotationMarks(_data),
                                    m_parameterTemplates));
                        }
                    }
                }
                catch (http_exception const &e) {
//                    fCallbackDisplayStatusMessage(e.what(), "getPartParameters()",
//                                                  IWareHouse::Display::_ERROR_DIALOG);
                }
            })
            .wait();
}

bool INVENTREE_DRIVER::addPartToWareHouse(std::map<wxString, wxString> parameters) {

    // TODO: do something....
    for (auto &p : parameters) {
        std::cout << p.first << ": " << p.first << std::endl;
    }

    return true;
}


/***** General helper functions ********/
size_t callbackFunctionWriteFile(void *ptr, size_t size, size_t nmemb, void *userdata) {
    FILE *stream = (FILE *) userdata;
    if (!stream) {
        std::cout << "!!! No stream" << std::endl;
        return 0;
    }

    try {
        size_t written = fwrite((FILE *) ptr, size, nmemb, stream);

        std::cout << "File has been saved..." << std::endl;

        return written;
    }
    catch (...) {
        std::cout << "error" << std::endl;
    }

    return 0;
}

bool downloadImagesFile(std::string url) {
    FILE *fp = fopen("part_image.tmpfile", "wb");
    if (!fp) {
        std::cout << "!!! Failed to create file on the disk" << std::endl;
        return false;
    }

    CURL *curlCtx = curl_easy_init();
    curl_easy_setopt(curlCtx, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curlCtx, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curlCtx, CURLOPT_WRITEFUNCTION, callbackFunctionWriteFile);
    curl_easy_setopt(curlCtx, CURLOPT_FOLLOWLOCATION, 1);

    CURLcode rc = curl_easy_perform(curlCtx);
    if (rc) {
        std::cout << "!!! Failed to download: " << url.c_str() << std::endl;
        return false;
    }

    long res_code = 0;
    curl_easy_getinfo(curlCtx, CURLINFO_RESPONSE_CODE, &res_code);
    if (!((res_code == 200 || res_code == 201) && rc != CURLE_ABORTED_BY_CALLBACK)) {
        std::cout << "!!! Response code: " << res_code << std::endl;
        return false;
    }

    curl_easy_cleanup(curlCtx);

    fclose(fp);

    return true;
}

/***** General evaluation functions ********/
pplx::task<json::value> INVENTREE_DRIVER::evaluateServerResponse(http_response response) {
    if (response.status_code() == status_codes::OK) {
        return response.extract_json();
    }

//    fCallbackDisplayStatusMessage(
//            wxString::Format("Error during communication with InvenTree.\nError Code: %d ( %s )",
//                             response.status_code(), response.reason_phrase()),
//            "Server Response", IWareHouse::Display::_ERROR_DIALOG);

    return pplx::task_from_result(json::value());
}

json::value INVENTREE_DRIVER::evaluateJSONResponse(pplx::task<json::value> jsonResponse) {
    json::value obj = jsonResponse.get();
    if (!obj.is_null()) {
        return obj;
    }

//    fCallbackDisplayStatusMessage(
//            "Server did not respond as expected. Not sure what to do... I think I am cooked... I am so sorry...",
//            "Server Response", IWareHouse::Display::_ERROR_DIALOG);

    return json::value();
}

wxString INVENTREE_DRIVER::removeQuotationMarks(std::string str) {
    // Check if " is present in first position and if so delete the on at the front and at the end
    // of the string
    if (str.find('\"') == 0) {
        str.erase(str.begin());
        str.erase(str.end() - 1);
    }

    return str;
}

bool INVENTREE_DRIVER::visibleAttributes(const wxString &term) {
    std::vector<wxString> selector = {
            "description", "default_location", "full_name", "in_stock", "link", "notes", "pk"
    };

    for (auto &s : selector) {
        if (s == term)
            return true;
    }

    return false;
}

wxString INVENTREE_DRIVER::formatNameString(wxString text) {
    text.Replace("_", " ");

    for (int x = 0; x < text.length(); x++) {
        if (x == 0)
            text[x] = toupper(text[x]);
        else if (text[x - 1] == ' ')
            text[x] = toupper(text[x]);

    }

    return text;
}

wxString INVENTREE_DRIVER::wareHouseShortDescription() {

    return "Open Source Inventory Management System";
//    return "Dummy Driver Which Connects To InvenTree";
}

wxString INVENTREE_DRIVER::driverVersion() {

    return "0.0.1 pre";
//    return "0.2.1 pre";
}

/***** Callback functions ********/
void INVENTREE_DRIVER::CallbackForFoundParts(std::function<void(std::vector<wxString>, int)> f) {
    fCallbackDisplayFoundParts = f;
}

void INVENTREE_DRIVER::CallbackForPartDetails(std::function<void(std::map<wxString, wxString>, int)> f) {
    fCallbackDisplayPartParameters = f;
}

void INVENTREE_DRIVER::CallbackForStatusMessage(
        std::function<void(const wxString &, const wxString &, IWareHouse::Display)> f) {
    fCallbackDisplayStatusMessage = f;
}

