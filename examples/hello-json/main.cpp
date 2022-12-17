#include <winrt/Windows.Data.Json.h>
#include <iostream>

using namespace winrt::Windows::Data::Json;

int main() {
    winrt::init_apartment();
    JsonObject jsonObj;
    jsonObj.SetNamedValue(L"myKey", JsonValue::CreateStringValue(L"This is my JSON string value."));
    auto str = jsonObj.Stringify();
    std::wcout << str.c_str() << std::endl;
    return 0;
}
