from endstone.command import Command, CommandSender
from endstone.plugin import Plugin
from endstone import ColorFormat,Player
import os
import json
import sqlite3
from contextlib import contextmanager
from endstone.form import ModalForm,Dropdown,Label,ActionForm,TextInput,Slider,MessageForm,Toggle
import hashlib

# 初始化
tty_data = "plugins/territory"
db_file = "plugins/territory/territory_data.db"
config_file = "plugins/territory/config.json"
lang_file = "plugins/territory/lang.json"
lang = {}
lang_status = False

all_tty = []
database_hash = None

def read_lang() -> bool:
    global lang
    if not os.path.exists(lang_file):
        return False
    else:
        with open(lang_file, 'r', encoding='utf-8') as f:
            lang = json.load(f)
            return True

read_lang()
class Territory_gui(Plugin):
    api_version = "0.7"

    # 连接数据库函数
    def connect(self):
        """建立到SQLite数据库的连接。"""
        try:
            self.conn = sqlite3.connect(db_file,check_same_thread=False)
            self.logger.info(self.getLocal('成功连接到数据库'))
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

    def close(self):
        """关闭到SQLite数据库的连接。"""
        if self.conn:
            try:
                self.conn.close()
                self.logger.info('Closing database successfully')
            except sqlite3.Error as e:
                self.logger.error(f"Database close error: {e}")
            finally:
                self.conn = None

    def calculate_file_hash(self,file_path, hash_algorithm='md5'):
        """计算给定文件的哈希值"""
        hash_obj = hashlib.new(hash_algorithm)
        with open(file_path, 'rb') as f:
            while chunk := f.read(8192):
                hash_obj.update(chunk)
        return hash_obj.hexdigest()

    def check_and_update_hash(self):
        global database_hash
        """
        校验数据库哈希函数
        """
        
        # 计算文件的哈希值
        current_file_hash = self.calculate_file_hash(db_file)
        
        if database_hash is None:
            # 如果全局变量为空，则将当前文件哈希赋值给全局变量，并返回False
            database_hash = current_file_hash
            return False
        else:
            if database_hash != current_file_hash:
                # 若文件哈希与全局变量不同，更新全局变量并返回False
                database_hash = current_file_hash
                return False
            else:
                # 文件哈希相同，返回True
                return True

    #多语言

    def getLocal(self,string:str) -> str:
        """
        用于获取语言文件中翻译的函数
        """
        try:
            if lang_status == True:
                return lang[string]
            else:
                return string
        except:
            return string

    # 读取全部领地
    def read_all_territories(self):
        """
        读取所有领地的信息。
        
        返回:
            list of dict: 包含所有领地信息的列表，每个元素是一个包含领地信息的字典。
        """
        with self.cursor() as cur:
            cur.execute('''
            SELECT name, pos1_x, pos1_y, pos1_z, pos2_x, pos2_y, pos2_z, tppos_x, tppos_y, tppos_z, owner, manager, member, if_jiaohu, if_break, if_tp, if_build, if_bomb, if_damage, dim ,father_tty 
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
                'if_damage': bool(row[18]),
                'dim': row[19],
                'father_tty': row[20]
            } for row in rows]

    # 列出玩家领地函数
    def list_player_tty(self,player_name):
        """
        列出玩家的所有领地
        
        player_name: 玩家名
        """
        global all_tty
        if not self.check_and_update_hash():
            all_tty = self.read_all_territories()
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
    
    # 列出所有玩家权限为成员及以上的领地的函数
    def list_member_tty(self,player_name):
        """
        用于列出所有玩家权限为成员及以上的领地的函数
        
        player_name: 玩家名
        """
        global all_tty
        if not self.check_and_update_hash():
            all_tty = self.read_all_territories()
        member_tty_list = []
        for row in all_tty:
            if player_name == row['owner'] or player_name in row['manager'].split(',') or player_name in row['member'].split(','):
                member_tty_list.append(row)
            
        if member_tty_list == []:
            return None
        else:
            return member_tty_list
        
    # 列出所有玩家为领地的主人、管理员的领地的函数
    def list_tty_op(self,player_name):
        """
        用于列出所有玩家为领地的主人、管理员的领地的函数
        
        player_name: 玩家名
        """
        global all_tty
        if not self.check_and_update_hash():
            all_tty = self.read_all_territories()
        op_tty_list = []
        for row in all_tty:
            if player_name == row['owner'] or player_name in row['manager'].split(','):
                op_tty_list.append(row)
            
        if op_tty_list == []:
            return None
        else:
            return op_tty_list
        
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
    
    def on_load(self) -> None:
        self.logger.info("on_load is called!")
        global lang_status
        lang_status = read_lang()

    def on_enable(self) -> None:
        global all_tty
        self.logger.info("on_enable is called!")
        try:
            tty_version = self.server.plugin_manager.get_plugin("territory")._get_description().version
            tty_gui_version = self.server.plugin_manager.get_plugin("territory_gui")._get_description().version
        except:
            self.server.logger.error(self.getLocal('未检测到Territory插件本体,插件拒绝加载'))
            self.server.plugin_manager.disable_plugin(self)
            return
        self.server.logger.info(f"{self.getLocal('Territory插件版本')}{tty_version},{self.getLocal('图形菜单版本')}{tty_gui_version}")
        self.connect()  # 确保在加载时连接数据库
        all_tty = self.read_all_territories()

    def on_disable(self) -> None:
        self.logger.info("on_disable is called!")
        try:
            self.close()
        except: 
            pass

    try:
        command_descriping = lang["领地菜单命令"]
    except:
        command_descriping = "领地菜单命令|Territory menu command"

    commands = {
        "ttygui": {
            "description": command_descriping,
            "usages": [
                "/ttygui"
                ],
            "permissions": ["ttygui.command.member"],
        }
    }
    permissions = {
        "ttygui.command.member": {
            "description": "1",
            "default": True, 
        }
    }
    
    commands = {
        "ttygui": {
            "description": command_descriping,
            "usages": [
                "/ttygui"
                ],
            "permissions": ["ttygui.command.member"],
        }
    }
    permissions = {
        "ttygui.command.member": {
            "description": "1",
            "default": True, 
        }
    }
    
    def on_command(self, sender: CommandSender, command: Command, args: list[str]) -> bool:
        global all_tty
        if command.name == "ttygui":
            if not isinstance(sender,Player):
                self.server.logger.error(self.getLocal('无法在控制台使用此命令'))
                return
            if len(args) == 0:
                # 图形界面
                
                # 创建领地功能
                def create_tty():
                    def run_create_tty(sender,json_str:str):
                        try:
                            fst_pos1 = json.loads(json_str)[0]
                            fst_pos2 = json.loads(json_str)[1]
                            pos1 = tuple(int(float(x)) for x in fst_pos1.split())
                            pos2 = tuple(int(float(x)) for x in fst_pos2.split())
                            sender.perform_command(f"tty add {pos1[0]} {pos1[1]} {pos1[2]} {pos2[0]} {pos2[1]} {pos2[2]}")
                        except:
                            sender.send_error_message(self.getLocal('错误的坐标!'))
                            return

                    def on_click(sender):
                        create_tty_form = ModalForm(
                            title=f"{ColorFormat.DARK_PURPLE}{self.getLocal('§l新建领地')}",
                            controls=[
                                TextInput(label=self.getLocal('§l输入领地边角坐标1 格式示例: 114 5 14')),
                                TextInput(label=self.getLocal('§l输入领地边角坐标2 格式示例: 191 98 10')),
                                Label(text=self.getLocal('三维领地为立方体,需要领地的两个对角坐标,并注意高度需要覆盖领地'))
                            ],
                            on_submit=run_create_tty,
                            on_close=run_command(com="ttygui")
                        )
                        sender.send_form(create_tty_form)
                    return on_click
                
                # 创建子领地功能
                def create_sub_tty():
                    def run_create_sub_tty(sender,json_str:str):
                        try:
                            fst_pos1 = json.loads(json_str)[0]
                            fst_pos2 = json.loads(json_str)[1]
                            pos1 = tuple(int(float(x)) for x in fst_pos1.split())
                            pos2 = tuple(int(float(x)) for x in fst_pos2.split())
                            sender.perform_command(f"tty add_sub {pos1[0]} {pos1[1]} {pos1[2]} {pos2[0]} {pos2[1]} {pos2[2]}")
                        except:
                            sender.send_error_message(self.getLocal('错误的坐标!'))
                            return

                    def on_click(sender):
                        create_sub_tty_form = ModalForm(
                            title=f"{ColorFormat.DARK_PURPLE}{self.getLocal('§l新建子领地')}",
                            controls=[
                                TextInput(label=self.getLocal('§l输入子领地边角坐标1 格式示例: 114 5 14')),
                                TextInput(label=self.getLocal('§l输入子领地边角坐标2 格式示例: 191 98 10')),
                                Label(text=self.getLocal('子领地需要在父领地之内创建,不能超出父领地,只有父领地的所有者和管理员有权限创建'))
                            ],
                            on_submit=run_create_sub_tty,
                            on_close=run_command(com="ttygui")
                        )
                        sender.send_form(create_sub_tty_form)
                    return on_click
                
                # 重命名领地功能
                def rename_tty():
                    tty_list = self.list_player_tty(sender.name)
                    
                    def run_rename_tty(sender,json_str:str):
                        try:
                            index = int(json.loads(json_str)[0])
                            newname = json.loads(json_str)[1]
                            oldname = tty_list[index]['name']
                            sender.perform_command(f'tty rename "{oldname}" "{newname}"')
                        except:
                            sender.send_error_message(self.getLocal('未知的错误'))
                    def on_click(sender):
                        #  按钮按下时及时检测领地存在
                        if tty_list == None:
                            sender.send_error_message(self.getLocal('未查找到领地'))
                            return
                        option = []
                        for idx, tty in enumerate(tty_list, start=1):
                            ttyname = tty['name']
                            option.append(ttyname)
                        rename_tty_form = ModalForm(
                            title=f"{ColorFormat.DARK_PURPLE}{self.getLocal('§l重命名领地')}",
                            controls=[
                                Dropdown(label=self.getLocal('§l选择要重命名的领地'),options=option),
                                TextInput(label=self.getLocal('§l新名字'))
                            ],
                            on_submit=run_rename_tty,
                            on_close=run_command(com="ttygui")
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
                            sender.perform_command(f'tty tp "{ttyname}"')
                        except:
                            sender.send_error_message(self.getLocal('未知的错误'))
                    
                    def on_click(sender):
                        #  按钮按下时及时检测领地存在
                        if tty_list == None:
                            sender.send_error_message(self.getLocal('未查找到领地'))
                            return
                        option = []
                        for tty in tty_list:
                            ttyname = tty['name']
                            option.append(ttyname)
                        tp_tty_form = ModalForm(
                            title=self.getLocal('传送领地'),
                            controls=[
                                Dropdown(label=self.getLocal('§l选择你要传送的领地'),options=option)
                            ],
                            on_submit=run_tp_tty,
                            on_close=run_command(com="ttygui")
                        )
                        sender.send_form(tp_tty_form)
                    return on_click
                
                # 传送全部领地功能
                def tp_all_tty():
                    def run_tp_all_tty(sender,json_str:str):
                        try:
                            ttyname = json.loads(json_str)[0]
                            sender.perform_command(f'tty tp "{ttyname}"')
                        except:
                            sender.send_error_message(self.getLocal('未知的错误'))
                    
                    def on_click(sender):
                        tp_all_tty_form = ModalForm(
                            title=self.getLocal('传送领地'),
                            controls=[
                                TextInput(label=self.getLocal('§l输入你要传送的领地')),
                            ],
                            on_submit=run_tp_all_tty,
                            on_close=run_command(com="ttygui")
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
                                if_damage_status = tty_list[index]['if_damage']
                                
                                # 执行领地权限更改
                                def run_mtsm(sender,json_str:str):
                                    permis = json.loads(json_str)
                                    if not if_jiaohu_status == permis[0] or not if_break_status == permis[1] or not if_tp_status == permis[2] or not if_build_status == permis[3] or not if_bomb_status == permis[4] or not if_damage_status == permis[5]:
                                        if not if_jiaohu_status == permis[0]:
                                            sender.perform_command(f'tty set if_jiaohu {str(permis[0]).lower()} "{ttyname}"')
                                            
                                        if not if_break_status == permis[1]:
                                            sender.perform_command(f'tty set if_break {str(permis[1]).lower()} "{ttyname}"')
                                            
                                        if not if_tp_status == permis[2]:
                                            sender.perform_command(f'tty set if_tp {str(permis[2]).lower()} "{ttyname}"')
                                            
                                        if not if_build_status == permis[3]:
                                            sender.perform_command(f'tty set if_build {str(permis[3]).lower()} "{ttyname}"')
                                            
                                        if not if_bomb_status == permis[4]:
                                            sender.perform_command(f'tty set if_bomb {str(permis[4]).lower()} "{ttyname}"')
                                        
                                        if not if_damage_status == permis[5]:
                                            sender.perform_command(f'tty set if_damage {str(permis[5]).lower()} "{ttyname}"')
                                    else:
                                        sender.send_error_message(self.getLocal('你未更改领地权限,领地权限不会变化'))
                                
                                def mtsm_on_click(sender):
                                    mtsm_form = ModalForm(
                                        title=f"{self.getLocal('§l管理领地')} {ttyname} {self.getLocal('的权限')}",
                                        controls=[
                                            Toggle(label=self.getLocal('§l是否允许外人领地内交互'),default_value=if_jiaohu_status),
                                            Toggle(label=self.getLocal('§l是否允许外人领地内破坏'),default_value=if_break_status),
                                            Toggle(label=self.getLocal('§l是否允许外人传送至领地'),default_value=if_tp_status),
                                            Toggle(label=self.getLocal('§l是否允许外人领地内放置'),default_value=if_build_status),
                                            Toggle(label=self.getLocal('§l是否允许领地内实体爆炸'),default_value=if_bomb_status),
                                            Toggle(label=self.getLocal('§l是否允许外人对实体攻击'),default_value=if_damage_status),
                                        ],
                                        on_submit=run_mtsm,
                                        on_close=man_tty_main_menu
                                    )
                                    sender.send_form(mtsm_form)
                                mtsm_on_click(sender)
                            
                            # 权限主菜单
                            def on_click(sender):
                                if tty_list == None:
                                    sender.send_error_message(self.getLocal('未查找到领地'))
                                    return
                                options = []
                                for tty in tty_list:
                                    ttyname = tty['name']
                                    options.append(ttyname)
                                man_main_form = ModalForm(
                                    title=self.getLocal('§l领地权限管理界面'),
                                    controls=[
                                        Dropdown(label=self.getLocal('§l选择要管理的领地'),options=options),
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
                                    sender.perform_command(f'tty member remove "{del_member_name}" "{ttyname}"')

                                
                                def del_member_tsm_on_click(sender):
                                    if tty_member_list == ['']:
                                        sender.send_error_message(f"{self.getLocal('在领地')} {ttyname} {self.getLocal('中没有任何成员')}")
                                        return
                                    del_member_tsm_form = ModalForm(
                                        title=f"{self.getLocal('§l管理领地')} {ttyname} {self.getLocal('的成员')}",
                                        controls=[
                                            Dropdown(label=self.getLocal('选择要删除的领地成员'),options=tty_member_list)
                                        ],
                                        on_submit=run_del_member_tsm,
                                        on_close=man_tty_main_menu
                                    )
                                    sender.send_form(del_member_tsm_form)
                                del_member_tsm_on_click(sender)
                            
                            # 删除领地成员主菜单
                            def on_click(sender):
                                if tty_list == None:
                                    sender.send_error_message(self.getLocal('未查找到领地'))
                                    return
                                options = []
                                for tty in tty_list:
                                    ttyname = tty['name']
                                    options.append(ttyname)
                                man_main_form = ModalForm(
                                    title=self.getLocal('§l领地成员删除界面'),
                                    controls=[
                                        Dropdown(label=self.getLocal('§l选择要删除成员的领地'),options=options),
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
                                        sender.perform_command(f'tty member add "{add_member_name}" "{ttyname}"')
                                    # 离线玩家不为空时采用离线玩家
                                    else:
                                        sender.perform_command(f'tty member add "{leave_player}" "{ttyname}"')

                                
                                def add_member_tsm_on_click(sender):
                                    add_member_tsm_form = ModalForm(
                                        title=f"{self.getLocal('§l添加成员到领地')} {ttyname} {self.getLocal('中')}",
                                        controls=[
                                            Dropdown(label=self.getLocal('§l选择要添加的在线玩家'),options=online_player_list),
                                            TextInput(label=self.getLocal('§l添加不在线的玩家'),placeholder=self.getLocal('只要这里写了一个字都会以此为输入值'))
                                        ],
                                        on_submit=run_add_member_tsm,
                                        on_close=man_tty_main_menu
                                    )
                                    sender.send_form(add_member_tsm_form)
                                add_member_tsm_on_click(sender)
                            
                            # 添加领地成员主菜单
                            def on_click(sender):
                                if tty_list == None:
                                    sender.send_error_message(self.getLocal('未查找到领地'))
                                    return
                                options = []
                                for tty in tty_list:
                                    ttyname = tty['name']
                                    options.append(ttyname)
                                man_main_form = ModalForm(
                                    title=self.getLocal('§l领地成员添加界面'),
                                    controls=[
                                        Dropdown(label=self.getLocal('§l选择要添加成员的领地'),options=options),
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
                                    sender.perform_command(f'tty manager remove "{del_manager_name}" "{ttyname}"')

                                
                                def del_manager_tsm_on_click(sender):
                                    if tty_manager_list == ['']:
                                        sender.send_error_message(f"{self.getLocal('在领地')} {ttyname} {self.getLocal('中没有任何领地管理员')}")
                                        return
                                    del_manager_tsm_form = ModalForm(
                                        title=f"{self.getLocal('§l管理领地')} {ttyname} {self.getLocal('的领地管理员')}",
                                        controls=[
                                            Dropdown(label=self.getLocal('选择要删除的领地管理员'),options=tty_manager_list)
                                        ],
                                        on_submit=run_del_manager_tsm,
                                        on_close=man_tty_main_menu
                                    )
                                    sender.send_form(del_manager_tsm_form)
                                del_manager_tsm_on_click(sender)
                            
                            # 删除领地管理员主菜单
                            def on_click(sender):
                                if tty_list == None:
                                    sender.send_error_message(self.getLocal('未查找到领地'))
                                    return
                                options = []
                                for tty in tty_list:
                                    ttyname = tty['name']
                                    options.append(ttyname)
                                man_main_form = ModalForm(
                                    title=self.getLocal('§l领地管理员删除界面'),
                                    controls=[
                                        Dropdown(label=self.getLocal('§l选择要删除领地管理员的领地'),options=options),
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
                                    #print(input_json)
                                    index = int(input_json[0])
                                    add_manager_name = online_player_list[index]
                                    leave_player = input_json[1]
                                    # 离线玩家为空时采用在线玩家
                                    if leave_player == "":
                                        sender.perform_command(f'tty manager add "{add_manager_name}" "{ttyname}"')
                                    # 离线玩家不为空时采用离线玩家
                                    else:
                                        sender.perform_command(f'tty manager add "{leave_player}" "{ttyname}"')

                                
                                def add_manager_tsm_on_click(sender):
                                    add_manager_tsm_form = ModalForm(
                                        title=f"{self.getLocal('§l添加领地管理员到领地')} {ttyname} {self.getLocal('中')}",
                                        controls=[
                                            Label(self.getLocal('§l领地管理员有领地权限设置、成员管理、领地传送点设置、创建子领地的权限,请把握好人选')),
                                            Dropdown(label=self.getLocal('§l选择要添加的在线玩家'),options=online_player_list),
                                            TextInput(label=self.getLocal('§l添加不在线的玩家'),placeholder=self.getLocal('只要这里写了一个字都会以此为输入值'))
                                        ],
                                        on_submit=run_add_manager_tsm,
                                        on_close=man_tty_main_menu
                                    )
                                    sender.send_form(add_manager_tsm_form)
                                add_manager_tsm_on_click(sender)
                            
                            # 添加领地管理员主菜单
                            def on_click(sender):
                                if tty_list == None:
                                    sender.send_error_message(self.getLocal('未查找到领地'))
                                    return
                                options = []
                                for tty in tty_list:
                                    ttyname = tty['name']
                                    options.append(ttyname)
                                man_main_form = ModalForm(
                                    title=self.getLocal('§l领地管理员添加界面'),
                                    controls=[
                                        Dropdown(label=self.getLocal('§l选择要添加领地管理员的领地'),options=options),
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
                                    sender.perform_command(f'tty settp {tppos} "{ttyname}"')
                                except:
                                    sender.send_error_message(self.getLocal('未知的错误'))
                            def on_click(sender):
                                if tty_list == None:
                                    sender.send_error_message(self.getLocal('未查找到领地'))
                                    return
                                option = []
                                for idx, tty in enumerate(tty_list, start=1):
                                    ttyname = tty['name']
                                    option.append(ttyname)
                                set_tp_tty_form = ModalForm(
                                    title=f"{ColorFormat.DARK_PURPLE}{self.getLocal('§l设置领地传送点')}",
                                    controls=[
                                        Dropdown(label=self.getLocal('§l选择要设置传送点的领地'),options=option),
                                        TextInput(label=self.getLocal('§l坐标'))
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
                                    sender.perform_command(f'tty transfer {ttyname} "{new_owner_name}"')
                                except:
                                    sender.send_error_message(self.getLocal('未知的错误'))
                            def on_click(sender):
                                if tty_list == None:
                                    sender.send_error_message(self.getLocal('未查找到领地'))
                                    return
                                option = []
                                for idx, tty in enumerate(tty_list, start=1):
                                    ttyname = tty['name']
                                    option.append(ttyname)
                                transfer_tty_form = ModalForm(
                                    title=f"{ColorFormat.DARK_PURPLE}{self.getLocal('§l转让领地')}",
                                    controls=[
                                        Dropdown(label=self.getLocal('§l选择要转让的领地'),options=option),
                                        TextInput(label=self.getLocal('§l接收领地的玩家名'))
                                    ],
                                    on_submit=run_trunsfer_tty,
                                    on_close=man_tty_main_menu
                                )
                                sender.send_form(transfer_tty_form)
                            return on_click
                        
                        # 领地删除菜单
                        
                        def del_tty():
                            tty_list = self.list_player_tty(sender.name)
                            
                            def run_del_tty(sender,json_str:str):
                                try:
                                    index = int(json.loads(json_str)[0])
                                    ttyname = tty_list[index]['name']
                                    sender.perform_command(f'tty del "{ttyname}"')
                                except:
                                    sender.send_error_message(self.getLocal('未知的错误'))
                            def on_click(sender):
                                if tty_list == None:
                                    sender.send_error_message(self.getLocal('未查找到领地'))
                                    return
                                option = []
                                for idx, tty in enumerate(tty_list, start=1):
                                    ttyname = tty['name']
                                    option.append(ttyname)
                                del_tty_form = ModalForm(
                                    title=f"{ColorFormat.DARK_PURPLE}{self.getLocal('§l删除领地')}",
                                    controls=[
                                        Dropdown(label=self.getLocal('§l选择要删除的领地'),options=option),
                                        Label(text=self.getLocal('§l§4删除领地不可恢复!'))
                                    ],
                                    on_submit=run_del_tty,
                                    on_close=man_tty_main_menu
                                )
                                sender.send_form(del_tty_form)
                            return on_click
                        
                        set_permis_button = ActionForm.Button(text=self.getLocal('§l§1管理自己管理的领地权限'),icon="textures/ui/accessibility_glyph_color",on_click=set_permis())
                        del_member_button = ActionForm.Button(text=self.getLocal('§l§1删除自己管理的领地成员'),icon="textures/ui/permissions_member_star",on_click=del_tty_member())
                        add_member_button = ActionForm.Button(text=self.getLocal('§l§1添加自己管理的领地成员'),icon="textures/ui/permissions_member_star_hover",on_click=add_tty_member())
                        
                        del_manager_button = ActionForm.Button(text=self.getLocal('§l§1删除自己领地的领地管理员'),icon="textures/ui/permissions_op_crown",on_click=del_tty_manager())
                        add_manager_button = ActionForm.Button(text=self.getLocal('§l§1添加自己领地的领地管理员'),icon="textures/ui/permissions_op_crown_hover",on_click=add_tty_manager())
                        
                        set_tp_button = ActionForm.Button(text=self.getLocal('§l§1设置自己管理的领地的传送点'),icon="textures/ui/csb_purchase_warning",on_click=set_tp_tty())

                        transfer_button = ActionForm.Button(text=self.getLocal('§l§1将自己的领地转让给其他玩家'),icon="textures/ui/trade_icon",on_click=transfer_tty())

                        del_button = ActionForm.Button(text=self.getLocal('§l§4删除自己的领地'),icon="textures/ui/book_trash_default",on_click=del_tty())
                        
                        main_menu = ActionForm(
                            title=f"{ColorFormat.DARK_PURPLE}{self.getLocal('§l领地管理界面')}",
                            buttons=[set_permis_button,set_tp_button,add_member_button,del_member_button,add_manager_button,del_manager_button,transfer_button,del_button],
                            on_close=run_command(com="ttygui")
                        )
                        sender.send_form(main_menu)
                    return man_tty_main_menu
                      
                def run_command(com):
                    def com_on_click(sender):
                        sender.perform_command(f'{com}')
                    return com_on_click
                def run_list():
                    def run_list_click(sender):
                        tty_list = self.list_player_tty(sender.name)
                        if tty_list == None:
                            sender.send_error_message(self.getLocal('你没有领地'))
                            return
                        
                        output_item = ""
                        for idx, tty in enumerate(tty_list, start=1):
                            # 简化输出，只展示关键信息
                            message = ColorFormat.YELLOW+str(idx)+self.getLocal('. 领地名称: ')+str(tty['name'])+self.getLocal('\n位置范围: ') + "\n" + str(tty['pos1'])+self.getLocal(' 到 ') + str(tty['pos2']) + self.getLocal('\n传送点位置: ') + "\n" + str(tty['tppos'])+self.getLocal('\n维度: ') + str(tty['dim']) + self.getLocal('\n是否允许玩家交互: ') + str(tty['if_jiaohu'])+self.getLocal('\n是否允许玩家破坏: ')+str(tty['if_break'])+self.getLocal('\n是否允许外人传送: ')+str(tty['if_tp'])+self.getLocal('\n是否允许放置方块: ')+str(tty['if_build'])+self.getLocal('\n是否允许实体爆炸: ')+str(tty['if_bomb'])+self.getLocal('\n是否允许实体伤害: ')+str(tty['if_damage'])+self.getLocal('\n领地管理员: ')+tty['manager']+self.getLocal('\n领地成员: ')+tty['member']+"\n"
                            output_item += message + "-" * 20 + "\n"
                        list_form = ActionForm(
                            title=self.getLocal('§l领地列表'),
                            content=f"{output_item}"
                        )
                        sender.send_form(list_form)
                    return run_list_click
                # 创建按钮
                create_tty_button = ActionForm.Button(text=self.getLocal('§l§5创建领地'),icon="textures/ui/color_plus",on_click=create_tty())
                create_sub_tty_button = ActionForm.Button(text=self.getLocal('§l§5创建子领地'),icon="textures/ui/copy",on_click=create_sub_tty())
                rename_tty_button = ActionForm.Button(text=self.getLocal('§l§5重命名领地'),icon="textures/ui/book_edit_default",on_click=rename_tty())
                tp_tty_button = ActionForm.Button(text=self.getLocal('§l§5传送自己及已加入的领地'),icon="textures/ui/csb_purchase_warning",on_click=tp_tty())
                tp_all_tty_button = ActionForm.Button(text=self.getLocal('§l§5传送全部领地'),icon="textures/ui/default_world",on_click=tp_all_tty())
                man_tty_button = ActionForm.Button(text=self.getLocal('§l§5管理领地'),icon="textures/ui/icon_setting",on_click=man_tty())
                help_tty_button = ActionForm.Button(text=self.getLocal('§l§5查看领地帮助'),icon="textures/ui/Feedback",on_click=run_command(com="tty help"))
                list_tty_button = ActionForm.Button(text=self.getLocal('§l§5列出自己的全部领地'),icon="textures/ui/infobulb",on_click=run_list())
                # 发送菜单
                form = ActionForm(
                    title=f"{ColorFormat.DARK_PURPLE}{self.getLocal('§lTerritory领地菜单')}",
                    buttons=[create_tty_button,create_sub_tty_button,rename_tty_button,tp_tty_button,tp_all_tty_button,man_tty_button,list_tty_button,help_tty_button]
                )
                sender.send_form(form)
                
        return True