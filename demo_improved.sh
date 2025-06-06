#!/bin/bash

echo "=== 改进的文件系统模拟器演示 ==="
echo "现在支持按文件名读写文件！"
echo ""

# 创建演示输入
cat > demo_improved_input.txt << 'EOF'
help
login 2116 dddd
create readme.txt
write readme.txt 欢迎使用文件系统模拟器！现在支持按文件名操作。
read readme.txt
create config.txt
write config.txt 系统配置：用户=2116，权限=读写，版本=2.0
read config.txt
ls
mkdir projects
cd projects
create project1.txt
write project1.txt 项目1：文件系统模拟器开发，支持按文件名操作
read project1.txt
create project2.txt
write project2.txt 项目2：用户界面改进
read project2.txt
ls
cd ..
pwd
ls
delete config.txt
ls
logout
login 2117 bbbb
create user2_notes.txt
write user2_notes.txt 用户2117的工作笔记：今天完成了文件系统测试
read user2_notes.txt
ls
logout
exit
EOF

echo "开始改进演示..."
echo "----------------------------------------"
./simple_filesystem < demo_improved_input.txt

echo ""
echo "演示完成！"
echo ""
echo "新功能："
echo "- 支持按文件名读写文件（不需要记住文件ID）"
echo "- 改进的用户界面和错误处理"
echo "- 完整的目录导航功能"
echo ""
echo "要手动测试，请运行: ./simple_filesystem"

# 清理
rm -f demo_improved_input.txt