#include "DrGrandetTaskPlugin.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"

#include <regex>

bool asst::DrGrandetTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string task = details.at("details").at("task").as_string();
    if (task == "Fight@StoneConfirm") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::DrGrandetTaskPlugin::_run()
{
    LogTraceFunction;

    int cur_ms = analyze_time_left();
    if (cur_ms < 0) {
        return false;
    }
    auto task_ptr = Task.get("DrGrandetUseOriginiums");
    int threshold = task_ptr->post_delay;
    if (threshold <= cur_ms) {
        return true;
    }

    int delay = cur_ms - task_ptr->pre_delay;
    if (delay > 0) {
        sleep(delay);
    }

    // 直接 return false, 下次进来会重试的
    return false;
}

int asst::DrGrandetTaskPlugin::analyze_time_left()
{
    LogTraceFunction;

    OCRer analyzer(ctrler()->get_image());
    analyzer.set_task_info("DrGrandetUseOriginiums");
    analyzer.set_replace(Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map);
    // 这里是汉字和数字混合的，用不了单独的en模型
    analyzer.set_use_char_model(false);

    if (!analyzer.analyze()) {
        return -1;
    }
    auto text = analyzer.get_result().front().text;

    std::regex regex = std::regex(R"(\d[:]\d\d)");
    std::smatch match;
    if (!std::regex_search(text, match, regex)) {
        return -1;
    }
    std::string time = match.str(0);
    Log.info("Time:", time);

    int min = std::stoi(time.substr(0, 1));
    int sec = std::stoi(time.substr(2, 2));
    int millisec = (min * 60 + sec) * 1000;

    Log.info("Time left ms:", millisec);
    return millisec;
}
