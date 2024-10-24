
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
    ofstream outfile("subscribers.txt", ios::app);
    if (outfile.is_open()) {
        outfile << email << endl;
        outfile.close();
        return true;
    }
    else {
        return false;
    }
}



int main(int argc, wchar_t* argv[])
{

    string method = getenv("REQUEST_METHOD");

    if (method == "POST") {
        // Retrieve POST data
        string postData = getenv("QUERY_STRING");
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
            // Handle successful subscription (e.g., send a success message)
            cout << "Content-type: text/plain\n\n";
            cout << "Email subscribed successfully";
        }
        else {
            // Handle subscription failure (e.g., send an error message)
            cout << "Content-type: text/plain\n\n";
            cout << "Error subscribing email";
        }
    }
    else {
       // Handle unsupported methods (e.g., send a 405 Method Not Allowed response)
       // cout << "Content-type: text/plain\n\n";
       // cout << "Method Not Allowed";


        printf("Content-type:text/html; charset=utf-8\n\n"); //http header : MIME type
        ifstream htmlFile("index_file.html");
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



