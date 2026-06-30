        # 任务5：电话号码簿管理系统

        ## 任务要求

        输入、输出、修改、删除联系人信息。

        ## 样品特点

        - 界面风格：通讯录便签风
        - 支持新增、修改、删除、保存、重载。
        - 使用文件流保存数据到 `data/records.csv`。
        - 使用 `std::vector` 管理记录。
        - 使用 EasyX4MinGW 实现 C++ 图形界面。
        - 特殊功能按钮：`查看详情`。

        ## 字段

        - 联系人姓名
- 电话号码
- 单位
- 邮箱
- 备注

        ## 编译

        ```powershell
        cd "C:\Users\lx\Desktop\project5"
        .\build.ps1
        ```

        生成程序：

        ```text
        Project5.exe
        ```

        ## 输入格式

        ```text
        请输入：联系人姓名,电话号码,单位,邮箱,备注
例如：张三,13800000000,学生会,zhang@example.com,同学
        ```
