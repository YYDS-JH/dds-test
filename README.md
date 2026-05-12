# dds-test

ROS2 CycloneDDS 跨子网通信测试工具。测试两个 ROS2 节点通过路由器 NAT 端口转发实现跨子网发布/订阅通信。

## 依赖

| 依赖 | 版本 |
|---|---|
| Ubuntu | 22.04 |
| ROS2 | Humble |
| CycloneDDS | ROS2 Humble 自带 |
| colcon | ROS2 Humble 自带 |
| CMake | >= 3.8 |
| GCC | 支持 C++17 |

## 构建

```bash
# 安装 ROS2 Humble（如未安装）
# https://docs.ros.org/en/humble/Installation.html

# 安装 CycloneDDS（如未默认安装）
sudo apt install ros-humble-rmw-cyclonedds-cpp

# 克隆并构建
cd dds-test
source /opt/ros/humble/setup.bash
colcon build --packages-select dds_cross_subnet_test

# 运行测试
colcon build --packages-select dds_cross_subnet_test --cmake-args -DBUILD_TESTING=ON
colcon test --packages-select dds_cross_subnet_test
```

## 网络拓扑

```
AGV 内网 (192.168.1.x)          外网 (10.18.64.x)
┌─────────────────┐            ┌──────────────────┐
│  slave_node     │            │  master_node     │
│  192.168.1.X    │            │  10.18.64.X      │
│  :7000 (UDP)    │            │  :7000 (UDP)     │
└────────┬────────┘            └────────┬─────────┘
         │                              │
    ┌────┴────┐                    ┌───┴─────┐
    │ Router A │ ←── Internet ────→│ Router B│
    │ (AGV侧)  │                    │ (外网侧) │
    └─────────┘                    └─────────┘
```

## NAT 转发配置

两边路由器均需配置 UDP 端口转发。

### Router A（AGV 侧，192.168.1.x 网络）

| 类型 | 协议 | WAN 端口 | 内网 IP | 内网端口 |
|---|---|---|---|---|
| DNAT | UDP  | 7000     | 192.168.1.X | 7000  |

### Router B（外网侧，10.18.64.x 网络）

| 类型 | 协议 | WAN 端口 | 内网 IP | 内网端口 |
|---|---|---|---|---|
| DNAT | UDP  | 7000     | 10.18.64.X | 7000  |

> 同时需要确保 SNAT（源地址转换）已启用，让内网设备的出站包能到达外网。

## 部署步骤

### 1. 修改 Launch 文件中的配置

**master.launch.py** — 修改 `_NETWORK_INTERFACE` 和 `_PEER_ADDRESS`：

```python
_NETWORK_INTERFACE = "eth0"
_PEER_ADDRESS = "192.168.1.1:7000"   # 替换为 slave 侧路由器 WAN IP:端口
```

**slave.launch.py** — 修改 `_NETWORK_INTERFACE` 和 `_PEER_ADDRESS`：

```python
_NETWORK_INTERFACE = "eth0"
_PEER_ADDRESS = "10.18.64.1:7000"    # 替换为 master 的实际 IP:端口
```

### 2. 配置 NAT 转发规则

参见上文 [NAT 转发配置](#nat-转发配置)。

### 3. 构建并部署到目标机器

```bash
# 在工作空间根目录构建
source /opt/ros/humble/setup.bash
colcon build --packages-select dds_cross_subnet_test

# 将整个工作空间拷贝到目标机器（或分别拷贝到两台机器）
# 两台机器都需要有 ROS2 Humble + CycloneDDS
```

### 4. 启动节点

**Master 侧（外网，10.18.64.x）：**

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 launch dds_cross_subnet_test master.launch.py
```

**Slave 侧（AGV，192.168.1.x）：**

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 launch dds_cross_subnet_test slave.launch.py
```

> 启动顺序无关，任一节点先启动均可，对端上线后会自动建立连接。

### 5. 验证通信

正常运行时，每个节点会：

- 每秒输出丢包统计（每 50 条消息一次，10Hz = 每 5 秒）

```
[master_node] STATS: expected=50 received=50 lost=0 (0.0% loss)
[master_node] STATS: expected=100 received=100 lost=0 (0.0% loss)
```

- 丢包时立即告警：

```
[master_node] WARNING: packet loss detected! missed 3 messages (seq 47 -> 51)
```

如果两边都能看到 STATS 输出且 lost=0，说明跨子网通信正常。

## 端口说明

两个节点各使用一个 UDP 端口（默认 7000），同时处理 DDS 发现和用户数据传输。

| 节点 | 监听端口 |
|---|---|
| master_node | 7000 |
| slave_node | 7000 |

## Topic 列表

| Topic | 方向 | QoS |
|---|---|---|
| `/master_to_slave` | master 发 → slave 收 | RELIABLE, VOLATILE, KEEP_LAST(10) |
| `/slave_to_master` | slave 发 → master 收 | RELIABLE, VOLATILE, KEEP_LAST(10) |
