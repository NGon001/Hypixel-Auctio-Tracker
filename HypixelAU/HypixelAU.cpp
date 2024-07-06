#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "nlohmann/json.hpp"
#include <sstream> // For std::stringstream
#pragma comment(lib, "wldap32.lib" )
#pragma comment(lib, "crypt32.lib" )
#pragma comment(lib, "Ws2_32.lib")

#define CURL_STATICLIB 
#include <curl/curl.h>

// For convenience
using json = nlohmann::json;
using namespace std;
// Function to handle the data received from libcurl
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to fetch data from the Hypixel API
std::string fetchData(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return readBuffer;
}

std::string transformString(const std::string& input) {
    std::string result = input;

    // Transform the string to uppercase
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);

    // Replace spaces with underscores
    std::replace(result.begin(), result.end(), ' ', '_');

    return result;
}

void lowestbinCheck(const json auction)
{
    std::string itemName = auction["item_name"];
    int binPrice = auction["starting_bid"];
    std::string seller = auction["auctioneer"];
    std::string auctionId = auction["uuid"];

    std::string convertedname = itemName;

    // Transform the string to uppercase
    std::transform(convertedname.begin(), convertedname.end(), convertedname.begin(), ::toupper);

    // Replace spaces with underscores
    std::replace(convertedname.begin(), convertedname.end(), ' ', '_');

    std::string url = "http://moulberry.codes/lowestbin.json"; // Replace with actual URL
    std::string data = fetchData(url);
    json response = json::parse(data);

    std::string lowestprice;
    if (response.find(convertedname) != response.end()) {
        // Check if the value is a number
        if (response[convertedname].is_number()) {
            lowestprice = std::to_string(response[convertedname].get<int>()); // Convert number to string
            std::stringstream ss(lowestprice);
            int lowestPriceInt;
            ss >> lowestPriceInt;
            if (binPrice < lowestPriceInt) 
            {
                std::cout << "{" << std::endl;
                std::cout << "  \"Item\": \"" << itemName << "\"," << std::endl;
                std::cout << "  \"BIN_Price\": " << binPrice << "," << std::endl;
                std::cout << "  \"Lowest_Price\": " << lowestprice << "," << std::endl;
                std::cout << "  \"Seller\": \"" << seller << "\"," << std::endl;
                std::cout << "  \"View_Auction\": \"/viewauction " << auctionId << "\"" << std::endl;
                std::cout << "}" << std::endl;
            }
        }
        else {
            // Handle case where the value is not a number
          //  std::cerr << "Value associated with key '" << convertedname << "' is not a number." << std::endl;
            // Optionally handle other types here
        }
    }
    else {
       // std::cerr << "Key '" << convertedname << "' not found in JSON." << std::endl;
    }
  //  cout << "lowestprice: " << response[result] << endl;
    //cout << "IMPALING;3: " << response["IMPALING;3"] << endl;

 
}

// Function to display BIN (buy-it-now) items
void displayBinItems(const json& auctions) {
    for (const auto& auction : auctions) {
        if (auction["bin"]) { // Only consider items with BIN (buy-it-now) option
            lowestbinCheck(auction);
        }
    }
}

int main() { 
    std::string apiKey = "api";
    std::string baseUrl = "https://api.hypixel.net/skyblock/auctions?key=" + apiKey;
    bool morePages = true;
    int page = 0;

    while (true) { // Infinite loop to keep fetching data
        morePages = true;
        page = 0;

        while (morePages) {
            std::string url = baseUrl + "&page=" + to_string(page);
            std::string data = fetchData(url);

            try {
                json response = json::parse(data);
                if (response["success"]) {
                    auto auctions = response["auctions"];
                    displayBinItems(auctions);
                    morePages = response["totalPages"] > page + 1;
                    page++;
                }
                else {
                    std::cerr << "API response was not successful!" << std::endl;
                    morePages = false;
                }
            }
            catch (json::parse_error& e) {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
                morePages = false;
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(30)); // Fetch data every 30 seconds
    }
    
    return 0;
}