#include <iostream>
#include "json/json.h"

int main() {
    json::JSON json;
    json.load_from_file("./resources/test.json");
    std::cout << json << std::endl;
    std::cout << json["dict2"] << std::endl;
    std::cout << json["list"] << std::endl;
    std::cin.get();
    return 0;
}
