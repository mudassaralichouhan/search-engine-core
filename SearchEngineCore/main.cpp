
#include<stdio.h>
#include<stdlib.h>
#include<iostream>
#include<fstream>
#include<string>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>

using namespace std;



 //mongo db
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
using namespace bsoncxx::v_noabi::builder::stream;


// Function to parse query string parameters
map<string, string> parseQueryString(const string& queryString) {
    map<string, string> params;
    size_t pos = 0;
    while ((pos = queryString.find('&')) != string::npos) {
        string key = queryString.substr(0, pos);
        string value = queryString.substr(pos + 1, queryString.find('=', pos) - pos - 1);
        params[key] = value;
        // queryString = queryString.substr(pos + 1);
    }
    if (!queryString.empty()) {
        string key = queryString.substr(0, queryString.find('='));
        string value = queryString.substr(queryString.find('=') + 1);
        params[key] = value;
    }
    return params;
}

// Function to validate email address
bool isValidEmail(const string& email) {
    // Implement a more robust email validation algorithm if needed
    return email.find('@') != string::npos;
}

// Function to subscribe email to a mailing list (replace with your actual implementation)
bool subscribeEmail(const string& email) {
    // Logic to add the email to your mailing list
    // For example, you could write the email to a file or database

    mongocxx::instance instance;

    mongocxx::uri uri("mongodb://localhost:27017");
    mongocxx::client client(uri);
    auto database = client["search-engine"];
    auto collection = database["news-subscriber"];

    std::string* strPtr = new std::string(email);
    const char* emailChars = strPtr->c_str();
  
    auto filter = document{} << "email" << emailChars << finalize;
    auto count = collection.count_documents(filter.view());

    if (count == 0)
    {
        auto result = collection.insert_one(make_document(kvp("email", emailChars)));
        delete strPtr;
        return true;
    }
    else {
        delete strPtr;
        return false;
    }
}
std::string getEnvVarValue(const char* envVarName) {
    char* buf = nullptr;
    size_t sz = 0;

    if (_dupenv_s(&buf, &sz, envVarName) == 0 && buf != nullptr) {
        std::string value(buf);
        free(buf);
        return value;
    }
    else {
        return "";
    }
}

bool caseInsensitiveCompare(const std::string& str1, const std::string& str2) {
    // Convert both strings to lowercase for case-insensitive comparison
    std::string lowerStr1 = str1;
    std::transform(lowerStr1.begin(), lowerStr1.end(), lowerStr1.begin(), ::tolower);

    std::string lowerStr2 = str2;
    std::transform(lowerStr2.begin(), lowerStr2.end(), lowerStr2.begin(), ::tolower);

    // Compare the lowercase strings
    return lowerStr1 == lowerStr2;
}


// (int argc, char* argv[], char* envp[])
int main(int argc, wchar_t* argv[], char* envp[])
{


    string method = getEnvVarValue("REQUEST_METHOD");
    string path = getEnvVarValue("PATH_INFO");


    //    int i = 0;
    //while (envp[i] != NULL)
    //{
    //    std::cout << "Status: 400\r\n";
    //    std::cout << "Content-type: text/html\r\n\r\n";
    //    std::cout << "[" << envp[i++] << "]\n";

    //}

    if (method == "POST") {

        if (caseInsensitiveCompare(path, "/v2/email-subscribe")) {

            // Retrieve POST data
            string postData = getEnvVarValue("QUERY_STRING");
            map<string, string> params = parseQueryString(postData);

            // Extract email from POST data
            string email = params["email"];

            // Validate email address
            if (!isValidEmail(email)) {
                // Handle invalid email (e.g., send an error response)
                cout << "Status: 400\r\n";
                cout << "Content-type: text/html\r\n\r\n";
                cout << "Invalid email address";
                return 1;
            }

            // Subscribe email to mailing list
            if (subscribeEmail(email)) {
                cout << "Status: 200\r\n";
                cout << "Content-type: text/plain\n\n";
                cout << "Email subscribed successfully";
            }
            else {
                cout << "Status: 400\r\n";
                cout << "Content-type: text/plain\n\n";
                cout << "duplicate";
            }
        }
        else
        {
            cout << "Status: 404\r\n";
            cout << "Content-type: text/plain\n\n";
            cout << "Invalid api address";
            return 1;
        }
    }
    else {
       // Handle unsupported methods (e.g., send a 405 Method Not Allowed response)
       // cout << "Content-type: text/plain\n\n";
       // cout << "Method Not Allowed";


        printf("Content-type:text/html; charset=utf-8\n\n"); //http header : MIME type
        ifstream htmlFile("index.html");
        // Check if the file is open
        if (!htmlFile.is_open()) {
            cerr << "Error opening file!" << endl;
            return 1;
        }

        // Read the file content
        string htmlContent((istreambuf_iterator<char>(htmlFile)),
            istreambuf_iterator<char>());

        htmlFile.close();
        cout << htmlContent;

    }

    return 0;



}



