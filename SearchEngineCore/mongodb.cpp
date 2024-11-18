#include "mongodb.h"
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
using namespace std;

    // Function to subscribe email to a mailing list (replace with your actual implementation)
bool mongodb::subscribeEmail(const string& email)
{
    // Logic to add the email to your mailing list
// For example, you could write the email to a file or database
    try
    {


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
        catch (const mongocxx::exception& e) {
        return false;
    }
    //catch (const std::exception& e)
    //{
    //    std::cout << "An exception occurred: " << e.what() << "\n";
    //    return false;
    //}
}
