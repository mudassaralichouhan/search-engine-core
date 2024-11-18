
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
#include "mongodb.h"
using namespace std;






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
            if (mongodb().subscribeEmail(email)) {
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



