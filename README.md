# ESP-NOWMESH

## A library for simple mesh networking between ESP32 nodes
The goal is to provide the simplest form of unmanaged mesh network. There's no routing, no path calculation - all and everything is broadcast. Potential application of this library would be devices that are part of very dynamic environments like people in a crowd or cars on the road.

## The nodes, the services
Every node uses the STA MAC address as an ID. A node can send to any destination service in the mesh network (a service is represented by a  MAC address), but can only receive packets from addresses it has subscribed to, and it's personal address - it's ID.

Recommended addresses for service IDs, these are guaranteed not to be used by any node as their ID:
- x**2** : xx : xx : xx : xx : xx
- x**6** : xx : xx : xx : xx : xx
- x**A** : xx : xx : xx : xx : xx
- x**E** : xx : xx : xx : xx : xx
- 
## The network, the packets
All nodes are part of the mesh, every node repeats everything even if it's not subscribed to that service. A packet can hop through maximum of 16 nodes before reaching it's end-of-life. Packets are repeat only once by each node to prevent broadcast storms.
You can choose to request ACK from the nodes that received that packet if you need that. There are also management packets which help the nodes organize themselves. The management packets sync the time between the nodes (provided there is a node connected to internet for NTP), and provide ping and traceroute capabilities, for now.