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
#include "infrastructure.h"
#include <mutex>
using namespace bsoncxx::v_noabi::builder::stream;
using namespace std;

// Singleton class to manage MongoDB instance
class MongoDBInstance {
private:
    static std::unique_ptr<mongocxx::instance> instance;
    static std::mutex mutex;

public:
    static mongocxx::instance& getInstance() {
        std::lock_guard<std::mutex> lock(mutex);
        if (!instance) {
            instance = std::make_unique<mongocxx::instance>();
        }
        return *instance;
    }
};

// Initialize static members
std::unique_ptr<mongocxx::instance> MongoDBInstance::instance;
std::mutex MongoDBInstance::mutex;

// Function to subscribe email to a mailing list
Result<bool> mongodb::subscribeEmail(const string& email)
{
    // Logic to add the email to your mailing list
    try
    {
        // Use the singleton instance instead of creating a new one
        mongocxx::instance& instance = MongoDBInstance::getInstance();
        
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
            return Result<bool>::Success(true, "registered");
        }
        else {
            delete strPtr;
            return Result<bool>::Failure("duplicate");
        }
    }
    catch (const mongocxx::exception& e) {
        return Result<bool>::Failure(e.what());
    }
}

