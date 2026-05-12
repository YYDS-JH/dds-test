#!/usr/bin/env python3
"""
Master 侧启动文件（外网，10.18.64.x）

用法:
  ros2 launch dds_cross_subnet_test master.launch.py

中间件: CycloneDDS（rmw_cyclonedds_cpp），配置内联于本文件
"""

import tempfile

from launch import LaunchDescription
from launch.actions import SetEnvironmentVariable
from launch_ros.actions import Node

# ── 部署前修改此处的配置 ──────────────────────────────────────────────────────
_NETWORK_INTERFACE = "enp2s0"
_PEER_ADDRESS = "192.168.1.1:7000"        # slave 侧路由器 WAN IP:端口

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
    </General>
    <Discovery>
      <ParticipantIndex>auto</ParticipantIndex>
      <MaxAutoParticipantIndex>50</MaxAutoParticipantIndex>
      <Peers>
        <Peer Address="{_PEER_ADDRESS}"/>
      </Peers>
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
        prefix='cyclonedds_dds_test_master_',
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
            executable='master_node',
            name='master_node',
            output='screen',
        ),
    ])
