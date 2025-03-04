from threading import Thread
from endstone.command import Command, CommandSender, CommandSenderWrapper
from endstone.plugin import Plugin
from endstone.event import event_handler,BlockBreakEvent,PlayerChatEvent,BlockPlaceEvent,ActorSpawnEvent,PlayerInteractActorEvent,PlayerInteractEvent,ActorExplodeEvent
from endstone import ColorFormat,Player
import os
import json
from datetime import datetime
import sqlite3
from contextlib import contextmanager
import time
import threading
from endstone.form import ModalForm,Dropdown,Label,ActionForm,TextInput,Slider,MessageForm,Toggle

# 初始化
tty_data = "plugins/territory"
land_data = "plugins/territory/landban.json"
db_file = "plugins/territory/territory_data.db"
config_file = "plugins/territory/config.json"
if not os.path.exists(tty_data):
    os.mkdir(tty_data)




class Territory(Plugin):
    api_version = "0.6"
    
    # 连接数据库函数
    def connect(self):
        """建立到SQLite数据库的连接。"""
        try:
            self.conn = sqlite3.connect(db_file,check_same_thread=False)
            self.logger.info("成功连接到数据库")
        except sqlite3.Error as e:
            self.logger.error(f"连接数据库时发生错误: {e}")
    
    # 游标对象
    @contextmanager
    def cursor(self):
        """提供一个游标对象用于执行SQL语句，并确保事务提交或回滚。"""
        cur = None
        try:
            cur = self.conn.cursor()
            yield cur
            self.conn.commit()
        except sqlite3.Error as e:
            self.logger.error(f"执行SQL语句时发生错误: {e}")
            self.conn.rollback()
            raise
        finally:
            if cur:
                cur.close()
    
    # 初始化数据库
    def init_db(self):
        """初始化数据库，创建必要的表"""
        with self.conn:
            cur = self.conn.cursor()
            cur.execute('''
                CREATE TABLE IF NOT EXISTS territories (
                    name TEXT PRIMARY KEY,
                    pos1_x REAL, pos1_y REAL, pos1_z REAL,
                    pos2_x REAL, pos2_y REAL, pos2_z REAL,
                    tppos_x REAL, tppos_y REAL, tppos_z REAL,
                    owner TEXT,
                    manager TEXT,
                    member TEXT,
                    if_jiaohu INTEGER,
                    if_break INTEGER,
                    if_tp INTEGER,
                    if_build INTEGER,
                    if_bomb INTEGER,
                    dim INTEGER,
                    father_tty TEXT
                )
            ''')
            self.conn.commit()
            self.logger.info("已初始化数据库")
    
    # 写入数据库函数
    def write_to_database(self, data_to_write):
        """
        向数据库写入数据。
        
        参数:
            data_to_write (tuple): 包含要插入的数据的元组。
        """
        with self.cursor() as cur:
            cur.execute('''
            INSERT INTO territories (name, pos1_x, pos1_y, pos1_z, pos2_x, pos2_y, pos2_z, tppos_x, tppos_y, tppos_z, owner, manager, member, if_jiaohu, if_break, if_tp, if_build, if_bomb, dim, father_tty)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            ''', data_to_write)
    
    # 读取配置文件
    def read_config(self):
        """
        读取配置文件的函数
        """
        global max_tty_num
        if not os.path.exists(config_file):
            de_config = {"player_max_tty_num": 20}
            with open(config_file, 'w', encoding='utf-8') as file:  # 指定编码为 UTF-8
                json.dump(de_config, file, ensure_ascii=False, indent=4)

            self.server.logger.info("未检测到配置文件,已自动创建")
            max_tty_num = 20
        else:
            with open(config_file,"r", encoding='utf-8') as file:
                tty_config = json.load(file)
            try:
                max_tty_num = int(tty_config["player_max_tty_num"])
            except:
                self.server.logger.error("配置文件异常,使用默认配置")
                max_tty_num = 20

    # 读取全部领地
    def read_all_territories(self):
        """
        读取所有领地的信息。
        
        返回:
            list of dict: 包含所有领地信息的列表，每个元素是一个包含领地信息的字典。
        """
        with self.cursor() as cur:
            cur.execute('''
            SELECT name, pos1_x, pos1_y, pos1_z, pos2_x, pos2_y, pos2_z, tppos_x, tppos_y, tppos_z, owner, manager, member, if_jiaohu, if_break, if_tp, if_build, if_bomb, dim ,father_tty 
            FROM territories
            ''')
            rows = cur.fetchall()
            return [{
                'name': row[0],
                'pos1': (row[1], row[2], row[3]),
                'pos2': (row[4], row[5], row[6]),
                'tppos': (row[7], row[8], row[9]),
                'owner': row[10],
                'manager': row[11],
                'member': row[12],
                'if_jiaohu': bool(row[13]),
                'if_break': bool(row[14]),
                'if_tp': bool(row[15]),
                'if_build': bool(row[16]),
                'if_bomb': bool(row[17]),
                'dim': row[18],
                'father_tty': row[19]
            } for row in rows]
    
    # 根据名字读取领地信息的函数        
    def read_territory_by_name(self, territory_name):
        """
        根据名字读取领地信息。
        
        参数:
            territory_name (str): 领地的名字。
        返回:
            dict or None: 如果找到对应的领地，则返回包含领地信息的字典；否则返回None。
        """
        for row in all_tty:
            if row['name'] == territory_name:
                return row
        return None
    
    # 检查坐标是否存在于对应空间的函数
    def is_point_in_cube(self,point, corner1, corner2):
        """
        判断第一个坐标是否存在于由第二个和第三个坐标构成的立方体空间中。

        :param point: Tuple[float, float, float]，要判断的点的坐标 (x, y, z)
        :param corner1: Tuple[float, float, float]，立方体一个角的坐标 (x, y, z)
        :param corner2: Tuple[float, float, float]，立方体对角的坐标 (x, y, z)
        :return: bool，第一个点是否在立方体内部或边界上
        """
        # 确定立方体在各轴上的最小值和最大值
        x_min, x_max = min(corner1[0], corner2[0]), max(corner1[0], corner2[0])
        y_min, y_max = min(corner1[1], corner2[1]), max(corner1[1], corner2[1])
        z_min, z_max = min(corner1[2], corner2[2]), max(corner1[2], corner2[2])

        # 检查点是否在范围内
        return (x_min <= point[0] <= x_max and
                y_min <= point[1] <= y_max and
                z_min <= point[2] <= z_max)
    
    # 检查立方体重合
    def is_overlapping(self, cube1, cube2):
        """
        cube1: 立方体两个坐标元组1
        cube2: 立方体两个坐标元组2
            重叠返回True
            不重叠返回False
        """
        # 处理cube1各轴范围
        x1a, y1a, z1a = cube1[0]
        x1b, y1b, z1b = cube1[1]
        x1_min, x1_max = sorted([x1a, x1b])
        y1_min, y1_max = sorted([y1a, y1b])
        z1_min, z1_max = sorted([z1a, z1b])
        
        # 处理cube2各轴范围
        x2a, y2a, z2a = cube2[0]
        x2b, y2b, z2b = cube2[1]
        x2_min, x2_max = sorted([x2a, x2b])
        y2_min, y2_max = sorted([y2a, y2b])
        z2_min, z2_max = sorted([z2a, z2b])
        
        # 检查是否有维度不重叠
        overlap_x = x1_max >= x2_min and x2_max >= x1_min
        overlap_y = y1_max >= y2_min and y2_max >= y1_min
        overlap_z = z1_max >= z2_min and z2_max >= z1_min
        
        return overlap_x and overlap_y and overlap_z
    
    # 检查重合函数
    def is_territory_overlapping(self, new_pos1, new_pos2, new_dim):
        """
        检查新设置的领地是否与数据库中已存在的领地重合。

        参数:
            new_pos1 (tuple): 新领地的第一个坐标点 (x, y, z)。
            new_pos2 (tuple): 新领地的第二个坐标点 (x, y, z)。
        
        返回:
            bool: 如果存在重合返回True，否则返回False。
        """
        rows = all_tty
        new_tty = (new_pos1,new_pos2)
        for row in rows:
            existing_pos1 = row['pos1']
            existing_pos2 = row['pos2']
            existing_dim = row['dim']

            # 检查是否重合
            if self.is_overlapping(new_tty,(existing_pos1,existing_pos2)) == True:
                # 再检查维度
                if new_dim == existing_dim:
                    return True

        return False
    
    # 检查玩家拥有领地的数量的函数
    def check_tty_num(self,player_name):
        """
        用于检查玩家拥有领地数量的函数
        player_name: 玩家名
        
        返回玩家拥有的领地数量
        """    
        rows = all_tty
        #初始化计数器
        tty_num = 0
        for row in rows:
            if player_name == row['owner']:
                # 找到一个就加1
                tty_num = tty_num + 1
        return tty_num
        
    # 玩家增加领地函数
    def player_add_tty(self,player_name,pos1,pos2,tppos,dim):
        """
        player_name: 玩家名
        pos1: 领地坐标1
        pos2: 领地坐标2
        tppos: 领地传送点
        dim: 领地维度
        """
        global all_tty
        
        if self.check_tty_num(player_name) >= max_tty_num:
            self.server.get_player(player_name).send_error_message("你的领地数量已达到上限,无法增加新的领地")
            return
        if self.is_territory_overlapping(pos1,pos2,dim) == True:
            self.server.get_player(player_name).send_error_message("此区域与其他玩家领地重叠")
            return
        if self.is_point_in_cube(tppos,pos1,pos2) == False:
            self.server.get_player(player_name).send_error_message("你当前所在的位置不在你要添加的领地上!禁止远程施法")
            return
        else:
            # 以当前时间作为领地名
            current_time = datetime.now().strftime("%Y%m%d%H%M%S")
            new_ttydata = (current_time,pos1[0],pos1[1],pos1[2],pos2[0],pos2[1],pos2[2],tppos[0],tppos[1],tppos[2],player_name,"","",False,False,False,False,False,dim,"")
            self.write_to_database(new_ttydata)
            self.server.get_player(player_name).send_message("成功添加领地")
            # 更新全局数据
            all_tty = self.read_all_territories()

    # 玩家增加子领地函数
    def player_add_child_tty(self,player_name,pos1,pos2,tppos,dim):
        """
        player_name: 玩家名
        pos1: 领地坐标1
        pos2: 领地坐标2
        tppos: 领地传送点
        dim: 领地维度
        """
        global all_tty
        
        if self.check_tty_num(player_name) >= max_tty_num:
            self.server.get_player(player_name).send_error_message("你的领地数量已达到上限,无法增加新的领地")
            return
        if self.is_point_in_cube(tppos,pos1,pos2) == False:
            self.server.get_player(player_name).send_error_message("你当前所在的位置不在你要添加的领地上!禁止远程施法")
            return
        # 父领地检查
        father_tty_info = self.list_true_father_tty(player_name,(pos1,pos2),dim)
        if father_tty_info[0] == False:
            self.server.get_player(player_name).send_error_message(father_tty_info[1])
            return
        else:
            # 以当前时间和父领地名作为领地名
            current_time = datetime.now().strftime("%Y%m%d%H%M%S")
            child_tty_name = f"{father_tty_info[1]}_{current_time}"
            new_ttydata = (child_tty_name,pos1[0],pos1[1],pos1[2],pos2[0],pos2[1],pos2[2],tppos[0],tppos[1],tppos[2],player_name,"","",False,False,False,False,False,dim,father_tty_info[1])
            self.write_to_database(new_ttydata)
            self.server.get_player(player_name).send_message(f"成功添加子领地,归属于父领地{father_tty_info[1]}")
            # 更新全局数据
            all_tty = self.read_all_territories()

    def is_subset_cube(self, cube1, cube2):
        """
        判断一个立方体是否为另一个立方体的子集。

        参数：
            cube1: 第一个立方体的对角坐标，格式为 ((x1, y1, z1), (x2, y2, z2))。
            cube2: 第二个立方体的对角坐标，格式为 ((x1, y1, z1), (x2, y2, z2))。

        返回：
            如果 cube1 是 cube2 的子集，则返回 True，否则返回 False。
        """

        # 获取立方体 1 的最小和最大坐标
        cube1_min_x = min(cube1[0][0], cube1[1][0])
        cube1_max_x = max(cube1[0][0], cube1[1][0])
        cube1_min_y = min(cube1[0][1], cube1[1][1])
        cube1_max_y = max(cube1[0][1], cube1[1][1])
        cube1_min_z = min(cube1[0][2], cube1[1][2])
        cube1_max_z = max(cube1[0][2], cube1[1][2])

        # 获取立方体 2 的最小和最大坐标
        cube2_min_x = min(cube2[0][0], cube2[1][0])
        cube2_max_x = max(cube2[0][0], cube2[1][0])
        cube2_min_y = min(cube2[0][1], cube2[1][1])
        cube2_max_y = max(cube2[0][1], cube2[1][1])
        cube2_min_z = min(cube2[0][2], cube2[1][2])
        cube2_max_z = max(cube2[0][2], cube2[1][2])

        # 判断 cube1 是否在 cube2 的范围内
        if (cube1_min_x >= cube2_min_x and cube1_max_x <= cube2_max_x and
                cube1_min_y >= cube2_min_y and cube1_max_y <= cube2_max_y and
                cube1_min_z >= cube2_min_z and cube1_max_z <= cube2_max_z):
            return True
        else:
            return False
        
    # 列出符合子领地条件的父领地
    def list_true_father_tty(self,player_name,child_cube,child_dim):
        """
        用于列出符合领地条件的父领地
        
        参数:
            player_name: 创建子领地的玩家名
            child_cube: 子领地坐标元组((x1,y1,z1),(x2,y2,z2))
            child_dim: 子领地的维度
        返回:
            成功:(True,领地名)
            失败:(False,错误信息)
        """
        true_tty_info = []
        for tty_info in all_tty:
            tty_owner = tty_info['owner']
            tty_manager = tty_info['manager']
            pos1 = tty_info['pos1']
            pos2 = tty_info['pos2']
            dim = tty_info['dim']
            tty_name = tty_info['name']
            # 玩家为领地主或领地管理员
            if player_name == tty_owner or player_name in tty_manager:
                # 领地维度相同
                if child_dim == dim:
                    # 检查全包围领地
                    if self.is_subset_cube(child_cube,(pos1,pos2)) == True:
                        true_tty_info.append(tty_name)
        if len(true_tty_info) > 1:
            msg = f"无法在子领地内创建子领地"
            return False,msg
        if true_tty_info == []:
            msg = f"未查找到符合条件的父领地,子领地创建失败"
            return False,msg
        if len(true_tty_info) == 1:
            return True,true_tty_info[0]
        else:
            msg = f"未知错误"
            return False,msg
    
    # 列出玩家领地函数
    def list_player_tty(self,player_name):
        """
        列出玩家的所有领地
        
        player_name: 玩家名
        """
        # 初始化一个空值
        player_all_tty = []
        # 获取全部领地
        rows = all_tty
        for row in rows:
            if row['owner'] == player_name:
                player_all_tty.append(row)
        if player_all_tty == []:
            return None
        return player_all_tty
    
    # 删除玩家领地函数
    def del_player_tty(self,tty_name):
        """
        用于删除玩家领地的函数
        
        ttyname: 领地名
        """
        global all_tty
        
        try:
            with self.cursor() as cur:
                # 查找所有 father_tty 为 tty_name 的领地
                cur.execute('SELECT name FROM territories WHERE father_tty=?', (tty_name,))
                related_territories = cur.fetchall()

                # 如果存在相关领地，则更新其 father_tty 为空字符串
                if related_territories:
                    for related_tty in related_territories:
                        cur.execute('UPDATE territories SET father_tty=? WHERE name=?', ('', related_tty[0]))

                # 执行删除操作
                cur.execute('DELETE FROM territories WHERE name=?', (tty_name,))
                if cur.rowcount > 0:
                    # 更新全局领地信息
                    all_tty = self.read_all_territories()
                    return True
                else:
                    return False
        except sqlite3.Error as e:
            self.logger.error(f"删除领地时发生错误: {e}")
            return False
        
    # 重命名玩家领地函数
    def rename_player_tty(self,oldname,newname):
        """
        用于重命名玩家领地的函数
        
        oldname: 领地现在名
        newname: 领地新名
        """
        global all_tty
        
        try:
            # 检查新名字是否已存在
            with self.cursor() as cur:
                cur.execute('SELECT COUNT(*) FROM territories WHERE name=?', (newname,))
                count = cur.fetchone()[0]
                if count > 0:
                    msg = f"无法重命名领地: 新名字 {newname} 已存在"
                    return False,msg

            # 执行重命名操作
            with self.cursor() as cur:
                # 查找所有 father_tty 为 旧父领地名的领地
                cur.execute('SELECT name FROM territories WHERE father_tty=?', (oldname,))
                related_territories = cur.fetchall()

                # 如果存在相关领地，则更新其 father_tty 为新的父领地名
                if related_territories:
                    for related_tty in related_territories:
                        cur.execute('UPDATE territories SET father_tty=? WHERE name=?', (newname, related_tty[0]))

                cur.execute('UPDATE territories SET name=? WHERE name=?', (newname, oldname))
                if cur.rowcount > 0:
                    msg = f"已重命名领地: 从 {oldname} 到 {newname}"
                    # 更新全局领地信息
                    all_tty = self.read_all_territories()
                    return True,msg
                else:
                    msg = f"尝试重命名领地但未找到: {oldname}"
                    return False,msg
        except sqlite3.Error as e:
            msg = f"重命名领地时发生错误: {e}"
            return False,msg
        
    # 检查领地主人函数
    def check_tty_owner(self,ttyname,player_name):
        """
        用于检查玩家是否是领地主人的函数
        
        ttyname: 领地名
        playername: 玩家名
        """
        for row in all_tty:
            if row['name'] == ttyname:
                # 领地主人匹配
                if row['owner'] == player_name:
                    return True
                # 不匹配
                else:
                    return False
        # 未找到领地
        return None
    
    # 领地权限变更函数
    def change_tty_permissions(self,ttyname,permission,value):
        """
        更改领地权限的函数
        ttyname: 领地名
        permission: 权限名
        value: 权限值
        """
        global all_tty
        # 定义允许变更的权限列表
        allowed_permissions = ['if_jiaohu', 'if_break', 'if_tp', 'if_build', 'if_bomb']
        
        # 检查权限名是否合法
        if permission not in allowed_permissions:
            msg = f"无效的权限名: {permission}"
            return False,msg
        
        try:
            with self.cursor() as cur:
                # 执行更新操作
                update_query = f'UPDATE territories SET {permission}=? WHERE name=?'
                cur.execute(update_query, (value, ttyname))
                
                if cur.rowcount > 0:
                    msg = f"已更新领地 {ttyname} 的权限 {permission} 为 {value}"
                    # 更新全局领地信息
                    all_tty = self.read_all_territories()
                    return True,msg
                else:
                    msg = f"尝试更新领地权限但未找到领地: {ttyname}"
                    return False,msg
        except sqlite3.Error as e:
            msg = f"更新领地权限时发生错误: {e}"
            return False,msg
        
    # 列出领地的主人、管理员、成员的函数
    def list_tty_everyone(self,ttyname):
        """
        用于列出领地的主人、管理员和成员的函数
        
        ttyname: 领地名
        """
        for row in all_tty:
            if row['name'] == ttyname:
                tty_everyone = {
                    'owner': row['owner'],
                    'manager': row['manager'],
                    'member': row['member']
                }
                return tty_everyone
        return None
    
    # 列出所有玩家为领地的主人、管理员的领地的函数
    def list_tty_op(self,player_name):
        """
        用于列出所有玩家为领地的主人、管理员的领地的函数
        
        player_name: 玩家名
        """
        op_tty_list = []
        for row in all_tty:
            if player_name == row['owner'] or player_name == row['manager']:
                op_tty_list.append(row)
            
        if op_tty_list == []:
            return None
        else:
            return op_tty_list
    
    
    # 添加或删除领地成员的函数
    def change_tty_member(self,ttyname,action,player_name):
        """
        用于添加或删除领地成员的函数
        ttyname: 领地名
        action: 操作add或remove
        playername: 玩家名
        """
        global all_tty
        if action not in ['add', 'remove']:
            msg = f"无效的操作类型: {action}"
            return False,msg
        
        try:        
            with self.cursor() as cur:
                # 获取当前成员列表
                cur.execute('SELECT member FROM territories WHERE name=?', (ttyname,))
                row = cur.fetchone()
                
                if not row:
                    msg = f"尝试更改领地成员但未找到领地: {ttyname}"
                    return False,msg
                
                current_members = row[0].split(',') if row[0] else []
                
                if action == 'add' and player_name not in current_members:
                    current_members.append(player_name)
                    msg = f"已添加成员到领地: {player_name} -> {ttyname}"
                elif action == 'remove' and player_name in current_members:
                    current_members.remove(player_name)
                    msg = f"已从领地中移除成员: {player_name} <- {ttyname}"
                else:
                    msg = f"无需变更成员: {player_name} 在领地 {ttyname} 中的状态未改变"
                    return True,msg  # 成员状态未变，但仍视为成功

                # 更新成员列表
                updated_members = ','.join(current_members) if current_members else ''
                cur.execute('UPDATE territories SET member=? WHERE name=?', (updated_members, ttyname))
                
                if cur.rowcount > 0:
                    # 更新全局领地信息
                    all_tty = self.read_all_territories()
                    return True,msg
                else:
                    msg = f"更新领地成员时发生未知错误: {ttyname}"
                    return False,msg
        except sqlite3.Error as e:
            msg = f"变更领地成员时发生错误: {e}"
            return False,msg
        
    # 领地转让函数
    def change_tty_owner(self,ttyname,old_owner_name,new_owner_name):
        """
        用于转让领地的函数
        ttyname: 领地名
        old_owner_name: 原主人玩家名
        new_owner_name: 新主人玩家名
        """
        global all_tty
        if self.check_tty_num(new_owner_name) >= max_tty_num:
            msg = f"玩家 {new_owner_name} 的领地数量已达到上限,无法增加新的领地,转让领地失败"
            return False,msg
        try:        
            with self.cursor() as cur:
                # 获取当前成员列表
                cur.execute('SELECT owner FROM territories WHERE name=?', (ttyname,))
                row = cur.fetchone()
                
                if not row:
                    msg = f"尝试更改领地成员但未找到领地: {ttyname}"
                    return False,msg
                
                current_owner = row[0]
                
                if current_owner == old_owner_name and not current_owner == new_owner_name:
                    current_owner = new_owner_name
                    msg = f"领地转让成功,领地主人已由 {old_owner_name} 变更为 {new_owner_name}"
                else:
                    msg = f"领地转让失败,请检查是否为领地主人以及转让对象是否合规"
                    return False,msg

                # 更新领地主人信息
                cur.execute('UPDATE territories SET owner=? WHERE name=?', (current_owner, ttyname))
                
                if cur.rowcount > 0:
                    # 更新全局领地信息
                    all_tty = self.read_all_territories()
                    return True,msg
                else:
                    msg = f"更新领地主人时发生未知错误: {ttyname}"
                    return False,msg
        except sqlite3.Error as e:
            msg = f"变更领地主人时发生错误: {e}"
            return False,msg
        
    # 添加或删除领地管理员的函数
    def change_tty_manager(self,ttyname,action,player_name):
        """
        用于添加或删除领地管理员的函数
        ttyname: 领地名
        action: 操作add或remove
        playername: 玩家名
        """
        global all_tty
        if action not in ['add', 'remove']:
            msg = f"无效的操作类型: {action}"
            return False,msg
        
        try:        
            with self.cursor() as cur:
                # 获取当前领地管理员列表
                cur.execute('SELECT manager FROM territories WHERE name=?', (ttyname,))
                row = cur.fetchone()
                
                if not row:
                    msg = f"尝试更改领地管理员但未找到领地: {ttyname}"
                    return False,msg
                
                current_members = row[0].split(',') if row[0] else []
                
                if action == 'add' and player_name not in current_members:
                    current_members.append(player_name)
                    msg = f"已添加领地管理员到领地: {player_name} -> {ttyname}"
                elif action == 'remove' and player_name in current_members:
                    current_members.remove(player_name)
                    msg = f"已从领地中移除领地管理员: {player_name} <- {ttyname}"
                else:
                    msg = f"无需变更领地管理员: {player_name} 在领地 {ttyname} 中的状态未改变"
                    return True,msg  # 领地管理员状态未变，但仍视为成功

                # 更新领地管理员列表
                updated_members = ','.join(current_members) if current_members else ''
                cur.execute('UPDATE territories SET manager=? WHERE name=?', (updated_members, ttyname))
                
                if cur.rowcount > 0:
                    # 更新全局领地信息
                    all_tty = self.read_all_territories()
                    return True,msg
                else:
                    msg = f"更新领地管理员时发生未知错误: {ttyname}"
                    return False,msg
        except sqlite3.Error as e:
            msg = f"变更领地管理员时发生错误: {e}"
            return False,msg
        
    # 领地传送点变更函数
    def change_tty_tppos(self,ttyname,tppos,dim):
        """
        更改领地传送点的函数
        ttyname: 领地名
        tppos: 领地传送点坐标
        """
        global all_tty
        
        # 检查领地传送点是否合法
        for row in all_tty:
            if row['name'] == ttyname:
                if not row['dim'] == dim:
                    msg = f"你当前所在的维度({dim})与领地维度({row['dim']})不匹配,无法设置领地传送点"
                    return False,msg
                if self.is_point_in_cube(tppos,row['pos1'],row['pos2']) == False:
                    msg = f"无法接受的坐标{tppos},领地传送点不能位于领地之外!"
                    return False,msg
        
        try:
            with self.cursor() as cur:
                # 执行更新操作
                update_query = 'UPDATE territories SET tppos_x=?, tppos_y=?, tppos_z=? WHERE name=?'
                cur.execute(update_query, (*tppos, ttyname))
                
                if cur.rowcount > 0:
                    msg = f"已更新领地 {ttyname} 的传送点为 {tppos}"
                    # 更新全局领地信息
                    all_tty = self.read_all_territories()
                    return True,msg
                else:
                    msg = f"尝试更新领地传送点但未找到领地: {ttyname}"
                    return False,msg
        except sqlite3.Error as e:
            msg = f"更新领地传送点时发生错误: {e}"
            return False,msg
        
    # 检查玩家是否为领地主人或管理员的函数
    def check_tty_op(self,ttyname,player_name):
        """
        检查玩家是否为领地主人或者管理员
        
        ttyname: 领地名
        play_name: 玩家名
        """
        everyone = self.list_tty_everyone(ttyname)
        if everyone == None:
            return None
        if player_name == everyone['owner'] or player_name == everyone['manager']:
            return True
        else:
            return False
        
    # 对在线玩家进行领地提示
    def tips_online_players(self):
        """
        用于对在线玩家进行领地提示的函数
        """

        # 遍历全部玩家
        for online_player in self.server.online_players:
            player_pos = (int(online_player.location.x), int(online_player.location.y), int(online_player.location.z))
            player_dim = online_player.location.dimension.name
            player_name = online_player.name
            
            for row in all_tty:
                if row['dim'] == player_dim and row['father_tty'] != '':  # 检查子领地
                    if self.is_point_in_cube(player_pos, row['pos1'], row['pos2']):
                        msg = f"你现在正在 {row['owner']} 的子领地 {row['name']} 上"
                        self.server.get_player(player_name).send_tip(msg)

                elif row['dim'] == player_dim and row['father_tty'] == '':  # 检查父领地
                    if self.is_point_in_cube(player_pos, row['pos1'], row['pos2']):
                        msg = f"你现在正在 {row['owner']} 的领地 {row['name']} 上"
                        self.server.get_player(player_name).send_tip(msg)
                        
                        
        
    # 用于获取在线玩家列表的函数
    def get_online_player_list(self):
        """
        用于获取在线玩家的函数
        """
        online_player_list = []
        for players in self.server.online_players:
            online_player_list.append(players.name)
        # 理论上玩家调用菜单的时候至少有一个玩家在线,列表不会为空,不过万一以后控制台命令需要的时候或许可以用上
        if online_player_list == []:
            return None
        return online_player_list
    
    # 列出所有玩家权限为成员及以上的领地的函数
    def list_member_tty(self,player_name):
        """
        用于列出所有玩家权限为成员及以上的领地的函数
        
        player_name: 玩家名
        """
        member_tty_list = []
        for row in all_tty:
            if player_name == row['owner'] or player_name == row['manager'] or player_name == row['member']:
                member_tty_list.append(row)
            
        if member_tty_list == []:
            return None
        else:
            return member_tty_list
        
    # 列出与坐标点重合的全部领地的函数
    def list_in_tty(self,pos,dim):
        """
        用于列出与坐标点重合的全部领地的函数
        
        参数:
            pos: 点坐标
            dim: 点维度
        返回:
            in_tty: 包含领地名0、交互权限1、破坏权限2、放置权限3、爆炸权限4、领地成员5、领地主6的元组的列表
        """
        # 初始化领地列表
        in_tty = []
        for row in all_tty:
            # 维度匹配
            if row['dim'] == dim:
                # 坐标匹配
                if self.is_point_in_cube(pos,row['pos1'],row['pos2']) == True:
                    # 子领地
                    if not row['father_tty'] == "":
                        # 清空领地列表,仅保留子领地
                        in_tty = []
                        in_tty.append((row['name'],row['if_jiaohu'],row['if_break'],row['if_build'],row['if_bomb'],(row['owner'].split(',') + row['manager'].split(',') + row['member'].split(',')),row['owner']))
                        return in_tty
                    # 非子领地
                    else:
                        in_tty.append((row['name'],row['if_jiaohu'],row['if_break'],row['if_build'],row['if_bomb'],(row['owner'].split(',') + row['manager'].split(',') + row['member'].split(',')),row['owner']))
        return in_tty

    
    
    def on_load(self) -> None:
        self.logger.info("on_load is called!")
        self.connect()  # 确保在加载时连接数据库
        self.init_db() # 初始化数据库

    def on_enable(self) -> None:
        global all_tty
        self.logger.info("on_enable is called!")
        self.logger.info(f"{ColorFormat.YELLOW}Territory领地插件已启用 版本{self.server.plugin_manager.get_plugin("territory")._get_description().version}")
        self.register_events(self)
        self.read_config()
        # 插件加载时获取一次全部领地信息
        all_tty = self.read_all_territories()
        # 启动周期后台任务
        self.server.scheduler.run_task(self,self.tips_online_players,delay=0,period=25)

    def on_disable(self) -> None:
        self.logger.info("on_disable is called!")
        # 关闭数据库连接
        if self.conn:
            self.conn.close()

    commands = {
        "territory": {
            "description": "领地命令--用法:",
            "usages": [
                "/territory (add)<opt: optadd> [pos: pos] [pos: pos]",
                "/territory (add_sub)<opt: optadd_sub> [pos: pos] [pos: pos]",
                "/territory (list)<opt: optlist>",
                "/territory (del)<opt: optdel> <msg: message>",
                "/territory (rename)<opt: optrename> <msg: message> <msg: message>",
                "/territory (set)<opt: optsetper> (if_jiaohu|if_break|if_tp|if_build|if_bomb)<opt: optpermission> <bool: bool> <msg: message>",
                "/territory (member)<opt: optmember> (add|remove)<opt: optmemar> <player: target> <msg: message>",
                "/territory (manager)<opt: optmanager> (add|remove)<opt: optman> <player: target> <msg: message>",
                "/territory (settp)<opt: optsettp> [pos: pos] <msg: message>",
                "/territory (transfer)<opt: opttransfer> <msg: message> <msg: message>",
                "/territory (tp)<opt: opttp> <msg: message>",
                "/territory (help)<opt: opthelp>",
                "/territory"
                ],
            "permissions": ["tty.command.member"],
        },
        "tty": {
            "description": "领地命令--用法:",
            "usages": [
                "/tty (add)<opt: optadd2> [pos: pos] [pos: pos]",
                "/tty (add_sub)<opt: optadd_sub2> [pos: pos] [pos: pos]",
                "/tty (list)<opt: optlist2>",
                "/tty (del)<opt: optdel2> <msg: message>",
                "/tty (rename)<opt: optrename2> <msg: message> <msg: message>",
                "/tty (set)<opt: optsetper2> (if_jiaohu|if_break|if_tp|if_build|if_bomb)<opt: optpermission2> <bool: bool> <msg: message>",
                "/tty (member)<opt: optmember2> (add|remove)<opt: optmemar2> <player: target> <msg: message>",
                "/tty (manager)<opt: optmanager2> (add|remove)<opt: optman2> <player: target> <msg: message>",
                "/tty (settp)<opt: optsettp2> [pos: pos] <msg: message>",
                "/tty (transfer)<opt: opttransfer2> <msg: message> <msg: message>",
                "/tty (tp)<opt: opttp2> <msg: message>",
                "/tty (help)<opt: opthelp2>",
                "/tty"
                ],
            "permissions": ["tty.command.member"],
        },
        "reloadtty": {
            "description": "重载领地命令",
            "usages": [
                "/reloadtty"
                ],
            "permissions": ["tty.command.op"],
        },
        "optty": {
            "description": "管理员管理领地",
            "usages": [
                "/optty (del)<opt: optdel3> <msg: message>",
                "/optty (delall)<opt: optdel4> <msg: message>"
                ],
            "permissions": ["tty.command.op"],
        }
    }
    permissions = {
        "tty.command.op": {
            "description": "1",
            "default": "op", 
        },
        "tty.command.member": {
            "description": "1",
            "default": True, 
        }
    }
    
    def on_command(self, sender: CommandSender, command: Command, args: list[str]) -> bool:
        global all_tty
        if command.name == "territory" or command.name == "tty":
            if not isinstance(sender,Player):
                self.server.logger.error("此命令无法在服务端执行")
                return
            if len(args) == 3:
                if not args[0] or not args[1] or not args[2]:
                    sender.send_error_message("缺失参数")
                    return
                # 添加领地
                if args[0] == "add":
                    try:
                        if "~" in args[1] or "~" in args[2]:
                            sender.send_error_message("请勿使用~来代表坐标,插件无法解析")
                            return
                        pos1 = tuple(map(float, args[1].split()))
                        pos2 = tuple(map(float, args[2].split()))
                        tppos = (int(self.server.get_player(sender.name).location.x),int(self.server.get_player(sender.name).location.y),int(self.server.get_player(sender.name).location.z))
                        dim = self.server.get_player(sender.name).location.dimension.name
                        self.player_add_tty(sender.name,pos1,pos2,tppos,dim)
                    except Exception as e:
                        sender.send_error_message(e)
                        return
                # 添加子领地
                if args[0] == "add_sub":
                    try:
                        if "~" in args[1] or "~" in args[2]:
                            sender.send_error_message("请勿使用~来代表坐标,插件无法解析")
                            return
                        pos1 = tuple(map(float, args[1].split()))
                        pos2 = tuple(map(float, args[2].split()))
                        tppos = (int(self.server.get_player(sender.name).location.x),int(self.server.get_player(sender.name).location.y),int(self.server.get_player(sender.name).location.z))
                        dim = self.server.get_player(sender.name).location.dimension.name
                        self.player_add_child_tty(sender.name,pos1,pos2,tppos,dim)
                    except Exception as e:
                        sender.send_error_message(e)
                        return
                # 重命名领地
                if args[0] == "rename":
                    try:
                        # 检查领地主人
                        checkresult = self.check_tty_owner(args[1],sender.name)
                        # 确认是主人
                        if checkresult == True:
                            status,msg = self.rename_player_tty(args[1],args[2])
                            if status == True:
                                sender.send_message(f"{ColorFormat.YELLOW}{msg}")
                                return
                            else:
                                sender.send_error_message(f"{msg}")
                                return
                        # 不是主人
                        elif checkresult == False:
                            sender.send_error_message("这不是你的领地,你无权重命名")
                        else:
                            sender.send_error_message("领地不存在")
                    except Exception as e:
                        sender.send_error_message(e)
                        return
                    
                # 设置领地传送点
                if args[0] == "settp":
                    # 检查权限
                    if not self.check_tty_op(args[2],sender.name) == True:
                        sender.send_error_message("你没有权限设置该领地的传送点,只有领地主人和领地管理员有权设置")
                        return
                    tppos = tuple(int(float(x)) for x in args[1].split())
                    dim = self.server.get_player(sender.name).location.dimension.name
                    status,msg = self.change_tty_tppos(args[2],tppos,dim)
                    if status == True:
                        sender.send_message(f"{ColorFormat.YELLOW}{msg}")
                    else:
                        sender.send_error_message(msg)

                
                # 领地转让
                if args[0] == "transfer":
                    tty_name = args[1]
                    everyone = self.list_tty_everyone(tty_name)
                    if everyone == None:
                        sender.send_error_message("领地不存在或错误的命令")
                        return
                    # 检测权限
                    if not sender.name == everyone['owner']:
                        sender.send_error_message("你没有转让此领地的权限,只有领地主人能转让")
                        return
                    online_players_name = []
                    for online_players in self.server.online_players:
                        online_players_name.append(online_players.name)
                    # 新主人玩家在线
                    if args[2] in online_players_name:
                        # 正常情况
                        status,msg = self.change_tty_owner(tty_name,sender.name,args[2])
                        if status == True:
                            sender.send_message(f"{ColorFormat.YELLOW}{msg}")
                            self.server.get_player(args[2]).send_message(f"{ColorFormat.YELLOW}{msg}")
                        else:
                            sender.send_error_message(msg)
                    # 不在线
                    else:
                        # 正常情况
                        status,msg = self.change_tty_owner(tty_name,sender.name,args[2])
                        if status == True:
                            sender.send_message(f"{ColorFormat.YELLOW}{msg}")
                        else:
                            sender.send_error_message(msg)
                    

            elif len(args) == 1:
                if not args[0]:
                    sender.send_error_message("缺失参数")
                    return
                # 列出领地
                if args[0] == "list":
                    tty_list = self.list_player_tty(sender.name)
                    if tty_list == None:
                        sender.send_error_message("你没有领地")
                        return
                    
                    sender.send_message(f"{ColorFormat.YELLOW}以下是你的全部领地:\n")
                    
                    output_item = ""
                    for idx, tty in enumerate(tty_list, start=1):
                        # 简化输出，只展示关键信息
                        message = f"{ColorFormat.YELLOW}{idx}. 领地名称: {tty['name']}\n位置范围: {tty['pos1']} 到 {tty['pos2']}\n传送点位置: {tty['tppos']}\n维度: {tty['dim']}\n是否允许玩家交互: {tty['if_jiaohu']}\n是否允许玩家破坏: {tty['if_break']}\n是否允许外人传送: {tty['if_tp']}\n是否允许放置方块: {tty['if_build']}\n是否允许实体爆炸: {tty['if_bomb']}\n领地管理员: {tty['manager']}\n领地成员: {tty['member']}\n"
                        output_item += message + "-" * 20 + "\n"
                    list_form = ActionForm(
                        title="§l领地列表",
                        content=f"{output_item}"
                    )
                    sender.send_form(list_form)
                if args[0] == "help":
                    sender.send_message("新建领地--/tty add 领地边角坐标1 领地边角坐标2\n列出领地--/tty list\n删除领地--/tty del 领地名\n重命名领地--/tty rename 旧领地名 新领地名\n设置领地权限--/tty set 权限名 权限值 领地名\n设置领地管理员--/tty manager add|remove(添加|删除) 玩家名 领地名\n设置领地成员--/tty member add|remove(添加|删除) 玩家名 领地名\n设置领地传送点--/tty settp 领地传送坐标 领地名\n传送领地--/tty tp 领地名\n")
            elif len(args) == 2:
                if not args[0] or not args[1]:
                    sender.send_error_message("缺失参数")
                    return
                # 删除领地
                if args[0] == "del":
                    tty_name = args[1]
                    # 获取领地信息
                    tty_info = self.read_territory_by_name(tty_name)
                    # 检查领地是否存在
                    if tty_info == None:
                        sender.send_error_message("该领地不存在")
                        return
                    if not tty_info['owner'] == sender.name:
                        sender.send_error_message("这不是你的领地,你无权删除")
                        return
                    self.del_player_tty(tty_name)
                    sender.send_message(f"{ColorFormat.YELLOW}已成功删除领地")
                    
                if args[0] == "tp":
                    tty_info = self.read_territory_by_name(args[1])
                    if tty_info == None:
                        sender.send_error_message("此领地不存在")
                        return
                    dim = tty_info['dim']
                    match dim:
                        case "TheEnd":
                            dim="the_end"
                    if tty_info == None:
                        sender.send_error_message("该领地不存在")
                        return
                    
                    if sender.name in tty_info['owner'] or sender.name in tty_info['manager'] or tty_info['member']:
                        sender.send_message(f"{ColorFormat.YELLOW}已将您传送至领地 {args[1]}")
                        self.server.dispatch_command(CommandSenderWrapper(self.server.command_sender), f'execute in {dim} run tp "{sender.name}" {tty_info['tppos'][0]} {tty_info['tppos'][1]} {tty_info['tppos'][2]}')
                        return
                    elif tty_info['if_tp'] == 1:
                        sender.send_message(f"{ColorFormat.YELLOW}已将您传送至领地 {args[1]}")
                        self.server.dispatch_command(CommandSenderWrapper(self.server.command_sender), f'execute in {dim} run tp "{sender.name}" {tty_info['tppos'][0]} {tty_info['tppos'][1]} {tty_info['tppos'][2]}')
                        return
                    else:
                        sender.send_error_message("你没有传送到该领地的权限")
                        return
            
            elif len(args) == 4:
                if not args[0] or not args[1] or not args[2] or not args[3]:
                    sender.send_error_message("缺失参数")
                    return
                # 设置领地权限
                if args[0] == "set":
                    # 检查领地主人
                    if args[2] == "true":
                        value = 1
                    elif args[2] == "false":
                        value = 0
                    else:
                        sender.send_error_message(f"未知的值{args[2]}")
                        return
                    if self.check_tty_owner(args[3],sender.name) == True:
                        status,msg = self.change_tty_permissions(args[3],args[1],value)
                        if status == True:
                            sender.send_message(f"{ColorFormat.YELLOW}{msg}")
                        else:
                            sender.send_error_message(f"{msg}")
                    else:
                        sender.send_error_message("领地不存在或者不是你的领地")
                        
                # 更改领地成员
                if args[0] == "member":
                    everyone = self.list_tty_everyone(args[3])
                    if everyone == None:
                        sender.send_error_message("领地不存在或错误的命令")
                        return
                    # 检测权限
                    if sender.name == everyone['owner'] or sender.name in everyone['manager']:
                        pass
                    else:
                        sender.send_error_message("你没有变更此领地的权限,只有领地主人和领地管理员能变更")
                        return
                    
                    if args[1] in ["add","remove"]:
                        
                        # 在成员不为空时进行成员检测
                        if not everyone == None:
                            # 添加一个重复玩家
                            if args[1] == "add" and args[2] in everyone['member']:
                                sender.send_error_message("该玩家已经是领地成员了,请勿重复添加")
                                return
                            # 删除一个不存在的玩家
                            if args[1] == "remove" and not args[2] in everyone['member']:
                                sender.send_error_message("该玩家不在领地成员中")
                                return
                        # 正常情况
                        status,msg = self.change_tty_member(args[3],args[1],args[2])
                        if status == True:
                            sender.send_message(f"{ColorFormat.YELLOW}{msg}")
                        else:
                            sender.send_error_message(msg)
                            
                # 更改领地管理员
                if args[0] == "manager":
                    everyone = self.list_tty_everyone(args[3])
                    if everyone == None:
                        sender.send_error_message("领地不存在或错误的命令")
                        return
                    # 检测权限
                    if not sender.name == everyone['owner']:
                        sender.send_error_message("你没有变更此领地管理员的权限,只有领地主人能变更")
                        return
                    
                    if args[1] in ["add","remove"]:
                        
                        # 在管理员不为空时进行管理员检测
                        if not everyone == None:
                            # 添加一个重复玩家
                            if args[1] == "add" and args[2] in everyone['manager']:
                                sender.send_error_message("该玩家已经是领地管理员了,请勿重复添加")
                                return
                            # 删除一个不存在的玩家
                            if args[1] == "remove" and not args[2] in everyone['manager']:
                                sender.send_error_message("该玩家不在领地管理员中")
                                return
                        # 正常情况
                        status,msg = self.change_tty_manager(args[3],args[1],args[2])
                        if status == True:
                            sender.send_message(f"{ColorFormat.YELLOW}{msg}")
                        else:
                            sender.send_error_message(msg)
                            
            elif len(args) == 0:
                # 图形界面
                
                # 创建领地功能
                def create_tty():
                    def run_create_tty(sender,json_str:str):
                        try:
                            fst_pos1 = json.loads(json_str)[0]
                            fst_pos2 = json.loads(json_str)[1]
                            pos1 = tuple(int(float(x)) for x in fst_pos1.split())
                            pos2 = tuple(int(float(x)) for x in fst_pos2.split())
                            sender.perform_command(f"territory add {pos1[0]} {pos1[1]} {pos1[2]} {pos2[0]} {pos2[1]} {pos2[2]}")
                        except:
                            sender.send_error_message("错误的坐标!")
                            return

                    def on_click(sender):
                        create_tty_form = ModalForm(
                            title=f"{ColorFormat.DARK_PURPLE}§l新建领地",
                            controls=[
                                TextInput(label="§l输入领地边角坐标1"),
                                TextInput(label="§l输入领地边角坐标2")
                            ],
                            on_submit=run_create_tty,
                            on_close=run_command(com="tty")
                        )
                        sender.send_form(create_tty_form)
                    return on_click
                
                # 重命名领地功能
                def rename_tty():
                    tty_list = self.list_player_tty(sender.name)
                    
                    def run_rename_tty(sender,json_str:str):
                        try:
                            index = int(json.loads(json_str)[0])
                            newname = json.loads(json_str)[1]
                            oldname = tty_list[index]['name']
                            sender.perform_command(f'territory rename "{oldname}" "{newname}"')
                        except:
                            sender.send_error_message("未知的错误")
                    def on_click(sender):
                        #  按钮按下时及时检测领地存在
                        if tty_list == None:
                            sender.send_error_message("未查找到领地")
                            return
                        option = []
                        for idx, tty in enumerate(tty_list, start=1):
                            ttyname = tty['name']
                            option.append(ttyname)
                        rename_tty_form = ModalForm(
                            title=f"{ColorFormat.DARK_PURPLE}§l重命名领地",
                            controls=[
                                Dropdown(label="§l选择要重命名的领地",options=option),
                                TextInput(label="§l新名字")
                            ],
                            on_submit=run_rename_tty,
                            on_close=run_command(com="tty")
                        )
                        sender.send_form(rename_tty_form)
                    return on_click
                
                # 领地传送功能
                def tp_tty():
                    tty_list = self.list_member_tty(sender.name)

                    def run_tp_tty(sender,json_str:str):
                        try:
                            index = int(json.loads(json_str)[0])
                            ttyname = tty_list[index]['name']
                            sender.perform_command(f'territory tp "{ttyname}"')
                        except:
                            sender.send_error_message("未知的错误")
                    
                    def on_click(sender):
                        #  按钮按下时及时检测领地存在
                        if tty_list == None:
                            sender.send_error_message("未查找到领地")
                            return
                        option = []
                        for tty in tty_list:
                            ttyname = tty['name']
                            option.append(ttyname)
                        tp_tty_form = ModalForm(
                            title="传送领地",
                            controls=[
                                Dropdown(label="§l选择你要传送的领地",options=option)
                            ],
                            on_submit=run_tp_tty,
                            on_close=run_command(com="tty")
                        )
                        sender.send_form(tp_tty_form)
                    return on_click
                
                # 传送全部领地功能
                def tp_all_tty():
                    def run_tp_all_tty(sender,json_str:str):
                        try:
                            ttyname = json.loads(json_str)[0]
                            sender.perform_command(f'territory tp "{ttyname}"')
                        except:
                            sender.send_error_message("未知的错误")
                    
                    def on_click(sender):
                        tp_all_tty_form = ModalForm(
                            title="传送领地",
                            controls=[
                                TextInput(label="§l输入你要传送的领地"),
                            ],
                            on_submit=run_tp_all_tty,
                            on_close=run_command(com="tty")
                        )
                        sender.send_form(tp_all_tty_form)
                    return on_click
                
                # 领地管理
                def man_tty():
                    def man_tty_main_menu(sender):
                        # 设置权限菜单
                        def set_permis():
                            tty_list = self.list_tty_op(sender.name)
                            
                            # 权限子菜单
                            def man_tty_sub_menu(sender,json_str:str):
                                index = int(json.loads(json_str)[0])
                                ttyname = tty_list[index]['name']
                                if_jiaohu_status = tty_list[index]['if_jiaohu']
                                if_break_status = tty_list[index]['if_break']
                                if_tp_status = tty_list[index]['if_tp']
                                if_build_status = tty_list[index]['if_build']
                                if_bomb_status = tty_list[index]['if_bomb']
                                
                                # 执行领地权限更改
                                def run_mtsm(sender,json_str:str):
                                    permis = json.loads(json_str)
                                    if not if_jiaohu_status == permis[0] or not if_break_status == permis[1] or not if_tp_status == permis[2] or not if_build_status == permis[3] or not if_bomb_status == permis[4]:
                                        if not if_jiaohu_status == permis[0]:
                                            sender.perform_command(f'territory set if_jiaohu {str(permis[0]).lower()} "{ttyname}"')
                                            
                                        if not if_break_status == permis[1]:
                                            sender.perform_command(f'territory set if_break {str(permis[1]).lower()} "{ttyname}"')
                                            
                                        if not if_tp_status == permis[2]:
                                            sender.perform_command(f'territory set if_tp {str(permis[2]).lower()} "{ttyname}"')
                                            
                                        if not if_build_status == permis[3]:
                                            sender.perform_command(f'territory set if_build {str(permis[3]).lower()} "{ttyname}"')
                                            
                                        if not if_bomb_status == permis[4]:
                                            sender.perform_command(f'territory set if_bomb {str(permis[4]).lower()} "{ttyname}"')
                                    else:
                                        sender.send_error_message("你未更改领地权限,领地权限不会变化")
                                
                                def mtsm_on_click(sender):
                                    mtsm_form = ModalForm(
                                        title=f"§l管理领地 {ttyname} 的权限",
                                        controls=[
                                            Toggle(label="§l是否允许外人交互",default_value=if_jiaohu_status),
                                            Toggle(label="§l是否允许外人破坏",default_value=if_break_status),
                                            Toggle(label="§l是否允许外人传送",default_value=if_tp_status),
                                            Toggle(label="§l是否允许外人放置",default_value=if_build_status),
                                            Toggle(label="§l是否允许实体爆炸",default_value=if_bomb_status),
                                        ],
                                        on_submit=run_mtsm,
                                        on_close=man_tty_main_menu
                                    )
                                    sender.send_form(mtsm_form)
                                mtsm_on_click(sender)
                            
                            # 权限主菜单
                            def on_click(sender):
                                if tty_list == None:
                                    sender.send_error_message("未查找到领地")
                                    return
                                options = []
                                for tty in tty_list:
                                    ttyname = tty['name']
                                    options.append(ttyname)
                                man_main_form = ModalForm(
                                    title="§l领地权限管理界面",
                                    controls=[
                                        Dropdown(label="§l选择要管理的领地",options=options),
                                    ],
                                    on_submit=man_tty_sub_menu,
                                    on_close=man_tty_main_menu
                                )
                                sender.send_form(man_main_form)
                            return on_click
                        
                        # 删除领地成员菜单
                        def del_tty_member():
                            tty_list = self.list_tty_op(sender.name)
                            
                            # 删除领地成员子菜单
                            def del_tty_member_sub_menu(sender,json_str:str):
                                index = int(json.loads(json_str)[0])
                                ttyname = tty_list[index]['name']
                                tty_member_list = tty_list[index]['member'].split(',')

                                
                                # 执行领地成员更改
                                def run_del_member_tsm(sender,json_str:str):
                                    index = int(json.loads(json_str)[0])
                                    del_member_name = tty_member_list[index]
                                    sender.perform_command(f'territory member remove "{del_member_name}" "{ttyname}"')

                                
                                def del_member_tsm_on_click(sender):
                                    if tty_member_list == ['']:
                                        sender.send_error_message(f"在领地 {ttyname} 中没有任何成员")
                                        return
                                    del_member_tsm_form = ModalForm(
                                        title=f"§l管理领地 {ttyname} 的成员",
                                        controls=[
                                            Dropdown(label="选择要删除的领地成员",options=tty_member_list)
                                        ],
                                        on_submit=run_del_member_tsm,
                                        on_close=man_tty_main_menu
                                    )
                                    sender.send_form(del_member_tsm_form)
                                del_member_tsm_on_click(sender)
                            
                            # 删除领地成员主菜单
                            def on_click(sender):
                                if tty_list == None:
                                    sender.send_error_message("未查找到领地")
                                    return
                                options = []
                                for tty in tty_list:
                                    ttyname = tty['name']
                                    options.append(ttyname)
                                man_main_form = ModalForm(
                                    title="§l领地成员删除界面",
                                    controls=[
                                        Dropdown(label="§l选择要删除成员的领地",options=options),
                                    ],
                                    on_submit=del_tty_member_sub_menu,
                                    on_close=man_tty_main_menu
                                )
                                sender.send_form(man_main_form)
                            return on_click
                        
                        # 添加领地成员菜单
                        def add_tty_member():
                            tty_list = self.list_tty_op(sender.name)
                            
                            # 添加领地成员子菜单
                            def add_tty_member_sub_menu(sender,json_str:str):
                                index = int(json.loads(json_str)[0])
                                ttyname = tty_list[index]['name']
                                online_player_list = self.get_online_player_list()

                                
                                # 执行领地成员更改
                                def run_add_member_tsm(sender,json_str:str):
                                    input_json = json.loads(json_str)
                                    index = int(input_json[0])
                                    add_member_name = online_player_list[index]
                                    leave_player = input_json[1]
                                    # 离线玩家为空时采用在线玩家
                                    if leave_player == "":
                                        sender.perform_command(f'territory member add "{add_member_name}" "{ttyname}"')
                                    # 离线玩家不为空时采用离线玩家
                                    else:
                                        sender.perform_command(f'territory member add "{leave_player}" "{ttyname}"')

                                
                                def add_member_tsm_on_click(sender):
                                    add_member_tsm_form = ModalForm(
                                        title=f"§l添加成员到领地 {ttyname} 中",
                                        controls=[
                                            Dropdown(label="§l选择要添加的在线玩家",options=online_player_list),
                                            TextInput(label="§l添加不在线的玩家",placeholder="只要这里写了一个字都会以此为输入值")
                                        ],
                                        on_submit=run_add_member_tsm,
                                        on_close=man_tty_main_menu
                                    )
                                    sender.send_form(add_member_tsm_form)
                                add_member_tsm_on_click(sender)
                            
                            # 添加领地成员主菜单
                            def on_click(sender):
                                if tty_list == None:
                                    sender.send_error_message("未查找到领地")
                                    return
                                options = []
                                for tty in tty_list:
                                    ttyname = tty['name']
                                    options.append(ttyname)
                                man_main_form = ModalForm(
                                    title="§l领地成员添加界面",
                                    controls=[
                                        Dropdown(label="§l选择要添加成员的领地",options=options),
                                    ],
                                    on_submit=add_tty_member_sub_menu,
                                    on_close=man_tty_main_menu
                                )
                                sender.send_form(man_main_form)
                            return on_click
                        
                        # 领地管理员
                        
                        # 删除领地管理员菜单
                        def del_tty_manager():
                            tty_list = self.list_player_tty(sender.name)
                            
                            # 删除领地管理员子菜单
                            def del_tty_manager_sub_menu(sender,json_str:str):
                                index = int(json.loads(json_str)[0])
                                ttyname = tty_list[index]['name']
                                tty_manager_list = tty_list[index]['manager'].split(',')

                                
                                # 执行领地管理员更改
                                def run_del_manager_tsm(sender,json_str:str):
                                    index = int(json.loads(json_str)[0])
                                    del_manager_name = tty_manager_list[index]
                                    sender.perform_command(f'territory manager remove "{del_manager_name}" "{ttyname}"')

                                
                                def del_manager_tsm_on_click(sender):
                                    if tty_manager_list == ['']:
                                        sender.send_error_message(f"在领地 {ttyname} 中没有任何领地管理员")
                                        return
                                    del_manager_tsm_form = ModalForm(
                                        title=f"§l管理领地 {ttyname} 的领地管理员",
                                        controls=[
                                            Dropdown(label="选择要删除的领地管理员",options=tty_manager_list)
                                        ],
                                        on_submit=run_del_manager_tsm,
                                        on_close=man_tty_main_menu
                                    )
                                    sender.send_form(del_manager_tsm_form)
                                del_manager_tsm_on_click(sender)
                            
                            # 删除领地管理员主菜单
                            def on_click(sender):
                                if tty_list == None:
                                    sender.send_error_message("未查找到领地")
                                    return
                                options = []
                                for tty in tty_list:
                                    ttyname = tty['name']
                                    options.append(ttyname)
                                man_main_form = ModalForm(
                                    title="§l领地管理员删除界面",
                                    controls=[
                                        Dropdown(label="§l选择要删除领地管理员的领地",options=options),
                                    ],
                                    on_submit=del_tty_manager_sub_menu,
                                    on_close=man_tty_main_menu
                                )
                                sender.send_form(man_main_form)
                            return on_click
                        
                        # 添加领地管理员菜单
                        def add_tty_manager():
                            tty_list = self.list_player_tty(sender.name)
                            
                            # 添加领地管理员子菜单
                            def add_tty_manager_sub_menu(sender,json_str:str):
                                index = int(json.loads(json_str)[0])
                                ttyname = tty_list[index]['name']
                                online_player_list = self.get_online_player_list()

                                
                                # 执行领地管理员更改
                                def run_add_manager_tsm(sender,json_str:str):
                                    input_json = json.loads(json_str)
                                    print(input_json)
                                    index = int(input_json[1])
                                    add_manager_name = online_player_list[index]
                                    leave_player = input_json[2]
                                    # 离线玩家为空时采用在线玩家
                                    if leave_player == "":
                                        sender.perform_command(f'territory manager add "{add_manager_name}" "{ttyname}"')
                                    # 离线玩家不为空时采用离线玩家
                                    else:
                                        sender.perform_command(f'territory manager add "{leave_player}" "{ttyname}"')

                                
                                def add_manager_tsm_on_click(sender):
                                    add_manager_tsm_form = ModalForm(
                                        title=f"§l添加领地管理员到领地 {ttyname} 中",
                                        controls=[
                                            Label("§l领地管理员有领地权限设置、成员管理、领地传送点设置的权限,请把握好人选"),
                                            Dropdown(label="§l选择要添加的在线玩家",options=online_player_list),
                                            TextInput(label="§l添加不在线的玩家",placeholder="只要这里写了一个字都会以此为输入值")
                                        ],
                                        on_submit=run_add_manager_tsm,
                                        on_close=man_tty_main_menu
                                    )
                                    sender.send_form(add_manager_tsm_form)
                                add_manager_tsm_on_click(sender)
                            
                            # 添加领地管理员主菜单
                            def on_click(sender):
                                if tty_list == None:
                                    sender.send_error_message("未查找到领地")
                                    return
                                options = []
                                for tty in tty_list:
                                    ttyname = tty['name']
                                    options.append(ttyname)
                                man_main_form = ModalForm(
                                    title="§l领地管理员添加界面",
                                    controls=[
                                        Dropdown(label="§l选择要添加领地管理员的领地",options=options),
                                    ],
                                    on_submit=add_tty_manager_sub_menu,
                                    on_close=man_tty_main_menu
                                )
                                sender.send_form(man_main_form)
                            return on_click
                        
                        # 领地传送点设置菜单
                        
                        def set_tp_tty():
                            tty_list = self.list_tty_op(sender.name)
                            
                            def run_set_tp_tty(sender,json_str:str):
                                try:
                                    index = int(json.loads(json_str)[0])
                                    tppos = json.loads(json_str)[1]
                                    ttyname = tty_list[index]['name']
                                    sender.perform_command(f'territory settp {tppos} "{ttyname}"')
                                except:
                                    sender.send_error_message("未知的错误")
                            def on_click(sender):
                                if tty_list == None:
                                    sender.send_error_message("未查找到领地")
                                    return
                                option = []
                                for idx, tty in enumerate(tty_list, start=1):
                                    ttyname = tty['name']
                                    option.append(ttyname)
                                set_tp_tty_form = ModalForm(
                                    title=f"{ColorFormat.DARK_PURPLE}§l设置领地传送点",
                                    controls=[
                                        Dropdown(label="§l选择要设置传送点的领地",options=option),
                                        TextInput(label="§l坐标")
                                    ],
                                    on_submit=run_set_tp_tty,
                                    on_close=man_tty_main_menu
                                )
                                sender.send_form(set_tp_tty_form)
                            return on_click
                        
                        # 领地转让菜单
                        
                        def transfer_tty():
                            tty_list = self.list_player_tty(sender.name)
                            
                            def run_trunsfer_tty(sender,json_str:str):
                                try:
                                    index = int(json.loads(json_str)[0])
                                    new_owner_name = json.loads(json_str)[1]
                                    ttyname = tty_list[index]['name']
                                    sender.perform_command(f'territory transfer {ttyname} "{new_owner_name}"')
                                except:
                                    sender.send_error_message("未知的错误")
                            def on_click(sender):
                                if tty_list == None:
                                    sender.send_error_message("未查找到领地")
                                    return
                                option = []
                                for idx, tty in enumerate(tty_list, start=1):
                                    ttyname = tty['name']
                                    option.append(ttyname)
                                transfer_tty_form = ModalForm(
                                    title=f"{ColorFormat.DARK_PURPLE}§l转让领地",
                                    controls=[
                                        Dropdown(label="§l选择要转让的领地",options=option),
                                        TextInput(label="§l接收领地的玩家名")
                                    ],
                                    on_submit=run_trunsfer_tty,
                                    on_close=man_tty_main_menu
                                )
                                sender.send_form(transfer_tty_form)
                            return on_click
                        
                        set_permis_button = ActionForm.Button(text="§l管理自己管理的领地权限",on_click=set_permis())
                        del_member_button = ActionForm.Button(text="§l删除自己管理的领地成员",on_click=del_tty_member())
                        add_member_button = ActionForm.Button(text="§l添加自己管理的领地成员",on_click=add_tty_member())
                        
                        del_manager_button = ActionForm.Button(text="§l删除自己领地的领地管理员",on_click=del_tty_manager())
                        add_manager_button = ActionForm.Button(text="§l添加自己领地的领地管理员",on_click=add_tty_manager())
                        
                        set_tp_button = ActionForm.Button(text="§l设置自己管理的领地的传送点",on_click=set_tp_tty())

                        transfer_button = ActionForm.Button(text="§l将自己的领地转让给其他玩家",on_click=transfer_tty())
                        
                        main_menu = ActionForm(
                            title=f"{ColorFormat.DARK_PURPLE}§l领地管理界面",
                            buttons=[set_permis_button,set_tp_button,add_member_button,del_member_button,add_manager_button,del_manager_button,transfer_button],
                            on_close=run_command(com="tty")
                        )
                        sender.send_form(main_menu)
                    return man_tty_main_menu
                      
                def run_command(com):
                    def com_on_click(sender):
                        sender.perform_command(f'{com}')
                    return com_on_click
                # 创建按钮
                create_tty_button = ActionForm.Button(text="§l§5创建领地",icon="textures/ui/color_plus",on_click=create_tty())
                rename_tty_button = ActionForm.Button(text="§l§5重命名领地",icon="textures/ui/book_edit_default",on_click=rename_tty())
                tp_tty_button = ActionForm.Button(text="§l§5传送自己及已加入的领地",icon="textures/ui/csb_purchase_warning",on_click=tp_tty())
                tp_all_tty_button = ActionForm.Button(text="§l§5传送全部领地",icon="textures/ui/default_world",on_click=tp_all_tty())
                man_tty_button = ActionForm.Button(text="§l§5管理领地",icon="textures/ui/icon_setting",on_click=man_tty())
                help_tty_button = ActionForm.Button(text="§l§5查看领地帮助",icon="textures/ui/Feedback",on_click=run_command(com="territory help"))
                list_tty_button = ActionForm.Button(text="§l§5列出自己的全部领地",icon="textures/ui/infobulb",on_click=run_command(com="tty list"))
                # 发送菜单
                form = ActionForm(
                    title=f"{ColorFormat.DARK_PURPLE}§lTerritory领地菜单",
                    buttons=[create_tty_button,rename_tty_button,tp_tty_button,tp_all_tty_button,man_tty_button,list_tty_button,help_tty_button]
                )
                sender.send_form(form)
                        
                    
        elif command.name == "reloadtty":
            # 更新全局领地信息
            all_tty = self.read_all_territories()
            self.read_config()
            if isinstance(sender,Player):
                sender.send_message(f"{ColorFormat.YELLOW}已重载领地数据")
            else:
                self.server.logger.info(f"{ColorFormat.YELLOW}已重载领地数据")
                
        elif command.name == "optty":
            if len(args) == 2:
                # 删除领地
                if args[0] == "del":
                    tty_name = args[1]
                    # 获取领地信息
                    tty_info = self.read_territory_by_name(tty_name)
                    # 检查领地是否存在
                    if tty_info == None:
                        sender.send_error_message("该领地不存在")
                        return
                    self.del_player_tty(tty_name)
                    if isinstance(sender,Player):
                        sender.send_message(f"{ColorFormat.YELLOW}已成功删除领地")
                    else:
                        self.logger.info(f"{ColorFormat.YELLOW}已成功删除领地")
                # 删除个人全部领地
                elif args[0] == "delall":
                    player_name = args[1]
                    # 获取领地信息
                    tty_info = self.list_player_tty(player_name)
                    # 检查领地是否存在
                    if tty_info == None:
                        if isinstance(sender,Player):
                            sender.send_error_message("该玩家无领地")
                        else:
                            self.logger.error("该玩家无领地")
                        return
                    del_all_tty_name = ""
                    for tty in tty_info:
                        tty_name = tty['name']
                        self.del_player_tty(tty_name)
                        del_all_tty_name += tty_name + ","
                    if isinstance(sender,Player):
                        sender.send_message(f"{ColorFormat.YELLOW}已成功删除玩家 {player_name} 的全部领地{del_all_tty_name}")
                    else:
                        self.logger.info(f"{ColorFormat.YELLOW}已成功删除玩家 {player_name} 的全部领地{del_all_tty_name}")
                        
        return True
    
    # 防方块破坏
    @event_handler
    def break_event(self,event:BlockBreakEvent):
        player_name = event.player.name
        block_dim = event.block.location.dimension.name
        block_pos = (int(event.block.location.x),int(event.block.location.y),int(event.block.location.z))
        in_tty_list = self.list_in_tty(block_pos,block_dim)
        # 不在任何领地上
        if in_tty_list == []:
            return
        # 领地上
        if len(in_tty_list) == 1:
            if in_tty_list[0][1] == 0:
                if player_name not in in_tty_list[0][5]:
                    event.is_cancelled = True
                    self.server.get_player(player_name).send_message(f"你不能破坏{in_tty_list[0][6]}的领地 {in_tty_list[0][0]} 上的方块")
                return
            else:
                return
                    
    # 防交互
    @event_handler
    def jiaohu_event(self,event:PlayerInteractEvent):
        player_name = event.player.name
        block_dim = event.block.location.dimension.name
        block_pos = (int(event.block.location.x),int(event.block.location.y),int(event.block.location.z))
        in_tty_list = self.list_in_tty(block_pos,block_dim)
        # 不在任何领地上
        if in_tty_list == []:
            return
        # 领地上
        if len(in_tty_list) == 1:
            if in_tty_list[0][1] == 0:
                if player_name not in in_tty_list[0][5]:
                    event.is_cancelled = True
                    self.server.get_player(player_name).send_message(f"你不能在{in_tty_list[0][6]}的领地 {in_tty_list[0][0]} 上交互")
                return
            else:
                return
                    
    # 防方块放置
    @event_handler
    def build_event(self,event:BlockPlaceEvent):
        player_name = event.player.name
        block_dim = event.block.location.dimension.name
        block_pos = (int(event.block.location.x),int(event.block.location.y),int(event.block.location.z))
        in_tty_list = self.list_in_tty(block_pos,block_dim)
        # 不在任何领地上
        if in_tty_list == []:
            return
        # 领地上
        if len(in_tty_list) == 1:
            if in_tty_list[0][3] == 0:
                if player_name not in in_tty_list[0][5]:
                    event.is_cancelled = True
                    self.server.get_player(player_name).send_message(f"你不能在{in_tty_list[0][6]}的领地 {in_tty_list[0][0]} 上放置方块")
                return
            else:
                return
    
    # 防生物爆炸
    @event_handler
    def bomb_event(self,event:ActorExplodeEvent):
        actor_dim = event.actor.location.dimension.name
        actor_pos = (int(event.actor.location.x),int(event.actor.location.y),int(event.actor.location.z))
        in_tty_list = self.list_in_tty(actor_pos,actor_dim)
        # 不在任何领地上
        if in_tty_list == []:
            return
        # 领地上
        if len(in_tty_list) == 1:
            if in_tty_list[0][4] == 0:
                event.is_cancelled = True
                return
            else:
                return
                    
    # 防玩家与生物交互
    @event_handler
    def player_jiaohu_shengwu_event(self,event:PlayerInteractActorEvent):
        player_name = event.player.name
        actor_dim = event.actor.location.dimension.name
        actor_pos = (int(event.actor.location.x),int(event.actor.location.y),int(event.actor.location.z))
        in_tty_list = self.list_in_tty(actor_pos,actor_dim)
        # 不在任何领地上
        if in_tty_list == []:
            return
        # 领地上
        if len(in_tty_list) == 1:
            if in_tty_list[0][1] == 0:
                if player_name not in in_tty_list[0][5]:
                    event.is_cancelled = True
                    self.server.get_player(player_name).send_message(f"你不能在{in_tty_list[0][6]}的领地 {in_tty_list[0][0]} 上交互")
                return
            else:
                return