
#include<stdlib.h>
#include<fstream>
#include <string>
#include <map>

using namespace std;

static class HttpUtility
{

    // Function to parse query string parameters
   static map<string, string> parseQueryString(const string& queryString) {
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
   static bool isValidEmail(const string& email) {
        // Implement a more robust email validation algorithm if needed
        return email.find('@') != string::npos;
    }

    // Function to subscribe email to a mailing list (replace with your actual implementation)
   static bool subscribeEmail(const string& email) {
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
};

