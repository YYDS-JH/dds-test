#!/usr/bin/env python3
"""
Slave 侧多节点启动文件（内网，3 个节点: node_3, node_4, node_5）

用法:
  ros2 launch dds_multi_node_test multi_slave.launch.py

每个节点发布 /n<N>_topic，订阅其他 5 个 topic，1Hz
"""

import tempfile

from launch import LaunchDescription
from launch.actions import SetEnvironmentVariable
from launch_ros.actions import Node

# ── 部署前修改此处的配置 ──────────────────────────────────────────────────────
_NETWORK_INTERFACE = "enp2s0"
_EXTERNAL_ADDRESS = "10.18.21.23"             # slave 侧公网 IP（不写端口）
_NODE_COUNT = 3
_PEER_PORTS = "7000,7001,7002"                # master 侧 3 个端口
_PEER_IP = "10.18.20.42"                       # master 侧公网 IP

# ── CycloneDDS XML（同一主机 3 个节点共用，PI=auto 自动分配 0/1/2）────────────
_PORT_BASE = 7000
_PEER_XML = "\n".join(
    f'        <Peer Address="{_PEER_IP}:{p}"/>'
    for p in _PEER_PORTS.split(",")
)

_CYCLONEDDS_XML = f"""\
<?xml version="1.0" encoding="UTF-8" ?>
<CycloneDDS xmlns="https://cdds.io/config"
            xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
            xsi:schemaLocation="https://cdds.io/config
              https://raw.githubusercontent.com/eclipse-cyclonedds/cyclonedds/master/etc/cyclonedds.xsd">
  <Domain>
    <General>
      <Interfaces>
        <NetworkInterface name="{_NETWORK_INTERFACE}" multicast="false"/>
      </Interfaces>
      <AllowMulticast>false</AllowMulticast>
      <ExternalNetworkAddress>{_EXTERNAL_ADDRESS}</ExternalNetworkAddress>
    </General>
    <Discovery>
      <ParticipantIndex>auto</ParticipantIndex>
      <MaxAutoParticipantIndex>{_NODE_COUNT}</MaxAutoParticipantIndex>
      <Peers>
{_PEER_XML}
      </Peers>
      <Ports>
        <Base>{_PORT_BASE}</Base>
        <DomainGain>0</DomainGain>
        <ParticipantGain>1</ParticipantGain>
        <UnicastMetaOffset>0</UnicastMetaOffset>
        <UnicastDataOffset>0</UnicastDataOffset>
      </Ports>
    </Discovery>
    <Internal>
      <Watermarks>
        <WhcHigh>500kB</WhcHigh>
      </Watermarks>
    </Internal>
    <Tracing>
      <Verbosity>warning</Verbosity>
      <OutputFile>stderr</OutputFile>
    </Tracing>
  </Domain>
</CycloneDDS>
"""


def _write_config() -> str:
    tmp = tempfile.NamedTemporaryFile(
        mode='w',
        prefix='cyclonedds_multi_slave_',
        suffix='.xml',
        delete=False,
    )
    tmp.write(_CYCLONEDDS_XML)
    tmp.flush()
    tmp.close()
    return tmp.name


def generate_launch_description():
    xml_path = _write_config()

    # 3 个节点，索引 3,4,5
    nodes = []
    for i in range(_NODE_COUNT):
        idx = i + 3  # slave 侧用 3,4,5
        nodes.append(Node(
            package='dds_multi_node_test',
            executable='multi_node',
            name=f'node_{idx}',
            output='screen',
            parameters=[{
                'node_index': idx,
                'total_nodes': 6,
                'node_name': f'node_{idx}',
            }],
        ))

    return LaunchDescription([
        SetEnvironmentVariable('RMW_IMPLEMENTATION', 'rmw_cyclonedds_cpp'),
        SetEnvironmentVariable('CYCLONEDDS_URI', 'file://' + xml_path),
    ] + nodes)
