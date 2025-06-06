#!/bin/bash

echo "=== 完整交互测试 ==="

# 创建测试输入
cat > test_full_input.txt << EOF
help
login 2116 dddd
ls
pwd
create file1.txt
create file2.txt
write 1 Hello World! This is file 1 content.
write 2 This is file 2 with different content.
read 1
read 2
ls
mkdir documents
mkdir photos
ls
cd documents
pwd
create doc1.txt
write 3 This is a document in the documents folder.
read 3
ls
cd ..
pwd
ls
cd photos
pwd
create photo1.jpg
write 4 This is photo metadata.
read 4
ls
cd ..
pwd
ls
delete file1.txt
ls
cd documents
delete doc1.txt
ls
cd ..
ls
logout
login 2117 bbbb
ls
create user2_file.txt
write 5 This file belongs to user 2117.
read 5
ls
logout
login 2118 abcd
ls
logout
exit
EOF

echo "运行完整交互测试..."
./simple_filesystem < test_full_input.txt

echo ""
echo "测试完成！"

# 清理
rm -f test_full_input.txt