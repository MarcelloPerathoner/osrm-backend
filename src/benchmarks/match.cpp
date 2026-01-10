#include "engine/engine_config.hpp"
#include "util/timing_util.hpp"

#include "osrm/match_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"

#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

int main(int argc, const char *argv[])
try
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " data.osrm\n";
        return EXIT_FAILURE;
    }

    using namespace osrm;

    // Configure based on a .osrm base path, and no datasets in shared mem from osrm-datastore
    EngineConfig config;
    config.storage_config = {argv[1]};
    config.algorithm = (argc > 2 && std::string{argv[2]} == "mld") ? EngineConfig::Algorithm::MLD
                                                                   : EngineConfig::Algorithm::CH;
    config.use_shared_memory = false;

    // Routing machine with several services (such as Route, Table, Nearest, Trip, Match)
    OSRM osrm{config};

    // Route in monaco
    MatchParameters params;
    params.overview = RouteParameters::OverviewType::False;
    params.steps = false;

    params.coordinates.push_back(7.422176599502563_lon + 43.73754595167546_lat);
    params.coordinates.push_back(7.421715259552002_lon + 43.73744517900973_lat);
    params.coordinates.push_back(7.421489953994752_lon + 43.73738316497729_lat);
    params.coordinates.push_back(7.421286106109619_lon + 43.737274640266_lat);
    params.coordinates.push_back(7.420910596847533_lon + 43.73714285999499_lat);
    params.coordinates.push_back(7.420696020126342_lon + 43.73699557581948_lat);
    params.coordinates.push_back(7.42049217224121_lon + 43.73690255404829_lat);
    params.coordinates.push_back(7.420309782028198_lon + 43.73672426191624_lat);
    params.coordinates.push_back(7.420159578323363_lon + 43.7366622471372_lat);
    params.coordinates.push_back(7.420148849487305_lon + 43.736623487867654_lat);
    params.coordinates.push_back(7.419934272766113_lon + 43.73647620241466_lat);
    params.coordinates.push_back(7.419805526733398_lon + 43.736228141885455_lat);
    params.coordinates.push_back(7.419601678848267_lon + 43.736142870841206_lat);
    params.coordinates.push_back(7.419376373291015_lon + 43.735956824504974_lat);
    params.coordinates.push_back(7.419247627258301_lon + 43.73574752168583_lat);
    params.coordinates.push_back(7.419043779373169_lon + 43.73566224995717_lat);
    params.coordinates.push_back(7.418732643127442_lon + 43.735406434042645_lat);
    params.coordinates.push_back(7.418657541275024_lon + 43.735321161828274_lat);
    params.coordinates.push_back(7.418593168258667_lon + 43.73521263337983_lat);
    params.coordinates.push_back(7.418367862701416_lon + 43.73508084857086_lat);
    params.coordinates.push_back(7.418346405029297_lon + 43.73484828643578_lat);
    params.coordinates.push_back(7.4180567264556885_lon + 43.734437424456566_lat);
    params.coordinates.push_back(7.417809963226318_lon + 43.73414284243448_lat);
    params.coordinates.push_back(7.417863607406615_lon + 43.73375523230292_lat);
    params.coordinates.push_back(7.417809963226318_lon + 43.73386376339265_lat);
    params.coordinates.push_back(7.417895793914795_lon + 43.73365445325776_lat);
    params.coordinates.push_back(7.418067455291747_lon + 43.73343739012297_lat);
    params.coordinates.push_back(7.41803526878357_lon + 43.73319706930599_lat);
    params.coordinates.push_back(7.418024539947509_lon + 43.73295674752463_lat);
    params.coordinates.push_back(7.417906522750854_lon + 43.73284821479115_lat);
    params.coordinates.push_back(7.417917251586914_lon + 43.7327551865773_lat);
    params.coordinates.push_back(7.417434453964233_lon + 43.73281720540258_lat);
    params.coordinates.push_back(7.4173808097839355_lon + 43.73307303237796_lat);
    params.coordinates.push_back(7.41750955581665_lon + 43.73328234454499_lat);
    params.coordinates.push_back(7.417563199996948_lon + 43.73352266501975_lat);
    params.coordinates.push_back(7.41750955581665_lon + 43.733770736756355_lat);
    params.coordinates.push_back(7.417466640472412_lon + 43.73409632935116_lat);
    params.coordinates.push_back(7.417230606079102_lon + 43.73428238146768_lat);
    params.coordinates.push_back(7.41724133491516_lon + 43.73405756842078_lat);
    params.coordinates.push_back(7.4169838428497314_lon + 43.73449168940785_lat);
    params.coordinates.push_back(7.41701602935791_lon + 43.734615723397525_lat);
    params.coordinates.push_back(7.41704821586609_lon + 43.73487929477265_lat);
    params.coordinates.push_back(7.41725206375122_lon + 43.734949063471895_lat);
    params.coordinates.push_back(7.4173808097839355_lon + 43.73533666587628_lat);
    params.coordinates.push_back(7.41750955581665_lon + 43.735623490040375_lat);
    params.coordinates.push_back(7.417799234390259_lon + 43.73577852955704_lat);
    params.coordinates.push_back(7.4180781841278085_lon + 43.735972328388435_lat);
    params.coordinates.push_back(7.41850733757019_lon + 43.73608860738618_lat);
    params.coordinates.push_back(7.418850660324096_lon + 43.736228141885455_lat);
    params.coordinates.push_back(7.419086694717407_lon + 43.73636767605958_lat);
    params.coordinates.push_back(7.419333457946777_lon + 43.73664674343239_lat);
    params.coordinates.push_back(7.419633865356444_lon + 43.73676302112054_lat);
    params.coordinates.push_back(7.419784069061279_lon + 43.737096349241845_lat);
    params.coordinates.push_back(7.420030832290649_lon + 43.73720487427631_lat);
    params.coordinates.push_back(7.419601678848267_lon + 43.73708084564945_lat);
    params.coordinates.push_back(7.419333457946777_lon + 43.73708084564945_lat);
    params.coordinates.push_back(7.419043779373169_lon + 43.737158363571325_lat);
    params.coordinates.push_back(7.418915033340454_lon + 43.737305647346446_lat);
    params.coordinates.push_back(7.41848587989807_lon + 43.7374916894919_lat);
    params.coordinates.push_back(7.418271303176879_lon + 43.73746843425534_lat);
    params.coordinates.push_back(7.417960166931152_lon + 43.73744517900973_lat);
    params.coordinates.push_back(7.417885065078735_lon + 43.737212626056944_lat);
    params.coordinates.push_back(7.417563199996948_lon + 43.73703433484817_lat);
    params.coordinates.push_back(7.4173057079315186_lon + 43.73692580950463_lat);
    params.coordinates.push_back(7.417144775390625_lon + 43.7367707729584_lat);
    params.coordinates.push_back(7.416973114013672_lon + 43.73653821738638_lat);
    params.coordinates.push_back(7.416855096817017_lon + 43.73639868360965_lat);
    params.coordinates.push_back(7.4167799949646_lon + 43.736142870841206_lat);
    params.coordinates.push_back(7.41675853729248_lon + 43.735848297208605_lat);
    params.coordinates.push_back(7.416619062423706_lon + 43.73567000193752_lat);
    params.coordinates.push_back(7.416543960571288_lon + 43.735406434042645_lat);
    params.coordinates.push_back(7.416479587554932_lon + 43.73529790574875_lat);
    params.coordinates.push_back(7.416415214538574_lon + 43.73515061703527_lat);
    params.coordinates.push_back(7.416350841522218_lon + 43.73490255101476_lat);
    params.coordinates.push_back(7.416340112686156_lon + 43.73475526132885_lat);
    params.coordinates.push_back(7.416222095489501_lon + 43.73446068087028_lat);
    params.coordinates.push_back(7.416243553161621_lon + 43.73430563794159_lat);
    params.coordinates.push_back(7.416050434112548_lon + 43.73403431185051_lat);
    params.coordinates.push_back(7.415814399719239_lon + 43.73382500231174_lat);
    params.coordinates.push_back(7.415750026702881_lon + 43.73354592178871_lat);
    params.coordinates.push_back(7.415513992309569_lon + 43.73347615145474_lat);
    params.coordinates.push_back(7.415342330932617_lon + 43.733251335381205_lat);

    auto run_benchmark = [&](std::optional<double> radiusInMeters)
    {
        params.radiuses = {};
        if (radiusInMeters)
        {
            for (size_t index = 0; index < params.coordinates.size(); ++index)
            {
                params.radiuses.emplace_back(*radiusInMeters);
            }
        }

        TIMER_START(routes);
        auto NUM = 100;
        for (int i = 0; i < NUM; ++i)
        {
            engine::api::ResultT result = json::Object();
            const auto rc = osrm.Match(params, result);
            auto &json_result = std::get<json::Object>(result);
            if (rc != Status::Ok ||
                std::get<json::Array>(json_result.values.at("matchings")).values.size() != 1)
            {
                throw std::runtime_error{"Couldn't match"};
            }
        }
        TIMER_STOP(routes);
        if (radiusInMeters)
        {
            std::cout << "Radius " << *radiusInMeters << "m: " << std::endl;
        }
        else
        {
            std::cout << "Default radius: " << std::endl;
        }
        std::cout << (TIMER_MSEC(routes) / NUM) << "ms/req at " << params.coordinates.size()
                  << " coordinate" << std::endl;
        std::cout << (TIMER_MSEC(routes) / NUM / params.coordinates.size()) << "ms/coordinate"
                  << std::endl;
    };

    for (auto radius : std::vector<std::optional<double>>{std::nullopt, 10.0})
    {
        run_benchmark(radius);
    }

    return EXIT_SUCCESS;
}
catch (const std::exception &e)
{
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
