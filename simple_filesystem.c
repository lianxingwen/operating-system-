#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 简化的文件系统模拟器 - 纯内存版本

#define MAX_FILES 100
#define MAX_DIRS 50
#define MAX_FILENAME 32
#define MAX_CONTENT 1024
#define MAX_USERS 10

// 用户结构
struct User {
    int uid;
    char password[16];
    int logged_in;
};

// 文件结构
struct File {
    char name[MAX_FILENAME];
    char content[MAX_CONTENT];
    int size;
    int owner_uid;
    int is_directory;
    int parent_dir;
};

// 全局变量
struct User users[MAX_USERS];
struct File files[MAX_FILES];
int current_user = -1;
int current_dir = 0;
int file_count = 1; // 从1开始，0是根目录

// 初始化系统
void init_system() {
    // 初始化用户
    users[0] = (struct User){2116, "dddd", 0};
    users[1] = (struct User){2117, "bbbb", 0};
    users[2] = (struct User){2118, "abcd", 0};
    users[3] = (struct User){2119, "cccc", 0};
    users[4] = (struct User){2220, "eeee", 0};
    
    // 初始化根目录
    strcpy(files[0].name, "/");
    files[0].is_directory = 1;
    files[0].parent_dir = -1;
    files[0].owner_uid = 0;
    files[0].size = 0;
    
    printf("文件系统初始化完成\n");
}

// 用户登录
int login(int uid, char* password) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].uid == uid && strcmp(users[i].password, password) == 0) {
            users[i].logged_in = 1;
            current_user = i;
            return 1;
        }
    }
    return 0;
}

// 用户登出
void logout() {
    if (current_user >= 0) {
        users[current_user].logged_in = 0;
        current_user = -1;
    }
}

// 查找文件
int find_file(char* name, int dir_id) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, name) == 0 && files[i].parent_dir == dir_id && strlen(files[i].name) > 0) {
            return i;
        }
    }
    return -1;
}

// 创建文件
int create_file(char* name) {
    if (current_user < 0) {
        printf("请先登录\n");
        return -1;
    }
    
    if (find_file(name, current_dir) >= 0) {
        printf("文件已存在: %s\n", name);
        return -1;
    }
    
    if (file_count >= MAX_FILES) {
        printf("文件系统已满\n");
        return -1;
    }
    
    strcpy(files[file_count].name, name);
    files[file_count].is_directory = 0;
    files[file_count].parent_dir = current_dir;
    files[file_count].owner_uid = users[current_user].uid;
    files[file_count].size = 0;
    memset(files[file_count].content, 0, MAX_CONTENT);
    
    printf("文件创建成功: %s (ID: %d)\n", name, file_count);
    return file_count++;
}

// 创建目录
int create_dir(char* name) {
    if (current_user < 0) {
        printf("请先登录\n");
        return -1;
    }
    
    if (find_file(name, current_dir) >= 0) {
        printf("目录已存在: %s\n", name);
        return -1;
    }
    
    if (file_count >= MAX_FILES) {
        printf("文件系统已满\n");
        return -1;
    }
    
    strcpy(files[file_count].name, name);
    files[file_count].is_directory = 1;
    files[file_count].parent_dir = current_dir;
    files[file_count].owner_uid = users[current_user].uid;
    files[file_count].size = 0;
    
    printf("目录创建成功: %s\n", name);
    return file_count++;
}

// 写文件
int write_file(int file_id, char* content) {
    if (file_id < 0 || file_id >= file_count) {
        printf("无效的文件ID\n");
        return -1;
    }
    
    if (files[file_id].is_directory) {
        printf("不能写入目录\n");
        return -1;
    }
    
    int len = strlen(content);
    if (len >= MAX_CONTENT) {
        len = MAX_CONTENT - 1;
        printf("内容被截断到 %d 字符\n", len);
    }
    
    strncpy(files[file_id].content, content, len);
    files[file_id].content[len] = '\0';
    files[file_id].size = len;
    
    printf("写入 %d 字节到文件 %s\n", len, files[file_id].name);
    return len;
}

// 读文件
int read_file(int file_id) {
    if (file_id < 0 || file_id >= file_count) {
        printf("无效的文件ID\n");
        return -1;
    }
    
    if (files[file_id].is_directory) {
        printf("不能读取目录\n");
        return -1;
    }
    
    printf("文件内容 (%s, %d 字节):\n", files[file_id].name, files[file_id].size);
    printf("%s\n", files[file_id].content);
    return files[file_id].size;
}

// 列出目录内容
void list_dir() {
    printf("\n当前目录内容:\n");
    printf("类型\t文件名\t\t大小\t所有者\n");
    printf("----------------------------------------\n");
    
    for (int i = 0; i < file_count; i++) {
        if (files[i].parent_dir == current_dir && strlen(files[i].name) > 0) {
            printf("%s\t%-16s\t%d\t%d\n", 
                   files[i].is_directory ? "目录" : "文件",
                   files[i].name,
                   files[i].size,
                   files[i].owner_uid);
        }
    }
    printf("\n");
}

// 切换目录
int change_dir(char* name) {
    if (strcmp(name, "..") == 0) {
        if (current_dir > 0) {
            current_dir = files[current_dir].parent_dir;
            printf("切换到上级目录\n");
            return 0;
        } else {
            printf("已在根目录\n");
            return -1;
        }
    }
    
    int dir_id = find_file(name, current_dir);
    if (dir_id < 0) {
        printf("目录不存在: %s\n", name);
        return -1;
    }
    
    if (!files[dir_id].is_directory) {
        printf("%s 不是目录\n", name);
        return -1;
    }
    
    current_dir = dir_id;
    printf("切换到目录: %s\n", name);
    return 0;
}

// 删除文件
int delete_file(char* name) {
    if (current_user < 0) {
        printf("请先登录\n");
        return -1;
    }
    
    int file_id = find_file(name, current_dir);
    if (file_id < 0) {
        printf("文件不存在: %s\n", name);
        return -1;
    }
    
    // 简单删除：清空文件名
    strcpy(files[file_id].name, "");
    files[file_id].size = 0;
    
    printf("文件已删除: %s\n", name);
    return 0;
}

// 显示帮助
void show_help() {
    printf("\n可用命令:\n");
    printf("login <用户ID> <密码>  - 登录系统\n");
    printf("logout                 - 登出当前用户\n");
    printf("ls                     - 显示当前目录内容\n");
    printf("mkdir <目录名>         - 创建目录\n");
    printf("cd <目录名>            - 切换目录\n");
    printf("create <文件名>        - 创建文件（返回文件ID）\n");
    printf("write <文件ID|文件名> <内容> - 写入文件\n");
    printf("read <文件ID|文件名>   - 读取文件\n");
    printf("delete <文件名>        - 删除文件\n");
    printf("pwd                    - 显示当前路径\n");
    printf("help                   - 显示帮助信息\n");
    printf("exit                   - 退出系统\n\n");
}

// 显示当前路径
void show_pwd() {
    if (current_dir == 0) {
        printf("当前路径: /\n");
    } else {
        printf("当前路径: %s\n", files[current_dir].name);
    }
}

// 主函数
int main() {
    char command[256];
    char cmd[64], arg1[64], arg2[256];
    int args_count;
    
    printf("=== 简单文件系统模拟器 ===\n");
    printf("正在初始化...\n");
    
    init_system();
    
    printf("\n可用用户: 2116(密码:dddd), 2117(密码:bbbb), 2118(密码:abcd), 2119(密码:cccc), 2220(密码:eeee)\n");
    printf("输入 'help' 查看可用命令\n\n");
    
    // 主循环
    while (1) {
        if (current_user < 0) {
            printf("未登录> ");
        } else {
            printf("[用户%d]> ", users[current_user].uid);
        }
        fflush(stdout);
        
        if (fgets(command, sizeof(command), stdin) == NULL) {
            printf("\n检测到输入结束，正在退出...\n");
            break;
        }
        
        // 检查是否为空行
        if (strlen(command) <= 1) {
            continue;
        }
        
        // 移除换行符
        command[strcspn(command, "\n")] = 0;
        
        // 解析命令
        args_count = sscanf(command, "%s %s %[^\n]", cmd, arg1, arg2);
        
        if (strcmp(cmd, "help") == 0) {
            show_help();
        }
        else if (strcmp(cmd, "exit") == 0) {
            printf("再见！\n");
            break;
        }
        else if (strcmp(cmd, "login") == 0) {
            if (args_count >= 3) {
                int uid = atoi(arg1);
                // 只取密码的第一个单词
                char password[64];
                sscanf(arg2, "%s", password);
                if (login(uid, password)) {
                    printf("登录成功！欢迎用户 %d\n", uid);
                } else {
                    printf("登录失败！\n");
                }
            } else {
                printf("用法: login <用户ID> <密码>\n");
            }
        }
        else if (current_user < 0) {
            printf("请先登录！使用: login <用户ID> <密码>\n");
        }
        else if (strcmp(cmd, "logout") == 0) {
            logout();
            printf("已登出。\n");
        }
        else if (strcmp(cmd, "ls") == 0) {
            list_dir();
        }
        else if (strcmp(cmd, "pwd") == 0) {
            show_pwd();
        }
        else if (strcmp(cmd, "mkdir") == 0) {
            if (args_count >= 2) {
                create_dir(arg1);
            } else {
                printf("用法: mkdir <目录名>\n");
            }
        }
        else if (strcmp(cmd, "cd") == 0) {
            if (args_count >= 2) {
                change_dir(arg1);
            } else {
                printf("用法: cd <目录名>\n");
            }
        }
        else if (strcmp(cmd, "create") == 0) {
            if (args_count >= 2) {
                create_file(arg1);
            } else {
                printf("用法: create <文件名>\n");
            }
        }
        else if (strcmp(cmd, "write") == 0) {
            if (args_count >= 3) {
                int file_id = atoi(arg1);
                // 如果第一个参数不是数字，尝试按文件名查找
                if (file_id == 0 && strcmp(arg1, "0") != 0) {
                    file_id = find_file(arg1, current_dir);
                    if (file_id < 0) {
                        printf("文件不存在: %s\n", arg1);
                        continue;
                    }
                }
                write_file(file_id, arg2);
            } else {
                printf("用法: write <文件ID或文件名> <内容>\n");
            }
        }
        else if (strcmp(cmd, "read") == 0) {
            if (args_count >= 2) {
                int file_id = atoi(arg1);
                // 如果第一个参数不是数字，尝试按文件名查找
                if (file_id == 0 && strcmp(arg1, "0") != 0) {
                    file_id = find_file(arg1, current_dir);
                    if (file_id < 0) {
                        printf("文件不存在: %s\n", arg1);
                        continue;
                    }
                }
                read_file(file_id);
            } else {
                printf("用法: read <文件ID或文件名>\n");
            }
        }
        else if (strcmp(cmd, "delete") == 0) {
            if (args_count >= 2) {
                delete_file(arg1);
            } else {
                printf("用法: delete <文件名>\n");
            }
        }
        else {
            printf("未知命令: %s\n", cmd);
            printf("输入 'help' 查看可用命令\n");
        }
    }
    
    return 0;
}