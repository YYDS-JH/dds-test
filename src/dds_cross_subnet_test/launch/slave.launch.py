#!/usr/bin/env python3
"""
Slave 侧启动文件（AGV 内网，192.168.1.x）

用法:
  ros2 launch dds_cross_subnet_test slave.launch.py

中间件: CycloneDDS（rmw_cyclonedds_cpp），配置内联于本文件

端口规则（固定 ParticipantIndex=1）:
  Discovery Port = 7000（Base）+ 0（DomainGain×0）+ 1×1（ParticipantGain×PI）+ 0（MetaOffset）= 7001
"""

import tempfile

from launch import LaunchDescription
from launch.actions import SetEnvironmentVariable
from launch_ros.actions import Node

# ── 部署前修改此处的配置 ──────────────────────────────────────────────────────
_NETWORK_INTERFACE = "enp2s0"
_PEER_ADDRESS = "10.18.20.42:7000"          # master 的地址:端口
_EXTERNAL_ADDRESS = "10.18.21.23:7000"      # 本机对外宣告的公网地址（路由器 WAN IP:端口）

# ── CycloneDDS 内联配置 ────────────────────────────────────────────────────────
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
      <ParticipantIndex>0</ParticipantIndex>
      <Peers>
        <Peer Address="{_PEER_ADDRESS}"/>
      </Peers>
      <Ports>
        <Base>7000</Base>
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


def _write_cyclonedds_config() -> str:
    tmp = tempfile.NamedTemporaryFile(
        mode='w',
        prefix='cyclonedds_dds_test_slave_',
        suffix='.xml',
        delete=False,
    )
    tmp.write(_CYCLONEDDS_XML)
    tmp.flush()
    tmp.close()
    return tmp.name


def generate_launch_description():
    cyclonedds_xml_path = _write_cyclonedds_config()

    return LaunchDescription([
        SetEnvironmentVariable('RMW_IMPLEMENTATION', 'rmw_cyclonedds_cpp'),
        SetEnvironmentVariable(
            'CYCLONEDDS_URI',
            'file://' + cyclonedds_xml_path,
        ),
        Node(
            package='dds_cross_subnet_test',
            executable='slave_node',
            name='slave_node',
            output='screen',
        ),
    ])
