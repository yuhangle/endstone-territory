//
// Created by yuhang on 2025/3/25.
//

#ifndef TERRITORY_DATABASE_H
#define TERRITORY_DATABASE_H

#include <iostream>
#include <sqlite3.h>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>
#include <map>
#include <random>

#include "fmt/format.h"

class DataBase {
public:
    // 构造时需要指定数据库文件名
    explicit DataBase(std::string  db_filename) : db_filename(std::move(db_filename)) {}


    // 函数用于检查文件是否存在
    static bool fileExists(const std::string& filename) {
        std::ifstream f(filename.c_str());
        return f.good();
    }

    // 初始化数据库
    [[nodiscard]] int init_database() const {
        sqlite3* db;
        int rc = sqlite3_open(db_filename.c_str(), &db);
        if (rc) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
            return rc;
        }

        // 创建 territories 表
        std::string create_territory_table = "CREATE TABLE IF NOT EXISTS territories ("
                                        "name TEXT PRIMARY KEY, "
                                        "pos1_x REAL, pos1_               v                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               y REAL, pos1_z REAL, "
                                        "pos2_x REAL, pos2_y REAL, pos2_z REAL, "
                                        "tppos_x REAL, tppos_y REAL, tppos_z REAL, "
                                        "owner TEXT, manager TEXT, member TEXT, "
                                        "if_jiaohu INTEGER, if_break INTEGER, if_tp INTEGER, "
                                        "if_build INTEGER, if_bomb INTEGER, if_damage INTEGER, dim TEXT, father_tty TEXT);";
        rc = sqlite3_exec(db, create_territory_table.c_str(), nullptr, nullptr, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "创建 territories 表失败: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return rc;
        }

                //0.2.0更新数据库
        // 检查并重新构建表结构以插入 if_damage 到 if_bomb 之后
        std::string checkSql = "PRAGMA table_info(territories);";
        sqlite3_stmt* stmt;
        rc = sqlite3_prepare_v2(db, checkSql.c_str(), -1, &stmt, nullptr);
        bool hasIfDamage = false;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* columnName = sqlite3_column_text(stmt, 1);
            if (strcmp(reinterpret_cast<const char *>(columnName), "if_damage") == 0) {
                hasIfDamage = true;
                break;
            }
        }
        sqlite3_finalize(stmt);

        if (!hasIfDamage) {
            // 如果没有 if_damage 字段，重新创建表结构
            char* err_msg = nullptr;

            // 1. 创建一个新的表，具有期望的列顺序
            std::string createTempTableSql = "CREATE TABLE territories_new ("
                                             "name TEXT PRIMARY KEY, "
                                             "pos1_x REAL, pos1_y REAL, pos1_z REAL, "
                                             "pos2_x REAL, pos2_y REAL, pos2_z REAL, "
                                             "tppos_x REAL, tppos_y REAL, tppos_z REAL, "
                                             "owner TEXT, manager TEXT, member TEXT, "
                                             "if_jiaohu INTEGER, if_break INTEGER, if_tp INTEGER, "
                                             "if_build INTEGER, if_bomb INTEGER, if_damage INTEGER DEFAULT 0, "
                                             "dim TEXT, father_tty TEXT);";
            rc = sqlite3_exec(db, createTempTableSql.c_str(), nullptr, nullptr, &err_msg);
            if (rc != SQLITE_OK) {
                std::cerr << "Create new table failed" << err_msg << std::endl;
                sqlite3_free(err_msg);
                sqlite3_close(db);
                return rc;
            }

            // 2. 将旧表的数据插入到新表中
            std::string copyDataSql = "INSERT INTO territories_new ("
                                      "name, pos1_x, pos1_y, pos1_z, "
                                      "pos2_x, pos2_y, pos2_z, "
                                      "tppos_x, tppos_y, tppos_z, "
                                      "owner, manager, member, "
                                      "if_jiaohu, if_break, if_tp, "
                                      "if_build, if_bomb, dim, father_tty) "
                                      "SELECT "
                                      "name, pos1_x, pos1_y, pos1_z, "
                                      "pos2_x, pos2_y, pos2_z, "
                                      "tppos_x, tppos_y, tppos_z, "
                                      "owner, manager, member, "
                                      "if_jiaohu, if_break, if_tp, "
                                      "if_build, if_bomb, dim, father_tty "
                                      "FROM territories;";
            rc = sqlite3_exec(db, copyDataSql.c_str(), nullptr, nullptr, &err_msg);
            if (rc != SQLITE_OK) {
                std::cerr << "copy data to new table failed" << err_msg << std::endl;
                sqlite3_free(err_msg);
                sqlite3_close(db);
                return rc;
            }

            // 3. 删除旧表
            std::string dropOldTableSql = "DROP TABLE territories;";
            rc = sqlite3_exec(db, dropOldTableSql.c_str(), nullptr, nullptr, &err_msg);
            if (rc != SQLITE_OK) {
                std::cerr << "delete old table failed" << err_msg << std::endl;
                sqlite3_free(err_msg);
                sqlite3_close(db);
                return rc;
            }

            // 4. 将新表重命名为旧表
            std::string renameTableSql = "ALTER TABLE territories_new RENAME TO territories;";
            rc = sqlite3_exec(db, renameTableSql.c_str(), nullptr, nullptr, &err_msg);
            if (rc != SQLITE_OK) {
                std::cerr << "rename new table failed" << err_msg << std::endl;
                sqlite3_free(err_msg);
                sqlite3_close(db);
                return rc;
            } else {
                std::cerr << "0.2.0 update over" << std::endl;
            }
        }

        sqlite3_close(db);
        return SQLITE_OK;
    }

    ///////////////////// 通用操作 /////////////////////

    // 通用执行 SQL 命令（用于添加、删除、修改操作）
    [[nodiscard]] int executeSQL(const std::string &sql) const {
        sqlite3* db;
        int rc = sqlite3_open(db_filename.c_str(), &db);
        if (rc) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
            return rc;
        }
        rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL 执行失败: " << sqlite3_errmsg(db) << std::endl;
        }
        sqlite3_close(db);
        return rc;
    }

// 通用查询 SQL（select查询使用回调函数返回结果为 vector<map<string, string>>）
    static int queryCallback(void* data, int argc, char** argv, char** azColName) {
        auto* result = static_cast<std::vector<std::map<std::string, std::string>>*>(data);
        std::map<std::string, std::string> row;
        for (int i = 0; i < argc; i++) {
            row[azColName[i]] = argv[i] ? argv[i] : "NULL";
        }
        result->push_back(row);
        return 0;
    }

    int querySQL(const std::string &sql, std::vector<std::map<std::string, std::string>> &result) const {
        sqlite3* db;
        int rc = sqlite3_open(db_filename.c_str(), &db);
        if (rc) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
            return rc;
        }
        rc = sqlite3_exec(db, sql.c_str(), queryCallback, &result, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL 查询失败: " << sqlite3_errmsg(db) << std::endl;
        }
        sqlite3_close(db);
        return rc;
    }

// 批量查询 SQL（select查询使用回调函数返回结果为 vector<map<string, string>>）
    static int queryCallback_many_dict(void* data, int argc, char** argv, char** azColName) {
        auto* result = static_cast<std::vector<std::map<std::string, std::string>>*>(data);
        std::map<std::string, std::string> row;
        for (int i = 0; i < argc; i++) {
            row[azColName[i]] = argv[i] ? argv[i] : "NULL";
        }
        result->push_back(row);
        return 0;
    }

    int querySQL_many(const std::string &sql, std::vector<std::map<std::string, std::string>> &result) const {
        sqlite3* db;
        int rc = sqlite3_open(db_filename.c_str(), &db);
        if (rc) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
            return rc;
        }
        rc = sqlite3_exec(db, sql.c_str(), queryCallback_many_dict, &result, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL 查询失败: " << sqlite3_errmsg(db) << std::endl;
        }
        sqlite3_close(db);
        return rc;
    }

    [[nodiscard]] int updateSQL(const std::string &table, const std::string &set_clause, const std::string &where_clause) const {
        sqlite3* db;
        int rc = sqlite3_open(db_filename.c_str(), &db);
        if (rc) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
            return rc;
        }

        std::string sql = "UPDATE " + table + " SET " + set_clause + " WHERE " + where_clause + ";";
        rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);

        if (rc != SQLITE_OK) {
            std::cerr << "SQL 更新失败: " << sqlite3_errmsg(db) << std::endl;
        }

        sqlite3_close(db);
        return rc;
    }

    // 查找指定表中是否存在指定值
    [[nodiscard]] bool isValueExists(const std::string &tableName, const std::string &columnName, const std::string &value) const {
        sqlite3* db;
        int rc = sqlite3_open(db_filename.c_str(), &db); // 打开数据库
        if (rc) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }
        std::string sql;
        if (columnName == "gid") {
            // 构造 SQL 查询语句
            sql = "SELECT COUNT(*) FROM " + tableName + " WHERE " + columnName + " = " + value + ";";
        } else {
            // 构造 SQL 查询语句
            sql = "SELECT COUNT(*) FROM " + tableName + " WHERE " + columnName + " = '" + value + "';";
        }
        sqlite3_stmt* stmt;
        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL 预处理失败: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return false;
        }

        // 执行查询并获取结果
        bool exists = false;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0); // 获取 COUNT(*) 的结果
            exists = (count > 0);
        }

        // 清理资源
        sqlite3_finalize(stmt);
        sqlite3_close(db);

        return exists;
    }

    // 修改指定表中的指定数据的指定值
    [[nodiscard]] bool updateValue(const std::string &tableName,
                     const std::string &targetColumn,
                     const std::string &newValue,
                     const std::string &conditionColumn,
                     const std::string &conditionValue) const {
        sqlite3* db;
        int rc = sqlite3_open(db_filename.c_str(), &db); // 打开数据库
        if (rc) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }
        std::string sql;
        if (targetColumn == "if_jiaohu" || targetColumn == "if_break" || targetColumn == "if_tp" || targetColumn == "if_build" || targetColumn == "if_bomb" || targetColumn == "if_damage") {
            int bool_int;
            if (newValue == "true") {
                bool_int = 1;
            } else {
                bool_int = 0;
            }
            sql = "UPDATE " + tableName +
                  " SET " + targetColumn + " = " + fmt::to_string(bool_int) +
                  " WHERE " + conditionColumn + " = '" + conditionValue + "';";
        }
        else {
            // 构造 SQL 更新语句
            sql = "UPDATE " + tableName +
                  " SET " + targetColumn + " = '" + newValue + "'" +
                  " WHERE " + conditionColumn + " = '" + conditionValue + "';";
        }
        // 执行 SQL 语句
        rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL 更新失败: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return false;
        }

        // 关闭数据库连接
        sqlite3_close(db);

        return true;
    }


    ///////////////////// territories 表操作 /////////////////////

    [[nodiscard]] int addTerritory(const std::string& name,
                   double pos1_x, double pos1_y, double pos1_z,
                   double pos2_x, double pos2_y, double pos2_z,
                   double tppos_x, double tppos_y, double tppos_z,
                   const std::string& owner,
                   const std::string& manager,
                   const std::string& member,
                   int if_jiaohu, int if_break, int if_tp,
                   int if_build, int if_bomb, int if_damage, const std::string& dim,
                   const std::string& father_tty) const  {
        std::string sql = "INSERT INTO territories (name, pos1_x, pos1_y, pos1_z, "
                          "pos2_x, pos2_y, pos2_z, tppos_x, tppos_y, tppos_z, "
                          "owner, manager, member, if_jiaohu, if_break, if_tp, "
                          "if_build, if_bomb, if_damage, dim, father_tty) VALUES ('" + name + "', "
                          + std::to_string(pos1_x) + ", " + std::to_string(pos1_y) + ", " + std::to_string(pos1_z) + ", "
                          + std::to_string(pos2_x) + ", " + std::to_string(pos2_y) + ", " + std::to_string(pos2_z) + ", "
                          + std::to_string(tppos_x) + ", " + std::to_string(tppos_y) + ", " + std::to_string(tppos_z) + ", '"
                          + owner + "', '" + manager + "', '" + member + "', "
                          + std::to_string(if_jiaohu) + ", " + std::to_string(if_break) + ", " + std::to_string(if_tp) + ", "
                          + std::to_string(if_build) + ", " + std::to_string(if_bomb) + ", " + std::to_string(if_damage) + ", '" + dim + "', '"
                          + father_tty + "');";
        return executeSQL(sql);
    }

    int getAllTty(std::vector<std::map<std::string, std::string>> &result) const {
        // 构造 SQL 查询语句，从 GOODS 表中获取所有数据
        std::string sql = "SELECT * FROM territories;";

        // 调用 querySQL_many 函数执行查询，并将结果存储到 result 中
        return querySQL_many(sql, result);
    }

    [[nodiscard]] int deleteTty(const std::string &tty_name) const {
        std::string sql = "DELETE FROM territories WHERE name = '" + tty_name + "';";
        return executeSQL(sql);
    }

    int getTtyByFather(const std::string& tty_name, std::vector<std::map<std::string, std::string>> &result) const {
        std::string sql = "SELECT * FROM territories WHERE father_tty = '" + tty_name + "';";
        return querySQL_many(sql, result);
    }

    [[nodiscard]] int updateUser(const std::string &uuid, const std::string &playername,
                   const std::string &username, const std::string &avatar,const std::string& item, const int money) const {
        std::string sql = "UPDATE USER SET playername = '" +
                          uuid + "', '" + playername + "', '" + username + "', '" + avatar +
                          "',' " + item + "',"+ std::to_string(money) +";";
        return executeSQL(sql);
    }

    int getUser(const std::string &uuid, std::vector<std::map<std::string, std::string>> &result) const {
        std::string sql = "SELECT * FROM USER WHERE uuid = '" + uuid + "';";
        return querySQL(sql, result);
    }

    int getUser_by_playername(const std::string &playername, std::vector<std::map<std::string, std::string>> &result) const {
        std::string sql = "SELECT * FROM USER WHERE playername = '" + playername + "';";
        return querySQL(sql, result);
    }


    //数据库工具

    //将逗号字符串分割为vector
    static std::vector<std::string> splitString(const std::string& input) {
        std::vector<std::string> result; // 用于存储分割后的结果
        std::string current;             // 当前正在构建的子字符串
        bool inBraces = false;           // 标记是否在 {} 内部

        for (char ch : input) {
            if (ch == '{') {
                // 遇到左大括号，标记进入 {} 内部
                inBraces = true;
                current += ch; // 将 { 添加到当前字符串
            } else if (ch == '}') {
                // 遇到右大括号，标记离开 {} 内部
                inBraces = false;
                current += ch; // 将 } 添加到当前字符串
            } else if (ch == ',' && !inBraces) {
                // 遇到逗号且不在 {} 内部时，分割字符串
                if (!current.empty()) {
                    result.push_back(current);
                    current.clear();
                }
            } else {
                // 其他情况，将字符添加到当前字符串
                current += ch;
            }
        }

        // 处理最后一个子字符串（如果非空）
        if (!current.empty()) {
            result.push_back(current);
        }

        return result;
    }

    //将数字字符逗号分割为vector
    static std::vector<int> splitStringInt(const std::string& input) {
        std::vector<int> result; // 用于存储分割后的结果
        std::stringstream ss(input);     // 使用 string stream 处理输入字符串
        std::string item;

        // 按逗号分割字符串
        while (std::getline(ss, item, ',')) {
            if (!item.empty()) {         // 如果分割出的元素非空，则添加到结果中
                std::stringstream sss(item);
                int groupid = 0;
                if (!(sss >> groupid)) {
                    return {}; // 如果解析失败，返回空
                }
                result.push_back(groupid);
            }
        }

        // 如果没有逗号且结果为空则返回空
        if (result.empty()) {
            return {};
        }

        return result;
    }

    //将vector(string)通过逗号分隔改为字符串
    static std::string vectorToString(const std::vector<std::string>& vec) {
        if (vec.empty()) {
            return ""; // 如果向量为空，返回空字符串
        }

        std::ostringstream oss; // 使用字符串流拼接结果
        for (size_t i = 0; i < vec.size(); ++i) {
            oss << vec[i]; // 添加当前元素
            if (i != vec.size() - 1 && !(vec[i].empty())) { // 如果不是最后一个元素且不为空，添加逗号
                oss << ",";
            }
        }

        return oss.str(); // 返回拼接后的字符串
    }

    //将vector(int)通过逗号分隔改为字符串
    static std::string IntVectorToString(const std::vector<int>& vec) {
        if (vec.empty()) {
            return ""; // 如果向量为空，返回空字符串
        }

        std::ostringstream oss; // 使用字符串流拼接结果
        for (size_t i = 0; i < vec.size(); ++i) {
            oss << vec[i]; // 添加当前元素
            if (i != vec.size() - 1) { // 如果不是最后一个元素，添加逗号
                oss << ",";
            }
        }

        return oss.str(); // 返回拼接后的字符串
    }

    //字符串改整数
    static int stringToInt(const std::string& str) {
        try {
            // 尝试将字符串转换为整数
            return std::stoi(str);
        } catch (const std::invalid_argument&) {
            // 捕获无效参数异常（例如无法解析为整数）
            return 0;
        } catch (const std::out_of_range&) {
            // 捕获超出范围异常（例如数值过大或过小）
            return 0;
        }
    }


    // 将附魔 map 转换为字符串
    static std::string enchantsToString(const std::unordered_map<std::string, int>& enchants) {
        if (enchants.empty()) {
            return ""; // 如果没有附魔，返回空字符串
        }

        std::ostringstream oss;

        for (auto it = enchants.begin(); it != enchants.end(); ++it) {
            oss << it->first << ":" << it->second; // 格式：enchant_name:level

            // 如果不是最后一个元素，添加逗号
            if (std::next(it) != enchants.end()) {
                oss << ",";
            }
        }

        return oss.str();
    }
    //还原附魔字符串为附魔map
    static std::unordered_map<std::string, int> stringToEnchants(const std::string& str) {
        std::unordered_map<std::string, int> result;
        std::istringstream ss(str);
        std::string pairStr;

        while (std::getline(ss, pairStr, ',')) {
            size_t colonPos = pairStr.find(':');
            if (colonPos != std::string::npos) {
                std::string key = pairStr.substr(0, colonPos);
                std::string valueStr = pairStr.substr(colonPos + 1);
                try {
                    int value = std::stoi(valueStr);
                    result[key] = value;
                } catch (...) {
                    // 忽略无效项
                }
            }
        }

        return result;
    }

    // 生成一个符合 RFC 4122 标准的 UUID v4
    static std::string generate_uuid_v4() {
        static thread_local std::mt19937 gen{std::random_device{}()};

        std::uniform_int_distribution<int> dis(0, 15);

        std::stringstream ss;
        ss << std::hex; // 设置为十六进制输出

        for (int i = 0; i < 8; ++i) ss << dis(gen);
        ss << "-";
        for (int i = 0; i < 4; ++i) ss << dis(gen);
        ss << "-";

        ss << "4"; // 版本号为 4
        for (int i = 0; i < 3; ++i) ss << dis(gen);
        ss << "-";

        ss << (dis(gen) & 0x3 | 0x8);
        for (int i = 0; i < 3; ++i) ss << dis(gen);
        ss << "-";

        for (int i = 0; i < 12; ++i) ss << dis(gen);

        return ss.str();
    }

private:
    std::string db_filename;
};

#endif // TERRITORY_DATABASE_H

