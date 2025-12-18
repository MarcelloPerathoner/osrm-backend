
#include "osrm/json_container.hpp"
#include "util/json_container.hpp"
#include "util/json_renderer.hpp"
#include "util/timing_util.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <rapidjson/document.h>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

using namespace osrm;

namespace
{

// we use std::string_view as a key in the object, so since here we have dynamic keys we have to
// "hold" them somewhere okay for tests...
static std::unordered_set<std::string> gKeysHolder;

void convert(const rapidjson::Value &value, json::Value &result)
{
    if (value.IsString())
    {
        result = json::String{value.GetString()};
    }
    else if (value.IsNumber())
    {
        result = json::Number{value.GetDouble()};
    }
    else if (value.IsObject())
    {
        json::Object object;
        for (auto itr = value.MemberBegin(); itr != value.MemberEnd(); ++itr)
        {
            json::Value member;
            convert(itr->value, member);
            auto keyItr = gKeysHolder.emplace(itr->name.GetString()).first;
            object.values.emplace(*keyItr, std::move(member));
        }
        result = std::move(object);
    }
    else if (value.IsArray())
    {
        json::Array array;
        for (auto itr = value.Begin(); itr != value.End(); ++itr)
        {
            json::Value member;
            convert(*itr, member);
            array.values.push_back(std::move(member));
        }
        result = std::move(array);
    }
    else if (value.IsBool())
    {
        if (value.GetBool())
        {
            result = json::True{};
        }
        else
        {
            result = json::False{};
        }
    }
    else if (value.IsNull())
    {
        result = json::Null{};
    }
    else
    {
        throw std::runtime_error("unknown type");
    }
}

json::Object load(const char *filename)
{
    // load file to std string
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    // load rapidjson document
    rapidjson::Document document;
    document.Parse(json.c_str());
    if (document.HasParseError())
    {
        throw std::runtime_error("Failed to parse JSON");
    }

    json::Value result;
    convert(document, result);
    return std::get<json::Object>(result);
}

} // namespace

const int ITERATIONS = 20;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " file.json\n";
        return EXIT_FAILURE;
    }

    const auto obj = load(argv[1]);

    TIMER_START(stringstream);
    for (int i = 0; i < ITERATIONS; ++i)
    {
        std::ostringstream oss;
        json::render(oss, obj);
        (void)oss;
    }
    TIMER_STOP(stringstream);

    TIMER_START(string);
    for (int i = 0; i < ITERATIONS; ++i)
    {
        std::string s;
        json::render(s, obj);
        (void)s;
    }
    TIMER_STOP(string);

    TIMER_START(vector);
    for (int i = 0; i < ITERATIONS; ++i)
    {
        std::vector<char> vec;
        json::render(vec, obj);
        (void)vec;
    }
    TIMER_STOP(vector);

    std::cout << "String: " << TIMER_MSEC(string) / ITERATIONS << "ms" << std::endl;
    std::cout << "Stringstream: " << TIMER_MSEC(stringstream) / ITERATIONS << "ms" << std::endl;
    std::cout << "Vector: " << TIMER_MSEC(vector) / ITERATIONS << "ms" << std::endl;

    std::ostringstream oss;
    json::render(oss, obj);
    std::string s;
    json::render(s, obj);
    std::vector<char> vec;
    json::render(vec, obj);

    if (std::string{vec.begin(), vec.end()} != s || oss.str() != s)
    {
        std::cerr << "Vector/string results are not equal\n";
        throw std::logic_error("Vector/stringstream/string results are not equal");
    }
    return EXIT_SUCCESS;
}
