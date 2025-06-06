#!/bin/bash

echo "=== 测试简化文件系统 ==="

# 创建测试输入
cat > test_simple_input.txt << EOF
help
login 2116 dddd
ls
pwd
create test.txt
write 1 Hello,World!This_is_a_test_file.
read 1
ls
mkdir mydir
ls
cd mydir
pwd
ls
cd ..
pwd
delete test.txt
ls
logout
exit
EOF

echo "运行测试..."
./simple_filesystem < test_simple_input.txt

echo ""
echo "测试完成！"

# 清理
rm -f test_simple_input.txt