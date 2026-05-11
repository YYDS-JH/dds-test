import urllib.parse

from launch import LaunchDescription
from launch.actions import SetEnvironmentVariable
from launch_ros.actions import Node

# Replace with actual IPs before deployment
LOCAL_IFACE = "eth0"
PEER_IP = "10.18.64.1"
PEER_PORT = "7000"

dds_xml = f"""\
<CycloneDDS>
  <Domain>
    <General>
      <NetworkInterfaceAddress>{LOCAL_IFACE}</NetworkInterfaceAddress>
      <AllowMulticast>false</AllowMulticast>
    </General>
    <Discovery>
      <Peers>
        <Peer Address="{PEER_IP}:{PEER_PORT}"/>
      </Peers>
    </Discovery>
  </Domain>
</CycloneDDS>"""

CYCLONEDDS_URI = "data:," + urllib.parse.quote(dds_xml, safe="")


def generate_launch_description():
    return LaunchDescription([
        SetEnvironmentVariable("CYCLONEDDS_URI", CYCLONEDDS_URI),
        Node(
            package="dds_cross_subnet_test",
            executable="slave_node",
            name="slave_node",
            output="screen",
        ),
    ])
