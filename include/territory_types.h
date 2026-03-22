// territory_types.h
#ifndef TERRITORY_TYPES_H
#define TERRITORY_TYPES_H
#include <string>
#include <tuple>

struct TerritoryData {
    std::string name;
    std::tuple<double, double, double> pos1 = {0,0,0};
    std::tuple<double, double, double> pos2 = {0,0,0};
    std::tuple<double, double, double> tppos = {0,0,0};
    std::string owner;
    std::string manager;
    std::string member;
    std::string dim;
    std::string father_tty;
    bool if_jiaohu = false;
    bool if_break = false;
    bool if_tp = false;
    bool if_build = false;
    bool if_bomb = false;
    bool if_damage = false;
    bool if_edge_piston = false;
    bool if_wither = false;
};

struct InTtyInfo {
    std::string name;
    std::vector<std::string> members;
    std::string owner;
    bool if_jiaohu;
    bool if_break;
    bool if_build;
    bool if_bomb;
    bool if_damage;
    bool if_edge_piston;
    bool if_wither;
};
#endif