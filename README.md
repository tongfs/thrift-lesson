## Thritf Lesson

### 相关目录

`game` ：实现 match_system 的 client 端

`match_system` ：实现 match_system 的 server 端和 save_system 的 client 端

`thritf` ：接口相关的 `.thrift` 文件


### 创建 match-server

```bash
cd thrift_lesson/match_system/src
thrift -r --gen cpp ../../thrift/match.thrift
mv gen-cpp match_server
rm match_server/Match_server.skeleton.cpp
```

### 创建 save-client

```bash
cd thrift_lesson/match_system/src
thrift -r --gen cpp ../../thrift/save.thrift
mv gen-cpp save_client
rm save_client/Save_server.skeleton.cpp
```

### 编译

```bash
g++ -c main.cpp match_server/*.cpp save_client/*.cpp
```

### 链接

```bash
g++ *.o -o main -lthrift -pthread
```

### 启动服务

```bash
./main
```
