//
// Created by yuhang on 2025/3/14.
//

#ifndef TERRITORY_TRANSLATE_H
#define TERRITORY_TRANSLATE_H

#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <utility>
#include <vector>
#include <fmt/format.h>

using namespace std;

class translate {
public:
    static constexpr auto DEFAULT_LANG_DIR = "plugins/territory/language/";
    using json = nlohmann::json;

    // 构造函数：加载语言目录下所有 .json 文件，并设置默认语言
    explicit translate(std::string  lang_dir = DEFAULT_LANG_DIR,
                       std::string  default_locale = "en_US")
        : lang_dir_(std::move(lang_dir)), default_locale_(std::move(default_locale)) {
        loadAllLanguages();
    };

    // 加载语言目录下所有语言文件
    pair<bool, string> loadAllLanguages() {
        namespace fs = std::filesystem;

        languages_.clear();

        if (!fs::exists(lang_dir_) || !fs::is_directory(lang_dir_)) {
            return {false, "language directory not found: " + lang_dir_};
        }

        for (const auto& entry : fs::directory_iterator(lang_dir_)) {
            if (!entry.is_regular_file()) continue;
            const auto& path = entry.path();
            if (path.extension() != ".json") continue;

            string locale_name = path.stem().string(); // e.g., "zh_CN", "en_US"

            try {
                if (std::ifstream f(path); f.is_open()) {
                    const json lang_data = json::parse(f);
                    languages_[locale_name] = lang_data;
                }
            } catch (const json::parse_error&) {
                // 跳过格式错误的语言文件
                continue;
            }
        }

        if (languages_.empty()) {
            return {false, "no valid language files found in " + lang_dir_};
        }

        // 如果默认语言未加载，回退到第一个可用语言
        if (!languages_.contains(default_locale_)) {
            default_locale_ = languages_.begin()->first;
        }

        return {true, "loaded " + std::to_string(languages_.size()) + " languages, default: " + default_locale_};
    }

    // 向后兼容：重新加载所有语言文件
    pair<bool, string> loadLanguage() {
        return loadAllLanguages();
    }

    // 获取指定语言的翻译，找不到则回退到英文，英文也没有则返回 key
    [[nodiscard]] std::string getLocal(const std::string& key, const std::string& locale) const {
        // 尝试指定语言
        if (const auto it = languages_.find(locale); it != languages_.end() && it->second.contains(key)) {
            return it->second[key].get<std::string>();
        }

        // 语言族回退：zh_TW/zh_HK → zh_CN, en_GB → en_US 等
        if (const auto underscore_pos = locale.find('_'); underscore_pos != std::string::npos) {
            const std::string lang_code = locale.substr(0, underscore_pos);
            for (const auto& [loaded_locale, lang_data] : languages_) {
                if (loaded_locale.size() > underscore_pos &&
                    loaded_locale[underscore_pos] == '_' &&
                    loaded_locale.substr(0, underscore_pos) == lang_code &&
                    loaded_locale != locale) {
                    if (lang_data.contains(key)) {
                        return lang_data[key].get<std::string>();
                    }
                }
            }
        }

        // 回退到英文
        if (locale != "en_US") {
            if (const auto en_it = languages_.find("en_US"); en_it != languages_.end() && en_it->second.contains(key)) {
                return en_it->second[key].get<std::string>();
            }
        }

        return key;
    }

    // 获取默认语言的翻译（向后兼容）
    [[nodiscard]] std::string getLocal(const std::string& key) const
    {
        return getLocal(key, default_locale_);
    }

    // 默认语言格式化翻译（向后兼容）
    template<typename... Args>
    std::string tr(const std::string& key, Args&&... args) {
        const std::string pattern = getLocal(key);
        return fmt::vformat(pattern, fmt::make_format_args(args...));
    }

    // 向后兼容：旧版 checkLanguageCommon 在新系统中不再需要
    static int checkLanguageCommon(const std::string&, const std::string&) {
        return 0;
    }

    // 获取已加载的语言列表
    [[nodiscard]] std::vector<std::string> getAvailableLocales() const {
        std::vector<std::string> locales;
        for (const auto& locale : languages_ | views::keys) {
            locales.push_back(locale);
        }
        return locales;
    }

private:
    std::string lang_dir_;
    std::string default_locale_;
    std::unordered_map<std::string, json> languages_;
};

#endif //TERRITORY_TRANSLATE_H
