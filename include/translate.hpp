//
// Created by yuhang on 2025/3/14.
//

#ifndef TERRITORY_TRANSLATE_H
#define TERRITORY_TRANSLATE_H

#include <fstream>
#include <nlohmann/json.hpp>

class translate_tty {
public:
    using json = nlohmann::json;
    json languageResource; // 存储从 lang.json 加载的语言资源

    // 构造函数中加载语言资源文件
    translate_tty() {
        loadLanguage();
    }

    // 加载语言资源文件
    std::pair<bool,std::string> loadLanguage() {
        std::ifstream file("plugins/territory/lang.json");
        if (file.is_open()) {
            languageResource = json::parse(file);
            file.close();
            return {true,"lang.json is normal"};
        } else {
            return {false,"you can download lang.json from github to change territory plugin language"};
        }
    }

    // 获取本地化字符串
    std::string getLocal(const std::string &key) {
        if (languageResource.find(key) != languageResource.end()) {
            return languageResource[key].get<std::string>();
        }
        return key; // 如果找不到，返回原始 key
    }
};
inline translate_tty LangTty;
#endif //TERRITORY_TRANSLATE_H